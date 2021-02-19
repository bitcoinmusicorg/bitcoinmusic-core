#pragma once
#include <btcm/chain/protocol/operations.hpp>
#include <btcm/chain/protocol/sign_state.hpp>
#include <btcm/chain/protocol/types.hpp>

#include <numeric>

namespace btcm { namespace chain {

   struct transaction
   {
      uint16_t           ref_block_num    = 0;
      uint32_t           ref_block_prefix = 0;

      fc::time_point_sec expiration;

      vector<operation>  operations;
      extensions_type    extensions;

      digest_type         digest()const;
      transaction_id_type id()const;
      void                validate() const;
      digest_type         sig_digest( const chain_id_type& chain_id )const;

      void set_expiration( fc::time_point_sec expiration_time );
      void set_reference_block( const block_id_type& reference_block );

      template<typename Visitor>
      vector<typename Visitor::result_type> visit( Visitor&& visitor )
      {
         vector<typename Visitor::result_type> results;
         for( auto& op : operations )
            results.push_back(op.visit( std::forward<Visitor>( visitor ) ));
         return results;
      }
      template<typename Visitor>
      vector<typename Visitor::result_type> visit( Visitor&& visitor )const
      {
         vector<typename Visitor::result_type> results;
         for( auto& op : operations )
            results.push_back(op.visit( std::forward<Visitor>( visitor ) ));
         return results;
      }

      void get_required_authorities( flat_set<string>& active,
                                     flat_set<string>& owner,
                                     flat_set<string>& basic,
                                     flat_set<string>& master_content,
                                     flat_set<string>& comp_content,
                                     vector<authority>& other )const;
   };

   struct signed_transaction : public transaction
   {
      signed_transaction( const transaction& trx = transaction() )
         : transaction(trx){}

      const signature_type& sign( const private_key_type& key, const chain_id_type& chain_id );

      signature_type sign( const private_key_type& key, const chain_id_type& chain_id )const;

      set<public_key_type> get_required_signatures(
         const chain_id_type& chain_id,
         const flat_set<public_key_type>& available_keys,
         const authority_getter& get_active,
         const authority_getter& get_owner,
         const authority_getter& get_basic,
         const authority_getter& get_master_content,
         const authority_getter& get_comp_content,
         uint32_t max_recursion = BTCM_MAX_SIG_CHECK_DEPTH
         )const;

      void verify_authority(
         const chain_id_type& chain_id,
         const authority_getter& get_active,
         const authority_getter& get_owner,
         const authority_getter& get_basic,
         const authority_getter& get_master_content,
         const authority_getter& get_comp_content,
         uint32_t max_recursion = BTCM_MAX_SIG_CHECK_DEPTH)const;

      set<public_key_type> minimize_required_signatures(
         const chain_id_type& chain_id,
         const flat_set<public_key_type>& available_keys,
         const authority_getter& get_active,
         const authority_getter& get_owner,
         const authority_getter& get_basic,
         const authority_getter& get_master_content,
         const authority_getter& get_comp_content,
         uint32_t max_recursion = BTCM_MAX_SIG_CHECK_DEPTH
         ) const;

      flat_set<public_key_type> get_signature_keys( const chain_id_type& chain_id )const;

      vector<signature_type> signatures;

      digest_type merkle_digest()const;

      void clear() { operations.clear(); signatures.clear(); }
   };

   void verify_authority( const vector<operation>& ops, const flat_set<public_key_type>& sigs,
                          const authority_getter& get_active,
                          const authority_getter& get_owner,
                          const authority_getter& get_basic,
                          const authority_getter& get_master_content,
                          const authority_getter& get_comp_content,
                          bool allow_extra_sigs,
                          uint32_t max_recursion = BTCM_MAX_SIG_CHECK_DEPTH,
                          const flat_set<string>& active_aprovals = flat_set<string>(),
                          const flat_set<string>& owner_aprovals = flat_set<string>(),
                          const flat_set<string>& basic_approvals = flat_set<string>());


   struct annotated_signed_transaction : public signed_transaction {
      annotated_signed_transaction(){}
      annotated_signed_transaction( const signed_transaction& trx )
      :signed_transaction(trx),transaction_id(trx.id()){}

      transaction_id_type transaction_id;
      uint32_t            block_num = 0;
      uint32_t            transaction_num = 0;
   };

 /**
  * @brief captures the result of evaluating the operations contained in the transaction
  *
  * When processing a transaction some operations generate
  * new object IDs and these IDs cannot be known until the
  * transaction is actually included into a block.  When a
  * block is produced these new ids are captured and included
  * with every transaction.  The index in operation_results should
  * correspond to the same index in operations.
  * 
  * If an operation did not create any new object IDs then 0
  * should be returned.
  **/
   struct processed_transaction : public signed_transaction
   {
      processed_transaction( const signed_transaction& trx = signed_transaction() )
         : signed_transaction(trx){}
      vector<operation_result> operation_results;
      digest_type merkle_digest()const;
   };
   /// @} transactions group

} } // btcm::chain

FC_REFLECT( btcm::chain::transaction, (ref_block_num)(ref_block_prefix)(expiration)(operations)(extensions) )
FC_REFLECT_DERIVED( btcm::chain::signed_transaction, (btcm::chain::transaction), (signatures) )
FC_REFLECT_DERIVED( btcm::chain::annotated_signed_transaction, (btcm::chain::signed_transaction), (transaction_id)(block_num)(transaction_num) )
FC_REFLECT_DERIVED( btcm::chain::processed_transaction, (btcm::chain::signed_transaction), (operation_results)); 
