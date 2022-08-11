#include <id.tonomy/id.tonomy.hpp>
#include <eosio.bios/eosio.bios.hpp>

namespace idtonomy
{

   void id::newperson(
       name creator,
       checksum256 username_hash,
       public_key password,
       std::string salt,
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

      const name randomname = "newrandomname"_n;
      // const bios::authority owner =
      eosiobios::bios::newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
      // modaction.send(creator, randomname, owner, active);

      check(false, "Check false");
   }

}
