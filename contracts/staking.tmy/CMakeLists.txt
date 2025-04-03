add_contract(staking.tmy staking.tmy ${CMAKE_CURRENT_SOURCE_DIR}/src/staking.tmy.cpp)

target_include_directories(staking.tmy
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(staking.tmy
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/staking.tmy.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/staking.tmy.contracts.md @ONLY )

target_compile_options( staking.tmy PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
