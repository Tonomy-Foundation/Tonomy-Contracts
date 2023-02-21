#!/bin/bash

ARG1=$1

if [ "$ARG1" == "local" ]; then
    WORKING_DIR="./contract"
else
    WORKING_DIR="/contract"
fi
CONTRACT_NAME="eosio.bios"

BUILD_COMMAND="eosio-cpp -abigen -I ${WORKING_DIR}/include -R ${WORKING_DIR}/ricardian -contract ${CONTRACT_NAME} -o ${WORKING_DIR}/${CONTRACT_NAME}.wasm ${WORKING_DIR}/src/${CONTRACT_NAME}.cpp"
echo $BUILD_COMMAND

if [ "$ARG1" == "local" ]; then
    bash -c "${BUILD_COMMAND}"
else
    docker run -v "${PARENT_PATH}:${WORKING_DIR}" eosio/eosio.cdt:v1.8.1 bash -c "${BUILD_COMMAND}"
fi