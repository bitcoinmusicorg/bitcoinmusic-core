#pragma once

#include <btcm/chain/protocol/ext.hpp>
#include <btcm/chain/protocol/types.hpp>
#include <btcm/chain/protocol/authority.hpp>
#include <btcm/chain/protocol/version.hpp>

#include <fc/time.hpp>

namespace btcm { namespace chain {

   struct base_operation
   {
      void get_required_authorities( vector<authority>& )const{}
      void get_required_active_authorities( flat_set<string>& )const{}
      void get_required_basic_authorities(flat_set<string> &)const{}
      void get_required_owner_authorities( flat_set<string>& )const{}
      void get_required_master_content_authorities(flat_set<string> &)const{}
      void get_required_comp_content_authorities(flat_set<string> &)const{}
      void validate()const{}
   };

   typedef static_variant<
      void_t,
      version,              // Normal witness version reporting, for diagnostics and voting
      hardfork_version_vote // Voting for the next hardfork to trigger
      >                                block_header_extensions;

   typedef static_variant<
      void_t
      >                                future_extensions;

   typedef flat_set<block_header_extensions > block_header_extensions_type;
   typedef flat_set<future_extensions> extensions_type;


} } // btcm::chain

FC_REFLECT_TYPENAME( btcm::chain::block_header_extensions )
FC_REFLECT_TYPENAME( btcm::chain::future_extensions )
