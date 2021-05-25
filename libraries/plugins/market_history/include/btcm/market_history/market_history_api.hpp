#pragma once

#include <btcm/market_history/market_history_plugin.hpp>

#include <btcm/chain/protocol/types.hpp>

#include <fc/api.hpp>

namespace btcm { namespace app {
   struct api_context;
} }

namespace btcm{ namespace market_history {

using chain::share_type;
using fc::time_point_sec;

namespace detail
{
   class market_history_api_impl;
}

struct market_volume
{
   asset      volume_a;
   asset      volume_b;
};

struct market_ticker : market_volume
{
   market_ticker(market_volume&& v) : market_volume( std::move(v) ) {}

   price      latest;
   price      lowest_ask;
   price      highest_bid;
   double     percent_change = 0;
};

class order
{
public:
   asset bid;
   asset ask;

   chain::price price()const{ return bid / ask; }
};

struct order_book
{
   vector< order > bids;
   vector< order > asks;
};

struct market_trade
{
   time_point_sec date;
   asset          current_pays;
   asset          open_pays;
};

class market_history_api
{
   public:
      market_history_api( const btcm::app::api_context& ctx );

      void on_api_startup();

      /**
       * @brief Returns the market ticker for the given market. The ticker includes trade data from the
       *        current day (UTC), 24h volume and the top of the order book.
       * @param asset_a symbol or asset id of one side of the market
       * @param asset_b symbol or asset id of the other side of the market
       */
      market_ticker get_ticker( const std::string& asset_a, const std::string& asset_b ) const;

      /**
       * @brief Returns the market volume for the past 24 hours
       * @param asset_a symbol or asset id of one side of the market
       * @param asset_b symbol or asset id of the other side of the market
       */
      market_volume get_volume( const std::string& asset_a, const std::string& asset_b ) const;

      /**
       * @brief Returns the current order book for the given market.
       * @param asset_a symbol or asset id of one side of the market
       * @param asset_b symbol or asset id of the other side of the market
       * @param limit The number of orders to have on each side of the order book. Maximum is 500
       */
      order_book get_order_book( const std::string& asset_a, const std::string& asset_b, uint32_t limit = 500 )const;

      /**
       * @brief Returns the trade history for the given market.
       * @param asset_a symbol or asset id of one side of the market
       * @param asset_b symbol or asset id of the other side of the market
       * @param start The start time of the trade history.
       * @param end The end time of the trade history.
       * @param limit The number of trades to return. Maximum is 1000.
       * @return A list of completed trades.
       */
      std::vector< market_trade > get_trade_history( const std::string& asset_a, const std::string& asset_b,
                                                     time_point_sec start, time_point_sec end, uint32_t limit = 1000 ) const;

      /**
       * @brief Returns the N most recent trades for the given market.
       * @param asset_a symbol or asset id of one side of the market
       * @param asset_b symbol or asset id of the other side of the market
       * @param limit The number of recent trades to return. Maximum is 1000.
       * @returns A list of completed trades.
       */
       std::vector< market_trade > get_recent_trades( const std::string& asset_a, const std::string& asset_b,
                                                      uint32_t limit = 1000 ) const;

      /**
       * @brief Returns the market history for the given market.
       * @param asset_a symbol or asset id of one side of the market
       * @param asset_b symbol or asset id of the other side of the market
       * @param bucket_seconds The size of buckets the history is broken into. The bucket size must be configured in the plugin options.
       * @param start The start time to get market history.
       * @param end The end time to get market history
       * @return A list of market history buckets.
       */
      std::vector< bucket_object > get_market_history( const std::string& asset_a, const std::string& asset_b,
                                                       uint32_t bucket_seconds, time_point_sec start, time_point_sec end ) const;

      /**
       * @brief Returns the bucket seconds being tracked by the plugin.
       */
      chain::flat_set< uint32_t > get_market_history_buckets() const;

   private:
      std::shared_ptr< detail::market_history_api_impl > my;
};

} } // btcm::market_history

FC_REFLECT( btcm::market_history::market_volume,
   (volume_a)(volume_b) );
FC_REFLECT_DERIVED( btcm::market_history::market_ticker, (btcm::market_history::market_volume),
   (latest)(lowest_ask)(highest_bid)(percent_change) );
FC_REFLECT( btcm::market_history::order,
   (bid)(ask) );
FC_REFLECT( btcm::market_history::order_book,
   (bids)(asks) );
FC_REFLECT( btcm::market_history::market_trade,
   (date)(current_pays)(open_pays) );

FC_API( btcm::market_history::market_history_api,
   (get_ticker)
   (get_volume)
   (get_order_book)
   (get_trade_history)
   (get_recent_trades)
   (get_market_history)
   (get_market_history_buckets)
);
