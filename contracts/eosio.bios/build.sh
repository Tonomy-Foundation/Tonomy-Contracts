#!/bin/bash

BUILD_METHOD=$1

PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "${PARENT_PATH}"

function compile_contract_id_tonomy {
    PARENT_PATH=$1
    CONTRACT_NAME=$2
    BUILD_METHOD=$3

    if [ "$BUILD_METHOD" == "local" ]; then
        WORKING_DIR="."
    else
        WORKING_DIR="/contracts/eosio.bios"
    fi

    BUILD_COMMAND="cdt-cpp -abigen -I ${WORKING_DIR}/include -R ${WORKING_DIR}/ricardian -contract ${CONTRACT_NAME} -o ${WORKING_DIR}/${CONTRACT_NAME}.wasm ${WORKING_DIR}/src/${CONTRACT_NAME}.cpp"
    echo $BUILD_COMMAND
    echo ""

    cp -r "${PARENT_PATH}/../id.tmy/include/id.tmy" "${PARENT_PATH}/include/id.tmy"
    if [ "$BUILD_METHOD" == "local" ]; then
        bash -c "${BUILD_COMMAND}"
    else
        docker run\
            -v "${PARENT_PATH}:${WORKING_DIR}"\
            -v "${PARENT_PATH}/..:${WORKING_DIR}/.."\
            tonomy/antelope bash -c "${BUILD_COMMAND}"
    fi
    rm -rf "${PARENT_PATH}/include/id.tmy"
}

compile_contract_id_tonomy "${PARENT_PATH}" "eosio.bios" "${BUILD_METHOD}"
