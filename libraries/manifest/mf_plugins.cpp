#include <boost/container/flat_map.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <btcm/manifest/plugins.hpp>
#include <mf_internal_plugins.inc>
#include <mf_external_plugins.inc>
#include <functional>

#define BTCM_DECLARE_PLUGIN_CREATOR( r, data, x ) \
   std::shared_ptr< btcm::app::abstract_plugin > BOOST_PP_CAT( create_, BOOST_PP_CAT( x,  _plugin ) )();

namespace btcm { namespace plugin {

BOOST_PP_SEQ_FOR_EACH( BTCM_DECLARE_PLUGIN_CREATOR, _, BTCM_INTERNAL_PLUGIN_LIST )
BOOST_PP_SEQ_FOR_EACH( BTCM_DECLARE_PLUGIN_CREATOR, _, BTCM_EXTERNAL_PLUGIN_LIST )

boost::container::flat_map< std::string, std::function< std::shared_ptr< btcm::app::abstract_plugin >() > > plugin_factories_by_name;

#define BTCM_REGISTER_PLUGIN_FACTORY( r, data, x ) \
   plugin_factories_by_name[ #x ] = []() -> std::shared_ptr< btcm::app::abstract_plugin >{ return BOOST_PP_CAT( create_, BOOST_PP_CAT( x, _plugin() ) ); };

void initialize_plugin_factories()
{
   BOOST_PP_SEQ_FOR_EACH( BTCM_REGISTER_PLUGIN_FACTORY, _, BTCM_INTERNAL_PLUGIN_LIST )
   BOOST_PP_SEQ_FOR_EACH( BTCM_REGISTER_PLUGIN_FACTORY, _, BTCM_EXTERNAL_PLUGIN_LIST )
}

std::shared_ptr< btcm::app::abstract_plugin > create_plugin( const std::string& name )
{
   auto it = plugin_factories_by_name.find( name );
   if( it == plugin_factories_by_name.end() )
      return std::shared_ptr< btcm::app::abstract_plugin >();
   return it->second();
}

std::vector< std::string > get_available_plugins()
{
   std::vector< std::string > result;
   for( const auto& e : plugin_factories_by_name )
      result.push_back( e.first );
   return result;
}

} }
