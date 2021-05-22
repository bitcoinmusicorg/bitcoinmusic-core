#include <btcm/market_history/market_history_api.hpp>

#include <btcm/chain/database.hpp>
#include <btcm/chain/history_object.hpp>

#include "normalized_ids.hxx"

#include <memory>

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
      uint32_t               _maximum_history_per_bucket_size = 5760; // 1 day at 15s, 4 days at 1m
      fc::microseconds       _max_age;
      boost::signals2::scoped_connection _db_conn;
};

static void update_bucket( bucket_object& bucket, const fill_order_operation& op )
{
   const price order_price = op.current_pays.asset_id == bucket.asset_a ? op.current_pays / op.open_pays
                                                                        : op.open_pays / op.current_pays;
   bucket.close_a = order_price.base.amount;
   bucket.close_b = order_price.quote.amount;
   if( bucket.high_a == 0 || bucket.high() < order_price ) {
      bucket.high_a = order_price.base.amount;
      bucket.high_b = order_price.quote.amount;
   }
   if( bucket.low_a == 0 || bucket.low() > order_price ) {
      bucket.low_a = order_price.base.amount;
      bucket.low_b = order_price.quote.amount;
   }
   bucket.volume_a += order_price.base.amount;
   bucket.volume_b += order_price.quote.amount;
}

void market_history_plugin_impl::update_market_histories( const operation_object& o )
{
   if( o.op.which() == operation::tag< fill_order_operation >::value )
   {
      const auto& op = o.op.get< fill_order_operation >();

      auto& db = _self.database();
      const auto& bucket_idx = db.get_index_type< bucket_index >().indices().get< by_bucket >();

      db.create< order_history_object >( [&db,&op]( order_history_object& ho )
      {
         ho.time = db.head_block_time();
         ho.op = op;
      });

      if( !_maximum_history_per_bucket_size ) return;
      if( db.head_block_time() < fc::time_point::now() - _max_age ) return;
      if( op.current_pays.amount == 0 || op.open_pays.amount == 0 ) return;

      for( auto bucket : _tracked_buckets )
      {
         auto open = fc::time_point_sec( ( db.head_block_time().sec_since_epoch() / bucket ) * bucket );
         auto seconds = bucket;

         asset_id_type asset_a = op.current_pays.asset_id;
         asset_id_type asset_b = op.open_pays.asset_id;
         normalize_asset_ids( asset_a, asset_b );

         auto itr = bucket_idx.find( boost::make_tuple( asset_a, asset_b, seconds, open ) );
         if( itr == bucket_idx.end() )
         {
            db.create< bucket_object >( [&op, open, bucket, asset_a, asset_b]( bucket_object &b )
            {
               b.start = open;
               b.seconds = bucket;
               b.asset_a = asset_a;
               b.asset_b = asset_b;
               update_bucket(b, op);
               b.open_a = b.close_a;
               b.open_b = b.close_b;
            });

            auto cutoff = db.head_block_time() - fc::seconds( bucket * _maximum_history_per_bucket_size );
            itr = bucket_idx.lower_bound( boost::make_tuple( asset_a, asset_b, seconds, fc::time_point_sec(0) ) );
            while( itr != bucket_idx.end() && itr->seconds == seconds && itr->start < cutoff )
            {
               auto old_itr = itr;
               ++itr;
               db.remove( *old_itr );
            }
         }
         else
            db.modify( *itr, [&op]( bucket_object& b ) { update_bucket(b, op); });
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
      _my->_db_conn = database().pre_apply_operation.connect(
             std::bind( &detail::market_history_plugin_impl::update_market_histories, _my.get(), std::placeholders::_1 ) );
      database().add_index< primary_index< bucket_index > >();
      database().add_index< primary_index< order_history_index > >();

      if( options.count("bucket-size" ) )
      {
         const std::string& buckets = options["bucket-size"].as< string >();
         _my->_tracked_buckets = fc::json::from_string( buckets ).as< flat_set< uint32_t > >( 2 );
         FC_ASSERT( _my->_tracked_buckets.empty() || *_my->_tracked_buckets.begin() > 0,
                    "Invalid bucket size, must be greater than 0!" );
      }
      if( options.count("history-per-size" ) )
         _my->_maximum_history_per_bucket_size = options["history-per-size"].as< uint32_t >();
      _my->_max_age = fc::seconds( _my->_tracked_buckets.empty() ? 0
                                     : _my->_maximum_history_per_bucket_size * *_my->_tracked_buckets.rbegin() );
   } FC_CAPTURE_AND_RETHROW()
}

void market_history_plugin::plugin_startup()
{
   app().register_api_factory< market_history_api >( "market_history_api" );
}

const flat_set< uint32_t >& market_history_plugin::get_tracked_buckets() const
{
   return _my->_tracked_buckets;
}

uint32_t market_history_plugin::get_max_history_per_bucket() const
{
   return _my->_maximum_history_per_bucket_size;
}

} } // btcm::market_history

BTCM_DEFINE_PLUGIN( market_history, btcm::market_history::market_history_plugin )
