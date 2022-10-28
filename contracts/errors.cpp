#include <eosio/eosio.hpp>
#include <string>

using eosio::check;
using std::string;

void throwError(string error_code, string message)
{
    check(false, error_code + ": " + message);
}

/*
List of which files have which error codes, to avoid duplicates:export function throwError(string error_code, string message) {

TCON10## = id.tonomy.cpp
*/
