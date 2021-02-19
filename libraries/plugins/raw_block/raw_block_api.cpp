
#include <btcm/app/api_context.hpp>
#include <btcm/app/application.hpp>

#include <btcm/plugins/raw_block/raw_block_api.hpp>
#include <btcm/plugins/raw_block/raw_block_plugin.hpp>

namespace btcm { namespace plugin { namespace raw_block {

namespace detail {

class raw_block_api_impl
{
   public:
      raw_block_api_impl( btcm::app::application& _app );

      std::shared_ptr< btcm::plugin::raw_block::raw_block_plugin > get_plugin();

      btcm::app::application& app;
};

raw_block_api_impl::raw_block_api_impl( btcm::app::application& _app ) : app( _app )
{}

std::shared_ptr< btcm::plugin::raw_block::raw_block_plugin > raw_block_api_impl::get_plugin()
{
   return app.get_plugin< raw_block_plugin >( "raw_block" );
}

} // detail

raw_block_api::raw_block_api( const btcm::app::api_context& ctx )
{
   my = std::make_shared< detail::raw_block_api_impl >(ctx.app);
}

get_raw_block_result raw_block_api::get_raw_block( get_raw_block_args args )
{
   get_raw_block_result result;
   std::shared_ptr< btcm::chain::database > db = my->app.chain_database();

   fc::optional<chain::signed_block> block = db->fetch_block_by_number( args.block_num );
   if( !block.valid() )
   {
      return result;
   }
   std::stringstream ss;
   fc::raw::pack( ss, *block );
   result.raw_block = fc::base64_encode( ss.str() );
   result.block_id = block->id();
   result.previous = block->previous;
   result.timestamp = block->timestamp;
   return result;
}

void raw_block_api::push_raw_block( std::string block_b64 )
{
   std::shared_ptr< btcm::chain::database > db = my->app.chain_database();

   std::string block_bin = fc::base64_decode( block_b64 );
   std::stringstream ss( block_bin );
   chain::signed_block block;
   fc::raw::unpack( ss, block );

   db->push_block( block );
}

void raw_block_api::on_api_startup() { }

} } } // btcm::plugin::raw_block
