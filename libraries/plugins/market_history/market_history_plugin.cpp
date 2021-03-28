#include <btcm/market_history/market_history_api.hpp>

#include <btcm/chain/database.hpp>
#include <btcm/chain/history_object.hpp>

namespace btcm { namespace market_history {

namespace detail {

class market_history_plugin_impl
{
   public:
      market_history_plugin_impl( market_history_plugin& plugin )
         :_self( plugin ) {}
      virtual ~market_history_plugin_impl() {}

      /**
       * This method is called as a callback after a block is applied
       * and will process/index all operations that were applied in the block.
       */
      void update_market_histories( const operation_object& b );

      market_history_plugin& _self;
      flat_set<uint32_t>     _tracked_buckets = { 15, 60, 300, 3600, 86400 };
      int32_t                _maximum_history_per_bucket_size = 5760;
};

void market_history_plugin_impl::update_market_histories( const operation_object& o )
{
   if( o.op.which() == operation::tag< fill_order_operation >::value )
   {
      fill_order_operation op = o.op.get< fill_order_operation >();

      auto& db = _self.database();
      const auto& bucket_idx = db.get_index_type< bucket_index >().indices().get< by_bucket >();

      db.create< order_history_object >( [&]( order_history_object& ho )
      {
         ho.time = db.head_block_time();
         ho.op = op;
      });

      if( !_maximum_history_per_bucket_size ) return;
      if( !_tracked_buckets.size() ) return;

      for( auto bucket : _tracked_buckets )
      {
         auto cutoff = db.head_block_time() - fc::seconds( bucket * _maximum_history_per_bucket_size );

         auto open = fc::time_point_sec( ( db.head_block_time().sec_since_epoch() / bucket ) * bucket );
         auto seconds = bucket;

         auto itr = bucket_idx.find( boost::make_tuple( seconds, open ) );
         if( itr == bucket_idx.end() )
         {
            db.create< bucket_object >( [&]( bucket_object &b )
            {
               b.open = open;
               b.seconds = bucket;

               if( op.open_pays.asset_id == BTCM_SYMBOL )
               {
                  b.high_btcm = op.open_pays.amount;
                  b.high_mbd = op.current_pays.amount;
                  b.low_btcm = op.open_pays.amount;
                  b.low_mbd = op.current_pays.amount;
                  b.open_btcm = op.open_pays.amount;
                  b.open_mbd = op.current_pays.amount;
                  b.close_btcm = op.open_pays.amount;
                  b.close_mbd = op.current_pays.amount;
                  b.btcm_volume = op.open_pays.amount;
                  b.mbd_volume = op.current_pays.amount;
               }
               else
               {
                  b.high_btcm = op.current_pays.amount;
                  b.high_mbd = op.open_pays.amount;
                  b.low_btcm = op.current_pays.amount;
                  b.low_mbd = op.open_pays.amount;
                  b.open_btcm = op.current_pays.amount;
                  b.open_mbd = op.open_pays.amount;
                  b.close_btcm = op.current_pays.amount;
                  b.close_mbd = op.open_pays.amount;
                  b.btcm_volume = op.current_pays.amount;
                  b.mbd_volume = op.open_pays.amount;
               }
            });
         }
         else
         {
            db.modify( *itr, [&]( bucket_object& b )
            {
               if( op.open_pays.asset_id == BTCM_SYMBOL )
               {
                  b.btcm_volume += op.open_pays.amount;
                  b.mbd_volume += op.current_pays.amount;
                  b.close_btcm = op.open_pays.amount;
                  b.close_mbd = op.current_pays.amount;

                  if( b.high() < price( op.current_pays, op.open_pays ) )
                  {
                     b.high_btcm = op.open_pays.amount;
                     b.high_mbd = op.current_pays.amount;
                  }

                  if( b.low() > price( op.current_pays, op.open_pays ) )
                  {
                     b.low_btcm = op.open_pays.amount;
                     b.low_mbd = op.current_pays.amount;
                  }
               }
               else
               {
                  b.btcm_volume += op.current_pays.amount;
                  b.mbd_volume += op.open_pays.amount;
                  b.close_btcm = op.current_pays.amount;
                  b.close_mbd = op.open_pays.amount;

                  if( b.high() < price( op.open_pays, op.current_pays ) )
                  {
                     b.high_btcm = op.current_pays.amount;
                     b.high_mbd = op.open_pays.amount;
                  }

                  if( b.low() > price( op.open_pays, op.current_pays ) )
                  {
                     b.low_btcm = op.current_pays.amount;
                     b.low_mbd = op.open_pays.amount;
                  }
               }
            });

            if( _maximum_history_per_bucket_size > 0 )
            {
               open = fc::time_point_sec();
               itr = bucket_idx.lower_bound( boost::make_tuple( seconds, open ) );

               while( itr->seconds == seconds && itr->open < cutoff )
               {
                  auto old_itr = itr;
                  ++itr;
                  db.remove( *old_itr );
               }
            }
         }
      }
   }
}

} // detail

market_history_plugin::market_history_plugin() :
   _my( new detail::market_history_plugin_impl( *this ) ) {}
market_history_plugin::~market_history_plugin() {}

void market_history_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
)
{
   cli.add_options()
         ("bucket-size", boost::program_options::value<string>()->default_value("[15,60,300,3600,86400]"),
           "Track market history by grouping orders into buckets of equal size measured in seconds specified as a JSON array of numbers")
         ("history-per-size", boost::program_options::value<uint32_t>()->default_value(5760),
           "How far back in time to track history for each bucket size, measured in the number of buckets (default: 5760)")
         ;
   cfg.add(cli);
}

void market_history_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   try
   {
      database().pre_apply_operation.connect( [this]( const operation_object& o ){ _my->update_market_histories( o ); } );
      database().add_index< primary_index< bucket_index > >();
      database().add_index< primary_index< order_history_index > >();

      if( options.count("bucket-size" ) )
      {
         const std::string& buckets = options["bucket-size"].as< string >();
         _my->_tracked_buckets = fc::json::from_string( buckets ).as< flat_set< uint32_t > >( 2 );
      }
      if( options.count("history-per-size" ) )
         _my->_maximum_history_per_bucket_size = options["history-per-size"].as< uint32_t >();
   } FC_CAPTURE_AND_RETHROW()
}

void market_history_plugin::plugin_startup()
{
   app().register_api_factory< market_history_api >( "market_history_api" );
}

flat_set< uint32_t > market_history_plugin::get_tracked_buckets() const
{
   return _my->_tracked_buckets;
}

uint32_t market_history_plugin::get_max_history_per_bucket() const
{
   return _my->_maximum_history_per_bucket_size;
}

} } // btcm::market_history

BTCM_DEFINE_PLUGIN( market_history, btcm::market_history::market_history_plugin )
