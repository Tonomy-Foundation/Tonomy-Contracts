#pragma once

#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>

namespace idtonomy
{

   using eosio::action_wrapper;
   using eosio::check;
   using eosio::checksum256;
   using eosio::name;
   using eosio::public_key;

   enum AccountType
   {
      Person,
      Organization
   };

   enum AccountStatus
   {
      Creating,
      Active,
      Deactivated,
      Upgrading
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
       * Constructor for the contract, which initializes the _accounts table
       */
      id(name receiver, name code, eosio::datastream<const char *> ds);

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
          checksum256 salt,
          public_key pin,
          public_key fingerprint);

      using newperson_action = action_wrapper<"newperson"_n, &id::newperson>;

      TABLE account
      {
         name account_name;
         AccountType type;
         AccountStatus status;
         checksum256 username_hash;
         checksum256 salt;

         // primary key automatically added by EOSIO method
         uint64_t primary_key() const { return account_name.value; }
         // also index by username hash
         checksum256 index_by_username_hash() const { return username_hash; }
      };

      // Create a multi-index-table with two indexes
      typedef eosio::multi_index<"accounts"_n, account,
                                 eosio::indexed_by<"usernamehash"_n, eosio::const_mem_fun<account, checksum256, &account::index_by_username_hash>>>
          accounts_table;

      // Create an instance of the table that can is initalized in the constructor
      accounts_table _accounts;
   };
   /** @}*/ // end of @defgroup idtonomy id.tonomy
} /// namespace idtonomy
