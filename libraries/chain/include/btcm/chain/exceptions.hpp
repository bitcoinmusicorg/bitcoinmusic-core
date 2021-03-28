#pragma once

#include <fc/exception/exception.hpp>
#include <btcm/chain/protocol/protocol.hpp>

#define BTCM_ASSERT( expr, exc_type, FORMAT, ... )                \
   FC_MULTILINE_MACRO_BEGIN                                           \
   if( !(expr) )                                                      \
      FC_THROW_EXCEPTION( exc_type, FORMAT, __VA_ARGS__ );            \
   FC_MULTILINE_MACRO_END


#define BTCM_DECLARE_OP_BASE_EXCEPTIONS( op_name )                \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _validate_exception,                                 \
      btcm::chain::operation_validate_exception,                  \
      3040000 + 100 * operation::tag< op_name ## _operation >::value, \
      #op_name "_operation validation exception"                      \
      )                                                               \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _evaluate_exception,                                 \
      btcm::chain::operation_evaluate_exception,                  \
      3050000 + 100 * operation::tag< op_name ## _operation >::value, \
      #op_name "_operation evaluation exception"                      \
      )

#define BTCM_DECLARE_OP_VALIDATE_EXCEPTION( exc_name, op_name, seqnum, msg ) \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _ ## exc_name,                                       \
      btcm::chain::op_name ## _validate_exception,                \
      3040000 + 100 * operation::tag< op_name ## _operation >::value  \
         + seqnum,                                                    \
      msg                                                             \
      )

#define BTCM_DECLARE_OP_EVALUATE_EXCEPTION( exc_name, op_name, seqnum, msg ) \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _ ## exc_name,                                       \
      btcm::chain::op_name ## _evaluate_exception,                \
      3050000 + 100 * operation::tag< op_name ## _operation >::value  \
         + seqnum,                                                    \
      msg                                                             \
      )

namespace btcm { namespace chain {

   FC_DECLARE_EXCEPTION( chain_exception, 3000000, "blockchain exception" )
   FC_DECLARE_DERIVED_EXCEPTION( database_query_exception,          btcm::chain::chain_exception, 3010000, "database query exception" )
   FC_DECLARE_DERIVED_EXCEPTION( block_validate_exception,          btcm::chain::chain_exception, 3020000, "block validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,             btcm::chain::chain_exception, 3030000, "transaction validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( operation_validate_exception,      btcm::chain::chain_exception, 3040000, "operation validation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( operation_evaluate_exception,      btcm::chain::chain_exception, 3050000, "operation evaluation exception" )
   FC_DECLARE_DERIVED_EXCEPTION( utility_exception,                 btcm::chain::chain_exception, 3060000, "utility method exception" )
   FC_DECLARE_DERIVED_EXCEPTION( undo_database_exception,           btcm::chain::chain_exception, 3070000, "undo database exception" )
   FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception,        btcm::chain::chain_exception, 3080000, "unlinkable block" )
   FC_DECLARE_DERIVED_EXCEPTION( unknown_hardfork_exception,        btcm::chain::chain_exception, 3090000, "chain attempted to apply unknown hardfork" )

   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_active_auth,            btcm::chain::transaction_exception, 3030001, "missing required active authority" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_owner_auth,             btcm::chain::transaction_exception, 3030002, "missing required owner authority" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_other_auth,             btcm::chain::transaction_exception, 3030003, "missing required other authority" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,                 btcm::chain::transaction_exception, 3030004, "irrelevant signature included" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,                  btcm::chain::transaction_exception, 3030005, "duplicate signature included" )
   FC_DECLARE_DERIVED_EXCEPTION( invalid_committee_approval,        btcm::chain::transaction_exception, 3030006, "committee account cannot directly approve transaction" )
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_fee,                  btcm::chain::transaction_exception, 3030007, "insufficient fee" )
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_basic_auth,           btcm::chain::transaction_exception, 3030008, "missing required basic authority" )

   FC_DECLARE_DERIVED_EXCEPTION( pop_empty_chain,                   btcm::chain::undo_database_exception, 3070001, "there are no blocks to pop" )

   BTCM_DECLARE_OP_BASE_EXCEPTIONS( transfer );
//   BTCM_DECLARE_OP_EVALUATE_EXCEPTION( from_account_not_whitelisted, transfer, 1, "owner mismatch" )

   BTCM_DECLARE_OP_BASE_EXCEPTIONS( account_create );
   BTCM_DECLARE_OP_EVALUATE_EXCEPTION( max_auth_exceeded, account_create, 1, "Exceeds max authority fan-out" )
   BTCM_DECLARE_OP_EVALUATE_EXCEPTION( auth_account_not_found, account_create, 2, "Auth account not found" )

   BTCM_DECLARE_OP_BASE_EXCEPTIONS( account_update );
   BTCM_DECLARE_OP_EVALUATE_EXCEPTION( max_auth_exceeded, account_update, 1, "Exceeds max authority fan-out" )
   BTCM_DECLARE_OP_EVALUATE_EXCEPTION( auth_account_not_found, account_update, 2, "Auth account not found" )


   #define BTCM_RECODE_EXC( cause_type, effect_type ) \
      catch( const cause_type& e ) \
      { throw( effect_type( e.what(), e.get_log() ) ); }

} } // btcm::chain
