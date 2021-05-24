#include <boost/test/unit_test.hpp>

#include <btcm/chain/protocol/ext.hpp>
#include <btcm/chain/account_object.hpp>
#include <btcm/chain/protocol/base_operations.hpp>

#include <btcm/market_history/market_history_api.hpp>

#include "../common/database_fixture.hpp"

using namespace btcm::chain;
using namespace btcm::chain::test;

BOOST_FIXTURE_TEST_SUITE( market_history, clean_database_fixture )

BOOST_AUTO_TEST_CASE( index_test )
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

BOOST_AUTO_TEST_CASE( api_test )
{ try {
   using namespace btcm::market_history;

   auto mh_plugin = app.register_plugin< market_history_plugin >();
   boost::program_options::variables_map options;
   mh_plugin->plugin_set_app( &app );
   mh_plugin->plugin_initialize( options );
   app.enable_plugin( MARKET_HISTORY_PLUGIN_NAME );

   ACTORS( (alice)(bob)(federation) );
   fund( "alice", ASSET( "2000.0 2.28.0" ).amount );
   fund( "bob", ASSET( "2000.0 2.28.0" ).amount );
   fund( "federation", 100 * BTCM_ASSET_CREATION_FEE );
   set_price_feed( price( ASSET( "0.500 2.28.0" ), ASSET( "1.000 2.28.2" ) ) );

   generate_block();
   db.modify( db.get_dynamic_global_properties(), [] ( dynamic_global_property_object& p )
   {
      p.mbd_interest_rate = 0;
   } );
   const witness_schedule_object& wso = witness_schedule_id_type()(db);
   for( const auto& wid : wso.current_shuffled_witnesses )
      db.modify( db.get_witness(wid), [] (witness_object& w) { w.props.mbd_interest_rate = 0; });
   convert( "alice", ASSET( "1000.0 2.28.0" ) );
   convert( "bob", ASSET( "1000.0 2.28.0" ) );

   {
      convert_operation cop;
      cop.owner = "federation";
      cop.amount = asset( 100 * BTCM_ASSET_CREATION_FEE, BTCM_SYMBOL );
      trx.operations.emplace_back(std::move(cop));
      asset_create_operation aco;
      aco.fee = asset( 10 * BTCM_ASSET_CREATION_FEE, XUSD_SYMBOL );
      aco.issuer = "federation";
      aco.symbol = "BTS";
      aco.precision = 5;
      aco.common_options.description = "IOU for BitShares core token";
      trx.operations.emplace_back(aco);
      aco.symbol = "BTC";
      aco.precision = 8;
      aco.common_options.description = "IOU for Bitcoin";
      trx.operations.emplace_back(aco);
      aco.symbol = "ETH";
      aco.precision = 6;
      aco.common_options.description = "IOU for Ether";
      trx.operations.emplace_back(aco);
      trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }

   generate_block();

   const auto& bts = db.get_asset("BTS");
   const auto& btc = db.get_asset("BTC");
   const auto& eth = db.get_asset("ETH");

   for( const auto& acct : { "alice", "bob" } )
   {
      asset_issue_operation aio;
      aio.issuer = "federation";
      aio.issue_to_account = acct;
      aio.asset_to_issue = bts.amount_from_string("100000");
      trx.operations.emplace_back(aio);
      aio.asset_to_issue = btc.amount_from_string("1000");
      trx.operations.emplace_back(aio);
      aio.asset_to_issue = eth.amount_from_string("10000");
      trx.operations.emplace_back(aio);
   }
   sign(trx, federation_private_key);
   PUSH_TX(db, trx);
   trx.clear();

   generate_block();

   for( const auto& acct : { "alice", "bob" } )
   {
      BOOST_CHECK( db.get_balance( acct, BTCM_SYMBOL ) == ASSET( "1000.0 2.28.0" ) );
      BOOST_CHECK( db.get_balance( acct, XUSD_SYMBOL ) == ASSET( "2000.0 2.28.2" ) );
      BOOST_CHECK( db.get_balance( acct, bts.id ) == bts.amount( 10000000000 ) );
      BOOST_CHECK( db.get_balance( acct, btc.id ) == btc.amount( 100000000000 ) );
      BOOST_CHECK( db.get_balance( acct, eth.id ) == eth.amount( 10000000000 ) );
   }

   market_history_api api( btcm::app::api_context( app, "market_history_api", {} ) );

   fc::time_point_sec t0( ( db.head_block_time().sec_since_epoch() / 86400 + 1 ) * 86400 );
   generate_blocks( t0 + fc::seconds( 1800 + 150 + 30 + 6 ) );

   int64_t total_bts = 0;
   int64_t sales = 0;
   std::vector<time_point_sec> times; times.reserve(75);
   for( uint32_t i = 0; i < 3; ++i )
      for( uint32_t steps : { 6, 30, 150, 1800, 43200 } )
         for( uint32_t j = 0; j < 5; ++j )
         {
	    asset bts_amt = bts.amount( 100000 + 100 * ((i << 3) + j) );
	    limit_order_create_operation loc;
	    loc.owner = "alice";
	    loc.orderid = (i << 22) | (steps << 6) | (j << 3); 
	    loc.amount_to_sell = bts_amt;
	    loc.min_to_receive = asset( 1000000, BTCM_SYMBOL );
	    trx.operations.emplace_back( loc );
            ++loc.orderid;
	    loc.min_to_receive = asset( 1000000, XUSD_SYMBOL );
	    trx.operations.emplace_back( loc );
            ++loc.orderid;
	    loc.min_to_receive = btc.amount( 1000 );
	    trx.operations.emplace_back( loc );
            ++loc.orderid;
	    loc.min_to_receive = eth.amount( 10000 );
	    trx.operations.emplace_back( loc );
            trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );
            sign(trx, alice_private_key);
            PUSH_TX(db, trx);
            trx.clear();

            auto order_book = api.get_order_book( "BTCM", "XUSD" );
            BOOST_CHECK( order_book.bids.empty() );
            BOOST_CHECK( order_book.asks.empty() );
            order_book = api.get_order_book( (string)bts.id, "XUSD" );
            BOOST_CHECK( order_book.bids.empty() );
            BOOST_REQUIRE_EQUAL( 1u, order_book.asks.size() );
            BOOST_CHECK( order_book.asks[0].bid == bts_amt );
            BOOST_CHECK( order_book.asks[0].ask == asset( 1000000, XUSD_SYMBOL ) );

	    loc.owner = "bob";
	    loc.orderid = (i << 22) | (steps << 6) | (j << 3); 
	    loc.min_to_receive = bts_amt;
	    loc.amount_to_sell = asset( 1000000, BTCM_SYMBOL );
	    trx.operations.emplace_back( loc );
            ++loc.orderid;
	    loc.amount_to_sell = asset( 1000000, XUSD_SYMBOL );
	    trx.operations.emplace_back( loc );
            ++loc.orderid;
	    loc.amount_to_sell = btc.amount( 1000 );
	    trx.operations.emplace_back( loc );
            ++loc.orderid;
	    loc.amount_to_sell = eth.amount( 10000 );
	    trx.operations.emplace_back( loc );
            sign(trx, bob_private_key);
            PUSH_TX(db, trx);
            trx.clear();

            order_book = api.get_order_book( (string)bts.id, "XUSD" );
            BOOST_CHECK( order_book.bids.empty() );
            BOOST_CHECK( order_book.asks.empty() );

            times.emplace_back( db.head_block_time() );
	    generate_blocks( db.head_block_time() + fc::seconds( steps ) );

	    total_bts += 4 * loc.min_to_receive.amount.value;
	    ++sales;
         }

      BOOST_CHECK_EQUAL( db.get_balance( "alice", BTCM_SYMBOL ).amount.value, ASSET( "1000.0 2.28.0" ).amount.value + sales * 1000000 );
      BOOST_CHECK_EQUAL( db.get_balance( "alice", XUSD_SYMBOL ).amount.value, ASSET( "2000.0 2.28.2" ).amount.value + sales * 1000000 );
      BOOST_CHECK_EQUAL( db.get_balance( "alice", bts.id ).amount.value, 10000000000 - total_bts );
      BOOST_CHECK_EQUAL( db.get_balance( "alice", btc.id ).amount.value, 100000000000 + sales * 1000 );
      BOOST_CHECK_EQUAL( db.get_balance( "alice", eth.id ).amount.value, 10000000000 + sales * 10000 );

      BOOST_CHECK_EQUAL( db.get_balance( "bob", BTCM_SYMBOL ).amount.value, ASSET( "1000.0 2.28.0" ).amount.value - sales * 1000000 );
      BOOST_CHECK_EQUAL( db.get_balance( "bob", XUSD_SYMBOL ).amount.value, ASSET( "2000.0 2.28.2" ).amount.value - sales * 1000000 );
      BOOST_CHECK_EQUAL( db.get_balance( "bob", bts.id ).amount.value, 10000000000 + total_bts );
      BOOST_CHECK_EQUAL( db.get_balance( "bob", btc.id ).amount.value, 100000000000 - sales * 1000 );
      BOOST_CHECK_EQUAL( db.get_balance( "bob", eth.id ).amount.value, 10000000000 - sales * 10000 );

      // test get_market_history_buckets
      auto sizes = api.get_market_history_buckets();
      BOOST_CHECK( sizes == (fc::flat_set<uint32_t>{ 15, 60, 300, 3600, 86400 }) );

      // test get_ticker
      auto ticker = api.get_ticker( "BTCM", "XUSD" );
      BOOST_CHECK( ticker.volume_a == asset( 0, XUSD_SYMBOL ) );
      BOOST_CHECK( ticker.volume_b == asset( 0, BTCM_SYMBOL ) );
      BOOST_CHECK( ticker.latest == price() );
      BOOST_CHECK( ticker.lowest_ask == price() );
      BOOST_CHECK( ticker.highest_bid == price() );
      BOOST_CHECK_EQUAL( 0, ticker.percent_change );
      ticker = api.get_ticker( (string)eth.id, (string)btc.id );
      BOOST_CHECK( ticker.volume_a == asset( 0, btc.id ) );
      BOOST_CHECK( ticker.volume_b == asset( 0, eth.id ) );
      BOOST_CHECK( ticker.latest == price() );
      BOOST_CHECK( ticker.lowest_ask == price() );
      BOOST_CHECK( ticker.highest_bid == price() );
      BOOST_CHECK_EQUAL( 0, ticker.percent_change );
      ticker = api.get_ticker( (string)bts.id, "XUSD" );
      BOOST_CHECK( ticker.volume_a == asset( 1000000, XUSD_SYMBOL ) );
      BOOST_CHECK( ticker.volume_b == bts.amount( 100000 + 100 * ((2 << 3) + 4) ) );
      BOOST_CHECK( ticker.latest == ticker.volume_a / ticker.volume_b );
      BOOST_CHECK( ticker.lowest_ask == price() );
      BOOST_CHECK( ticker.highest_bid == price() );
      BOOST_CHECK_EQUAL( 0, ticker.percent_change );

      // test get_volume
      auto volume = api.get_volume( "BTCM", "XUSD" );
      BOOST_CHECK( volume.volume_a == asset( 0, XUSD_SYMBOL ) );
      BOOST_CHECK( volume.volume_b == asset( 0, BTCM_SYMBOL ) );
      volume = api.get_volume( (string)eth.id, (string)btc.id );
      BOOST_CHECK( volume.volume_a == btc.amount( 0 ) );
      BOOST_CHECK( volume.volume_b == eth.amount( 0 ) );
      volume = api.get_volume( (string)bts.id, "XUSD" );
      BOOST_CHECK( volume.volume_a == asset( 1000000, XUSD_SYMBOL ) );
      BOOST_CHECK( volume.volume_b == bts.amount( 100000 + 100 * ((2 << 3) + 4) ) );
      volume = api.get_volume( "XUSD", (string)bts.id );
      BOOST_CHECK( volume.volume_a == asset( 1000000, XUSD_SYMBOL ) );
      BOOST_CHECK( volume.volume_b == bts.amount( 100000 + 100 * ((2 << 3) + 4) ) );

      // test get_recent_trades
      auto trades = api.get_recent_trades( "BTCM", "XUSD", 10 );
      BOOST_CHECK( trades.empty() );
      trades = api.get_recent_trades( (string)eth.id, (string)btc.id, 10 );
      BOOST_CHECK( trades.empty() );
      trades = api.get_recent_trades( (string)bts.id, "XUSD", 10 );
      BOOST_CHECK_EQUAL( 10u, trades.size() );
      trades = api.get_recent_trades( "2.28.2", "BTS", 80 );
      BOOST_REQUIRE_EQUAL( 75u, trades.size() );
      auto titr = times.rbegin();
      auto itr = trades.begin();
      for( int i = 74; itr != trades.end(); ++itr, ++titr, --i ) {
         BOOST_REQUIRE( titr != times.rend() );
         BOOST_CHECK( itr->date == *titr );
         BOOST_CHECK( itr->current_pays.asset_id == XUSD_SYMBOL );
         BOOST_CHECK_EQUAL( 1000000, itr->current_pays.amount.value );
         BOOST_CHECK( itr->open_pays.asset_id == bts.id );
         BOOST_CHECK_EQUAL( 100000 + 100 * (((i/25) << 3) + (i % 5)), itr->open_pays.amount.value );
      }

      // test get_trade_history
      auto trades2 = api.get_trade_history( "BTCM", "XUSD", trades[67].date, trades[7].date );
      BOOST_CHECK( trades2.empty() );
      trades2 = api.get_trade_history( (string)bts.id, "XUSD", fc::time_point_sec(0), trades[74].date - fc::seconds(1), 10 );
      BOOST_CHECK( trades2.empty() );
      trades2 = api.get_trade_history( (string)bts.id, "XUSD", trades[0].date - fc::hours(2), trades[0].date - fc::seconds(1), 10 );
      BOOST_CHECK( trades2.empty() );
      trades2 = api.get_trade_history( (string)bts.id, "XUSD", trades[0].date + fc::seconds(1), trades[0].date + fc::days(1), 10 );
      BOOST_CHECK( trades2.empty() );
      trades2 = api.get_trade_history( (string)bts.id, "XUSD", trades[67].date, trades[7].date, 10 );
      BOOST_CHECK_EQUAL( 10u, trades2.size() );
      trades2 = api.get_trade_history( (string)bts.id, "XUSD", trades[67].date, trades[7].date );
      BOOST_REQUIRE_EQUAL( 61u, trades2.size() );
      BOOST_CHECK( trades2[0].date == trades[67].date );
      BOOST_CHECK( trades2[60].date == trades[7].date );

      // test get_market_history
      // first trade was at t0 + 1800 + 150 + 30 + 6
      // then 3*(+5*6, +5*30, +5*150, 5*1800, +5*43200)
      int64_t bts_end = 100000 + 100 * ((2 << 3) + 4);
      auto buckets = api.get_market_history( "BTCM", "XUSD", 86400, fc::time_point_sec(0), db.head_block_time() );
      BOOST_CHECK( buckets.empty() );
      buckets = api.get_market_history( (string)bts.id, "XUSD", 86399, fc::time_point_sec(0), db.head_block_time() );
      BOOST_CHECK( buckets.empty() );
      buckets = api.get_market_history( (string)bts.id, "XUSD", 86400, fc::time_point_sec(0), db.head_block_time() );
      BOOST_REQUIRE_EQUAL( 8u, buckets.size() );
      BOOST_CHECK( t0 == buckets.front().start );
      BOOST_CHECK( fc::time_point_sec( t0 + fc::days(7) ) == buckets.back().start );
      BOOST_CHECK( XUSD_SYMBOL == buckets.front().asset_a );
      BOOST_CHECK( bts.id == buckets.front().asset_b );
      BOOST_CHECK_EQUAL( 1000000, buckets.front().open_a.value );
      BOOST_CHECK_EQUAL( 100000, buckets.front().open_b.value );
      BOOST_CHECK_EQUAL( 1000000, buckets.front().high_a.value );
      BOOST_CHECK_EQUAL( 100000, buckets.front().high_b.value );
      BOOST_CHECK_EQUAL( 1000000, buckets.back().close_a.value );
      BOOST_CHECK_EQUAL( bts_end, buckets.back().close_b.value );
      BOOST_CHECK_EQUAL( 1000000, buckets.back().low_a.value );
      BOOST_CHECK_EQUAL( bts_end, buckets.back().low_b.value );
      buckets = api.get_market_history( (string)bts.id, "XUSD", 3600, fc::time_point_sec(0), db.head_block_time() );
      BOOST_REQUIRE( !buckets.empty() );
      BOOST_CHECK( t0 == buckets.front().start );
      BOOST_CHECK( fc::time_point_sec( t0 + fc::days(7) + fc::hours(8) ) == buckets.back().start );
      BOOST_CHECK( XUSD_SYMBOL == buckets.front().asset_a );
      BOOST_CHECK( bts.id == buckets.front().asset_b );
      BOOST_CHECK_EQUAL( 1000000, buckets.front().open_a.value );
      BOOST_CHECK_EQUAL( 100000, buckets.front().open_b.value );
      BOOST_CHECK_EQUAL( 1000000, buckets.front().high_a.value );
      BOOST_CHECK_EQUAL( 100000, buckets.front().high_b.value );
      BOOST_CHECK_EQUAL( 1000000, buckets.back().close_a.value );
      BOOST_CHECK_EQUAL( bts_end, buckets.back().close_b.value );
      BOOST_CHECK_EQUAL( 1000000, buckets.back().low_a.value );
      BOOST_CHECK_EQUAL( bts_end, buckets.back().low_b.value );
      buckets = api.get_market_history( (string)bts.id, "XUSD", 300, fc::time_point_sec(0), db.head_block_time() );
      BOOST_REQUIRE( !buckets.empty() );
      BOOST_CHECK( fc::time_point_sec( t0 + fc::seconds(1800) ) == buckets.front().start );
      BOOST_CHECK( XUSD_SYMBOL == buckets.front().asset_a );
      BOOST_CHECK( bts.id == buckets.front().asset_b );
      BOOST_CHECK_EQUAL( 1000000, buckets.front().open_a.value );
      BOOST_CHECK_EQUAL( 100000, buckets.front().open_b.value );
      BOOST_CHECK_EQUAL( 1000000, buckets.front().high_a.value );
      BOOST_CHECK_EQUAL( 100000, buckets.front().high_b.value );
      BOOST_CHECK_EQUAL( 1000000, buckets.back().close_a.value );
      BOOST_CHECK_EQUAL( bts_end, buckets.back().close_b.value );
      BOOST_CHECK_EQUAL( 1000000, buckets.back().low_a.value );
      BOOST_CHECK_EQUAL( bts_end, buckets.back().low_b.value );
      buckets = api.get_market_history( (string)bts.id, "XUSD", 60, fc::time_point_sec(0), db.head_block_time() );
      BOOST_REQUIRE( !buckets.empty() );
      BOOST_CHECK( fc::time_point_sec( t0 + fc::days(3) ) < buckets.front().start );
      BOOST_CHECK_EQUAL( 1000000, buckets.back().close_a.value );
      BOOST_CHECK_EQUAL( bts_end, buckets.back().close_b.value );
      BOOST_CHECK_EQUAL( 1000000, buckets.back().low_a.value );
      BOOST_CHECK_EQUAL( bts_end, buckets.back().low_b.value );
      buckets = api.get_market_history( (string)bts.id, "XUSD", 15, fc::time_point_sec(0), db.head_block_time() );
      BOOST_REQUIRE( !buckets.empty() );
      BOOST_CHECK( fc::time_point_sec( t0 + fc::days(5) ) < buckets.front().start );
      BOOST_CHECK_EQUAL( 1000000, buckets.back().close_a.value );
      BOOST_CHECK_EQUAL( bts_end, buckets.back().close_b.value );
      BOOST_CHECK_EQUAL( 1000000, buckets.back().low_a.value );
      BOOST_CHECK_EQUAL( bts_end, buckets.back().low_b.value );
      buckets = api.get_market_history( (string)bts.id, "XUSD", 15, fc::time_point_sec(0), t0 + fc::days(5) );
      BOOST_CHECK( buckets.empty() );
      buckets = api.get_market_history( (string)bts.id, "XUSD", 60, fc::time_point_sec(0), t0 + fc::days(3) );
      BOOST_CHECK( buckets.empty() );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
