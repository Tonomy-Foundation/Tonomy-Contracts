#!/bin/bash

BUILD_METHOD=$1

PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "${PARENT_PATH}"

# source ../compile_contract.sh

# compile_contract "${PARENT_PATH}" "tonomy" "${BUILD_METHOD}"

CONTRACT_NAME="tonomy"

WORKING_DIR="/contracts"

BUILD_COMMAND="cdt-cpp -abigen -I ${WORKING_DIR}/include -R ${WORKING_DIR}/ricardian -contract ${CONTRACT_NAME} -o ${WORKING_DIR}/${CONTRACT_NAME}.wasm ${WORKING_DIR}/src/${CONTRACT_NAME}.cpp ${WORKING_DIR}/src/native.cpp"

echo $BUILD_COMMAND

docker run -v "${PARENT_PATH}:${WORKING_DIR}" antelope_blockchain bash -c "${BUILD_COMMAND}"