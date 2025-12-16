#include <tonomy/apps.hpp>
#include <tonomy/tonomy.hpp>
#include <eosio/transaction.hpp>
#include <eosio/symbol.hpp>
#include <eosio.tonomy/eosio.tonomy.hpp>
#include <cmath>
#include <cstring>
#include <vector>

namespace tonomysystem {

apps::apps(name receiver, name code, eosio::datastream<const char *> ds)
    : native(receiver, code, ds),
      _appsv2(receiver, receiver.value),
      _appsv3(receiver, receiver.value),
      _smartcontracts(receiver, receiver.value) {}

void apps::newapp(string json_data,
                  checksum256 username_hash,
                  string origin,
                  public_key key)
{
    eosio::require_auth(get_self());

    auto json_hash = eosio::sha256(json_data.c_str(), std::strlen(json_data.c_str()));
    const eosio::name random_name = random_account_name(username_hash, json_hash, enum_account_type::App);

    authority owner_authority = create_authority_with_account(app_controller_account);
    authority active_authority = create_authority_with_key(key);
    active_authority.accounts.push_back({.permission = create_eosio_code_permission_level(get_self()), .weight = 1});

    newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
    newaccountaction.send(get_self(), random_name, owner_authority, active_authority);

    auto apps_by_username_hash_itr = _appsv2.get_index<"usernamehash"_n>();
    const auto username_itr = apps_by_username_hash_itr.find(username_hash);
    if (username_itr != apps_by_username_hash_itr.end()) {
        throwError("TCON1001", "This app username is already taken");
    }

    auto origin_hash = eosio::sha256(origin.c_str(), std::strlen(origin.c_str()));
    auto apps_by_origin_hash_itr = _appsv2.get_index<"originhash"_n>();
    const auto origin_itr = apps_by_origin_hash_itr.find(origin_hash);
    if (origin_itr != apps_by_origin_hash_itr.end()) {
        throwError("TCON1002", "This app origin is already taken");
    }

    tonomy::resource_config_table _resource_config(get_self(), get_self().value);
    auto config = _resource_config.get();
    config.total_cpu_weight_allocated = this->initial_cpu_weight_allocation;
    config.total_net_weight_allocated = this->initial_net_weight_allocation;
    _resource_config.set(config, get_self());

    _appsv2.emplace(get_self(), [&](auto &app_itr) {
        app_itr.account_name = random_name;
        app_itr.json_data = json_data;
        app_itr.version = 2;
        app_itr.username_hash = username_hash;
        app_itr.origin = origin;
    });

    tonomy::account_type_table account_type(get_self(), get_self().value);
    account_type.emplace(get_self(), [&](auto &row) {
        row.account_name = random_name;
        row.acc_type = enum_account_type::App;
        row.version = 1;
    });
}

void apps::eraseoldapps()
{
    eosio::require_auth(get_self());
    while (_apps.begin() != _apps.end()) {
        _apps.erase(_apps.begin());
    }
}

void apps::check_app_username(const checksum256 &username_hash)
{
    auto apps_by_username_hash_itr = _appsv2.get_index<"usernamehash"_n>();
    const auto username_itr = apps_by_username_hash_itr.find(username_hash);
    if (username_itr != apps_by_username_hash_itr.end()) {
        throwError("TCON1001", "This app username is already taken");
    }
}

void apps::check_app_origin(const string &origin)
{
    auto origin_hash = eosio::sha256(origin.c_str(), std::strlen(origin.c_str()));
    auto apps_by_origin_hash_itr = _appsv2.get_index<"originhash"_n>();
    const auto origin_itr = apps_by_origin_hash_itr.find(origin_hash);
    if (origin_itr != apps_by_origin_hash_itr.end()) {
        throwError("TCON1002", "This app origin is already taken");
    }
}

void apps::adminsetapp(name account_name,
                       string json_data,
                       checksum256 username_hash,
                       string origin)
{
    eosio::require_auth(get_self());
    eosio::check(is_account(account_name), "Account does not exist");

    tonomy::account_type_table account_type(get_self(), get_self().value);
    auto itr = account_type.find(account_name.value);
    if (itr == account_type.end()) {
        account_type.emplace(get_self(), [&](auto &row) {
            row.account_name = account_name;
            row.acc_type = enum_account_type::App;
            row.version = 1;
        });
    }

    auto apps_itr = _appsv2.find(account_name.value);

    if (apps_itr != _appsv2.end()) {
        if (apps_itr->origin != origin) {
            check_app_origin(origin);
        }
        if (apps_itr->username_hash != username_hash) {
            check_app_username(username_hash);
        }
        _appsv2.modify(apps_itr, get_self(), [&](auto &app_itr) {
            app_itr.account_name = account_name;
            app_itr.origin = origin;
            app_itr.username_hash = username_hash;
            app_itr.json_data = json_data;
            app_itr.version = 2;
        });
    } else {
        check_app_username(username_hash);
        check_app_origin(origin);
        _appsv2.emplace(get_self(), [&](auto &app_itr) {
            app_itr.account_name = account_name;
            app_itr.origin = origin;
            app_itr.username_hash = username_hash;
            app_itr.json_data = json_data;
            app_itr.version = 2;
        });
    }
}

void apps::deleteapp(name account_name)
{
    eosio::require_auth(get_self());

    auto itr1 = _apps.find(account_name.value);
    if (itr1 != _apps.end()) {
        _apps.erase(itr1);
    }

    auto itr2 = _appsv2.find(account_name.value);
    if (itr2 != _appsv2.end()) {
        _appsv2.erase(itr2);
    }
}

void apps::loginwithapp(name account,
                        name app,
                        name parent,
                        public_key key)
{
    auto app_itr = _appsv2.find(app.value);
    check(app_itr != _appsv2.end(), "App does not exist");

    authority authority = create_authority_with_key(key);

    eosiotonomy::bios::updateauth_action updateauthaction("eosio"_n, {account, parent});
    updateauthaction.send(account, app, parent, authority);
}

void apps::buyram(const name &dao_owner, const name &app, const asset &quant)
{
    require_auth(app);

    tonomy::account_type_table account_type(get_self(), get_self().value);
    auto itr = account_type.find(app.value);
    eosio::check(itr != account_type.end(), "Could not find account");
    eosio::check(itr->acc_type == enum_account_type::App, "Only apps can buy and sell RAM");

    eosio::check(quant.symbol == tonomy::system_resource_currency, "must buy ram with core token");
    eosio::check(quant.amount > 0, "Amount must be positive");

    tonomy::resource_config_table config_table(get_self(), get_self().value);
    tonomy::resource_config config;
    if (config_table.exists()) {
        config = config_table.get();
    } else {
        eosio::check(false, "Resource config does not exist");
    }

    eosio::check(config.ram_price != 0, "Failed to retrieve ram_price from resource config");
    eosio::check(config.ram_fee != 0, "Failed to retrieve ram_fee from resource config");

    double ram_price = config.ram_price;
    double ram_fee = (1.0 + config.ram_fee);
    double amount = static_cast<double>(quant.amount) / pow(10, quant.symbol.precision());
    uint64_t ram_purchase = amount * ram_price / ram_fee;
    eosio::check(config.total_ram_available >= config.total_ram_used + ram_purchase, "Not enough RAM available");

    config.total_ram_used += ram_purchase;
    config_table.set(config, get_self());

    int64_t myRAM, myNET, myCPU;
    eosio::get_resource_limits(app, myRAM, myNET, myCPU);
    eosio::print("{\"event_log\":{\"account\":\"tonomy\",\"action\":\"buyram\"},\"time\":\"", eosio::current_time_point().to_string(), "Z\",\"events\":[{\"previous\":", myRAM, ",\"current\":", myRAM + ram_purchase, "}]");
    eosio::set_resource_limits(app, myRAM + ram_purchase, myNET, myNET);

    eosio::action(permission_level{dao_owner, "active"_n},
                  tonomy::token_contract_name,
                  "transfer"_n,
                  std::make_tuple(dao_owner, native::governance_name, quant, std::string("buy ram")))
        .send();
}

void apps::sellram(eosio::name dao_owner, eosio::name app, eosio::asset quant)
{
    require_auth(app);

    tonomy::account_type_table account_type(get_self(), get_self().value);
    auto itr = account_type.find(app.value);
    eosio::check(itr != account_type.end(), "Could not find account");
    eosio::check(itr->acc_type == enum_account_type::App, "Only apps can buy and sell RAM");

    eosio::check(quant.symbol == tonomy::system_resource_currency, "must sell ram with core token");
    eosio::check(quant.amount > 0, "Amount must be positive");

    tonomy::resource_config_table resource_config_singleton(get_self(), get_self().value);
    auto config = resource_config_singleton.get();

    double ram_price = config.ram_price;
    double ram_fee = (1.0 + config.ram_fee);
    double amount = static_cast<double>(quant.amount) / pow(10, quant.symbol.precision());
    uint64_t ram_sold = ram_price * ram_fee * amount;

    config.total_ram_used -= ram_sold;
    eosio::check(config.total_ram_used >= 0, "Cannot have less than 0 RAM used");
    resource_config_singleton.set(config, get_self());

    int64_t myRAM, myNET, myCPU;
    eosio::get_resource_limits(app, myRAM, myNET, myCPU);
    eosio::print("{\"event_log\":{\"account\":\"tonomy\",\"action\":\"sellram\"},\"time\":\"", eosio::current_time_point().to_string(), "Z\",\"events\":[{\"previous\":", myRAM, ",\"current\":", myRAM - ram_sold, "}]");
    eosio::check(myRAM - ram_sold >= 0, "Account cannot have less than 0 RAM");
    eosio::set_resource_limits(app, myRAM - ram_sold, myNET, myNET);

    eosio::action(permission_level{get_self(), "active"_n},
                  tonomy::token_contract_name,
                  "transfer"_n,
                  std::make_tuple(native::governance_name, dao_owner, eosio::asset(ram_sold, tonomy::system_resource_currency), std::string("sell ram")))
        .send();
}

const name apps::get_app_permission_by_origin(string origin, name contract_name)
{
   appsv3_table appsv3(contract_name, contract_name.value);
   auto apps_by_origin_hash_itr = appsv3.get_index<"originhash"_n>();

   eosio::checksum256 origin_hash = eosio::sha256(origin.c_str(), std::strlen(origin.c_str()));
   const auto origin_itr = apps_by_origin_hash_itr.find(origin_hash);
   check(origin_itr != apps_by_origin_hash_itr.end(), "No app with this origin found");

   return origin_itr->account_name;
}

const name apps::get_app_permission_by_username(string username, name contract_name)
{
   appsv3_table appsv3(contract_name, contract_name.value);
   auto apps_by_username_hash_itr = appsv3.get_index<"usernamehash"_n>();

   eosio::checksum256 username_hash = eosio::sha256(username.c_str(), std::strlen(username.c_str()));
   const auto username_itr = apps_by_username_hash_itr.find(username_hash);
   check(username_itr != apps_by_username_hash_itr.end(), "No app with this username found");

   return username_itr->account_name;
}

} // namespace tonomysystem
