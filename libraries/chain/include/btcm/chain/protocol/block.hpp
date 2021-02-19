#pragma once
#include <btcm/chain/protocol/block_header.hpp>
#include <btcm/chain/protocol/transaction.hpp>

namespace btcm { namespace chain {

   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root()const;
      vector<signed_transaction> transactions;
   };

} } // btcm::chain

FC_REFLECT_DERIVED( btcm::chain::signed_block, (btcm::chain::signed_block_header), (transactions) )
