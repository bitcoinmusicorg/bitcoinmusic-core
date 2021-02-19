#!/usr/bin/env python3

templates = {
"CMakeLists.txt" :
"""file(GLOB HEADERS "include/{plugin_provider}/plugins/{plugin_name}/*.hpp")

add_library( {plugin_provider}_{plugin_name}
             ${{HEADERS}}
             {plugin_name}_plugin.cpp
             {plugin_name}_api.cpp
           )

target_link_libraries( {plugin_provider}_{plugin_name} btcm_app btcm_chain fc graphene_db )
target_include_directories( {plugin_provider}_{plugin_name}
                            PUBLIC "${{CMAKE_CURRENT_SOURCE_DIR}}/include" )
""",

"include/{plugin_provider}/plugins/{plugin_name}/{plugin_name}_api.hpp" :
"""
#pragma once

#include <fc/api.hpp>

namespace btcm {{ namespace app {{
   struct api_context;
}} }}

namespace {plugin_provider} {{ namespace plugin {{ namespace {plugin_name} {{

namespace detail {{
class {plugin_name}_api_impl;
}}

class {plugin_name}_api
{{
   public:
      {plugin_name}_api( const btcm::app::api_context& ctx );

      void on_api_startup();

      // TODO:  Add API methods here

   private:
      std::shared_ptr< detail::{plugin_name}_api_impl > my;
}};

}} }} }}

FC_API( {plugin_provider}::plugin::{plugin_name}::{plugin_name}_api,
   // TODO:  Add method bubble list here
   )
""",

"include/{plugin_provider}/plugins/{plugin_name}/{plugin_name}_plugin.hpp" :
"""
#pragma once

#include <btcm/app/plugin.hpp>

namespace {plugin_provider} {{ namespace plugin {{ namespace {plugin_name} {{

class {plugin_name}_plugin : public btcm::app::plugin
{{
   public:
      {plugin_name}_plugin();
      virtual ~{plugin_name}_plugin();

      virtual std::string plugin_name()const override;
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;
}};

}} }} }}
""",

"{plugin_name}_api.cpp" :
"""
#include <btcm/app/api_context.hpp>
#include <btcm/app/application.hpp>

#include <{plugin_provider}/plugins/{plugin_name}/{plugin_name}_api.hpp>
#include <{plugin_provider}/plugins/{plugin_name}/{plugin_name}_plugin.hpp>

namespace {plugin_provider} {{ namespace plugin {{ namespace {plugin_name} {{

namespace detail {{

class {plugin_name}_api_impl
{{
   public:
      {plugin_name}_api_impl( btcm::app::application& _app );

      std::shared_ptr< {plugin_provider}::plugin::{plugin_name}::{plugin_name}_plugin > get_plugin();

      btcm::app::application& app;
}};

{plugin_name}_api_impl::{plugin_name}_api_impl( btcm::app::application& _app ) : app( _app )
{{}}

std::shared_ptr< {plugin_provider}::plugin::{plugin_name}::{plugin_name}_plugin > {plugin_name}_api_impl::get_plugin()
{{
   return app.get_plugin< {plugin_name}_plugin >( "{plugin_name}" );
}}

}} // detail

{plugin_name}_api::{plugin_name}_api( const btcm::app::api_context& ctx )
{{
   my = std::make_shared< detail::{plugin_name}_api_impl >(ctx.app);
}}

void {plugin_name}_api::on_api_startup() {{ }}

}} }} }} // {plugin_provider}::plugin::{plugin_name}
""",

"{plugin_name}_plugin.cpp" :
"""

#include <{plugin_provider}/plugins/{plugin_name}/{plugin_name}_api.hpp>
#include <{plugin_provider}/plugins/{plugin_name}/{plugin_name}_plugin.hpp>

#include <string>

namespace {plugin_provider} {{ namespace plugin {{ namespace {plugin_name} {{

{plugin_name}_plugin::{plugin_name}_plugin() {{}}
{plugin_name}_plugin::~{plugin_name}_plugin() {{}}

std::string {plugin_name}_plugin::plugin_name()const
{{
   return "{plugin_name}";
}}

void {plugin_name}_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{{
}}

void {plugin_name}_plugin::plugin_startup()
{{
   chain::database& db = database();

   app().register_api_factory< {plugin_name}_api >( "{plugin_name}_api" );
}}

void {plugin_name}_plugin::plugin_shutdown()
{{
}}

}} }} }} // {plugin_provider}::plugin::{plugin_name}

BTCM_DEFINE_PLUGIN( {plugin_name}, {plugin_provider}::plugin::{plugin_name}::{plugin_name}_plugin )
""",
}

import argparse
import os
import sys

def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("provider", help="Name of plugin provider (btcm for plugins developed by Bitcoin Music)")
    parser.add_argument("name", help="Name of plugin to create")
    args = parser.parse_args(argv[1:])
    ctx = {
           "plugin_provider" : args.provider,
           "plugin_name" : args.name,
          }

    outdir = os.path.join("libraries", "plugins", ctx["plugin_name"])
    for t_fn, t_content in templates.items():
        content = t_content.format(**ctx)
        fn = os.path.join(outdir, t_fn.format(**ctx))
        dn = os.path.dirname(fn)
        if not os.path.exists(dn):
            os.makedirs(dn)
        with open(fn, "w") as f:
            f.write(content)

    return

if __name__ == "__main__":
    main(sys.argv)
