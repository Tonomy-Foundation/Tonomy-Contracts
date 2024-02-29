add_contract(vesting.token vesting.token ${CMAKE_CURRENT_SOURCE_DIR}/src/vesting.token.cpp)

target_include_directories(vesting.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(vesting.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/vesting.token.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/vesting.token.contracts.md @ONLY )

target_compile_options( vesting.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
