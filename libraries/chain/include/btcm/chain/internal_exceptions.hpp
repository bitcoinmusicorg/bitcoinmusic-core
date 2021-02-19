#pragma once

#include <fc/exception/exception.hpp>
#include <btcm/chain/exceptions.hpp>

#define BTCM_DECLARE_INTERNAL_EXCEPTION( exc_name, seqnum, msg )  \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      internal_ ## exc_name,                                          \
      btcm::chain::internal_exception,                            \
      3990000 + seqnum,                                               \
      msg                                                             \
      )

namespace btcm { namespace chain {

FC_DECLARE_DERIVED_EXCEPTION( internal_exception, btcm::chain::chain_exception, 3990000, "internal exception" )

BTCM_DECLARE_INTERNAL_EXCEPTION( verify_auth_max_auth_exceeded, 1, "Exceeds max authority fan-out" )
BTCM_DECLARE_INTERNAL_EXCEPTION( verify_auth_account_not_found, 2, "Auth account not found" )

} } // btcm::chain
