#include <boost/test/unit_test.hpp>

#include <btcm/chain/database.hpp>
#include <btcm/chain/exceptions.hpp>
#include <btcm/chain/hardfork.hpp>
#include <btcm/chain/history_object.hpp>
#include <btcm/chain/base_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

#include <cmath>

using namespace btcm::chain;
using namespace btcm::chain::test;

BOOST_FIXTURE_TEST_SUITE( operation_time_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( vesting_withdrawals )
{
   try
   {
      ACTORS( (alice) )
      fund( "alice", 100000 );
      vest( "alice", 100000 );

      const auto& new_alice = db.get_account( "alice" );

      BOOST_TEST_MESSAGE( "Setting up withdrawal" );

      signed_transaction tx;
      withdraw_vesting_operation op;
      op.account = "alice";
      op.vesting_shares = asset( new_alice.vesting_shares.amount / 2, VESTS_SYMBOL );
      tx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      auto next_withdrawal = db.head_block_time() + BTCM_VESTING_WITHDRAW_INTERVAL_SECONDS;
      asset vesting_shares = new_alice.vesting_shares;
      asset original_vesting = vesting_shares;
      asset withdraw_rate = new_alice.vesting_withdraw_rate;

      BOOST_TEST_MESSAGE( "Generating block up to first withdrawal" );
      generate_blocks( next_withdrawal - ( BTCM_BLOCK_INTERVAL / 2 ), true);

      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).vesting_shares.amount.value, vesting_shares.amount.value );

      BOOST_TEST_MESSAGE( "Generating block to cause withdrawal" );
      generate_block();

      auto fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
      auto gpo = db.get_dynamic_global_properties();

      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).vesting_shares.amount.value, ( vesting_shares - withdraw_rate ).amount.value );
      BOOST_REQUIRE( ( withdraw_rate * gpo.get_vesting_share_price() ).amount.value - db.get_account( "alice" ).balance.amount.value <= 1 ); // Check a range due to differences in the share price
      BOOST_REQUIRE_EQUAL( fill_op.from_account, "alice" );
      BOOST_REQUIRE_EQUAL( fill_op.to_account, "alice" );
      BOOST_REQUIRE_EQUAL( fill_op.withdrawn.amount.value, withdraw_rate.amount.value );
      BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );
      validate_database();

      BOOST_TEST_MESSAGE( "Generating the rest of the blocks in the withdrawal" );

      vesting_shares = db.get_account( "alice" ).vesting_shares;
      auto balance = db.get_account( "alice" ).balance;
      auto old_next_vesting = db.get_account( "alice" ).next_vesting_withdrawal;

      for( int i = 1; i < BTCM_VESTING_WITHDRAW_INTERVALS - 1; i++ )
      {
         generate_blocks( db.head_block_time() + BTCM_VESTING_WITHDRAW_INTERVAL_SECONDS );

         const auto& alice = db.get_account( "alice" );

         gpo = db.get_dynamic_global_properties();
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();

         BOOST_REQUIRE_EQUAL( alice.vesting_shares.amount.value, ( vesting_shares - withdraw_rate ).amount.value );
         BOOST_REQUIRE( balance.amount.value + ( withdraw_rate * gpo.get_vesting_share_price() ).amount.value - alice.balance.amount.value <= 1 );
         BOOST_REQUIRE_EQUAL( fill_op.from_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.to_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.withdrawn.amount.value, withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );

         if ( i == BTCM_VESTING_WITHDRAW_INTERVALS - 1 )
            BOOST_REQUIRE( alice.next_vesting_withdrawal == fc::time_point_sec::maximum() );
         else
            BOOST_REQUIRE_EQUAL( alice.next_vesting_withdrawal.sec_since_epoch(), ( old_next_vesting + BTCM_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );

         validate_database();

         vesting_shares = alice.vesting_shares;
         balance = alice.balance;
         old_next_vesting = alice.next_vesting_withdrawal;
      }

      if (  original_vesting.amount.value % withdraw_rate.amount.value != 0 )
      {
         BOOST_TEST_MESSAGE( "Generating one more block to take care of remainder" );
         generate_blocks( db.head_block_time() + BTCM_VESTING_WITHDRAW_INTERVAL_SECONDS, true );
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();

         BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch(), ( old_next_vesting + BTCM_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );
         BOOST_REQUIRE_EQUAL( fill_op.from_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.to_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.withdrawn.amount.value, withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );

         generate_blocks( db.head_block_time() + BTCM_VESTING_WITHDRAW_INTERVAL_SECONDS, true );
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();

         BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch(), ( old_next_vesting + BTCM_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );
         BOOST_REQUIRE_EQUAL( fill_op.to_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.from_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.withdrawn.amount.value, original_vesting.amount.value % withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );

         validate_database();
      }
      else
      {
         generate_blocks( db.head_block_time() + BTCM_VESTING_WITHDRAW_INTERVAL_SECONDS, true );

         BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch(), fc::time_point_sec::maximum().sec_since_epoch() );

         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
         BOOST_REQUIRE_EQUAL( fill_op.from_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.to_account, "alice" );
         BOOST_REQUIRE_EQUAL( fill_op.withdrawn.amount.value, withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );
      }

      BOOST_REQUIRE_EQUAL( db.get_account( "alice" ).vesting_shares.amount.value, ( original_vesting - op.vesting_shares ).amount.value );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( vesting_withdraw_route )
{
   try
   {
      ACTORS( (alice)(bob)(sam) )

      auto original_vesting = alice.vesting_shares;

      fund( "alice", 1040000 );
      vest( "alice", 1040000 );

      auto withdraw_amount = alice.vesting_shares - original_vesting;

      BOOST_TEST_MESSAGE( "Setup vesting withdraw" );
      withdraw_vesting_operation wv;
      wv.account = "alice";
      wv.vesting_shares = withdraw_amount;

      signed_transaction tx;
      tx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( wv );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      BOOST_TEST_MESSAGE( "Setting up bob destination" );
      set_withdraw_vesting_route_operation op;
      op.from_account = "alice";
      op.to_account = "bob";
      op.percent = BTCM_1_PERCENT * 50;
      op.auto_vest = true;
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "Setting up sam destination" );
      op.to_account = "sam";
      op.percent = BTCM_1_PERCENT * 30;
      op.auto_vest = false;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Setting up first withdraw" );

      auto vesting_withdraw_rate = alice.vesting_withdraw_rate;
      auto old_alice_balance = alice.balance;
      auto old_alice_vesting = alice.vesting_shares;
      auto old_bob_balance = bob.balance;
      auto old_bob_vesting = bob.vesting_shares;
      auto old_sam_balance = sam.balance;
      auto old_sam_vesting = sam.vesting_shares;
      generate_blocks( alice.next_vesting_withdrawal, true );

      {
         const auto& alice = db.get_account( "alice" );
         const auto& bob = db.get_account( "bob" );
         const auto& sam = db.get_account( "sam" );

         BOOST_REQUIRE( alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate );
         BOOST_REQUIRE( alice.balance == old_alice_balance + asset( ( vesting_withdraw_rate.amount * BTCM_1_PERCENT * 20 ) / BTCM_100_PERCENT, VESTS_SYMBOL ) * db.get_dynamic_global_properties().get_vesting_share_price() );
         BOOST_REQUIRE( bob.vesting_shares == old_bob_vesting + asset( ( vesting_withdraw_rate.amount * BTCM_1_PERCENT * 50 ) / BTCM_100_PERCENT, VESTS_SYMBOL ) );
         BOOST_REQUIRE( bob.balance == old_bob_balance );
         BOOST_REQUIRE( sam.vesting_shares == old_sam_vesting );
         BOOST_REQUIRE( sam.balance ==  old_sam_balance + asset( ( vesting_withdraw_rate.amount * BTCM_1_PERCENT * 30 ) / BTCM_100_PERCENT, VESTS_SYMBOL ) * db.get_dynamic_global_properties().get_vesting_share_price() );

         old_alice_balance = alice.balance;
         old_alice_vesting = alice.vesting_shares;
         old_bob_balance = bob.balance;
         old_bob_vesting = bob.vesting_shares;
         old_sam_balance = sam.balance;
         old_sam_vesting = sam.vesting_shares;
      }

      BOOST_TEST_MESSAGE( "Test failure with greater than 100% destination assignment" );

      tx.operations.clear();
      tx.signatures.clear();

      op.to_account = "sam";
      op.percent = BTCM_1_PERCENT * 50 + 1;
      tx.operations.push_back( op );
      tx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      BTCM_REQUIRE_THROW( db.push_transaction( tx, 0 ), fc::assert_exception );

      BOOST_TEST_MESSAGE( "Test from_account receiving no withdraw" );

      tx.operations.clear();
      tx.signatures.clear();

      op.to_account = "sam";
      op.percent = BTCM_1_PERCENT * 50;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.get_account( "alice" ).next_vesting_withdrawal, true );
      {
         const auto& alice = db.get_account( "alice" );
         const auto& bob = db.get_account( "bob" );
         const auto& sam = db.get_account( "sam" );

         BOOST_REQUIRE( alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate );
         BOOST_REQUIRE( alice.balance == old_alice_balance );
         BOOST_REQUIRE( bob.vesting_shares == old_bob_vesting + asset( ( vesting_withdraw_rate.amount * BTCM_1_PERCENT * 50 ) / BTCM_100_PERCENT, VESTS_SYMBOL ) );
         BOOST_REQUIRE( bob.balance == old_bob_balance );
         BOOST_REQUIRE( sam.vesting_shares == old_sam_vesting );
         BOOST_REQUIRE( sam.balance ==  old_sam_balance + asset( ( vesting_withdraw_rate.amount * BTCM_1_PERCENT * 50 ) / BTCM_100_PERCENT, VESTS_SYMBOL ) * db.get_dynamic_global_properties().get_vesting_share_price() );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( feed_publish_mean )
{
   try
   {
      ACTORS( (alice0)(alice1)(alice2)(alice3)(alice4)(alice5)(alice6) )

      BOOST_TEST_MESSAGE( "Setup" );

      generate_blocks( 30 / BTCM_BLOCK_INTERVAL );

      vector< string > accounts;
      accounts.push_back( "alice0" );
      accounts.push_back( "alice1" );
      accounts.push_back( "alice2" );
      accounts.push_back( "alice3" );
      accounts.push_back( "alice4" );
      accounts.push_back( "alice5" );
      accounts.push_back( "alice6" );

      vector< private_key_type > keys;
      keys.push_back( alice0_private_key );
      keys.push_back( alice1_private_key );
      keys.push_back( alice2_private_key );
      keys.push_back( alice3_private_key );
      keys.push_back( alice4_private_key );
      keys.push_back( alice5_private_key );
      keys.push_back( alice6_private_key );

      vector< feed_publish_operation > ops;
      vector< signed_transaction > txs;

      // Upgrade accounts to witnesses
      for( int i = 0; i < 7; i++ )
      {
         transfer( BTCM_INIT_MINER_NAME, accounts[i], 10000 );
         witness_create( accounts[i], keys[i], "foo.bar", keys[i].get_public_key(), 1000 );

         ops.push_back( feed_publish_operation() );
         ops[i].publisher = accounts[i];

         txs.push_back( signed_transaction() );
      }

      ops[0].exchange_rate = price( asset( 100000, BTCM_SYMBOL ), asset( 1000, XUSD_SYMBOL ) );
      ops[1].exchange_rate = price( asset( 105000, BTCM_SYMBOL ), asset( 1000, XUSD_SYMBOL ) );
      ops[2].exchange_rate = price( asset(  98000, BTCM_SYMBOL ), asset( 1000, XUSD_SYMBOL ) );
      ops[3].exchange_rate = price( asset(  97000, BTCM_SYMBOL ), asset( 1000, XUSD_SYMBOL ) );
      ops[4].exchange_rate = price( asset(  99000, BTCM_SYMBOL ), asset( 1000, XUSD_SYMBOL ) );
      ops[5].exchange_rate = price( asset(  97500, BTCM_SYMBOL ), asset( 1000, XUSD_SYMBOL ) );
      ops[6].exchange_rate = price( asset( 102000, BTCM_SYMBOL ), asset( 1000, XUSD_SYMBOL ) );

      for( int i = 0; i < 7; i++ )
      {
         txs[i].set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );
         txs[i].operations.push_back( ops[i] );
         txs[i].sign( keys[i], db.get_chain_id() );
         db.push_transaction( txs[i], 0 );
      }

      BOOST_TEST_MESSAGE( "Jump forward an hour" );

      generate_blocks( BTCM_BLOCKS_PER_HOUR ); // Jump forward 1 hour
      BOOST_TEST_MESSAGE( "Get feed history object" );
      feed_history_object feed_history = db.get_feed_history();
      BOOST_TEST_MESSAGE( "Check state" );
      BOOST_REQUIRE( feed_history.actual_median_history == price( asset( 99000, BTCM_SYMBOL), asset( 1000, XUSD_SYMBOL ) ) );
      BOOST_REQUIRE( feed_history.effective_median_history == price( asset( 99000, BTCM_SYMBOL), asset( 1000, XUSD_SYMBOL ) ) );
      BOOST_REQUIRE( feed_history.price_history[ 0 ] == price( asset( 99000, BTCM_SYMBOL), asset( 1000, XUSD_SYMBOL ) ) );
      validate_database();

      for ( int i = 0; i < 23; i++ )
      {
         BOOST_TEST_MESSAGE( "Updating ops" );

         for( int j = 0; j < 7; j++ )
         {
            txs[j].operations.clear();
            txs[j].signatures.clear();
            ops[j].exchange_rate = price( ops[j].exchange_rate.base, asset( ops[j].exchange_rate.quote.amount + 10, XUSD_SYMBOL ) );
            txs[j].set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );
            txs[j].operations.push_back( ops[j] );
            txs[j].sign( keys[j], db.get_chain_id() );
            db.push_transaction( txs[j], 0 );
         }

         BOOST_TEST_MESSAGE( "Generate Blocks" );

         generate_blocks( BTCM_BLOCKS_PER_HOUR  ); // Jump forward 1 hour

         BOOST_TEST_MESSAGE( "Check feed_history" );

         feed_history = feed_history_id_type()( db );
         BOOST_REQUIRE( feed_history.actual_median_history == feed_history.price_history[ ( i + 1 ) / 2 ] );
         BOOST_REQUIRE( feed_history.effective_median_history == feed_history.price_history[ ( i + 1 ) / 2 ] );
         BOOST_REQUIRE( feed_history.price_history[ i + 1 ] == ops[4].exchange_rate );
         validate_database();
      }
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( tx_rate_limit )
{ try {
   ACTORS( (alice)(bobby)(charlie) )

   fund( "alice", 10000000 );
   vest( "alice", 1000000 );
   fund( "bobby", 10000000 );
   fund( "charlie", 100000000 );
   vest( "charlie", 100000000 );

   generate_block();

   const dynamic_global_property_object& _dgp = dynamic_global_property_id_type(0)(db);
   db.modify( _dgp, []( dynamic_global_property_object& dgp ) {
      dgp.max_virtual_bandwidth = ( 5000 * BTCM_BANDWIDTH_PRECISION * BTCM_BANDWIDTH_AVERAGE_WINDOW_SECONDS )
                                  / BTCM_BLOCK_INTERVAL;
   });

   signed_transaction tx;
   tx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );

   delegate_vesting_shares_operation dvs;
   dvs.delegator = "charlie";
   dvs.delegatee = "bobby";
   dvs.vesting_shares = asset( alice_id(db).vesting_shares.amount.value / 2 - bobby_id(db).vesting_shares.amount.value, VESTS_SYMBOL );
   tx.operations.push_back( dvs );
   tx.sign( charlie_private_key, db.get_chain_id() );
   db.push_transaction( tx, 0 );
   tx.clear();

   transfer_operation op;
   op.from = "alice";
   op.to = "charlie";
   op.amount = asset( 1, BTCM_SYMBOL );
   op.memo = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
   for( int i = 0; i < 100; i++ )
      tx.operations.push_back( op );
   int alice_count = 0;
   try {
      while( alice_count++ < 100000 ) {
         tx.set_expiration( db.head_block_time() + 60 + (alice_count & 0x7ff) );
         db.push_transaction( tx, database::skip_transaction_signatures );
         if( !(alice_count & 0x7ff) )
         {
            op.amount.amount.value++;
            tx.operations[0] = op;
         }
      }
   } catch( fc::assert_exception& e ) {
       BOOST_REQUIRE( e.to_detail_string().find( "bandwidth" ) != string::npos );
       alice_count--;
   }
   tx.clear();
   BOOST_CHECK_LT( 10, alice_count );

   op.from = "bobby";
   op.amount = asset( 1, BTCM_SYMBOL );
   for( int i = 0; i < 100; i++ )
      tx.operations.push_back( op );
   int bobby_count = 0;
   try {
      while( bobby_count++ < 100000 ) {
         tx.set_expiration( db.head_block_time() + 60 + (bobby_count & 0x7ff) );
         db.push_transaction( tx, database::skip_transaction_signatures );
         if( !(bobby_count & 0x7ff) )
         {
            op.amount.amount.value++;
            tx.operations[0] = op;
         }
      }
   } catch( fc::assert_exception& e ) {
       BOOST_REQUIRE( e.to_detail_string().find( "bandwidth" ) != string::npos );
       bobby_count--;
   }
   tx.clear();

   // bobby has half as many VESTS as alice, so should have about half the bandwidth
   BOOST_CHECK_EQUAL( alice_count / 2, bobby_count );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
