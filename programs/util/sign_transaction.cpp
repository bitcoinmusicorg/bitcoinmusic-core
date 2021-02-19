
#include <iostream>
#include <string>

#include <boost/algorithm/string.hpp>

#include <fc/io/json.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/variant.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <btcm/chain/protocol/transaction.hpp>
#include <btcm/chain/protocol/types.hpp>


using namespace btcm::chain;


struct tx_signing_request
{
   btcm::chain::transaction        tx;
   std::string                        wif;
};

struct tx_signing_result
{
   btcm::chain::transaction        tx;
   fc::sha256                         digest;
   fc::sha256                         sig_digest;
   btcm::chain::public_key_type    key;
   btcm::chain::signature_type     sig;
};

FC_REFLECT( tx_signing_request, (tx)(wif) )
FC_REFLECT( tx_signing_result, (digest)(sig_digest)(key)(sig) )

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
      tx_signing_request sreq;
      fc::from_variant( v, sreq, 10 );
      tx_signing_result sres;
      sres.tx = sreq.tx;
      sres.digest = sreq.tx.digest();
      sres.sig_digest = sreq.tx.sig_digest(BTCM_CHAIN_ID);

      fc::ecc::private_key priv_key = *graphene::utilities::wif_to_key( sreq.wif );
      sres.sig = priv_key.sign_compact( sres.sig_digest );
      sres.key = btcm::chain::public_key_type( priv_key.get_public_key() );
      std::cout << fc::json::to_string( sres ) << std::endl;
   }
   return 0;
}
