/*
 * Copyright (c) 2018 Peertracks, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/test/unit_test.hpp>

#include <btcm/chain/asset_object.hpp>
#include <btcm/chain/protocol/asset_ops.hpp>
#include <btcm/app/database_api.hpp>

#include "../common/database_fixture.hpp"

// testnet only
#define FEE_ASSET_SWITCH_TIME (fc::time_point_sec(1614502800))

using namespace btcm::chain;
using namespace graphene::db;

BOOST_FIXTURE_TEST_SUITE(asset_tests, clean_database_fixture)

BOOST_AUTO_TEST_CASE(create_asset_test)
{ try {
    ACTORS((bob)(federation));

    generate_blocks(FEE_ASSET_SWITCH_TIME + fc::minutes(1));

    fund( "federation", 5000000000 );

    const auto& treasury = db.get_account( BTCM_TREASURY_ACCOUNT );
    BOOST_CHECK_EQUAL( 0, treasury.balance.amount.value );
    BOOST_CHECK_EQUAL( 0, treasury.mbd_balance.amount.value );

    set_price_feed( price( ASSET( "1.000 2.28.0" ), ASSET( "1.000 2.28.2" ) ) );

    trx.clear();
    trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );

    {
        asset_create_operation aco;
        aco.fee = asset(BTCM_ASSET_CREATION_FEE_0_1, BTCM_SYMBOL);
        aco.issuer = "federation";
        aco.symbol = "BTS";
        aco.precision = 5;
        aco.common_options.description = "IOU for BitShares core token";
        trx.operations.emplace_back(std::move(aco));
        sign(trx, federation_private_key);
        PUSH_TX(db, trx);
        trx.clear();
    }
    BOOST_CHECK_EQUAL( BTCM_ASSET_CREATION_FEE_0_1, treasury.balance.amount.value );
    BOOST_CHECK_EQUAL( 0, treasury.mbd_balance.amount.value );

    const asset_object& bts = db.get_asset("BTS");
    BOOST_CHECK_EQUAL(bts.options.max_supply.value, bts.current_supply.value);
    BOOST_CHECK_EQUAL("BTS", bts.symbol_string);
    BOOST_CHECK_EQUAL(5, bts.precision);
    BOOST_CHECK(federation_id == bts.issuer);

    {
        asset_reserve_operation aro;
        aro.payer = "federation";
        aro.amount_to_reserve = bts.amount(bts.options.max_supply - 150);
        trx.operations.emplace_back(std::move(aro));

        transfer_operation to;
        to.from = "federation";
        to.to = "bob";
        to.amount = bts.amount(50);
        trx.operations.emplace_back(std::move(to));
        sign(trx, federation_private_key);
        PUSH_TX(db, trx);
        trx.clear();

        asset_issue_operation aio;
        aio.issuer = "bob";
        aio.asset_to_issue = bts.amount(5000);
        aio.issue_to_account = "bob";
        trx.operations.emplace_back(std::move(aio));
        sign(trx, bob_private_key);
        BOOST_CHECK_THROW(PUSH_TX(db, trx), fc::assert_exception);
        trx.clear();
    }

    asset amount = db.get_balance("federation", bts.id);
    BOOST_CHECK(bts.id == amount.asset_id);
    BOOST_CHECK_EQUAL(100, amount.amount.value);
    amount = db.get_balance("bob", bts.id);
    BOOST_CHECK(bts.id == amount.asset_id);
    BOOST_CHECK_EQUAL(50, amount.amount.value);

    {
        transfer_operation top;
        top.from = "federation";
        top.to = "bob";
        top.amount = bts.amount(10);
        trx.operations.emplace_back(std::move(top));
        sign(trx, federation_private_key);
        PUSH_TX(db, trx);
        trx.clear();
    }

    amount = db.get_balance("federation", bts.id);
    BOOST_CHECK(bts.id == amount.asset_id);
    BOOST_CHECK_EQUAL(90, amount.amount.value);
    amount = db.get_balance("bob", bts.id);
    BOOST_CHECK(bts.id == amount.asset_id);
    BOOST_CHECK_EQUAL(60, amount.amount.value);
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE( flags_test, database_fixture )
{ try {
    initialize_clean( 0 );

    ACTORS((federation)(treasury));
    fund( "federation", 5000000000 );

    set_price_feed( price( ASSET( "1.000 2.28.0" ), ASSET( "1.000 2.28.2" ) ) );

    generate_block();

    trx.clear();
    trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );

    {
	// convert some for asset_create fees
        convert_operation cop;
        cop.owner = "federation";
        cop.amount = asset(BTCM_ASSET_CREATION_FEE, BTCM_SYMBOL);
        trx.operations.emplace_back(std::move(cop));

	// create an asset with classic flags
        asset_create_operation aco;
        aco.fee = asset(BTCM_ASSET_CREATION_FEE, XUSD_SYMBOL);
        aco.issuer = "federation";
        aco.symbol = "BTS";
        aco.precision = 5;
        aco.common_options.description = "IOU for BitShares core token";
        aco.common_options.issuer_permissions = 73;
        aco.common_options.flags = 73;
        trx.operations.emplace_back(std::move(aco));
        sign(trx, federation_private_key);
        PUSH_TX(db, trx);
        trx.clear();
    }

    generate_block();

    db.set_hardfork( BTCM_HARDFORK_0_1, true );

    generate_block();

    // verify classic flags have been unset when applying hf
    const auto& bts = db.get_asset( "BTS" );
    BOOST_CHECK_EQUAL( 0u, bts.options.flags );
    BOOST_CHECK_EQUAL( 0u, bts.options.issuer_permissions );

    {
	// verify classic flags are no longer allowed
        asset_create_operation aco;
        aco.fee = asset(BTCM_ASSET_CREATION_FEE_0_1, BTCM_SYMBOL);
        aco.issuer = "federation";
        aco.symbol = "BTS2";
        aco.precision = 5;
        aco.common_options.description = "IOU for BitShares core token";
        aco.common_options.issuer_permissions = 73;
        aco.common_options.flags = 73;
        trx.operations.emplace_back(std::move(aco));
        sign(trx, federation_private_key);
        BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
        trx.clear();

	// ...but it works without any flags
        aco.common_options.issuer_permissions = 0;
        aco.common_options.flags = 0;
        trx.operations.emplace_back(std::move(aco));
        sign(trx, federation_private_key);
        PUSH_TX(db, trx);
        trx.clear();
    }

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(trade_asset_test)
{ try {
    ACTORS((bob)(federation));
    fund("bob");
    fund( "federation", 5000000000 );
    
    generate_blocks(FEE_ASSET_SWITCH_TIME + fc::minutes(1));

    set_price_feed( price( ASSET( "1.000 2.28.0" ), ASSET( "1.000 2.28.2" ) ) );

    // give bob some fake XUSD
    generate_block();
    db.modify( bob_id(db), [] ( account_object& acct ) {
       acct.mbd_balance.amount = share_type(500000);
    });
    db.modify( db.get_dynamic_global_properties(), [] ( dynamic_global_property_object& gpo ) {
       gpo.current_mbd_supply.amount = share_type(500000);
    });
    db.modify( db.get_feed_history(), [] ( feed_history_object& fho ){
       fho.effective_median_history = fho.actual_median_history = asset(1) / asset(1, XUSD_SYMBOL);
    });

    trx.clear();
    trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );

    {
        asset_create_operation aco;
        aco.fee = asset(BTCM_ASSET_CREATION_FEE_0_1, BTCM_SYMBOL);
        aco.issuer = "federation";
        aco.symbol = "BTS";
        aco.precision = 5;
        aco.common_options.description = "IOU for BitShares core token";
        trx.operations.emplace_back(std::move(aco));
        sign(trx, federation_private_key);
        PUSH_TX(db, trx);
        trx.clear();
    }

    const asset_object& bts = db.get_asset("BTS");
    {
        asset_reserve_operation aro;
        aro.payer = "federation";
        aro.amount_to_reserve = bts.amount(bts.options.max_supply - 500000);
        trx.operations.emplace_back(std::move(aro));
        sign(trx, federation_private_key);
        PUSH_TX(db, trx);
        trx.clear();
    }

    {
        limit_order_create_operation loc;
        loc.owner = "federation";
        loc.amount_to_sell = bts.amount(100000);
        loc.min_to_receive = XUSD_SYMBOL(db).amount(100000);
        trx.operations.emplace_back(std::move(loc));
        sign(trx, federation_private_key);
        PUSH_TX(db, trx);
        trx.clear();
    }

    asset amount = db.get_balance("federation", bts.id);
    BOOST_CHECK(bts.id == amount.asset_id);
    BOOST_CHECK_EQUAL(400000, amount.amount.value);

    {
        limit_order_create_operation loc;
        loc.owner = "bob";
        loc.amount_to_sell = XUSD_SYMBOL(db).amount(200000);
        loc.min_to_receive = bts.amount(200000);
        trx.operations.emplace_back(std::move(loc));
        sign(trx, bob_private_key);
        PUSH_TX(db, trx);
        trx.clear();
    }

    amount = db.get_balance("bob", bts.id);
    BOOST_CHECK(bts.id == amount.asset_id);
    BOOST_CHECK_EQUAL(100000, amount.amount.value);
    amount = db.get_balance("federation", XUSD_SYMBOL);
    BOOST_CHECK(XUSD_SYMBOL == amount.asset_id);
    BOOST_CHECK_EQUAL(100000, amount.amount.value);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(trade_assets_test)
{ try {
    btcm::app::database_api db_api( db );

    ACTORS( (alice)(bob)(federation));
    fund("alice");
    vest("alice", 50000);
    fund("bob", 5000000);
    vest("bob", 50000);
    fund( "federation", 5000000000 );

    generate_blocks(FEE_ASSET_SWITCH_TIME + fc::minutes(1));

    set_price_feed( price( ASSET( "1.000 2.28.0" ), ASSET( "1.000 2.28.2" ) ) );

    trx.clear();
    trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );

    {
        asset_create_operation aco;
        aco.fee = asset(BTCM_ASSET_CREATION_FEE_0_1, BTCM_SYMBOL);
        aco.issuer = "federation";
        aco.symbol = "BTS";
        aco.precision = 5;
        aco.common_options.description = "IOU for BitShares core token";
        trx.operations.emplace_back(aco);
        aco.symbol = "BTC";
        aco.precision = 8;
        aco.common_options.description = "IOU for Bitcoin";
        trx.operations.emplace_back(std::move(aco));
        sign(trx, federation_private_key);
        PUSH_TX(db, trx);
        trx.clear();
    }

    const asset_object& bts = db.get_asset("BTS");
    const asset_object& btc = db.get_asset("BTC");

    {
        asset_reserve_operation aro;
        aro.payer = "federation";
        aro.amount_to_reserve = bts.amount(bts.options.max_supply - 5500000);
        trx.operations.emplace_back(std::move(aro));

        transfer_operation to;
        to.from = "federation";
        to.to = "bob";
        to.amount = bts.amount(5000000);
        trx.operations.emplace_back(to);

        to.to = "alice";
        to.amount = btc.amount(500000);
        trx.operations.emplace_back(to);
        sign(trx, federation_private_key);
        PUSH_TX(db, trx);
        trx.clear();
    }

    {
        limit_order_create_operation loc;
        loc.owner = "alice";
        loc.amount_to_sell = btc.amount(10000);
        loc.min_to_receive = XUSD_SYMBOL(db).amount(30000000);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = btc.amount(11000);
        loc.min_to_receive = XUSD_SYMBOL(db).amount(34000000);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = btc.amount(12000);
        loc.min_to_receive = XUSD_SYMBOL(db).amount(38000000);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = btc.amount(20000);
        loc.min_to_receive = BTCM_SYMBOL(db).amount(3000000000);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = btc.amount(21000);
        loc.min_to_receive = BTCM_SYMBOL(db).amount(3250000000);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = btc.amount(22000);
        loc.min_to_receive = BTCM_SYMBOL(db).amount(3500000000);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = BTCM_SYMBOL(db).amount(1000);
        loc.min_to_receive = XUSD_SYMBOL(db).amount(10);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = BTCM_SYMBOL(db).amount(1100);
        loc.min_to_receive = XUSD_SYMBOL(db).amount(12);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = BTCM_SYMBOL(db).amount(1200);
        loc.min_to_receive = XUSD_SYMBOL(db).amount(14);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        sign(trx, alice_private_key);
        PUSH_TX(db, trx);
        trx.clear();

        loc.owner = "bob";
        loc.amount_to_sell = bts.amount(100000);
        loc.min_to_receive = XUSD_SYMBOL(db).amount(200000);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = bts.amount(110000);
        loc.min_to_receive = XUSD_SYMBOL(db).amount(230000);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = bts.amount(120000);
        loc.min_to_receive = XUSD_SYMBOL(db).amount(260000);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = bts.amount(100000);
        loc.min_to_receive = BTCM_SYMBOL(db).amount(30000);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = bts.amount(110000);
        loc.min_to_receive = BTCM_SYMBOL(db).amount(34000);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = bts.amount(120000);
        loc.min_to_receive = BTCM_SYMBOL(db).amount(38000);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = BTCM_SYMBOL(db).amount(300000);
        loc.min_to_receive = btc.amount(20);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = BTCM_SYMBOL(db).amount(325000);
        loc.min_to_receive = btc.amount(21);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        loc.amount_to_sell = BTCM_SYMBOL(db).amount(350000);
        loc.min_to_receive = btc.amount(22);
        trx.operations.emplace_back(loc);
        loc.orderid++;
        sign(trx, bob_private_key);
        PUSH_TX(db, trx);
        trx.clear();
    }

    auto orderbook = db_api.get_order_book(1000);
    BOOST_CHECK_EQUAL("BTCM", orderbook.base);
    BOOST_CHECK_EQUAL("XUSD", orderbook.quote);
    BOOST_CHECK(orderbook.bids.empty());
    BOOST_REQUIRE_EQUAL(3u, orderbook.asks.size());
    BOOST_CHECK(XUSD_SYMBOL(db).amount(10) / BTCM_SYMBOL(db).amount(1000) == orderbook.asks[0].order_price);
    BOOST_CHECK_EQUAL(10, orderbook.asks[0].quote.value);
    BOOST_CHECK_EQUAL(1000, orderbook.asks[0].base.value);

    orderbook = db_api.get_order_book_for_asset(bts.id, 1000);
    BOOST_CHECK_EQUAL("BTS", orderbook.base);
    BOOST_CHECK_EQUAL("XUSD", orderbook.quote);
    BOOST_CHECK(orderbook.bids.empty());
    BOOST_REQUIRE_EQUAL(3u, orderbook.asks.size());
    BOOST_CHECK(XUSD_SYMBOL(db).amount(200000) / bts.amount(100000) == orderbook.asks[0].order_price);
    BOOST_CHECK_EQUAL(200000, orderbook.asks[0].quote.value);
    BOOST_CHECK_EQUAL(100000, orderbook.asks[0].base.value);

    orderbook = db_api.get_order_book_for_assets(btc.id, BTCM_SYMBOL, 1);
    BOOST_CHECK_EQUAL("BTC", orderbook.base);
    BOOST_CHECK_EQUAL("BTCM", orderbook.quote);
    BOOST_REQUIRE_EQUAL(1u, orderbook.bids.size());
    BOOST_CHECK(BTCM_SYMBOL(db).amount(350000) / btc.amount(22) == orderbook.bids[0].order_price);
    BOOST_CHECK_EQUAL(350000, orderbook.bids[0].quote.value);
    BOOST_CHECK_EQUAL(22, orderbook.bids[0].base.value);
    BOOST_REQUIRE_EQUAL(1u, orderbook.asks.size());
    BOOST_CHECK(BTCM_SYMBOL(db).amount(3000000000) / btc.amount(20000) == orderbook.asks[0].order_price);
    BOOST_CHECK_EQUAL(3000000000, orderbook.asks[0].quote.value);
    BOOST_CHECK_EQUAL(20000, orderbook.asks[0].base.value);

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE( create_hashtag_flag, database_fixture )
{ try {
   initialize_clean( 0 );

   ACTORS( (federation)(treasury) );
   fund( "federation", 5000000000 );
   set_price_feed( price( ASSET( "1.000 2.28.0" ), ASSET( "1.000 2.28.2" ) ) );
   trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );
   trx.clear();

   generate_block();

   BOOST_REQUIRE( !db.has_hardfork( BTCM_HARDFORK_0_1 ) );

   {
      // convert some for asset_create fees
      convert_operation cop;
      cop.owner = "federation";
      cop.amount = asset(BTCM_ASSET_CREATION_FEE, BTCM_SYMBOL);
      trx.operations.emplace_back(std::move(cop));
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();

      // create a pre-hardfork asset with hashtag flag
      asset_create_operation aco;
      aco.fee = asset(BTCM_ASSET_CREATION_FEE, XUSD_SYMBOL);
      aco.issuer = "federation";
      aco.symbol = "PREHF";
      aco.precision = 0;
      aco.common_options.max_supply = 1;
      aco.common_options.description = "Hashtag test";
      aco.common_options.flags = hashtag;
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }

   generate_block();

   db.set_hardfork( BTCM_HARDFORK_0_1, true );

   generate_block();

   const auto& _treasury = db.get_account( BTCM_TREASURY_ACCOUNT );
   BOOST_CHECK_EQUAL( 0, _treasury.balance.amount.value );
   BOOST_CHECK_EQUAL( 0, _treasury.mbd_balance.amount.value );

   // verify flag has been cleared in hf
   const auto& pre = db.get_asset( "PREHF" );
   BOOST_CHECK_EQUAL( 0u, pre.options.flags );
   BOOST_CHECK_EQUAL( 0u, pre.options.issuer_permissions );

   {
      // hashtag create fails with wrong precision / supply
      asset_create_operation aco;
      aco.fee = asset(BTCM_ASSET_CREATION_FEE_0_1, BTCM_SYMBOL);
      aco.issuer = "federation";
      aco.symbol = "HASH";
      aco.precision = 5;
      aco.common_options.max_supply = 100000;
      aco.common_options.description = "Hashtag test";
      aco.common_options.flags = hashtag;
      aco.common_options.issuer_permissions = hashtag;
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // hashtag create proposal fails with wrong precision / supply
      proposal_create_operation pop;
      pop.expiration_time = db.head_block_time() + fc::days(1);
      pop.proposed_ops.emplace_back( aco );
      trx.operations.emplace_back(pop);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // hashtag create fails with wrong supply
      aco.precision = 0;
      aco.common_options.max_supply = 100000;
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // hashtag create proposal fails with wrong supply
      pop.proposed_ops.clear();
      pop.proposed_ops.emplace_back( aco );
      trx.operations.emplace_back(pop);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // hashtag create fails with wrong precision
      aco.precision = 5;
      aco.common_options.max_supply = 1;
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // hashtag create proposal fails with wrong precision
      pop.proposed_ops.clear();
      pop.proposed_ops.emplace_back( aco );
      trx.operations.emplace_back(pop);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // hashtag create works
      aco.precision = 0;
      aco.common_options.max_supply = 1;
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();

      BOOST_CHECK_EQUAL( BTCM_ASSET_CREATION_FEE_0_1, _treasury.balance.amount.value );
      BOOST_CHECK_EQUAL( 0, _treasury.mbd_balance.amount.value );

      // hashtag create proposal works
      pop.proposed_ops.clear();
      pop.proposed_ops.emplace_back( aco );
      trx.operations.emplace_back(pop);
      PUSH_TX(db, trx);
      trx.clear();

      // create without hashtag works with other precision/supply
      aco.symbol = "SOME";
      aco.precision = 5;
      aco.common_options.max_supply = 100000;
      aco.common_options.flags = 0;
      aco.common_options.issuer_permissions = 0;
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( create_subasset )
{ try {

   ACTORS( (alice)(bob)(federation) );
   fund( "alice", 500000000 );
   fund( "bob", 500000000 );
   fund( "federation", 5000000000 );

   set_price_feed( price( ASSET( "1.000 2.28.0" ), ASSET( "1.000 2.28.2" ) ) );

   trx.clear();
   trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );

   {
      // create with hashtag flag
      asset_create_operation aco;
      aco.fee = asset(BTCM_ASSET_CREATION_FEE_0_1, BTCM_SYMBOL);
      aco.issuer = "federation";
      aco.symbol = "FASHION";
      aco.precision = 0;
      aco.common_options.max_supply = 1;
      aco.common_options.issuer_permissions = hashtag;
      aco.common_options.flags = hashtag;
      aco.common_options.description = "IOU for BitShares core token";
      trx.operations.emplace_back(aco);

      // create without hashtag flag
      aco.symbol = "BTC";
      aco.precision = 8;
      aco.common_options.max_supply = 2100000000000;
      aco.common_options.issuer_permissions = 0;
      aco.common_options.flags = 0;
      aco.common_options.description = "IOU for Bitcoin";
      trx.operations.emplace_back(std::move(aco));
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }

   generate_block();

   const auto& fashion = db.get_asset( "FASHION" );

   {
      // alice can't create subasset of FASHION
      asset_create_operation aco;
      aco.fee = asset(BTCM_SUBASSET_CREATION_FEE, BTCM_SYMBOL);
      aco.issuer = "alice";
      aco.symbol = "FASHION.SHIRT";
      aco.common_options.description = "Test sub";
      trx.operations.emplace_back(aco);
      sign(trx, alice_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // federation can create subasset of FASHION
      aco.issuer = "federation";
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();

      // federation transfers FASHION to alice
      transfer_operation top;
      top.from = "federation";
      top.to = "alice";
      top.amount = fashion.amount(1);
      trx.operations.emplace_back(top);
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();

      // now federation can't create subasset of FASHION
      aco.symbol = "FASHION.HAT";
      aco.common_options.description = "Another test sub";
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // ...but alice can
      aco.issuer = "alice";
      trx.operations.emplace_back(aco);

      // alice transfers FASHION to bob
      top.from = "alice";
      top.to = "bob";
      trx.operations.emplace_back(top);
      sign(trx, alice_private_key);
      PUSH_TX(db, trx);
      trx.clear();

      // now alice can't create subasset anymore
      aco.symbol = "FASHION.SHOE";
      aco.common_options.description = "Yet another test sub";
      trx.operations.emplace_back(aco);
      sign(trx, alice_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // ...but bob can
      aco.issuer = "bob";
      trx.operations.emplace_back(aco);
      sign(trx, bob_private_key);
      PUSH_TX(db, trx);
      trx.clear();

      // bob can't create subasset of BTC
      aco.symbol = "BTC.SHOE";
      aco.common_options.description = "BTC test sub";
      trx.operations.emplace_back(aco);
      sign(trx, bob_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // ...but federation can
      aco.issuer = "federation";
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE( allow_subasset_flag, database_fixture )
{ try {
   initialize_clean( 0 );

   ACTORS( (alice)(bob)(federation)(treasury) );
   fund( "alice", 500000000 );
   fund( "bob", 500000000 );
   fund( "federation", 5000000000 );

   set_price_feed( price( ASSET( "1.000 2.28.0" ), ASSET( "1.000 2.28.2" ) ) );

   trx.clear();
   trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );

   {
      // convert some for asset create fee
      convert_operation cop;
      cop.owner = "federation";
      cop.amount = asset(BTCM_ASSET_CREATION_FEE, BTCM_SYMBOL);
      trx.operations.emplace_back(std::move(cop));

      // create PREHF asset with hashtag flag
      asset_create_operation aco;
      aco.fee = asset(BTCM_ASSET_CREATION_FEE, XUSD_SYMBOL);
      aco.issuer = "federation";
      aco.symbol = "PREHF";
      aco.precision = 0;
      aco.common_options.max_supply = 1;
      aco.common_options.issuer_permissions = hashtag | allow_subasset_creation;
      aco.common_options.flags = hashtag;
      aco.common_options.description = "Pre-HF test token";
      trx.operations.emplace_back(std::move(aco));
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();

      const auto& prehf = db.get_asset( "PREHF" );

      // issue PREHF to alice
      asset_issue_operation aio;
      aio.issuer = "federation";
      aio.issue_to_account = "alice";
      aio.asset_to_issue = prehf.amount( 1 );
      trx.operations.emplace_back(std::move(aio));
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();

      // federation can't update
      asset_update_operation auo;
      auo.asset_to_update = prehf.id;
      auo.issuer = "federation";
      auo.new_options.flags = hashtag | allow_subasset_creation;
      trx.operations.emplace_back(auo);
      sign(trx, federation_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // alice can't update
      auo.issuer = "alice";
      trx.operations.emplace_back(auo);
      sign(trx, alice_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // alice can't update through proposal
      proposal_create_operation pop;
      pop.expiration_time = db.head_block_time() + fc::days(1);
      pop.proposed_ops.emplace_back( auo );
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();
   }

   generate_block();

   db.set_hardfork( BTCM_HARDFORK_0_1, true );

   generate_block();
   db.modify( db.get_dynamic_global_properties(), [] ( dynamic_global_property_object& p )
   {
      p.mbd_interest_rate = 0;
   } );

   const auto& _treasury = db.get_account( BTCM_TREASURY_ACCOUNT );
   BOOST_CHECK_EQUAL( 0, _treasury.balance.amount.value );
   BOOST_CHECK_EQUAL( 0, _treasury.mbd_balance.amount.value );

   {
      // create FASHION with hashtag flag and allow_subasset permission
      asset_create_operation aco;
      aco.fee = asset(BTCM_ASSET_CREATION_FEE_0_1, BTCM_SYMBOL);
      aco.issuer = "federation";
      aco.symbol = "FASHION";
      aco.precision = 0;
      aco.common_options.max_supply = 1;
      aco.common_options.issuer_permissions = hashtag | allow_subasset_creation;
      aco.common_options.flags = hashtag;
      aco.common_options.description = "Hashtag test token";
      trx.operations.emplace_back(aco);

      // create HASH with hashtag flag but without allow_subasset permission
      aco.symbol = "HASH";
      aco.common_options.description = "Hashtag test token without subasset";
      aco.common_options.issuer_permissions = hashtag;
      trx.operations.emplace_back(std::move(aco));
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }

   generate_block();

   BOOST_CHECK_EQUAL( 2*BTCM_ASSET_CREATION_FEE_0_1, _treasury.balance.amount.value );
   BOOST_CHECK_EQUAL( 0, _treasury.mbd_balance.amount.value );
   const auto& _alice = db.get_account( "alice" );
   BOOST_CHECK_EQUAL( 500000000, _alice.balance.amount.value );
   BOOST_CHECK_EQUAL( 0, _alice.mbd_balance.amount.value );
   const auto& _bob = db.get_account( "bob" );
   BOOST_CHECK_EQUAL( 500000000, _bob.balance.amount.value );
   BOOST_CHECK_EQUAL( 0, _bob.mbd_balance.amount.value );

   const auto& fashion = db.get_asset( "FASHION" );
   const auto& hash = db.get_asset( "HASH" );
   const auto& pre = db.get_asset( "PREHF" );

   {
      // asset_update proposal can now be created
      asset_update_operation auo;
      auo.asset_to_update = pre.id;
      auo.issuer = "federation";
      auo.new_options.flags = hashtag | allow_subasset_creation;
      proposal_create_operation pop;
      pop.expiration_time = db.head_block_time() + fc::days(1);
      pop.proposed_ops.emplace_back( auo );
      trx.operations.emplace_back(pop);
      PUSH_TX(db, trx);
      trx.clear();

      // issue FASHION and HASH to alice
      transfer_operation to;
      to.from = "federation";
      to.to = "alice";
      to.amount = asset( 1, db.get_asset( "FASHION" ).id );
      trx.operations.emplace_back(to);

      to.amount = asset( 1, db.get_asset( "HASH" ).id );
      trx.operations.emplace_back(to);
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }

   generate_block();

   {
      // bob can't create any subassets
      asset_create_operation aco;
      aco.fee = asset(2*BTCM_SUBASSET_CREATION_FEE, BTCM_SYMBOL);
      aco.issuer = "bob";
      aco.symbol = "FASHION.SHIRT";
      aco.common_options.description = "Test sub";
      trx.operations.emplace_back(aco);
      sign(trx, bob_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      aco.symbol = "HASH.SHIRT";
      trx.operations.emplace_back(aco);
      sign(trx, bob_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      aco.symbol = "PREHF.SHIRT";
      trx.operations.emplace_back(aco);
      sign(trx, bob_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // alice can't update HASH for allow_subasset_create
      asset_update_operation auo;
      auo.issuer = "alice";
      auo.asset_to_update = hash.id;
      auo.new_options = hash.options;
      auo.new_options.flags = hashtag | allow_subasset_creation;
      trx.operations.emplace_back(auo);
      sign(trx, alice_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // alice can update FASHION for allow_subasset_create
      auo.asset_to_update = fashion.id;
      auo.new_options = fashion.options;
      auo.new_options.flags = hashtag | allow_subasset_creation;
      trx.operations.emplace_back(auo);
      sign(trx, alice_private_key);
      PUSH_TX(db, trx);
      trx.clear();

      // bob still can't create subasset with normal fee
      aco.fee = asset(BTCM_SUBASSET_CREATION_FEE, BTCM_SYMBOL);
      aco.symbol = "FASHION.SHIRT";
      trx.operations.emplace_back(aco);
      sign(trx, bob_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // ...but with twice the normal fee it works
      aco.fee = asset(2*BTCM_SUBASSET_CREATION_FEE, BTCM_SYMBOL);
      aco.symbol = "FASHION.SHIRT";
      trx.operations.emplace_back(aco);
      sign(trx, bob_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }

   BOOST_CHECK_EQUAL( 2*BTCM_ASSET_CREATION_FEE_0_1 + BTCM_SUBASSET_CREATION_FEE,
                      _treasury.balance.amount.value );
   BOOST_CHECK_EQUAL( 0, _treasury.mbd_balance.amount.value );
   BOOST_CHECK_EQUAL( 500000000 + BTCM_SUBASSET_CREATION_FEE, _alice.balance.amount.value );
   BOOST_CHECK_EQUAL( 0, _alice.mbd_balance.amount.value );
   BOOST_CHECK_EQUAL( 500000000 - 2*BTCM_SUBASSET_CREATION_FEE, _bob.balance.amount.value );
   BOOST_CHECK_EQUAL( 0, _bob.mbd_balance.amount.value );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( asset_create_fees )
{ try {
   ACTORS( (federation) );
   fund( "federation", 5000000000 );

   set_price_feed( price( ASSET( "1.000 2.28.0" ), ASSET( "2.000 2.28.2" ) ) );

   trx.clear();
   trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );

   {
      // create at current feed price
      asset_create_operation aco;
      aco.fee = asset(BTCM_ASSET_CREATION_FEE_0_1 / 2, BTCM_SYMBOL);
      aco.issuer = "federation";
      aco.symbol = "FASHION";
      aco.common_options.description = "Test main";
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }

   db.modify( db.get_feed_history(), [] (feed_history_object& fho) {
      fho.previous_actual_median = fho.actual_median_history;
      fho.previous_effective_median = fho.effective_median_history;
      fho.actual_median_history = fho.effective_median_history = ASSET( "1.000 2.28.0" ) / ASSET( "1.000 2.28.2" );
   });

   {
      // create at previous feed price
      asset_create_operation aco;
      aco.fee = asset(BTCM_SUBASSET_CREATION_FEE / 2, BTCM_SYMBOL);
      aco.issuer = "federation";
      aco.symbol = "FASHION.SHIRT";
      aco.common_options.description = "Test sub 1";
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }

   db.modify( db.get_feed_history(), [] (feed_history_object& fho) {
      fho.previous_actual_median = fho.actual_median_history;
      fho.previous_effective_median = fho.effective_median_history;
   });

   {
      // create at expired feed price is not possible
      asset_create_operation aco;
      aco.fee = asset(BTCM_SUBASSET_CREATION_FEE / 2, BTCM_SYMBOL);
      aco.issuer = "federation";
      aco.symbol = "FASHION.HAT";
      aco.common_options.description = "Test sub 2";
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();

      // but at current price it is
      aco.fee = asset(BTCM_SUBASSET_CREATION_FEE, BTCM_SYMBOL);
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE( create_nft_sub, database_fixture )
{ try {
   initialize_clean( 0 );

   ACTORS( (alice)(federation)(treasury) );
   fund( "alice", 500000000 );
   fund( "federation", 500000000 );

   set_price_feed( price( ASSET( "1.000 2.28.0" ), ASSET( "2.000 2.28.2" ) ) );

   trx.clear();
   trx.set_expiration( db.head_block_time() + BTCM_MAX_TIME_UNTIL_EXPIRATION );

   {
      // convert some for asset create fee
      convert_operation cop;
      cop.owner = "federation";
      cop.amount = asset(BTCM_ASSET_CREATION_FEE, BTCM_SYMBOL);
      trx.operations.emplace_back(std::move(cop));

      // create NFT
      asset_create_operation aco;
      aco.fee = asset(BTCM_ASSET_CREATION_FEE, XUSD_SYMBOL);
      aco.issuer = "federation";
      aco.symbol = "NFT";
      aco.common_options.description = "NFT main";
      trx.operations.emplace_back(aco);
      sign(trx, federation_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }

   generate_block();

   {
      // convert some for asset create fee
      convert_operation cop;
      cop.owner = "alice";
      cop.amount = asset(BTCM_SUBASSET_CREATION_FEE, BTCM_SYMBOL);
      trx.operations.emplace_back(std::move(cop));

      // alice can't create NFT subasset
      asset_create_operation aco;
      aco.fee = asset(BTCM_SUBASSET_CREATION_FEE, XUSD_SYMBOL);
      aco.issuer = "alice";
      aco.symbol = "NFT.SHIRT";
      aco.common_options.description = "NFT sub";
      trx.operations.emplace_back(aco);
      sign(trx, alice_private_key);
      BOOST_CHECK_THROW( PUSH_TX(db, trx), fc::assert_exception );
      trx.clear();
   }

   generate_block();
   db.set_hardfork( BTCM_HARDFORK_0_1, true );
   generate_block();

   {
      // now she can
      asset_create_operation aco;
      aco.fee = asset(BTCM_SUBASSET_CREATION_FEE, BTCM_SYMBOL);
      aco.issuer = "alice";
      aco.symbol = "NFT.SHIRT";
      aco.common_options.description = "NFT sub";
      trx.operations.emplace_back(aco);
      sign(trx, alice_private_key);
      PUSH_TX(db, trx);
      trx.clear();
   }

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
