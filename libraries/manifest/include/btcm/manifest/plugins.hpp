
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace btcm { namespace app {

class abstract_plugin;

} }

namespace btcm { namespace plugin {

void initialize_plugin_factories();
std::shared_ptr< btcm::app::abstract_plugin > create_plugin( const std::string& name );
std::vector< std::string > get_available_plugins();

} }
