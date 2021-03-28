
#include <iostream>
#include <string>

#include <boost/algorithm/string.hpp>

#include <fc/io/json.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/variant.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <btcm/chain/protocol/types.hpp>
using namespace btcm::chain;


struct signing_request
{
   fc::sha256                         dig;
   std::string                        wif;
};

struct signing_result
{
   fc::sha256                         dig;
   btcm::chain::public_key_type    key;
   btcm::chain::signature_type     sig;
};

FC_REFLECT( signing_request, (dig)(wif) )
FC_REFLECT( signing_result, (dig)(key)(sig) )

int main()
{
   // hash key pairs on stdin
   while( std::cin )
   {
      std::string line;
      std::getline( std::cin, line );
      boost::trim(line);
      if( line == "" )
         continue;

      fc::variant v = fc::json::from_string( line );
      signing_request sreq;
      fc::from_variant( v, sreq, 10 );
      signing_result sres;
      sres.dig = sreq.dig;
      fc::ecc::private_key priv_key = *graphene::utilities::wif_to_key( sreq.wif );
      sres.sig = priv_key.sign_compact( sreq.dig );
      sres.key = btcm::chain::public_key_type( priv_key.get_public_key() );
      std::cout << fc::json::to_string( sres ) << std::endl;
   }
   return 0;
}
