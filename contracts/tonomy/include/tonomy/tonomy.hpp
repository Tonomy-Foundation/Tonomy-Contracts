#pragma once

#include <eosio/action.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/fixed_bytes.hpp>
#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include "native.hpp"

namespace tonomysystem
{
   using eosio::action_wrapper;
   using eosio::asset;
   using eosio::check;
   using eosio::checksum256;
   using eosio::ignore;
   using eosio::name;
   using eosio::permission_level;
   using eosio::print;
   using eosio::public_key;
   using eosio::singleton;
   using std::string;

   using eosiotonomy::authority;

   // Create an enum type and an eosio type for enums
   // https://eosio.stackexchange.com/questions/4950/store-enum-value-in-table
   enum enum_account_type
   {
      Person,
      Organization,
      App,
      Gov,
      Service
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

   enum enum_permission_level_name
   {
      Owner,
      Active,
      Password,
      Pin,
      Biometric,
      Local
   };
   typedef uint8_t permission_level_name;

   // Shared helpers implemented in tonomy.cpp
   void throwError(string error_code, string message);
   uint64_t uint64_t_from_checksum256(const checksum256 &hash);
   name tidy_name(const name &account_name, const uint8_t random_number, const enum_account_type &account_type);
   name random_account_name(const checksum256 &hash1, const checksum256 &hash2, const enum_account_type &account_type);
   authority create_authority_with_key(const eosio::public_key &key);
   authority create_authority_with_account(const eosio::name &account);
   permission_level create_eosio_code_permission_level(const name &account);

   /**
    * The `eosio.tonomy` is the first sample of system contract provided by `block.one` through the EOSIO platform. It is a minimalist system contract because it only supplies the actions that are absolutely critical to bootstrap a chain and nothing more. This allows for a chain agnostic approach to bootstrapping a chain.
    *
    * Just like in the `eosio.system` sample contract implementation, there are a few actions which are not implemented at the contract level (`newaccount`, `updateauth`, `deleteauth`, `linkauth`, `unlinkauth`, `canceldelay`, `onerror`, `setabi`, `setcode`), they are just declared in the contract so they will show in the contract's ABI and users will be able to push those actions to the chain via the account holding the `eosio.system` contract, but the implementation is at the EOSIO core level. They are referred to as EOSIO native actions.
    */
   class [[eosio::contract("tonomy")]] tonomy : public native
   {
   public:
      uint64_t inital_ram_bytes = 6000;
      uint64_t initial_cpu_weight_allocation = 1000;
      uint64_t initial_net_weight_allocation = 1000;

      static constexpr eosio::symbol system_resource_currency = eosio::symbol("TONO", 6);
      static constexpr eosio::name token_contract_name = "eosio.token"_n;

      /**
       * Constructor for the contract, which initializes the _accounts table
       */
      tonomy(name receiver, name code, eosio::datastream<const char *> ds);

      /**
       * Create a new account for a person
       *
       * @details Creates a new account for a person.
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
      [[eosio::action]] void updatekeyper(name account,
                                          permission_level_name permission,
                                          public_key key,
                                          bool link_auth = false);


      /**
       * Update active of a person
       * (this is so that users can add staking.tmy@eosio.code authorization to use the staking contract)
       *
       * @param account - name of the account to update
       * @param permission - permission level of the key to update
       * @param key - public key to update
       */
      [[eosio::action]] void updateactive(name account,
                                          authority active);

      /**
       * Set the resource parameters for the system
       * @param ram_price - the price of RAM (bytes per token)
       * @param total_ram_available - the total amount of RAM available (bytes)
       * @param ram_fee - the fee RAM purchases in fraction (0.01 = 1% fee)
       */
      [[eosio::action]] void setresparams(double ram_price, uint64_t total_ram_available, double ram_fee);

      struct [[eosio::table]] account_type_struct
      {
         name account_name;
         account_type acc_type;
         uint16_t version; // used for upgrading the account structure

         uint64_t primary_key() const { return account_name.value; }
         EOSLIB_SERIALIZE(struct account_type_struct, (account_name)(acc_type)(version))
      };

      typedef eosio::multi_index<"acctypes"_n, account_type_struct> account_type_table;

      struct [[eosio::table]] person
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
      typedef eosio::multi_index<"people"_n, person,
                                 eosio::indexed_by<"usernamehash"_n, eosio::const_mem_fun<person, checksum256, &person::index_by_username_hash>>>
          people_table;

      // Create an instance of the table that can is initalized in the constructor
      people_table _people;

      struct [[eosio::table]] resource_config
      {
         double ram_fee;                      // RAM fee fraction (0.01 = 1% fee)
         double ram_price;                    // RAM price (bytes per token)
         uint64_t total_ram_available;        // Total available RAM (bytes)
         uint64_t total_ram_used;             // Total RAM used (bytes)
         uint64_t total_cpu_weight_allocated; // Total allocated (CPU weight)
         uint64_t total_net_weight_allocated; // Total allocated (NET weight)

         EOSLIB_SERIALIZE(resource_config, (ram_fee)(ram_price)(total_ram_available)(total_ram_used)(total_cpu_weight_allocated)(total_net_weight_allocated))
      };
      typedef eosio::singleton<"resconfig"_n, resource_config> resource_config_table;
      // Following line needed to correctly generate ABI. See https://github.com/EOSIO/eosio.cdt/issues/280#issuecomment-439666574
      typedef eosio::multi_index<"resconfig"_n, resource_config> resource_config_table_dump;

      using newperson_action = action_wrapper<"newperson"_n, &tonomy::newperson>;
      using updatekeyper_action = action_wrapper<"updatekeyper"_n, &tonomy::updatekeyper>;
      using setresparams_action = action_wrapper<"setresparams"_n, &tonomy::setresparams>;
   };
}