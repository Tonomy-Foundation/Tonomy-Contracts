#!/bin/bash

# If ARG1=local then will use local cdt-cpp, otherwise will use docker cdt-cpp
ARG1=$1

set -u ## exit if you try to use an uninitialised variable
set -e ## exit if any statement fails

# Make sure working dir is same as this dir, so that script can be excuted from another working directory
PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

echo "Building smart contracts"

CONTRACTS=(
    "demo.tmy"
    "eosio.bios"
    "eosio.boot"
    "eosio.msig"
    "eosio.token"
    "eosio.tonomy"
    "tonomy"
    "vestng.token"
)

for CONTRACT in "${CONTRACTS[@]}"
do
    cd "${PARENT_PATH}/contracts/${CONTRACT}"
    if [ -e "${CONTRACT}.wasm" ]
    then
        echo "${CONTRACT} already built"
    else
        ./build.sh "${ARG1}"
    fi
done

echo "All contracts built or skipped sucessfully"