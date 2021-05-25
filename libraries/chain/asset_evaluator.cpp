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
#include <btcm/chain/asset_object.hpp>
#include <btcm/chain/account_object.hpp>
#include <btcm/chain/database.hpp>
#include <btcm/chain/exceptions.hpp>
#include <btcm/chain/hardfork.hpp>

#include <functional>

namespace btcm { namespace chain {


bool _is_authorized_asset( const database& d, const account_object& acct, const asset_object& asset_obj )
{
    return true; //TODO_BTCM - check asset trading restriction
}


void asset_create_evaluator::do_apply( const asset_create_operation& op )
{ try {
   if( db().has_hardfork( BTCM_HARDFORK_0_1 ) )
   {
      FC_ASSERT( !(op.common_options.issuer_permissions & ~ALLOWED_ASSET_PERMISSIONS),
                 "Disallowed permissions detected!" );
      FC_ASSERT( !(op.common_options.flags & ~ALLOWED_ASSET_PERMISSIONS),
                 "Disallowed flags detected!" );
   }

   auto& asset_indx = db().get_index_type<asset_index>().indices().get<by_symbol>();
   auto asset_symbol_itr = asset_indx.find( op.symbol );
   FC_ASSERT( asset_symbol_itr == asset_indx.end(), "Asset with symbol ${s} already exist", ("s",op.symbol) );

   const account_object& issuer = db().get_account(op.issuer);

   bool is_sub = op.symbol.find( '.' ) != std::string::npos;
   asset min_fee = asset( is_sub ? BTCM_SUBASSET_CREATION_FEE : BTCM_ASSET_CREATION_FEE, XUSD_SYMBOL );
   FC_ASSERT( op.fee >= min_fee, "Insufficient fee, need ${m}", ("m",min_fee) );
   FC_ASSERT( db().get_balance( issuer, op.fee.asset_id ) >= op.fee, "Insufficient balance, can't pay fee!" );

   auto dotpos = op.symbol.rfind( '.' );
   if( dotpos != std::string::npos )
   {
      auto prefix = op.symbol.substr( 0, dotpos );
      auto asset_symbol_itr = asset_indx.find( prefix );
      FC_ASSERT( asset_symbol_itr != asset_indx.end(), "Sub-asset ${s} may only be created if ${p} exists",
                 ("s",op.symbol)("p",prefix) );
      if( db().has_hardfork( BTCM_HARDFORK_0_1 ) && (asset_symbol_itr->options.flags & hashtag) )
      {
	 account_id_type holder = db().get_nft_holder( *asset_symbol_itr );
	 if( asset_symbol_itr->options.flags & allow_subasset_creation )
	 {
            FC_ASSERT( op.fee.amount >= 2*BTCM_SUBASSET_CREATION_FEE, "Insufficient fee, need ${m}",
		       ("m",2*BTCM_SUBASSET_CREATION_FEE) );
	 }
	 else
            FC_ASSERT( holder == issuer.id, "Asset ${s} may only be created by holder of ${p}",
                       ("s",op.symbol)("p",prefix) );
      }
      else
         FC_ASSERT( asset_symbol_itr->issuer == issuer.id, "Asset ${s} may only be created by issuer of ${p}",
                    ("s",op.symbol)("p",prefix) );
   }

   if( db().has_hardfork( BTCM_HARDFORK_0_1 ) )
   {
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
   }

   db().pay_fee( issuer, op.fee );
   db().create<asset_object>( [&issuer,&op]( asset_object& a ) {
      a.issuer = issuer.id;
      a.symbol_string = op.symbol;
      a.precision = op.precision;
      a.options = op.common_options;
      a.current_supply = 0;
   });

   return; 
} FC_CAPTURE_AND_RETHROW( (op) ) }

void asset_issue_evaluator::do_apply( const asset_issue_operation& o )
{ try {
   auto& asset_indx = db().get_index_type<asset_index>().indices().get<by_id>();
   auto asset_symbol_itr = asset_indx.find( o.asset_to_issue.asset_id );
   FC_ASSERT( asset_symbol_itr != asset_indx.end(), "Asset with symbol id ${d} does not exist exist", ("d",o.asset_to_issue.asset_id) );
   FC_ASSERT( db().get_account(o.issuer).id == asset_symbol_itr -> issuer );
   FC_ASSERT( _is_authorized_asset( db(), db().get_account(o.issue_to_account), *asset_symbol_itr ) );
   FC_ASSERT( (asset_symbol_itr -> current_supply + o.asset_to_issue.amount) <= asset_symbol_itr -> options.max_supply );

   db().adjust_balance( db().get_account(o.issue_to_account), o.asset_to_issue );
   db().modify( *asset_symbol_itr, [&]( asset_object& data ){
        data.current_supply += o.asset_to_issue.amount;
   });

   return ;
} FC_CAPTURE_AND_RETHROW( (o) ) }

void asset_reserve_evaluator::do_apply( const asset_reserve_operation& o )
{ try {
   auto& asset_indx = db().get_index_type<asset_index>().indices().get<by_id>();
   auto asset_symbol_itr = asset_indx.find( o.amount_to_reserve.asset_id );
   FC_ASSERT( asset_symbol_itr != asset_indx.end(), "Asset with symbol id ${d} does not exist exist", ("d",o.amount_to_reserve.asset_id) );

   FC_ASSERT( _is_authorized_asset( db(), db().get_account(o.payer), *asset_symbol_itr ) );
   db().adjust_balance( db().get_account(o.payer), -o.amount_to_reserve );
   db().modify( *asset_symbol_itr, [&]( asset_object& data ){
        data.current_supply -= o.amount_to_reserve.amount;
   });

   return ;
} FC_CAPTURE_AND_RETHROW( (o) ) }

void asset_update_evaluator::do_apply(const asset_update_operation& o)
{ try {
   database& d = db();

   FC_ASSERT( d.has_hardfork( BTCM_HARDFORK_0_1 ), "asset_update not allowed yet!" );

   const auto& asset_indx = db().get_index_type<asset_index>().indices().get<by_id>();
   auto asset_symbol_itr = asset_indx.find( o.asset_to_update );
   FC_ASSERT( asset_symbol_itr != asset_indx.end(), "Asset with symbol id ${d} does not exist exist", ("d",o.asset_to_update) );

   const auto& a = *asset_symbol_itr;

   if( o.new_issuer )
   {
       d.get_account(*o.new_issuer);
   }

   FC_ASSERT(!(o.new_options.issuer_permissions & ~a.options.issuer_permissions),
             "Cannot reinstate previously revoked issuer permissions on an asset.");

   // changed flags must be subset of old issuer permissions
   FC_ASSERT(!((o.new_options.flags ^ a.options.flags) & ~a.options.issuer_permissions),
             "Flag change is forbidden by issuer permissions");

   FC_ASSERT( (o.new_options.flags ^ a.options.flags) == allow_subasset_creation,
              "Only allow_subasset_creation flag can be changed at this time!" );
   
   if( (o.new_options.flags ^ a.options.flags) == allow_subasset_creation )
   {
      const auto& acct = d.get_nft_holder( a )( d );
      FC_ASSERT( o.issuer == acct.name, "Only token holder can update allow_subasset_creation flag!" );
   }
   else
      FC_ASSERT( d.get_account(o.issuer).id == a.issuer, "Only the asset issuer can update" );


   db().modify( *asset_symbol_itr, [&](asset_object& a) {
      if( o.new_issuer )
         a.issuer = d.get_account(*o.new_issuer).id;
      //a.options = o.new_options;
      a.options.flags = o.new_options.flags;
   });
   return ;
} FC_CAPTURE_AND_RETHROW((o)) }

} } // btcm::chain
