#include <boost/test/unit_test.hpp>

#include <btcm/chain/protocol/ext.hpp>
#include <btcm/chain/account_object.hpp>
#include <btcm/chain/protocol/base_operations.hpp>

#include <btcm/market_history/market_history_plugin.hpp>

#include "../common/database_fixture.hpp"

using namespace btcm::chain;
using namespace btcm::chain::test;

BOOST_FIXTURE_TEST_SUITE( market_history, clean_database_fixture )

BOOST_AUTO_TEST_CASE( mh_test )
{
   using namespace btcm::market_history;

   try
   {
      auto mh_plugin = app.register_plugin< market_history_plugin >();
      boost::program_options::variables_map options;
      mh_plugin->plugin_set_app( &app );
      mh_plugin->plugin_initialize( options );

      set_price_feed( price( ASSET( "0.500 2.28.0" ), ASSET( "1.000 2.28.2" ) ) );

      ACTORS( (alice)(bob)(sam) );
      fund( "alice", 100000000 );
      fund( "bob", 100000000 );
      fund( "sam", 100000000 );

      generate_block();

      convert( "alice", ASSET( "50.0 2.28.0" ) );
      convert( "bob", ASSET( "50.0 2.28.0" ) );
      convert( "sam", ASSET( "50.0 2.28.0" ) );

      const auto& bucket_idx = db.get_index_type< bucket_index >().indices().get< by_bucket >();
      const auto& order_hist_idx = db.get_index_type< order_history_index >().indices().get< by_id >();

      BOOST_REQUIRE( bucket_idx.begin() == bucket_idx.end() );
      BOOST_REQUIRE( order_hist_idx.begin() == order_hist_idx.end() );
      validate_database();

      trx.clear();

      auto fill_order_a_time = db.head_block_time();
      auto time_a = fc::time_point_sec( ( fill_order_a_time.sec_since_epoch() / 15 ) * 15 );

      limit_order_create_operation op;
      op.owner = "alice";
      op.amount_to_sell = ASSET( "1.000 2.28.2" );
      op.min_to_receive = ASSET( "2.000 2.28.0" );
      trx.operations.push_back( op );
      trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( trx,  0 );

      trx.clear();

      op.owner = "bob";
      op.amount_to_sell = ASSET( "1.500 2.28.0" );
      op.min_to_receive = ASSET( "0.750 2.28.2" );
      trx.operations.push_back( op );
      trx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( trx, 0 );

      generate_blocks( db.head_block_time() + ( 60 * 90 ) );

      auto fill_order_b_time = db.head_block_time();

      trx.clear();

      op.owner = "sam";
      op.amount_to_sell = ASSET( "1.000 2.28.0" );
      op.min_to_receive = ASSET( "0.500 2.28.2" );
      trx.operations.push_back( op );
      trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( trx, 0 );

      generate_blocks( db.head_block_time() + 60 );

      auto fill_order_c_time = db.head_block_time();

      trx.clear();

      op.owner = "alice";
      op.amount_to_sell = ASSET( "0.500 2.28.2" );
      op.min_to_receive = ASSET( "0.900 2.28.0" );
      trx.operations.push_back( op );
      trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( trx, 0 );

      trx.clear();

      op.owner = "bob";
      op.amount_to_sell = ASSET( "0.450 2.28.0" );
      op.min_to_receive = ASSET( "0.250 2.28.2" );
      trx.operations.push_back( op );
      trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );
      trx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( trx, 0 );
      validate_database();

      auto bucket = bucket_idx.begin();
      BOOST_REQUIRE( bucket != bucket_idx.end() );

      BOOST_CHECK( bucket->asset_a == XUSD_SYMBOL );
      BOOST_CHECK( bucket->asset_b == BTCM_SYMBOL );
      BOOST_CHECK( bucket->seconds == 15 );
      BOOST_CHECK( bucket->start == time_a );
      BOOST_CHECK( bucket->high_b == ASSET( "1.500 2.28.0 " ).amount );
      BOOST_CHECK( bucket->high_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->low_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->low_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->open_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->open_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->close_b == ASSET( "1.500 2.28.0").amount );
      BOOST_CHECK( bucket->close_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->volume_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->volume_a == ASSET( "0.750 2.28.2" ).amount );
      bucket++;
      BOOST_REQUIRE( bucket != bucket_idx.end() );

      BOOST_CHECK( bucket->asset_a == XUSD_SYMBOL );
      BOOST_CHECK( bucket->asset_b == BTCM_SYMBOL );
      BOOST_CHECK( bucket->seconds == 15 );
      BOOST_CHECK( bucket->start == time_a + ( 60 * 90 ) );
      BOOST_CHECK( bucket->high_b == ASSET( "0.500 2.28.0 " ).amount );
      BOOST_CHECK( bucket->high_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->low_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->low_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->open_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->open_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->close_b == ASSET( "0.500 2.28.0").amount );
      BOOST_CHECK( bucket->close_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->volume_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->volume_a == ASSET( "0.250 2.28.2" ).amount );
      bucket++;
      BOOST_REQUIRE( bucket != bucket_idx.end() );

      BOOST_CHECK( bucket->asset_a == XUSD_SYMBOL );
      BOOST_CHECK( bucket->asset_b == BTCM_SYMBOL );
      BOOST_CHECK( bucket->seconds == 15 );
      BOOST_CHECK( bucket->start == time_a + ( 60 * 90 ) + 60 );
      BOOST_CHECK( bucket->high_b == ASSET( "0.450 2.28.0 " ).amount );
      BOOST_CHECK( bucket->high_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->low_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->low_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->open_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->open_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->close_b == ASSET( "0.450 2.28.0").amount );
      BOOST_CHECK( bucket->close_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->volume_b == ASSET( "0.950 2.28.0" ).amount );
      BOOST_CHECK( bucket->volume_a == ASSET( "0.500 2.28.2" ).amount );
      bucket++;
      BOOST_REQUIRE( bucket != bucket_idx.end() );

      BOOST_CHECK( bucket->asset_a == XUSD_SYMBOL );
      BOOST_CHECK( bucket->asset_b == BTCM_SYMBOL );
      BOOST_CHECK( bucket->seconds == 60 );
      BOOST_CHECK( bucket->start == time_a );
      BOOST_CHECK( bucket->high_b == ASSET( "1.500 2.28.0 " ).amount );
      BOOST_CHECK( bucket->high_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->low_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->low_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->open_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->open_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->close_b == ASSET( "1.500 2.28.0").amount );
      BOOST_CHECK( bucket->close_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->volume_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->volume_a == ASSET( "0.750 2.28.2" ).amount );
      bucket++;
      BOOST_REQUIRE( bucket != bucket_idx.end() );

      BOOST_CHECK( bucket->asset_a == XUSD_SYMBOL );
      BOOST_CHECK( bucket->asset_b == BTCM_SYMBOL );
      BOOST_CHECK( bucket->seconds == 60 );
      BOOST_CHECK( bucket->start == time_a + ( 60 * 90 ) );
      BOOST_CHECK( bucket->high_b == ASSET( "0.500 2.28.0 " ).amount );
      BOOST_CHECK( bucket->high_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->low_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->low_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->open_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->open_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->close_b == ASSET( "0.500 2.28.0").amount );
      BOOST_CHECK( bucket->close_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->volume_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->volume_a == ASSET( "0.250 2.28.2" ).amount );
      bucket++;
      BOOST_REQUIRE( bucket != bucket_idx.end() );

      BOOST_CHECK( bucket->asset_a == XUSD_SYMBOL );
      BOOST_CHECK( bucket->asset_b == BTCM_SYMBOL );
      BOOST_CHECK( bucket->seconds == 60 );
      BOOST_CHECK( bucket->start == time_a + ( 60 * 90 ) + 60 );
      BOOST_CHECK( bucket->high_b == ASSET( "0.450 2.28.0 " ).amount );
      BOOST_CHECK( bucket->high_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->low_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->low_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->open_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->open_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->close_b == ASSET( "0.450 2.28.0").amount );
      BOOST_CHECK( bucket->close_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->volume_b == ASSET( "0.950 2.28.0" ).amount );
      BOOST_CHECK( bucket->volume_a == ASSET( "0.500 2.28.2" ).amount );
      bucket++;
      BOOST_REQUIRE( bucket != bucket_idx.end() );

      BOOST_CHECK( bucket->asset_a == XUSD_SYMBOL );
      BOOST_CHECK( bucket->asset_b == BTCM_SYMBOL );
      BOOST_CHECK( bucket->seconds == 300 );
      BOOST_CHECK( bucket->start == time_a );
      BOOST_CHECK( bucket->high_b == ASSET( "1.500 2.28.0 " ).amount );
      BOOST_CHECK( bucket->high_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->low_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->low_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->open_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->open_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->close_b == ASSET( "1.500 2.28.0").amount );
      BOOST_CHECK( bucket->close_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->volume_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->volume_a == ASSET( "0.750 2.28.2" ).amount );
      bucket++;
      BOOST_REQUIRE( bucket != bucket_idx.end() );

      BOOST_CHECK( bucket->asset_a == XUSD_SYMBOL );
      BOOST_CHECK( bucket->asset_b == BTCM_SYMBOL );
      BOOST_CHECK( bucket->seconds == 300 );
      BOOST_CHECK( bucket->start == time_a + ( 60 * 90 ) );
      BOOST_CHECK( bucket->high_b == ASSET( "0.450 2.28.0 " ).amount );
      BOOST_CHECK( bucket->high_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->low_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->low_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->open_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->open_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->close_b == ASSET( "0.450 2.28.0").amount );
      BOOST_CHECK( bucket->close_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->volume_b == ASSET( "1.450 2.28.0" ).amount );
      BOOST_CHECK( bucket->volume_a == ASSET( "0.750 2.28.2" ).amount );
      bucket++;
      BOOST_REQUIRE( bucket != bucket_idx.end() );

      BOOST_CHECK( bucket->asset_a == XUSD_SYMBOL );
      BOOST_CHECK( bucket->asset_b == BTCM_SYMBOL );
      BOOST_CHECK( bucket->seconds == 3600 );
      BOOST_CHECK( bucket->start == time_a );
      BOOST_CHECK( bucket->high_b == ASSET( "1.500 2.28.0 " ).amount );
      BOOST_CHECK( bucket->high_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->low_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->low_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->open_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->open_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->close_b == ASSET( "1.500 2.28.0").amount );
      BOOST_CHECK( bucket->close_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->volume_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->volume_a == ASSET( "0.750 2.28.2" ).amount );
      bucket++;
      BOOST_REQUIRE( bucket != bucket_idx.end() );

      BOOST_CHECK( bucket->asset_a == XUSD_SYMBOL );
      BOOST_CHECK( bucket->asset_b == BTCM_SYMBOL );
      BOOST_CHECK( bucket->seconds == 3600 );
      BOOST_CHECK( bucket->start == time_a + ( 60 * 60 ) );
      BOOST_CHECK( bucket->high_b == ASSET( "0.450 2.28.0 " ).amount );
      BOOST_CHECK( bucket->high_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->low_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->low_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->open_b == ASSET( "0.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->open_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->close_b == ASSET( "0.450 2.28.0").amount );
      BOOST_CHECK( bucket->close_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->volume_b == ASSET( "1.450 2.28.0" ).amount );
      BOOST_CHECK( bucket->volume_a == ASSET( "0.750 2.28.2" ).amount );
      bucket++;
      BOOST_REQUIRE( bucket != bucket_idx.end() );

      BOOST_CHECK( bucket->asset_a == XUSD_SYMBOL );
      BOOST_CHECK( bucket->asset_b == BTCM_SYMBOL );
      BOOST_CHECK( bucket->seconds == 86400 );
      BOOST_CHECK( bucket->start == fc::time_point_sec( BTCM_GENESIS_TIME.sec_since_epoch() / 86400 * 86400 ) );
      BOOST_CHECK( bucket->high_b == ASSET( "0.450 2.28.0 " ).amount );
      BOOST_CHECK( bucket->high_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->low_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->low_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->open_b == ASSET( "1.500 2.28.0" ).amount );
      BOOST_CHECK( bucket->open_a == ASSET( "0.750 2.28.2" ).amount );
      BOOST_CHECK( bucket->close_b == ASSET( "0.450 2.28.0").amount );
      BOOST_CHECK( bucket->close_a == ASSET( "0.250 2.28.2" ).amount );
      BOOST_CHECK( bucket->volume_b == ASSET( "2.950 2.28.0" ).amount );
      BOOST_CHECK( bucket->volume_a == ASSET( "1.500 2.28.2" ).amount );
      bucket++;

      BOOST_CHECK( bucket == bucket_idx.end() );

      auto order = order_hist_idx.begin();
      BOOST_REQUIRE( order != order_hist_idx.end() );

      BOOST_CHECK( order->time == fill_order_a_time );
      BOOST_CHECK( order->op.current_owner == "bob" );
      BOOST_CHECK( order->op.current_orderid == 0 );
      BOOST_CHECK( order->op.current_pays == ASSET( "1.500 2.28.0" ) );
      BOOST_CHECK( order->op.open_owner == "alice" );
      BOOST_CHECK( order->op.open_orderid == 0 );
      BOOST_CHECK( order->op.open_pays == ASSET( "0.750 2.28.2" ) );
      order++;
      BOOST_REQUIRE( order != order_hist_idx.end() );

      BOOST_CHECK( order->time == fill_order_b_time );
      BOOST_CHECK( order->op.current_owner == "sam" );
      BOOST_CHECK( order->op.current_orderid == 0 );
      BOOST_CHECK( order->op.current_pays == ASSET( "0.500 2.28.0" ) );
      BOOST_CHECK( order->op.open_owner == "alice" );
      BOOST_CHECK( order->op.open_orderid == 0 );
      BOOST_CHECK( order->op.open_pays == ASSET( "0.250 2.28.2" ) );
      order++;
      BOOST_REQUIRE( order != order_hist_idx.end() );

      BOOST_CHECK( order->time == fill_order_c_time );
      BOOST_CHECK( order->op.current_owner == "alice" );
      BOOST_CHECK( order->op.current_orderid == 0 );
      BOOST_CHECK( order->op.current_pays == ASSET( "0.250 2.28.2" ) );
      BOOST_CHECK( order->op.open_owner == "sam" );
      BOOST_CHECK( order->op.open_orderid == 0 );
      BOOST_CHECK( order->op.open_pays == ASSET( "0.500 2.28.0" ) );
      order++;
      BOOST_REQUIRE( order != order_hist_idx.end() );

      BOOST_CHECK( order->time == fill_order_c_time );
      BOOST_CHECK( order->op.current_owner == "bob" );
      BOOST_CHECK( order->op.current_orderid == 0 );
      BOOST_CHECK( order->op.current_pays == ASSET( "0.450 2.28.0" ) );
      BOOST_CHECK( order->op.open_owner == "alice" );
      BOOST_CHECK( order->op.open_orderid == 0 );
      BOOST_CHECK( order->op.open_pays == ASSET( "0.250 2.28.2" ) );
      order++;

      BOOST_CHECK( order == order_hist_idx.end() );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
