/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
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
#include <btcm/chain/base_evaluator.hpp>
#include <btcm/chain/content_object.hpp>
#include <btcm/chain/proposal_object.hpp>
#include <btcm/chain/account_object.hpp>
#include <btcm/chain/exceptions.hpp>
#include <btcm/chain/database.hpp>
#include <fc/smart_ref_impl.hpp>
#include <boost/container/detail/flat_tree.hpp>

namespace btcm { namespace chain {
using namespace graphene::db;


namespace impl {
   class proposal_op_visitor {
      public:
         typedef void result_type;

         explicit proposal_op_visitor( database& db ) : _db(db) {}

         template<typename T>
         void operator()( const T& v )const { /* do nothing by default */ }

         void operator()( const btcm::chain::asset_create_operation& op )const {
            if( _db.has_hardfork( BTCM_HARDFORK_0_1 ) )
            {
               FC_ASSERT( !(op.common_options.issuer_permissions & ~ALLOWED_ASSET_PERMISSIONS),
                          "Disallowed permissions detected!" );
               FC_ASSERT( !(op.common_options.flags & ~ALLOWED_ASSET_PERMISSIONS),
                          "Disallowed flags detected!" );
               if( (op.common_options.flags & hashtag) || (op.common_options.issuer_permissions & hashtag) )
               {
                  FC_ASSERT( op.precision == 0 && op.common_options.max_supply == 1,
                             "hashtag flag requires precision 0 and max_supply 1" );
                  FC_ASSERT( (op.common_options.flags & hashtag) || !(op.common_options.flags & allow_subasset_creation),
                             "allow_subasset_creation flag requires hashtag" );
                  FC_ASSERT( (op.common_options.issuer_permissions & hashtag)
                             || !(op.common_options.issuer_permissions & allow_subasset_creation),
                             "allow_subasset_creation permission requires hashtag" );
               }
               else
               {
                  FC_ASSERT( !(op.common_options.flags & allow_subasset_creation)
                             && !(op.common_options.issuer_permissions & allow_subasset_creation),
                             "allow_subasset_creation flag/permission requires hashtag" );
               }
               return;
            }
         }

         void operator()( const btcm::chain::proposal_create_operation& v )const {
            bool proposal_update_seen = false;
            for (const op_wrapper &op : v.proposed_ops)
            {
               op.op.visit(*this);
               if( op.op.which() == operation::tag<proposal_update_operation>().value )
               {
                  FC_ASSERT( !proposal_update_seen, "At most one proposal update can be nested in a proposal!" );
                  proposal_update_seen = true;
               }
            }
         }
      private:
         database& _db;
   };

   // BitShares issue #1479: disallow updating/deleting future proposals
   class hardfork_visitor_1479
   {
   public:
      typedef void result_type;

      uint64_t max_update_instance = 0;
      uint64_t nested_update_count = 0;

      template<typename T>
      void operator()(const T &v) const {}

      void operator()(const proposal_delete_operation &v)
      {
         if( nested_update_count == 0 || v.proposal.instance.value > max_update_instance )
            max_update_instance = v.proposal.instance.value;
         nested_update_count++;
      }

      void operator()(const proposal_update_operation &v)
      {
         if( nested_update_count == 0 || v.proposal.instance.value > max_update_instance )
            max_update_instance = v.proposal.instance.value;
         nested_update_count++;
      }

      // loop and self visit in proposals
      void operator()(const btcm::chain::proposal_create_operation &v)
      {
         for (const op_wrapper &op : v.proposed_ops)
            op.op.visit(*this);
      }
   };
}

struct authority_collector {
   authority_collector( database& db, flat_set<string>& dest )
      : _db(db), _dest(dest) {}

   void collect( const authority& auth, uint32_t depth = 0 )
   {
      for( const auto& a : auth.account_auths )
      {
         const authority& parent = _db.get_account( a.first ).active;
         if( parent.key_auths.size() > 0 )
            _dest.emplace( a.first );
         if( depth < BTCM_MAX_SIG_CHECK_DEPTH )
            collect( parent, depth + 1 );
      }
   }

   database& _db;
   flat_set<string>& _dest;
};

void proposal_create_evaluator::do_apply(const proposal_create_operation& o)
{ try {
   database& d = db();

   btcm::chain::impl::proposal_op_visitor vtor(d);
   vtor( o );
   impl::hardfork_visitor_1479 vtor_1479;
   vtor_1479( o );

   transaction _proposed_trx;
   const auto& global_parameters = d.get_dynamic_global_properties();

   FC_ASSERT( o.expiration_time > d.head_block_time(), "Proposal has already expired on creation." );
   FC_ASSERT( o.expiration_time <= d.head_block_time() + global_parameters.maximum_proposal_lifetime,
              "Proposal expiration time is too far in the future.");
   FC_ASSERT( !o.review_period_seconds || fc::seconds(*o.review_period_seconds) < (o.expiration_time - d.head_block_time()),
              "Proposal review period must be less than its overall lifetime." );
   for( const op_wrapper& op : o.proposed_ops )
   {
      FC_ASSERT( !is_proposal_operation( op.op ), "Cannot propose a proposal");
      _proposed_trx.operations.push_back(op.op);
   }
   _proposed_trx.validate();

   dlog( "Proposal: ${op}", ("op",o) );

   d.create<proposal_object>([&o, &_proposed_trx, &d, &vtor_1479](proposal_object& proposal) {
      FC_ASSERT( vtor_1479.nested_update_count == 0 || proposal.id.instance() > vtor_1479.max_update_instance,
                 "Cannot update/delete a proposal with a future id!" );
      _proposed_trx.expiration = o.expiration_time;
      proposal.proposed_transaction = _proposed_trx;
      proposal.expiration_time = o.expiration_time;
      if( o.review_period_seconds )
         proposal.review_period_time = d.head_block_time() + *o.review_period_seconds;

      //Populate the required approval sets
      flat_set<string> required_active;
      vector<authority> other;

      for( auto& op : _proposed_trx.operations )
         operation_get_required_authorities(op, required_active, proposal.required_owner_approvals,
                                            proposal.required_basic_approvals,
                                            proposal.required_master_content_approvals,
                                            proposal.required_comp_content_approvals, other);

      proposal.can_veto.insert( required_active.begin(), required_active.end() );
      proposal.can_veto.insert( proposal.required_owner_approvals.begin(), proposal.required_owner_approvals.end() );
      proposal.can_veto.insert( proposal.required_basic_approvals.begin(), proposal.required_basic_approvals.end() );

      // Active or owner authorities also cover basic authority
      for( const string& a : required_active )
         proposal.required_basic_approvals.erase( a );
      for( const string& o : proposal.required_owner_approvals )
         proposal.required_basic_approvals.erase( o );

      FC_ASSERT( other.size() == 0, "Cannot propose operations that require other authority!" );

      FC_ASSERT( proposal.required_basic_approvals.size() == 0
                 || ( required_active.size() == 0
                      && proposal.required_owner_approvals.size() == 0
                      && proposal.required_master_content_approvals.size() == 0
                      && proposal.required_comp_content_approvals.size() == 0
                      && other.size() == 0 ),
                 "Cannot combine operations with basic approval and others!" );

      authority_collector collector( d, required_active );
      for( const string& url : proposal.required_master_content_approvals )
         collector.collect( d.get_content(url).manage_master );
      for( const string& url : proposal.required_comp_content_approvals )
         collector.collect( d.get_content(url).manage_comp );

      //All accounts which must provide both owner and active authority should be omitted from the active authority set
      //owner authority approval implies active authority approval.
      std::set_difference(required_active.begin(), required_active.end(),
                          proposal.required_owner_approvals.begin(), proposal.required_owner_approvals.end(),
                          std::inserter(proposal.required_active_approvals, proposal.required_active_approvals.begin()));

      dlog( "Proposal: ${p}", ("p",proposal) );
   });

} FC_CAPTURE_AND_RETHROW( (o) ) }

void proposal_update_evaluator::do_apply(const proposal_update_operation& o)
{ try {
   database& d = db();
   auto _proposal = &o.proposal(d);
   if( _proposal->review_period_time && d.head_block_time() <= *_proposal->review_period_time )
      FC_ASSERT( o.active_approvals_to_add.empty() && o.owner_approvals_to_add.empty(),
                 "This proposal is in its review period. No new approvals may be added." );
   for( const string& id : o.active_approvals_to_remove )
   {
      FC_ASSERT( _proposal->available_active_approvals.find(id) != _proposal->available_active_approvals.end(),
                 "", ("id", id)("available", _proposal->available_active_approvals) );
   }
   for( const string& id : o.owner_approvals_to_remove )
   {
      FC_ASSERT( _proposal->available_owner_approvals.find(id) != _proposal->available_owner_approvals.end(),
                 "", ("id", id)("available", _proposal->available_owner_approvals) );
   }

   for( const string& id : o.active_approvals_to_add )
   {
      FC_ASSERT( _proposal->available_active_approvals.find(id) == _proposal->available_active_approvals.end(),
                 "Already approved by active authority ${id}",
                 ("id", id)("available", _proposal->available_active_approvals) );
      FC_ASSERT( _proposal->required_active_approvals.find(id) != _proposal->required_active_approvals.end()
                 || _proposal->required_basic_approvals.find(id) != _proposal->required_basic_approvals.end(),
                 "Active approval from ${id} is not required", ("id", id) );
   }
   for( const string& id : o.owner_approvals_to_add )
   {
      FC_ASSERT( _proposal->available_owner_approvals.find(id) == _proposal->available_owner_approvals.end(),
                 "Already approved by owner authority ${id}",
                 ("id", id)("available", _proposal->available_owner_approvals) );
      FC_ASSERT( _proposal->required_owner_approvals.find(id) != _proposal->required_owner_approvals.end()
                 || _proposal->required_active_approvals.find(id) != _proposal->required_active_approvals.end()
                 || _proposal->required_basic_approvals.find(id) != _proposal->required_basic_approvals.end(),
                 "Owner approval from ${id} is not required", ("id", id) );
   }

   dlog( "Proposal: ${op}", ("op",o) );

   // Potential optimization: if _executed_proposal is true, we can skip the modify step and make push_proposal skip
   // signature checks. This isn't done now because I just wrote all the proposals code, and I'm not yet 100% sure the
   // required approvals are sufficient to authorize the transaction.
   d.modify(*_proposal, [&o, &d](proposal_object& p) {
      p.available_active_approvals.insert(o.active_approvals_to_add.begin(), o.active_approvals_to_add.end());
      p.available_owner_approvals.insert(o.owner_approvals_to_add.begin(), o.owner_approvals_to_add.end());
      for( const string& id : o.active_approvals_to_remove )
         p.available_active_approvals.erase(id);
      for( const string& id : o.owner_approvals_to_remove )
         p.available_owner_approvals.erase(id);
      for( const auto& id : o.key_approvals_to_add )
         p.available_key_approvals.insert(id);
      for( const auto& id : o.key_approvals_to_remove )
         p.available_key_approvals.erase(id);

      dlog( "Proposal: ${p}", ("p",p) );
   });

   // If the proposal has a review period, don't bother attempting to authorize/execute it.
   // Proposals with a review period may never be executed except at their expiration.
   if( _proposal->review_period_time )
      return ;
   if( _proposal->is_authorized_to_execute(d) )
      try {
         d.push_proposal(*_proposal);
      } catch(fc::exception& e) {
         ilog("Proposed transaction ${id} failed to apply once approved with exception:\n----\n${reason}\n----\nWill try again when it expires.",
              ("id", o.proposal)("reason", e.to_detail_string()));
      }
} FC_CAPTURE_AND_RETHROW( (o) ) }

void proposal_delete_evaluator::do_apply(const proposal_delete_operation& o)
{ try {
   const proposal_object& proposal = o.proposal( db() );

   FC_ASSERT( proposal.can_veto.find( o.vetoer ) != proposal.can_veto.end(),
              "Provided authority '${provided}' can not veto this proposal.",
              ("provided",o.vetoer));

   db().remove( proposal );

} FC_CAPTURE_AND_RETHROW( (o) ) }

} } // btcm::chain
