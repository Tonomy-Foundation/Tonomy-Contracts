#!/bin/bash

BUILD_METHOD=$1

PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "${PARENT_PATH}"

# DOCKER_WORKING_DIR="/contract/id.tonomy"
# CONTRACT_NAME="id.tonomy"
# docker run\
#     -v "${PARENT_PATH}/../eosio.bios/include/eosio.bios:${DOCKER_WORKING_DIR}/include/eosio.bios"\
#     -v "${PARENT_PATH}/../:${DOCKER_WORKING_DIR}/../"\
#     -v "${PARENT_PATH}:${DOCKER_WORKING_DIR}"\

function compile_contract_id_tonomy {
    PARENT_PATH=$1
    CONTRACT_NAME=$2
    BUILD_METHOD=$3

    if [ "$BUILD_METHOD" == "local" ]; then
        WORKING_DIR="."
    else
        WORKING_DIR="/contracts/id.tonomy"
    fi

    BUILD_COMMAND="eosio-cpp -abigen -I ${WORKING_DIR}/include -R ${WORKING_DIR}/ricardian -contract ${CONTRACT_NAME} -o ${WORKING_DIR}/${CONTRACT_NAME}.wasm ${WORKING_DIR}/src/${CONTRACT_NAME}.cpp"
    echo $BUILD_COMMAND
    echo ""

    cp -r "${PARENT_PATH}/../eosio.bios/include/eosio.bios" "${PARENT_PATH}/include/eosio.bios"
    if [ "$BUILD_METHOD" == "local" ]; then
        bash -c "${BUILD_COMMAND}"
    else
        docker run\
            -v "${PARENT_PATH}:${WORKING_DIR}"\
            -v "${PARENT_PATH}/..:${WORKING_DIR}/.."\
            eosio/eosio.cdt:v1.8.1 bash -c "${BUILD_COMMAND}"
    fi
    rm -rf "${PARENT_PATH}/include/eosio.bios"
}

compile_contract_id_tonomy "${PARENT_PATH}" "id.tonomy" "${BUILD_METHOD}"
