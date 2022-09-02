#pragma once

#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>

namespace idtonomy
{
   using eosio::action_wrapper;
   using eosio::check;
   using eosio::checksum256;
   using eosio::name;
   using eosio::print;
   using eosio::public_key;

   // Create an enum type and an eosio type for enums
   // https://eosio.stackexchange.com/questions/4950/store-enum-value-in-table
   enum enum_account_type
   {
      Person,
      Organization,
      SmartContract,
      Goverment
   };
   typedef uint8_t account_type;

   enum enum_account_status
   {
      Creating_Status,
      Active_Status,
      Deactivated_Status,
      Upgrading_Status
   };
   typedef uint8_t account_status;

   std::map<enum_account_type, char> account_type_letters = {
       {enum_account_type::Person, 'p'},
       {enum_account_type::SmartContract, 's'},
       {enum_account_type::Organization, 'o'},
       {enum_account_type::Goverment, 'g'}};

   enum enum_permission_level
   {
      Owner,
      Active,
      Password,
      Pin,
      Fingerprint,
      Local
   };
   typedef uint8_t permission_level;

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
       * Create a new account for a person
       *
       * @details Creates a new account.
       *
       * @param username_hash - hash of the username of the account
       * @param password_key - public key generated from the account's password
       * @param password_salt - salt used to generate the password_key with the password
       */
      [[eosio::action]] void newperson(
          checksum256 username_hash,
          public_key password_key,
          checksum256 password_salt);

      /**
       * Update a key of a person
       *
       * @param account - name of the account to update
       * @param permission - permission level of the key to update
       * @param key - public key to update
       */
      [[eosio::action]] void updatekey(name account,
                                       permission_level permission,
                                       public_key key);

      using newperson_action = action_wrapper<"newperson"_n, &id::newperson>;
      using updatekey_action = action_wrapper<"updatekey"_n, &id::updatekey>;

      TABLE account
      {
         name account_name;
         account_status status;
         checksum256 username_hash;
         checksum256 password_salt;

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
