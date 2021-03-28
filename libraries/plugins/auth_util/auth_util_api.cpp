
#include <fc/container/flat.hpp>

#include <btcm/app/api_context.hpp>
#include <btcm/app/application.hpp>

#include <btcm/chain/protocol/authority.hpp>
#include <btcm/chain/protocol/sign_state.hpp>

#include <btcm/chain/account_object.hpp>
#include <btcm/chain/database.hpp>

#include <btcm/plugins/auth_util/auth_util_api.hpp>
#include <btcm/plugins/auth_util/auth_util_plugin.hpp>

namespace btcm { namespace plugin { namespace auth_util {

using boost::container::flat_set;

namespace detail {

class auth_util_api_impl
{
   public:
      auth_util_api_impl( btcm::app::application& _app );
      void check_authority_signature( const check_authority_signature_params& args, check_authority_signature_result& result );

      std::shared_ptr< btcm::plugin::auth_util::auth_util_plugin > get_plugin();

      btcm::app::application& app;
};

auth_util_api_impl::auth_util_api_impl( btcm::app::application& _app ) : app( _app )
{}

std::shared_ptr< btcm::plugin::auth_util::auth_util_plugin > auth_util_api_impl::get_plugin()
{
   return app.get_plugin< auth_util_plugin >( "auth_util" );
}

void auth_util_api_impl::check_authority_signature( const check_authority_signature_params& args, check_authority_signature_result& result )
{
   std::shared_ptr< chain::database > db = app.chain_database();
   const chain::account_object& acct = db->get_account( args.account_name );
   const chain::authority* auth = nullptr;
   if( (args.level == "basic") || (args.level == "p") )
   {
      auth = &acct.basic;
   }
   else if( (args.level == "active") || (args.level == "a") || (args.level == "") )
   {
      auth = &acct.active;
   }
   else if( (args.level == "owner") || (args.level == "o") )
   {
      auth = &acct.owner;
   }
   else
   {
      FC_ASSERT( false, "invalid level specified" );
   }
   flat_set< chain::public_key_type > signing_keys;
   for( const chain::signature_type& sig : args.sigs )
   {
      result.keys.emplace_back( fc::ecc::public_key( sig, args.dig, true ) );
      signing_keys.insert( result.keys.back() );
   }

   chain::sign_state ss( signing_keys, [&db]( const std::string& account_name )
   {
      return &db->get_account( account_name ).active;
   } );

   bool has_authority = ss.check_authority( auth );
   FC_ASSERT( has_authority );
}

} // detail

auth_util_api::auth_util_api( const btcm::app::api_context& ctx )
{
   my = std::make_shared< detail::auth_util_api_impl >(ctx.app);
}

void auth_util_api::on_api_startup() { }

check_authority_signature_result auth_util_api::check_authority_signature( check_authority_signature_params args )
{
   check_authority_signature_result result;
   my->check_authority_signature( args, result );
   return result;
}

} } } // btcm::plugin::auth_util
