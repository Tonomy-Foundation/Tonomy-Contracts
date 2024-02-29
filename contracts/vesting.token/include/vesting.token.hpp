#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>

#define TONOMY_SYMBOL symbol("LEOS", 6)

namespace vestingtoken {

   using eosio::action_wrapper;
   using eosio::check;
   using eosio::ignore;
   using eosio::name;
   using eosio::symbol;
   using eosio::symbol_code;
   using eosio::microseconds;
   using eosio::time_point_sec;
   using eosio::datastream;

   /**
    * @defgroup vestingtoken Vesting Token Contract
    * @ingroup eosiocontracts
    * @{
    */

   class [[eosio::contract("vesting.token")]] boot : public eosio::contract
  {
  public:
    using contract::contract;

    /**
    * @brief Configuration table for the vesting token contract.
    */
    TABLE config {
      microseconds default_vesting_period; ///< Default vesting period in microseconds.

      // Explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(config, (default_vesting_period))
    };
    typedef eosio::singleton<"config"_n, config> config_t;

    /**
    * @brief Structure representing a vest.
    */
    struct [[eosio::table("vests")]] vest {
      uint64_t id; ///< Vest ID.
      time_point_sec matures_at; ///< Time when the vest matures.
      eosio::asset quantity; ///< Quantity of the vest.

      /**
      * @brief Get the primary key of the vest.
      * @return The primary key of the vest.
      */
      auto primary_key() const { return id; }

      /**
      * @brief Get the vest maturity time in seconds since epoch.
      * @return The vest maturity time in seconds since epoch.
      */
      uint64_t by_time() const { return matures_at.sec_since_epoch(); }

      // Explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(vest, (id)(matures_at)(quantity))
    };
    typedef eosio::multi_index<
      "vests"_n, vest,
      eosio::indexed_by<"time"_n,
                eosio::const_mem_fun<vest, uint64_t, &vest::by_time>>
    > vests_t;

    /**
    * @brief Get the configuration of the vesting token contract.
    * @return The configuration of the vesting token contract.
    */
    config get_config() {
      config c =
        cfg.get_or_default(config{.default_vesting_period = eosio::days(180)});
      return c;
    }

    /**
    * @defgroup vestingtoken_actions Vesting Token Actions
    * @{
    */

    /**
    * @brief Action to reset for testing purposes.
    *
    * @param scope - The scope for testing reset.
    */
    [[eosio::action]] void testreset(eosio::name scope);

    /**
    * @brief Action to withdraw vested tokens.
    *
    * @param to - The account to withdraw tokens to.
    */
    [[eosio::action]] void withdraw(eosio::name to);

    /**
    * @brief Action to change the maturity time of a vest.
    *
    * @param to - The account owning the vest.
    * @param vest_id - The ID of the vest to change.
    * @param new_matures_at - The new maturity time for the vest.
    */
    [[eosio::action]] void changevest(eosio::name to, uint64_t vest_id,
                      eosio::time_point_sec new_matures_at);

    /**
    * @brief Action to set the contract configuration.
    *
    * @param default_vesting_period_seconds - The default vesting period in seconds.
    */
    [[eosio::action]] void setconfig(uint64_t default_vesting_period_seconds);

    /**
    * @brief Action triggered on token transfer.
    *
    * @param from - The sender of the transfer.
    * @param to - The recipient of the transfer.
    * @param quantity - The transferred quantity of tokens.
    * @param memo - The transfer memo.
    */
    [[eosio::on_notify("eosio.token::transfer")]]
    void on_transfer(eosio::name from, eosio::name to, eosio::asset quantity,
                     std::string memo);

    using testreset_action = action_wrapper<"testreset"_n, &vestingtoken::testreset>;
    using withdraw_action = action_wrapper<"withdraw"_n, &vestingtoken::withdraw>;
    using changevest_action = action_wrapper<"changevest"_n, &vestingtoken::changevest>;
    using setconfig_action = action_wrapper<"setconfig"_n, &vestingtoken::setconfig>;
    using on_transfer_action = action_wrapper<"on_transfer"_n, &vestingtoken::on_transfer>;
  };
} // namespace vestingtoken
