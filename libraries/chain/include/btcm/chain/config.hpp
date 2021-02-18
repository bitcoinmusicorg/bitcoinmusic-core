/*
 * Copyright (c) 2016 Peer Tracks, Inc., and contributors.
 * Copyright (c) 2021 Bitcoin Music
 */
#pragma once

#define BTCM_BLOCKCHAIN_VERSION              ( version(0, 0, 1) )
#define BTCM_BLOCKCHAIN_HARDFORK_VERSION     ( hardfork_version( BTCM_BLOCKCHAIN_VERSION ) )

#define BTCM_INIT_PUBLIC_KEY_STR             "BTST8PNkdK4evQCT5HFfsLjbrpPVgHLgeFoTY5GgBVEJgAwKWWEBWo"
#define BTCM_CHAIN_ID                        (fc::sha256::hash("BitcoinMusic TEST chain"))
#define BASE_SYMBOL                          "BTST"
#define BTCM_ADDRESS_PREFIX                  "BTST"
#define BTCM_SYMBOL_STRING   (BASE_SYMBOL)
#define VESTS_SYMBOL_STRING   "VESTS"
#define MBD_SYMBOL_STRING     "MBD"

#define NULL_SYMBOL  (uint64_t(3) )

#define BTCM_GENESIS_TIME                    (fc::time_point_sec(1613721600))
#define BTCM_VOTE_CHANGE_LOCKOUT_PERIOD      (60*60*2) /// 2 hours

#define BTCM_MIN_ACCOUNT_CREATION_FEE           1

#define BTCM_OWNER_AUTH_RECOVERY_PERIOD                  fc::days(30)
#define BTCM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::days(1)
#define BTCM_OWNER_UPDATE_LIMIT                          fc::minutes(60)

#define BTCM_MIN_ASSET_SYMBOL_LENGTH            2
#define BTCM_MAX_ASSET_SYMBOL_LENGTH            13

#define BTCM_BLOCK_INTERVAL                  3
#define BTCM_BLOCKS_PER_YEAR                 (365*24*60*60/BTCM_BLOCK_INTERVAL) //10512000
#define BTCM_BLOCKS_PER_DAY                  (24*60*60/BTCM_BLOCK_INTERVAL)
#define BTCM_START_VESTING_BLOCK             (BTCM_BLOCKS_PER_DAY * 1)
#define BTCM_START_MINER_VOTING_BLOCK        (BTCM_BLOCKS_PER_DAY * 1)
 
#define BTCM_INIT_MINER_NAME                 "initminer"
#define BTCM_NUM_INIT_MINERS                 3
#define BTCM_INIT_TIME                       (fc::time_point_sec());
#define BTCM_MAX_VOTED_WITNESSES             20
#define BTCM_MAX_RUNNER_WITNESSES            1
#define BTCM_MAX_MINERS (BTCM_MAX_VOTED_WITNESSES+BTCM_MAX_RUNNER_WITNESSES) /// 21 is more than enough
#define BTCM_MAX_VOTED_STREAMING_PLATFORMS      10
#define BTCM_MIN_STREAMING_PLATFORM_CREATION_FEE    (21000000 * 1000000LL)
#define BTCM_ASSET_CREATION_FEE                     (100 * 1000000)
#define BTCM_SUBASSET_CREATION_FEE                  (50 * 1000000)

#define BTCM_HARDFORK_REQUIRED_WITNESSES     17 // 17 of the 20 dpos witnesses (19 elected and 1 virtual time) required for hardfork. This guarantees 75% participation on all subsequent rounds.
#define BTCM_MAX_TIME_UNTIL_EXPIRATION       (60*60) // seconds,  aka: 1 hour
#define BTCM_MAX_MEMO_SIZE                   2048
#define BTCM_MAX_PROXY_RECURSION_DEPTH       4
#define BTCM_VESTING_WITHDRAW_INTERVALS      13
#define BTCM_VESTING_WITHDRAW_INTERVAL_SECONDS (60*60*24*7) /// 1 week per interval
#define BTCM_MAX_WITHDRAW_ROUTES             10
#define BTCM_VOTE_REGENERATION_SECONDS       (5*60*60*24) // 5 day
#define BTCM_MAX_VOTE_CHANGES                5
#define BTCM_UPVOTE_LOCKOUT                  (fc::minutes(1))
#define BTCM_REVERSE_AUCTION_WINDOW_SECONDS  (60*30) /// 30 minutes
#define BTCM_MIN_VOTE_INTERVAL_SEC           3

#define BTCM_MIN_ROOT_COMMENT_INTERVAL       (fc::seconds(60*5)) // 5 minutes
#define BTCM_MIN_REPLY_INTERVAL              (fc::seconds(20)) // 20 seconds
#define BTCM_POST_AVERAGE_WINDOW             (60*60*24u) // 1 day
#define BTCM_POST_MAX_BANDWIDTH              (4*BTCM_100_PERCENT) // 2 posts per 1 days, average 1 every 12 hours
#define BTCM_POST_WEIGHT_CONSTANT            (uint64_t(BTCM_POST_MAX_BANDWIDTH) * BTCM_POST_MAX_BANDWIDTH)

#define BTCM_MAX_ACCOUNT_WITNESS_VOTES       30

#define BTCM_100_PERCENT                     10000
#define BTCM_1_PERCENT                       (BTCM_100_PERCENT/100)
#define BTCM_DEFAULT_SBD_INTEREST_RATE       (10*BTCM_1_PERCENT) ///< 10% APR

#define BTCM_MINER_PAY_PERCENT               (BTCM_1_PERCENT) // 1%
#define BTCM_MIN_RATION                      100000
#define BTCM_MAX_RATION_DECAY_RATE           (1000000)
#define BTCM_FREE_TRANSACTIONS_WITH_NEW_ACCOUNT 100

#define BTCM_BANDWIDTH_AVERAGE_WINDOW_SECONDS (60*60*24*7) ///< 1 week
#define BTCM_BANDWIDTH_PRECISION             uint64_t(1000000) ///< 1 million
#define BTCM_MAX_COMMENT_DEPTH               6

#define BTCM_MAX_RESERVE_RATIO   (20000)


#define BTCM_MINING_REWARD                   asset( 5000, BTCM_SYMBOL )

#define BTCM_MIN_CONTENT_REWARD              BTCM_MINING_REWARD
#define BTCM_MIN_CURATE_REWARD               BTCM_MINING_REWARD
#define BTCM_MIN_PRODUCER_REWARD             BTCM_MINING_REWARD

#define BTCM_ACTIVE_CHALLENGE_FEE            asset( 20000, BTCM_SYMBOL )
#define BTCM_OWNER_CHALLENGE_FEE             asset( 300000, BTCM_SYMBOL )
#define BTCM_ACTIVE_CHALLENGE_COOLDOWN       fc::days(1)
#define BTCM_OWNER_CHALLENGE_COOLDOWN        fc::days(1)

#define BTCM_ASSET_PRECISION                    6
#define BTCM_PAYOUT_TIME_OF_DAY                 2
#define BTCM_MAX_LISTENING_PERIOD               3600

// For the following constants, see comments in compound.hpp and the
// script programs/util/calc_inflation_constants.js

// 5ccc e802 de5f
// int(expm1( log1p( 1 ) / BLOCKS_PER_YEAR ) * 2**BTCM_APR_PERCENT_SHIFT_PER_BLOCK / 100000 + 0.5)
// we use 100000 here instead of 10000 because we end up creating an additional 9x for vesting
#define BTCM_APR_PERCENT_MULTIPLY_PER_BLOCK          ( (uint64_t( 0x5ccc ) << 0x20) \
                                                        | (uint64_t( 0xe802 ) << 0x10) \
                                                        | (uint64_t( 0xde5f )        ) \
                                                        )
// chosen to be the maximal value such that BTCM_APR_PERCENT_MULTIPLY_PER_BLOCK * 2**64 * 100000 < 2**128
#define BTCM_APR_PERCENT_SHIFT_PER_BLOCK             87

#define BTCM_APR_PERCENT_MULTIPLY_PER_BLOCK_0_2      (uint64_t(0x3e214e64a7380ULL))
#define BTCM_APR_PERCENT_SHIFT_PER_BLOCK_0_2         (uint8_t(91))

#define BTCM_APR_PERCENT_MULTIPLY_PER_DAY            ( (uint64_t( 0x1347 ) << 20 ) \
                                                        | (uint64_t( 0xdcd1 ) << 10 ) \
                                                        | (uint64_t( 0x906D )       ) )
#define BTCM_APR_PERCENT_SHIFT_PER_DAY               73

#define BTCM_APR_PERCENT_MULTIPLY_PER_DAY_0_2        (uint64_t(0x369c2966a19c8ULL))
#define BTCM_APR_PERCENT_SHIFT_PER_DAY_0_2           (uint8_t(76))

#define BTCM_CURATE_APR_PERCENT_RESERVE           10
#define BTCM_CONTENT_APR_PERCENT                 712
#define BTCM_CONTENT_APR_PERCENT_0_2            5000
#define BTCM_VESTING_ARP_PERCENT                 143
#define BTCM_VESTING_ARP_PERCENT_0_2            3000
#define BTCM_PRODUCER_APR_PERCENT                 95
#define BTCM_PRODUCER_APR_PERCENT_0_2           2000

#define BTCM_CURATION_THRESHOLD1                1000
#define BTCM_CURATION_THRESHOLD2                2000
#define BTCM_CURATION_DURATION                  (uint64_t(14*24*60*60))


#define BTCM_MIN_PAYOUT_SBD                  (asset(20,MBD_SYMBOL))

#define BTCM_MIN_ACCOUNT_NAME_LENGTH          3
#define BTCM_MAX_ACCOUNT_NAME_LENGTH         16

#define BTCM_CREATE_ACCOUNT_DELEGATION_RATIO    5
#define BTCM_CREATE_ACCOUNT_DELEGATION_TIME     fc::days(30)

#define BTCM_MIN_PERMLINK_LENGTH             0
#define BTCM_MAX_PERMLINK_LENGTH             256
#define BTCM_MAX_WITNESS_URL_LENGTH          2048
#define BTCM_MAX_STREAMING_PLATFORM_URL_LENGTH          2048

#define BTCM_INIT_SUPPLY                     int64_t(0)
#define BTCM_MAX_SHARE_SUPPLY                int64_t(30000000000000ll)
#define BTCM_MAX_SIG_CHECK_DEPTH             2

#define BTCM_MIN_TRANSACTION_SIZE_LIMIT      1024
#define BTCM_SECONDS_PER_YEAR                (uint64_t(60*60*24*365ll))

#define BTCM_SBD_INTEREST_COMPOUND_INTERVAL_SEC  (60*60*24*30)
#define BTCM_MAX_TRANSACTION_SIZE            (1024*64)
#define BTCM_MIN_BLOCK_SIZE_LIMIT            (BTCM_MAX_TRANSACTION_SIZE)
#define BTCM_MAX_BLOCK_SIZE                  (BTCM_MAX_TRANSACTION_SIZE*BTCM_BLOCK_INTERVAL*2000)
#define BTCM_BLOCKS_PER_HOUR                 (60*60/BTCM_BLOCK_INTERVAL)
#define BTCM_FEED_INTERVAL_BLOCKS            (BTCM_BLOCKS_PER_HOUR)
#define BTCM_FEED_HISTORY_WINDOW             (24*7) /// 7 days * 24 hours per day
#define BTCM_MAX_FEED_AGE                    (fc::days(7))
#define BTCM_MIN_FEEDS                       1 ///(BTCM_MAX_MINERS/3) /// protects the network from conversions before price has been established
#define BTCM_CONVERSION_DELAY                (fc::hours( 3 * 24 + 12 ) )

#define BTCM_MIN_UNDO_HISTORY                10
#define BTCM_MAX_UNDO_HISTORY                10000

#define BTCM_MIN_TRANSACTION_EXPIRATION_LIMIT (BTCM_BLOCK_INTERVAL * 5) // 5 transactions per block

#define BTCM_MAX_INSTANCE_ID                 (uint64_t(-1)>>16)
/** NOTE: making this a power of 2 (say 2^15) would greatly accelerate fee calcs */
#define BTCM_MAX_AUTHORITY_MEMBERSHIP        10
#define BTCM_MAX_ASSET_WHITELIST_AUTHORITIES 10
#define BTCM_MAX_URL_LENGTH                  127

#define GRAPHENE_CURRENT_DB_VERSION          "BTCM_0_1_0"

#define BTCM_IRREVERSIBLE_THRESHOLD          (51 * BTCM_1_PERCENT)

/**
 *  Reserved Account IDs with special meaning
 */
///@{
/// Represents the current witnesses
#define BTCM_MINER_ACCOUNT                   "miners"
/// Represents the canonical account with NO authority (nobody can access funds in null account)
#define BTCM_NULL_ACCOUNT                    "null"
/// Represents the canonical account with WILDCARD authority (anybody can access funds in temp account)
#define BTCM_TEMP_ACCOUNT                    "temp"
/// Represents the canonical account for specifying you will vote for directly (as opposed to a proxy)
#define BTCM_PROXY_TO_SELF_ACCOUNT           ""
///@}

#define BTCM_1ST_LEVEL_SCORING_PERCENTAGE 50
#define BTCM_2ST_LEVEL_SCORING_PERCENTAGE 10

#define BTCM_DELEGATION_RETURN_PERIOD     (60*60*24*7)  /// 7 days

#define GRAPHENE_MAX_NESTED_OBJECTS (200)
