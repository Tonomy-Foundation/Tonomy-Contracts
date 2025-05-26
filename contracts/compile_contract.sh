#!/bin/bash

function compile_contract {
    PARENT_PATH=$1
    CONTRACT_NAME=$2
    BUILD_METHOD=$3

    if [ "$BUILD_METHOD" == "local" ]; then
        WORKING_DIR="."
    else
        WORKING_DIR="/contracts"
    fi
    
    # if BUILD_TEST environment variable is "true" add flag https://t.me/antelopedevs/209018
    TEST_FLAG=""
    if [ "$BUILD_TEST" == "true" ]; then
        TEST_FLAG="-DBUILD_TEST"
    fi
    BUILD_COMMAND="cdt-cpp ${TEST_FLAG} -abigen -I ${WORKING_DIR}/include -R ${WORKING_DIR}/ricardian -contract ${CONTRACT_NAME} -o ${WORKING_DIR}/${CONTRACT_NAME}.wasm ${WORKING_DIR}/src/${CONTRACT_NAME}.cpp"
    echo $BUILD_COMMAND

    if [ "$BUILD_METHOD" == "local" ]; then
        bash -c "${BUILD_COMMAND}"
    else
        docker run -v "${PARENT_PATH}:${WORKING_DIR}" antelope_blockchain bash -c "${BUILD_COMMAND}"
    fi

    # copy all the .abi files in ${WORKING_DIR} to .abi.json
    cp "${PARENT_PATH}/${CONTRACT_NAME}.abi" "${PARENT_PATH}/${CONTRACT_NAME}.abi.json"
}