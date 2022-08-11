#include <id.tonomy/id.tonomy.hpp>
#include <eosio.bios/eosio.bios.hpp>
#include <vector>

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

      // TODO generate new random account name
      const name randomname = "newrandomname"_n;

      eosiobios::authority owner{}; // zero initialized
      owner.threshold = 1;
      eosiobios::key_weight password_key;
      password_key.weight = 1;
      password_key.key = password;
      owner.keys.push_back(password_key);

      eosiobios::bios::newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
      newaccountaction.send(creator, randomname, owner, owner);

      // TODO:
      // update key with pin
      // update key with fingerprint
      // Store the salt and hashed username in table, with type = Person

      check(false, "Check false");
   }

}
