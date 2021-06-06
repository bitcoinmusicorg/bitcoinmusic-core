#include <btcm/market_history/market_history_api.hpp>

#include <btcm/chain/base_objects.hpp>

#include "normalized_ids.hxx"

namespace btcm { namespace market_history {

namespace detail
{

class market_history_api_impl
{
   public:
      market_history_api_impl( btcm::app::application& _app )
         :app( _app ), db( *_app.chain_database() ) {}

      market_ticker get_ticker( const std::string& asset_a, const std::string& asset_b ) const;
      market_volume get_volume( const std::string& asset_a, const std::string& asset_b ) const;
      order_book get_order_book( const std::string& asset_a, const std::string& asset_b, uint32_t limit ) const;
      vector< market_trade > get_trade_history( const std::string& asset_a, const std::string& asset_b,
                                                time_point_sec start, time_point_sec end, uint32_t limit ) const;
      vector< market_trade > get_recent_trades( const std::string& asset_a, const std::string& asset_b, uint32_t limit ) const;
      vector< bucket_object > get_market_history( const std::string& asset_a, const std::string& asset_b,
                                                  uint32_t bucket_seconds, time_point_sec start, time_point_sec end ) const;
      const flat_set< uint32_t >& get_market_history_buckets() const;

      asset_id_type get_asset( const std::string& symbol_or_id )const;

      btcm::app::application& app;
      chain::database& db;
};

asset_id_type market_history_api_impl::get_asset( const std::string& symbol_or_id ) const
{
   try {
      asset_id_type result;
      fc::from_variant( symbol_or_id, result, 1 );
      return result;
   } catch( ... ) {
      const auto& ass = db.get_asset( symbol_or_id );
      return ass.id;
   }
}

market_ticker market_history_api_impl::get_ticker( const std::string& asset_a, const std::string& asset_b ) const
{
   const auto& buckets = app.get_plugin< market_history_plugin >( MARKET_HISTORY_PLUGIN_NAME )->get_tracked_buckets();
   FC_ASSERT( buckets.find(86400) != buckets.end(), "get_ticker requires bucket size 86400" );

   asset_id_type asset_id_a = get_asset( asset_a );
   asset_id_type asset_id_b = get_asset( asset_b );
   normalize_asset_ids( asset_id_a, asset_id_b );

   const auto& bucket_idx = db.get_index_type< bucket_index >().indices().get< by_bucket >();
   auto itr = bucket_idx.lower_bound( boost::make_tuple( asset_id_a, asset_id_b, 86400, db.head_block_time() - 86400 ) );

   market_ticker result( get_volume( asset_a, asset_b ) );
   if( itr != bucket_idx.end() && itr->asset_a == asset_id_a && itr->asset_b == asset_id_b && itr->seconds == 86400 )
   {
      auto open = itr->open().to_real();
      result.latest = itr->close();
      result.percent_change = ( ( result.latest.to_real() - open ) / open ) * 100;
   }
   else
      result.percent_change = 0;

   auto orders = get_order_book( asset_a, asset_b, 1 );
   if( orders.bids.size() )
      result.highest_bid = orders.bids[0].price();
   if( orders.asks.size() )
      result.lowest_ask = ~orders.asks[0].price();

   return result;
}

market_volume market_history_api_impl::get_volume( const std::string& asset_a, const std::string& asset_b ) const
{
   const auto& buckets = app.get_plugin< market_history_plugin >( MARKET_HISTORY_PLUGIN_NAME )->get_tracked_buckets();
   FC_ASSERT( !buckets.empty() && *buckets.begin() < 86400, "get_volume requires a bucket size less than 86400" );
   const uint32_t bucket_size = *buckets.begin();

   asset_id_type asset_id_a = get_asset( asset_a );
   asset_id_type asset_id_b = get_asset( asset_b );
   normalize_asset_ids( asset_id_a, asset_id_b );

   const auto& bucket_idx = db.get_index_type< bucket_index >().indices().get< by_bucket >();
   auto itr = bucket_idx.lower_bound( boost::make_tuple( asset_id_a, asset_id_b,
                                                         bucket_size, db.head_block_time() - 86400 ) );

   market_volume result = { asset( 0, asset_id_a ), asset( 0, asset_id_b ) };
   while( itr != bucket_idx.end() && itr->asset_a == asset_id_a && itr->asset_b == asset_id_b
          && itr->seconds == bucket_size )
   {
      result.volume_a.amount += itr->volume_a;
      result.volume_b.amount += itr->volume_b;
      ++itr;
   }

   return result;
}

order_book market_history_api_impl::get_order_book( const std::string& asset_a, const std::string& asset_b, uint32_t limit ) const
{
   FC_ASSERT( limit <= 500 );

   asset_id_type asset_id_a = get_asset( asset_a );
   asset_id_type asset_id_b = get_asset( asset_b );
   normalize_asset_ids( asset_id_a, asset_id_b );

   const auto& order_idx = db.get_index_type< btcm::chain::limit_order_index >().indices().get< btcm::chain::by_price >();
   auto itr = order_idx.lower_bound( price::max( asset_id_a, asset_id_b ) );

   order_book result;
   result.bids.reserve( limit );
   result.asks.reserve( limit );
   while( itr != order_idx.end() && itr->sell_price.base.asset_id == asset_id_a
          && itr->sell_price.quote.asset_id == asset_id_b && result.bids.size() < limit )
   {
      asset bid( itr->for_sale, asset_id_a );
      result.bids.emplace_back( bid, bid * itr->sell_price );
      ++itr;
   }

   itr = order_idx.lower_bound( price::max( asset_id_b, asset_id_a ) );
   while( itr != order_idx.end() && itr->sell_price.base.asset_id == asset_id_b
          && itr->sell_price.quote.asset_id == asset_id_a && result.asks.size() < limit )
   {
      asset bid( itr->for_sale, asset_id_b );
      result.asks.emplace_back( bid, bid * itr->sell_price );
      ++itr;
   }

   return result;
}

std::vector< market_trade > market_history_api_impl::get_trade_history( const std::string& asset_a, const std::string& asset_b,
                                                                        time_point_sec start, time_point_sec end, uint32_t limit ) const
{
   FC_ASSERT( limit <= 1000 );

   asset_id_type asset_id_a = get_asset( asset_a );
   asset_id_type asset_id_b = get_asset( asset_b );
   if( asset_id_a > asset_id_b ) std::swap( asset_id_a, asset_id_b );

   const auto& bucket_idx = db.get_index_type< order_history_index >().indices().get< by_time >();
   auto itr = bucket_idx.lower_bound( boost::make_tuple( asset_id_a, asset_id_b, start ) );

   std::vector< market_trade > result;
   result.reserve( limit );
   while( itr != bucket_idx.end() && itr->asset_a() == asset_id_a && itr->asset_b() == asset_id_b
          && itr->time <= end && result.size() < limit )
   {
      result.emplace_back( *itr );
      ++itr;
   }

   return result;
}

vector< market_trade > market_history_api_impl::get_recent_trades( const std::string& asset_a, const std::string& asset_b, uint32_t limit = 1000 ) const
{
   FC_ASSERT( limit <= 1000 );

   asset_id_type asset_id_a = get_asset( asset_a );
   asset_id_type asset_id_b = get_asset( asset_b );
   if( asset_id_a > asset_id_b ) std::swap( asset_id_a, asset_id_b );

   const auto& order_idx = db.get_index_type< order_history_index >().indices().get< by_time >();
   auto itr = order_idx.upper_bound( boost::make_tuple( asset_id_a, asset_id_b ) );

   vector< market_trade > result;
   result.reserve( limit );
   while( itr != order_idx.begin() && (--itr)->asset_a() == asset_id_a && itr->asset_b() == asset_id_b && result.size() < limit )
   {
      result.emplace_back( *itr );
   }

   return result;
}

std::vector< bucket_object > market_history_api_impl::get_market_history( const std::string& asset_a, const std::string& asset_b,
                                                                          uint32_t bucket_seconds, time_point_sec start, time_point_sec end ) const
{
   asset_id_type asset_id_a = get_asset( asset_a );
   asset_id_type asset_id_b = get_asset( asset_b );
   normalize_asset_ids( asset_id_a, asset_id_b );

   if( end > db.get_slot_time(1) ) end = db.get_slot_time(1);

   const auto& bucket_idx = db.get_index_type< bucket_index >().indices().get< by_bucket >();
   auto itr = bucket_idx.lower_bound( boost::make_tuple( asset_id_a, asset_id_b, bucket_seconds, start ) );

   std::vector< bucket_object > result;
   if( itr != bucket_idx.end() && itr->asset_a == asset_id_a && itr->asset_b == asset_id_b
          && itr->seconds == bucket_seconds && itr->start < end )
      result.reserve( (end - itr->start).to_seconds() / bucket_seconds + 1 );
   while( itr != bucket_idx.end() && itr->asset_a == asset_id_a && itr->asset_b == asset_id_b
          && itr->seconds == bucket_seconds && itr->start < end )
   {
      result.emplace_back( *itr );
      ++itr;
   }

   return result;
}

const flat_set< uint32_t >& market_history_api_impl::get_market_history_buckets() const
{
   return app.get_plugin< market_history_plugin >( MARKET_HISTORY_PLUGIN_NAME )->get_tracked_buckets();
}

} // detail

market_history_api::market_history_api( const btcm::app::api_context& ctx )
{
   my = std::make_shared< detail::market_history_api_impl >( ctx.app );
}

void market_history_api::on_api_startup() {}

market_ticker market_history_api::get_ticker( const std::string& asset_a, const std::string& asset_b ) const
{
   return my->get_ticker( asset_a, asset_b );
}

market_volume market_history_api::get_volume( const std::string& asset_a, const std::string& asset_b ) const
{
   return my->get_volume( asset_a, asset_b );
}

order_book market_history_api::get_order_book( const std::string& asset_a, const std::string& asset_b, uint32_t limit ) const
{
   return my->get_order_book( asset_a, asset_b, limit );
}

std::vector< market_trade > market_history_api::get_trade_history( const std::string& asset_a, const std::string& asset_b,
                                                                   time_point_sec start, time_point_sec end, uint32_t limit ) const
{
   return my->get_trade_history( asset_a, asset_b, start, end, limit );
}

std::vector< market_trade > market_history_api::get_recent_trades( const std::string& asset_a, const std::string& asset_b, uint32_t limit ) const
{
   return my->get_recent_trades( asset_a, asset_b, limit );
}

std::vector< bucket_object > market_history_api::get_market_history( const std::string& asset_a, const std::string& asset_b,
                                                                     uint32_t bucket_seconds, time_point_sec start, time_point_sec end ) const
{
   return my->get_market_history( asset_a, asset_b, bucket_seconds, start, end );
}

flat_set< uint32_t > market_history_api::get_market_history_buckets() const
{
   return my->get_market_history_buckets();
}

} } // btcm::market_history
