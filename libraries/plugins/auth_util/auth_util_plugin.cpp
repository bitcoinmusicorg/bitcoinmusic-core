#include <btcm/chain/protocol/ext.hpp>

#include <btcm/plugins/auth_util/auth_util_api.hpp>
#include <btcm/plugins/auth_util/auth_util_plugin.hpp>

#include <string>

namespace btcm { namespace plugin { namespace auth_util {

auth_util_plugin::auth_util_plugin() {}
auth_util_plugin::~auth_util_plugin() {}

std::string auth_util_plugin::plugin_name()const
{
   return "auth_util";
}

void auth_util_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
}

void auth_util_plugin::plugin_startup()
{
   app().register_api_factory< auth_util_api >( "auth_util_api" );
}

void auth_util_plugin::plugin_shutdown()
{
}

} } } // btcm::plugin::auth_util

BTCM_DEFINE_PLUGIN( auth_util, btcm::plugin::auth_util::auth_util_plugin )
