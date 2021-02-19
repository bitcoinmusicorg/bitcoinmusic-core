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
#include <cctype>

#include <btcm/app/api.hpp>
#include <btcm/app/api_access.hpp>
#include <btcm/app/application.hpp>
#include <btcm/app/impacted.hpp>
#include <btcm/chain/database.hpp>
#include <btcm/chain/get_config.hpp>
#include <btcm/chain/base_objects.hpp>
#include <btcm/chain/transaction_object.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/crypto/hex.hpp>
#include <fc/smart_ref_impl.hpp>

namespace btcm { namespace app {

    login_api::login_api(const api_context& ctx)
    :_ctx(ctx)
    {
    }

    login_api::~login_api()
    {
    }

    void login_api::on_api_startup() {
    }

    bool login_api::login(const string& user, const string& password)
    {
       optional< api_access_info > acc = _ctx.app.get_api_access_info( user );
       if( !acc.valid() )
          return false;
       if( acc->password_hash_b64 != "*" )
       {
          std::string password_salt = fc::base64_decode( acc->password_salt_b64 );
          std::string acc_password_hash = fc::base64_decode( acc->password_hash_b64 );

          fc::sha256 hash_obj = fc::sha256::hash( password + password_salt );
          if( hash_obj.data_size() != acc_password_hash.length() )
             return false;
          if( memcmp( hash_obj.data(), acc_password_hash.c_str(), hash_obj.data_size() ) != 0 )
             return false;
       }

       std::shared_ptr< api_session_data > session = _ctx.session.lock();
       FC_ASSERT( session );

       std::map< std::string, api_ptr >& _api_map = session->api_map;

       for( const std::string& api_name : acc->allowed_apis )
       {
          auto it = _api_map.find( api_name );
          if( it != _api_map.end() ) {
             continue;
          }
          api_context new_ctx( _ctx.app, api_name, _ctx.session );
          _api_map[ api_name ] = _ctx.app.create_api_by_name( new_ctx );
       }
       return true;
    }

    fc::api_ptr login_api::get_api_by_name( const string& api_name )const
    {
       std::shared_ptr< api_session_data > session = _ctx.session.lock();
       FC_ASSERT( session );

       const std::map< std::string, api_ptr >& _api_map = session->api_map;
       auto it = _api_map.find( api_name );
       if( it == _api_map.end() )
       {
          wlog( "unknown api: ${api}", ("api",api_name) );
          return fc::api_ptr();
       }
       if( it->second )
       {
          ilog( "found api: ${api}", ("api",api_name) );
       }
       FC_ASSERT( it->second != nullptr );
       return it->second;
    }

    network_broadcast_api::network_broadcast_api(const api_context& a):_app(a.app)
    {
       /// NOTE: cannot register callbacks in constructor because shared_from_this() is not valid.
    }

    void network_broadcast_api::on_api_startup()
    {
       /// note cannot capture shared pointer here, because _applied_block_connection will never
       /// be freed if the lambda holds a reference to it.
       _applied_block_connection = connect_signal( _app.chain_database()->applied_block, *this, &network_broadcast_api::on_applied_block );
    }

    void network_broadcast_api::on_applied_block( const signed_block& b )
    {
       int32_t block_num = int32_t(b.block_num());
       if( _callbacks.size() )
       {
          /// we need to ensure the database_api is not deleted for the life of the async operation
          auto capture_this = shared_from_this();
          for( size_t trx_num = 0; trx_num < b.transactions.size(); ++trx_num )
          {
             const auto& trx = b.transactions[trx_num];
             auto id = trx.id();
             auto itr = _callbacks.find(id);
             if( itr != _callbacks.end() )
             {
                auto callback = _callbacks.find(id)->second;
                fc::async( [capture_this,this,id,block_num,trx_num,callback](){ callback( fc::variant( transaction_confirmation{ id, block_num, int32_t(trx_num), false}, GRAPHENE_MAX_NESTED_OBJECTS ) ); } );
                itr->second = []( const variant& ){};
             }
          }
       }

       /// clear all expirations
       if( _callbacks_expirations.size() ) {
          auto end = _callbacks_expirations.upper_bound( b.timestamp );
          auto itr = _callbacks_expirations.begin();
          while( itr != end ) {
             for( const auto trx_id : itr->second ) {
               auto cb_itr = _callbacks.find( trx_id );
               if( cb_itr != _callbacks.end() ) {
                   auto capture_this = shared_from_this();
                   auto callback = _callbacks.find(trx_id)->second; 
                   fc::async( [capture_this,block_num,trx_id,callback](){ callback( fc::variant( transaction_confirmation{ trx_id, block_num, -1, true}, GRAPHENE_MAX_NESTED_OBJECTS ) ); } );
                   _callbacks.erase(cb_itr);
               }
             }
             _callbacks_expirations.erase( itr );
             itr = _callbacks_expirations.begin();
          }
       }
    }

    void network_broadcast_api::broadcast_transaction(const signed_transaction& trx)
    {
       trx.validate();
       _app.chain_database()->push_transaction(trx);
       _app.p2p_node()->broadcast_transaction(trx);
    }
    fc::variant network_broadcast_api::broadcast_transaction_synchronous(const signed_transaction& trx)
    {
       promise<fc::variant>::ptr prom( new fc::promise<fc::variant>() );
       broadcast_transaction_with_callback( [=]( const fc::variant& v ){ 
          prom->set_value(v);
       }, trx );
       return future<fc::variant>(prom).wait();
    }

    void network_broadcast_api::broadcast_block( const signed_block& b )
    {
       _app.chain_database()->push_block(b);
       _app.p2p_node()->broadcast( graphene::net::block_message( b ));
    }

    void network_broadcast_api::broadcast_transaction_with_callback(confirmation_callback cb, const signed_transaction& trx)
    {
       trx.validate();
       _callbacks[trx.id()] = cb;
       _callbacks_expirations[trx.expiration].push_back(trx.id());

       _app.chain_database()->push_transaction(trx);
       _app.p2p_node()->broadcast_transaction(trx);
    }

    network_node_api::network_node_api( const api_context& a ) : _app( a.app )
    {
    }

    void network_node_api::on_api_startup() {}

    fc::variant_object network_node_api::get_info() const
    {
       fc::mutable_variant_object result = _app.p2p_node()->network_get_info();
       result["connection_count"] = _app.p2p_node()->get_connection_count();
       return result;
    }

    void network_node_api::add_node(const fc::ip::endpoint& ep)
    {
       _app.p2p_node()->add_node(ep);
    }

    std::vector<graphene::net::peer_status> network_node_api::get_connected_peers() const
    {
       return _app.p2p_node()->get_connected_peers();
    }

    std::vector<graphene::net::potential_peer_record> network_node_api::get_potential_peers() const
    {
       return _app.p2p_node()->get_potential_peers();
    }

    fc::variant_object network_node_api::get_advanced_node_parameters() const
    {
       return _app.p2p_node()->get_advanced_node_parameters();
    }

    void network_node_api::set_advanced_node_parameters(const fc::variant_object& params)
    {
       return _app.p2p_node()->set_advanced_node_parameters(params);
    }
} } // btcm::app
