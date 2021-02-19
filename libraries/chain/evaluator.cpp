#include <btcm/chain/database.hpp>
#include <btcm/chain/evaluator.hpp>
#include <btcm/chain/exceptions.hpp>
#include <btcm/chain/hardfork.hpp>
#include <btcm/chain/transaction_evaluation_state.hpp>

#include <fc/uint128.hpp>

namespace btcm { namespace chain {
database& generic_evaluator::db()const { return trx_state->db(); }

   void generic_evaluator::start_evaluate( transaction_evaluation_state& eval_state, const operation& op, bool apply )
   { try {
      trx_state   = &eval_state;
      //check_required_authorities(op);
      evaluate( op );
      if( apply ) this->apply( op );
   } FC_CAPTURE_AND_RETHROW() }


} }
