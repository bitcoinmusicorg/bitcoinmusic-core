#include <btcm/chain/protocol/version.hpp>

#include <fc/exception/exception.hpp>
#include <fc/variant.hpp>

#include <sstream>

namespace btcm { namespace chain {

version::version( uint8_t m, uint8_t h, uint16_t r )
{
   v_num = ( 0 | m ) << 8;
   v_num = ( v_num | h ) << 16;
   v_num =   v_num | r;
}

version::operator fc::string()const
{
   std::stringstream s;
   s << ( ( v_num >> 24 ) & 0x00FF )
     << '.'
     << ( ( v_num >> 16 ) & 0x00FF )
     << '.'
     << (   v_num         & 0xFFFF );

   return s.str();
}

} } // btcm::chain

namespace fc
{
   void to_variant( const btcm::chain::version& v, variant& var, uint32_t max_depth )
   {
      var = fc::string( v );
   }

   void from_variant( const variant& var, btcm::chain::version& v, uint32_t max_depth )
   {
      uint32_t major = 0, hardfork = 0, revision = 0;
      char dot_a, dot_b;

      std::stringstream s( var.as_string() );
      s >> major >> dot_a >> hardfork >> dot_b >> revision;

      // We'll accept either m.h.v or m_h_v as canonical version strings
      FC_ASSERT( ( dot_a == '.' || dot_a == '_' ) && dot_a == dot_b, "Variant does not contain proper dotted decimal format" );
      FC_ASSERT( major <= 0xFF, "Major version is out of range" );
      FC_ASSERT( hardfork <= 0xFF, "Hardfork version is out of range" );
      FC_ASSERT( revision <= 0xFFFF, "Revision version is out of range" );
      FC_ASSERT( s.eof(), "Extra information at end of version string" );

      v.v_num = 0 | ( major << 24 ) | ( hardfork << 16 ) | revision;
   }

   void to_variant( const btcm::chain::hardfork_version& hv, variant& var, uint32_t max_depth )
   {
      to_variant( (const btcm::chain::version&) hv, var, max_depth );
   }

   void from_variant( const variant& var, btcm::chain::hardfork_version& hv, uint32_t max_depth )
   {
      from_variant( var, (btcm::chain::version&) hv, max_depth );
      FC_ASSERT( !(hv.v_num & 0xFFFF), "Hardfork revision must be 0!" );
   }
}