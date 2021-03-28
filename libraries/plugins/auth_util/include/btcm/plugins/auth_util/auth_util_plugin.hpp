
#pragma once

#include <btcm/app/plugin.hpp>

namespace btcm { namespace plugin { namespace auth_util {

class auth_util_plugin : public btcm::app::plugin
{
   public:
      auth_util_plugin();
      virtual ~auth_util_plugin();

      virtual std::string plugin_name()const override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;
};

} } }
