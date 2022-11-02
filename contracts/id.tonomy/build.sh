#!/bin/bash

PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )

DOCKER_WORKING_DIR="/contract/id.tonomy"
CONTRACT_NAME="id.tonomy"
docker run\
    -v "${PARENT_PATH}/../eosio.bios/include/eosio.bios:${DOCKER_WORKING_DIR}/include/eosio.bios"\
    -v "${PARENT_PATH}/../:${DOCKER_WORKING_DIR}/../"\
    -v "${PARENT_PATH}:${DOCKER_WORKING_DIR}"\
    eosio/eosio.cdt:v1.8.1\
    eosio-cpp\
    -abigen\
    -I "${DOCKER_WORKING_DIR}/include"\
    -R "${DOCKER_WORKING_DIR}"/ricardian\
    -contract "${CONTRACT_NAME}"\
    -o "${DOCKER_WORKING_DIR}/${CONTRACT_NAME}.wasm"\
    "${DOCKER_WORKING_DIR}/src/${CONTRACT_NAME}.cpp"