#include <btcm/chain/get_config.hpp>
#include <btcm/chain/config.hpp>
#include <btcm/chain/protocol/asset.hpp>
#include <btcm/chain/protocol/types.hpp>
#include <btcm/chain/protocol/version.hpp>

namespace btcm { namespace chain {

fc::variant_object get_config()
{
   fc::mutable_variant_object result;

   result[ "IS_TEST_NET" ] = false;

   result["GRAPHENE_CURRENT_DB_VERSION"] = GRAPHENE_CURRENT_DB_VERSION;
   result["MBD_SYMBOL"] = MBD_SYMBOL;
   result["BTCM_100_PERCENT"] = BTCM_100_PERCENT;
   result["BTCM_1_PERCENT"] = BTCM_1_PERCENT;
   result["BTCM_ADDRESS_PREFIX"] = BTCM_ADDRESS_PREFIX;
   result["BTCM_APR_PERCENT_MULTIPLY_PER_BLOCK"] = BTCM_APR_PERCENT_MULTIPLY_PER_BLOCK_0_2;
   result["BTCM_APR_PERCENT_MULTIPLY_PER_DAY"] = BTCM_APR_PERCENT_MULTIPLY_PER_DAY_0_2;
   result["BTCM_APR_PERCENT_SHIFT_PER_BLOCK"] = BTCM_APR_PERCENT_SHIFT_PER_BLOCK_0_2;
   result["BTCM_APR_PERCENT_SHIFT_PER_DAY"] = BTCM_APR_PERCENT_SHIFT_PER_DAY_0_2;
   result["BTCM_BANDWIDTH_AVERAGE_WINDOW_SECONDS"] = BTCM_BANDWIDTH_AVERAGE_WINDOW_SECONDS;
   result["BTCM_BANDWIDTH_PRECISION"] = BTCM_BANDWIDTH_PRECISION;
   result["BTCM_BLOCKCHAIN_HARDFORK_VERSION"] = BTCM_BLOCKCHAIN_HARDFORK_VERSION;
   result["BTCM_BLOCKCHAIN_VERSION"] = BTCM_BLOCKCHAIN_VERSION;
   result["BTCM_BLOCK_INTERVAL"] = BTCM_BLOCK_INTERVAL;
   result["BTCM_BLOCKS_PER_DAY"] = BTCM_BLOCKS_PER_DAY;
   result["BTCM_BLOCKS_PER_HOUR"] = BTCM_BLOCKS_PER_HOUR;
   result["BTCM_BLOCKS_PER_YEAR"] = BTCM_BLOCKS_PER_YEAR;
   result["BTCM_CHAIN_ID"] = BTCM_CHAIN_ID;
   result["BTCM_CONTENT_APR_PERCENT"] = BTCM_CONTENT_APR_PERCENT;
   result["BTCM_CONVERSION_DELAY"] = fc::variant( BTCM_CONVERSION_DELAY, 1 );
   result["BTCM_CURATE_APR_PERCENT_RESERVE"] = BTCM_CURATE_APR_PERCENT_RESERVE;
   result["BTCM_DEFAULT_SBD_INTEREST_RATE"] = BTCM_DEFAULT_SBD_INTEREST_RATE;
   result["BTCM_FEED_HISTORY_WINDOW"] = BTCM_FEED_HISTORY_WINDOW;
   result["BTCM_FEED_INTERVAL_BLOCKS"] = BTCM_FEED_INTERVAL_BLOCKS;
   result["BTCM_FREE_TRANSACTIONS_WITH_NEW_ACCOUNT"] = BTCM_FREE_TRANSACTIONS_WITH_NEW_ACCOUNT;
   result["BTCM_GENESIS_TIME"] = BTCM_GENESIS_TIME;
   result["BTCM_HARDFORK_REQUIRED_WITNESSES"] = BTCM_HARDFORK_REQUIRED_WITNESSES;
   result["BTCM_INIT_MINER_NAME"] = BTCM_INIT_MINER_NAME;
   result["BTCM_INIT_PUBLIC_KEY_STR"] = BTCM_INIT_PUBLIC_KEY_STR;
   result["BTCM_INIT_SUPPLY"] = BTCM_INIT_SUPPLY;
   result["BTCM_INIT_TIME"] = BTCM_INIT_TIME;
   result["BTCM_IRREVERSIBLE_THRESHOLD"] = BTCM_IRREVERSIBLE_THRESHOLD;
   result["BTCM_MAX_ACCOUNT_NAME_LENGTH"] = BTCM_MAX_ACCOUNT_NAME_LENGTH;
   result["BTCM_MAX_ACCOUNT_WITNESS_VOTES"] = BTCM_MAX_ACCOUNT_WITNESS_VOTES;
   result["BTCM_MAX_ASSET_WHITELIST_AUTHORITIES"] = BTCM_MAX_ASSET_WHITELIST_AUTHORITIES;
   result["BTCM_MAX_AUTHORITY_MEMBERSHIP"] = BTCM_MAX_AUTHORITY_MEMBERSHIP;
   result["BTCM_MAX_BLOCK_SIZE"] = BTCM_MAX_BLOCK_SIZE;
   result["BTCM_MAX_COMMENT_DEPTH"] = BTCM_MAX_COMMENT_DEPTH;
   result["BTCM_MAX_FEED_AGE"] = fc::variant( BTCM_MAX_FEED_AGE, 1 );
   result["BTCM_MAX_INSTANCE_ID"] = BTCM_MAX_INSTANCE_ID;
   result["BTCM_MAX_MEMO_SIZE"] = BTCM_MAX_MEMO_SIZE;
   result["BTCM_MAX_MINERS"] = BTCM_MAX_MINERS;
   result["BTCM_MAX_PROXY_RECURSION_DEPTH"] = BTCM_MAX_PROXY_RECURSION_DEPTH;
   result["BTCM_MAX_RATION_DECAY_RATE"] = BTCM_MAX_RATION_DECAY_RATE;
   result["BTCM_MAX_RESERVE_RATIO"] = BTCM_MAX_RESERVE_RATIO;
   result["BTCM_MAX_RUNNER_WITNESSES"] = BTCM_MAX_RUNNER_WITNESSES;
   result["BTCM_MAX_SHARE_SUPPLY"] = BTCM_MAX_SHARE_SUPPLY;
   result["BTCM_MAX_SIG_CHECK_DEPTH"] = BTCM_MAX_SIG_CHECK_DEPTH;
   result["BTCM_MAX_TIME_UNTIL_EXPIRATION"] = BTCM_MAX_TIME_UNTIL_EXPIRATION;
   result["BTCM_MAX_TRANSACTION_SIZE"] = BTCM_MAX_TRANSACTION_SIZE;
   result["BTCM_MAX_UNDO_HISTORY"] = BTCM_MAX_UNDO_HISTORY;
   result["BTCM_MAX_URL_LENGTH"] = BTCM_MAX_URL_LENGTH;
   result["BTCM_MAX_VOTE_CHANGES"] = BTCM_MAX_VOTE_CHANGES;
   result["BTCM_MAX_VOTED_WITNESSES"] = BTCM_MAX_VOTED_WITNESSES;
   result["BTCM_MAX_WITHDRAW_ROUTES"] = BTCM_MAX_WITHDRAW_ROUTES;
   result["BTCM_MAX_WITNESS_URL_LENGTH"] = BTCM_MAX_WITNESS_URL_LENGTH;
   result["BTCM_MIN_ACCOUNT_CREATION_FEE"] = BTCM_MIN_ACCOUNT_CREATION_FEE;
   result["BTCM_MIN_ACCOUNT_NAME_LENGTH"] = BTCM_MIN_ACCOUNT_NAME_LENGTH;
   result["BTCM_MIN_BLOCK_SIZE_LIMIT"] = BTCM_MIN_BLOCK_SIZE_LIMIT;
   result["BTCM_MIN_CONTENT_REWARD"] = fc::variant( BTCM_MIN_CONTENT_REWARD, 2 );
   result["BTCM_MIN_CURATE_REWARD"] = fc::variant( BTCM_MIN_CURATE_REWARD, 2 );
   result["BTCM_MINER_ACCOUNT"] = BTCM_MINER_ACCOUNT;
   result["BTCM_MINER_PAY_PERCENT"] = BTCM_MINER_PAY_PERCENT;
   result["BTCM_MIN_FEEDS"] = BTCM_MIN_FEEDS;
   result["BTCM_MINING_REWARD"] = fc::variant( BTCM_MINING_REWARD, 2 );
   result["BTCM_MIN_PAYOUT_SBD"] = fc::variant( BTCM_MIN_PAYOUT_SBD, 2 );
   result["BTCM_MIN_PRODUCER_REWARD"] = fc::variant( BTCM_MIN_PRODUCER_REWARD, 2 );
   result["BTCM_MIN_RATION"] = BTCM_MIN_RATION;
   result["BTCM_MIN_TRANSACTION_EXPIRATION_LIMIT"] = BTCM_MIN_TRANSACTION_EXPIRATION_LIMIT;
   result["BTCM_MIN_TRANSACTION_SIZE_LIMIT"] = BTCM_MIN_TRANSACTION_SIZE_LIMIT;
   result["BTCM_MIN_UNDO_HISTORY"] = BTCM_MIN_UNDO_HISTORY;
   result["BTCM_NULL_ACCOUNT"] = BTCM_NULL_ACCOUNT;
   result["BTCM_NUM_INIT_MINERS"] = BTCM_NUM_INIT_MINERS;
   result["BTCM_PRODUCER_APR_PERCENT"] = BTCM_PRODUCER_APR_PERCENT;
   result["BTCM_PROXY_TO_SELF_ACCOUNT"] = BTCM_PROXY_TO_SELF_ACCOUNT;
   result["BTCM_SBD_INTEREST_COMPOUND_INTERVAL_SEC"] = BTCM_SBD_INTEREST_COMPOUND_INTERVAL_SEC;
   result["BTCM_SECONDS_PER_YEAR"] = BTCM_SECONDS_PER_YEAR;
   result["BTCM_REVERSE_AUCTION_WINDOW_SECONDS"] = BTCM_REVERSE_AUCTION_WINDOW_SECONDS;
   result["BTCM_START_MINER_VOTING_BLOCK"] = BTCM_START_MINER_VOTING_BLOCK;
   result["BTCM_START_VESTING_BLOCK"] = BTCM_START_VESTING_BLOCK;
   result["BTCM_SYMBOL"] = BTCM_SYMBOL;
   result["BTCM_TEMP_ACCOUNT"] = BTCM_TEMP_ACCOUNT;
   result["BTCM_UPVOTE_LOCKOUT"] = fc::variant( BTCM_UPVOTE_LOCKOUT, 1 );
   result["BTCM_VESTING_WITHDRAW_INTERVALS"] = BTCM_VESTING_WITHDRAW_INTERVALS;
   result["BTCM_VESTING_WITHDRAW_INTERVAL_SECONDS"] = BTCM_VESTING_WITHDRAW_INTERVAL_SECONDS;
   result["BTCM_VOTE_CHANGE_LOCKOUT_PERIOD"] = BTCM_VOTE_CHANGE_LOCKOUT_PERIOD;
   result["BTCM_VOTE_REGENERATION_SECONDS"] = BTCM_VOTE_REGENERATION_SECONDS;
   result["BTCM_SYMBOL"] = BTCM_SYMBOL;
   result["VESTS_SYMBOL"] = VESTS_SYMBOL;

   return result;
}

} } // btcm::chain
