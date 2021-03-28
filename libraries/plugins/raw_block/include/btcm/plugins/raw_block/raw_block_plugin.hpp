
#pragma once

#include <btcm/app/plugin.hpp>

namespace btcm { namespace plugin { namespace raw_block {

class raw_block_plugin : public btcm::app::plugin
{
   public:
      raw_block_plugin();
      virtual ~raw_block_plugin();

      virtual std::string plugin_name()const override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;
};

} } }
