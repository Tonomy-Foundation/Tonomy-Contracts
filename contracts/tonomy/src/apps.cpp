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

// Admin create app with random account name
void apps::admncrtapp(string json_data,
                      string username,
                      string origin)
{
    require_auth(get_self());

    check_app_username_chars(username);

    // Uniqueness checks for username and origin
    checksum256 username_hash = eosio::sha256(username.c_str(), std::strlen(username.c_str()));
    {
        auto uidx = _appsv3.get_index<"usernamehash"_n>();
        check(uidx.find(username_hash) == uidx.end(), "Username already taken");
    }
    checksum256 origin_hash = eosio::sha256(origin.c_str(), std::strlen(origin.c_str()));
    {
        auto oidx = _appsv3.get_index<"originhash"_n>();
        check(oidx.find(origin_hash) == oidx.end(), "Origin already taken");
    }

    // Generate random account name from username and json_data hashes
    auto json_hash = eosio::sha256(json_data.c_str(), std::strlen(json_data.c_str()));
    const eosio::name random_name = random_account_name(username_hash, json_hash, enum_account_type::App);

    // Create account with owner=gov.tmy, active=contract
    authority owner_authority = create_authority_with_account(app_controller_account);
    authority active_authority = create_authority_with_account(get_self());
    active_authority.accounts.push_back({.permission = create_eosio_code_permission_level(get_self()), .weight = 1});

    newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
    newaccountaction.send(get_self(), random_name, owner_authority, active_authority);

    // Update resource config
    tonomy::resource_config_table _resource_config(get_self(), get_self().value);
    auto config = _resource_config.get();
    config.total_cpu_weight_allocated = this->initial_cpu_weight_allocation;
    config.total_net_weight_allocated = this->initial_net_weight_allocation;
    _resource_config.set(config, get_self());

    // Set resource limits: ram=0, net=initial, cpu=initial
    eosio::set_resource_limits(random_name, 0, this->initial_net_weight_allocation, this->initial_cpu_weight_allocation);

    // Register in appsv3
    _appsv3.emplace(get_self(), [&](auto &row) {
        row.account_name = random_name;
        row.json_data = json_data;
        row.version = 3;
        row.username = username;
        row.origin = origin;
        row.plan = static_cast<uint8_t>(plan_t::plan_basic);
    });

    // Set account type
    tonomy::account_type_table account_type(get_self(), get_self().value);
    account_type.emplace(get_self(), [&](auto &row) {
        row.account_name = random_name;
        row.acc_type = enum_account_type::App;
        row.version = 1;
    });
}


void apps::check_app_username(const checksum256 &username_hash)
{
    auto apps_by_username_hash_itr = _appsv3.get_index<"usernamehash"_n>();
    const auto username_itr = apps_by_username_hash_itr.find(username_hash);
    if (username_itr != apps_by_username_hash_itr.end()) {
        throwError("TCON1001", "This app username is already taken");
    }
}

void apps::check_app_origin(const string &origin)
{
    auto origin_hash = eosio::sha256(origin.c_str(), std::strlen(origin.c_str()));
    auto apps_by_origin_hash_itr = _appsv3.get_index<"originhash"_n>();
    const auto origin_itr = apps_by_origin_hash_itr.find(origin_hash);
    if (origin_itr != apps_by_origin_hash_itr.end()) {
        throwError("TCON1002", "This app origin is already taken");
    }
}

void apps::check_app_username_chars(const string &username)
{
    for (const char c : username) {
        const bool is_upper = c >= 'A' && c <= 'Z';
        const bool is_lower = c >= 'a' && c <= 'z';
        const bool is_digit = c >= '0' && c <= '9';
        const bool is_allowed_symbol = c == '_' || c == '-';
        if (!(is_upper || is_lower || is_digit || is_allowed_symbol)) {
            check(false, "Username may only contain A-Z, a-z, 0-9, '_' or '-' characters");
        }
    }
}

void apps::admnupdapp(name account_name,
                       string json_data,
                       string username,
                       string origin,
                       uint8_t plan)
{
    require_auth(get_self());
    check(is_account(account_name), "Account does not exist");

    auto itr = _appsv3.find(account_name.value);
    check(itr != _appsv3.end(), "App does not exist; use admncrtapp to create");

    // validate and uniqueness checks if changed
    if (itr->username != username) {
        check_app_username_chars(username);
        auto uidx = _appsv3.get_index<"usernamehash"_n>();
        checksum256 username_hash = eosio::sha256(username.c_str(), std::strlen(username.c_str()));
        check(uidx.find(username_hash) == uidx.end(), "Username already taken");
    }
    if (itr->origin != origin) {
        auto oidx = _appsv3.get_index<"originhash"_n>();
        checksum256 origin_hash = eosio::sha256(origin.c_str(), std::strlen(origin.c_str()));
        check(oidx.find(origin_hash) == oidx.end(), "Origin already taken");
    }
    _appsv3.modify(itr, get_self(), [&](auto &row) {
        row.json_data = json_data;
        row.username = username;
        row.origin = origin;
        row.plan = plan;
        row.version = 3;
    });
}

void apps::admndelapp(name account_name)
{
    require_auth(get_self());
    auto itr = _appsv3.find(account_name.value);
    check(itr != _appsv3.end(), "App does not exist");
    _appsv3.erase(itr);
}

void apps::loginwithapp(name account,
                        name app,
                        name parent,
                        public_key key)
{
    auto app_itr = _appsv3.find(app.value);
    check(app_itr != _appsv3.end(), "App does not exist");

    authority authority = create_authority_with_key(key);

    eosiotonomy::bios::updateauth_action updateauthaction("eosio"_n, {account, parent});
    updateauthaction.send(account, app, parent, authority);
}

void apps::scbuyram(const name &account_name, const asset &quant)
{
    require_auth(account_name);

    tonomy::account_type_table account_type(get_self(), get_self().value);
    auto itr = account_type.find(account_name.value);
    eosio::check(itr != account_type.end(), "Could not find account");
    eosio::check(itr->acc_type == enum_account_type::App, "Only apps can buy and sell RAM");

    eosio::check(quant.symbol == tonomy::system_resource_currency, "must buy ram with core token");
    eosio::check(quant.amount > 0, "Amount must be positive");

    tonomy::resource_config_table config_table(get_self(), get_self().value);
    tonomy::resource_config config;
    eosio::check(config_table.exists(), "Resource config does not exist");
    config = config_table.get();

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
    eosio::get_resource_limits(account_name, myRAM, myNET, myCPU);
    eosio::print("{\"event_log\":{\"account\":\"tonomy\",\"action\":\"scbuyram\"},\"time\":\"", eosio::current_time_point().to_string(), "Z\",\"events\":[{\"previous\":", myRAM, ",\"current\":", myRAM + ram_purchase, "}]\"}");
    eosio::set_resource_limits(account_name, myRAM + ram_purchase, myNET, myCPU);

    eosio::action(permission_level{account_name, "active"_n},
                  tonomy::token_contract_name,
                  "transfer"_n,
                  std::make_tuple(account_name, native::governance_name, quant, std::string("buy ram")))
        .send();
}

void apps::scsellram(const name &account_name, const asset &quant)
{
    require_auth(account_name);

    tonomy::account_type_table account_type(get_self(), get_self().value);
    auto itr = account_type.find(account_name.value);
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
    eosio::get_resource_limits(account_name, myRAM, myNET, myCPU);
    eosio::print("{\"event_log\":{\"account\":\"tonomy\",\"action\":\"scsellram\"},\"time\":\"", eosio::current_time_point().to_string(), "Z\",\"events\":[{\"previous\":", myRAM, ",\"current\":", myRAM - ram_sold, "}]\"}");
    eosio::check(myRAM - ram_sold >= 0, "Account cannot have less than 0 RAM");
    eosio::set_resource_limits(account_name, myRAM - ram_sold, myNET, myCPU);

    eosio::action(permission_level{get_self(), "active"_n},
                  tonomy::token_contract_name,
                  "transfer"_n,
                  std::make_tuple(native::governance_name, account_name, eosio::asset(ram_sold, tonomy::system_resource_currency), std::string("sell ram")))
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

void apps::appcreate(name creator,
                     string json_data,
                     string username,
                     string origin)
{
    require_auth(creator);

    // Uniqueness checks for username and origin
    check_app_username_chars(username);
    checksum256 username_hash = eosio::sha256(username.c_str(), std::strlen(username.c_str()));
    {
        auto uidx = _appsv3.get_index<"usernamehash"_n>();
        check(uidx.find(username_hash) == uidx.end(), "Username already taken");
    }
    checksum256 origin_hash = eosio::sha256(origin.c_str(), std::strlen(origin.c_str()));
    {
        auto oidx = _appsv3.get_index<"originhash"_n>();
        check(oidx.find(origin_hash) == oidx.end(), "Origin already taken");
    }

    // Generate random account name from username and json_data hashes
    auto json_hash = eosio::sha256(json_data.c_str(), std::strlen(json_data.c_str()));
    const eosio::name random_name = random_account_name(username_hash, json_hash, enum_account_type::App);

    // Create account with owner=gov.tmy, active=creator
    authority owner_authority = create_authority_with_account(app_controller_account);
    authority active_authority = create_authority_with_account(creator);
    active_authority.accounts.push_back({.permission = create_eosio_code_permission_level(get_self()), .weight = 1});

    newaccount_action newaccountaction("eosio"_n, {get_self(), "active"_n});
    newaccountaction.send(get_self(), random_name, owner_authority, active_authority);

    // Update resource config
    tonomy::resource_config_table _resource_config(get_self(), get_self().value);
    auto config = _resource_config.get();
    config.total_cpu_weight_allocated = this->initial_cpu_weight_allocation;
    config.total_net_weight_allocated = this->initial_net_weight_allocation;
    _resource_config.set(config, get_self());

    // Set resource limits: ram=0, net=initial, cpu=initial
    eosio::set_resource_limits(random_name, 0, this->initial_net_weight_allocation, this->initial_cpu_weight_allocation);

    // Register in appsv3
    _appsv3.emplace(get_self(), [&](auto &row) {
        row.account_name = random_name;
        row.json_data = json_data;
        row.version = 3;
        row.username = username;
        row.origin = origin;
        row.plan = static_cast<uint8_t>(plan_t::plan_basic);
    });

    // Set account type
    tonomy::account_type_table account_type(get_self(), get_self().value);
    account_type.emplace(get_self(), [&](auto &row) {
        row.account_name = random_name;
        row.acc_type = enum_account_type::App;
        row.version = 1;
    });
}

void apps::appupdate(name account_name,
                     string json_data,
                     string username)
{
    require_auth(account_name);
    auto itr = _appsv3.find(account_name.value);
    check(itr != _appsv3.end(), "App does not exist");

    // If username changed, ensure uniqueness
    if (itr->username != username) {
        check_app_username_chars(username);
        checksum256 username_hash = eosio::sha256(username.c_str(), std::strlen(username.c_str()));
        auto uidx = _appsv3.get_index<"usernamehash"_n>();
        check(uidx.find(username_hash) == uidx.end(), "Username already taken");
    }

    _appsv3.modify(itr, get_self(), [&](auto &row) {
        row.json_data = json_data;
        row.username = username;
        // keep origin unchanged here; only admin can change origin
        row.version = 3;
    });
}

void apps::appupdplan(name account_name,
                      uint8_t plan)
{
    // Plan updates assumed admin-governed
    require_auth(get_self());
    auto itr = _appsv3.find(account_name.value);
    check(itr != _appsv3.end(), "App does not exist");
    _appsv3.modify(itr, get_self(), [&](auto &row) {
        row.plan = plan;
    });
}

void apps::scdeploy(name account_name,
                    uint8_t vmtype,
                    uint8_t vmversion,
                    const std::vector<char> &code,
                    const std::vector<char> &abi,
                    string source_code_url)
{
    require_auth(get_self());
    // TODO: Implement smart contract deployment
    check(false, "scdeploy not yet implemented");
}

void apps::scupdate(name account_name,
                    uint8_t vmtype,
                    uint8_t vmversion,
                    const std::vector<char> &code,
                    const std::vector<char> &abi,
                    string source_code_url)
{
    require_auth(get_self());
    // TODO: Implement smart contract update
    check(false, "scupdate not yet implemented");
}

void apps::appaddkey(name account_name,
                     public_key key)
{
    require_auth(get_self());
    // TODO: Implement key addition to app account's active permission
    check(false, "appaddkey not yet implemented");
}

void apps::appremkey(name account_name,
                     public_key key)
{
    require_auth(get_self());
    // TODO: Implement key removal from app account's active permission
    check(false, "appremkey not yet implemented");
}

void apps::admnmigapp(name account_name,
                      string username,
                      uint8_t plan,
                      public_key key)
{
    require_auth(get_self());
    // TODO: Implement V2 to V3 app migration
    check(false, "admnmigapp not yet implemented");
}

void apps::admnmigsc(name account_name,
                     string source_code_url)
{
    require_auth(get_self());
    // TODO: Implement smart contract metadata migration
    check(false, "admnmigsc not yet implemented");
}

} // namespace tonomysystem
