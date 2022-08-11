#pragma once

#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>

namespace idtonomy
{

   using eosio::action_wrapper;
   using eosio::check;
   using eosio::checksum256;
   using eosio::ignore;
   using eosio::name;
   using eosio::permission_level;
   using eosio::public_key;

   enum AccountType
   {
      Person,
      Organization
   };

   /**
    * @defgroup idtonomy id.tonomy
    * @ingroup tonomycontracts
    *
    * id.tonomy controls the identity management system for Tonomy ecosystems. It allows people and organizations
    * to create and manage their account, resources and smart contracts
    *
    * @{
    */
   class [[eosio::contract("id.tonomy")]] id : public eosio::contract
   {
   public:
      using contract::contract;
      /**
       * New account action
       *
       * @details Creates a new account.
       *
       * @param creator - the creator of the account
       */
      [[eosio::action]] void newperson(
          name creator,
          checksum256 username_hash,
          public_key password,
          std::string salt,
          public_key pin,
          public_key fingerprint);

      using newperson_action = action_wrapper<"newperson"_n, &id::newperson>;

      TABLE user
      {
         // primary key automatically added by EOSIO method
         name account;
         AccountType type;
         checksum256 username_hash;
         std::string salt;

         auto primary_key() const { return account }
         // also index by username hash to find easier
         auto by_username_hash() const { return username_hash }
      };

      typedef eosio::multi_index<N(users), user,
                                 indexed_by<eosio::N(usernamehash), const_mem_fun<user, checksum256, &user::by_username_hash>>>
          people_table;
   };
   /** @}*/ // end of @defgroup idtonomy id.tonomy
} /// namespace idtonomy
