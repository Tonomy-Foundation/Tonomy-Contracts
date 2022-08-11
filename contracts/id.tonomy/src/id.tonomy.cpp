#include <id.tonomy/id.tonomy.hpp>

namespace idtonomy
{

   void id::newperson(
       name creator,
       checksum256 username_hash,
       public_key password,
       eosio::string salt,
       public_key pin,
       public_key fingerprint)
   {
      eosio::require_auth(creator);

      // TODO:
      // generate new random account name
      // use password key for owner and active
      // update key with pin
      // update key with fingerprint
      // Store the salt and hashed username in table, with type = Person

      check(false, "Check false");
   }

}
