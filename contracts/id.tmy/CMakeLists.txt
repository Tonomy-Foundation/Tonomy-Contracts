add_contract(id.tmy id.tmy ${CMAKE_CURRENT_SOURCE_DIR}/src/id.tmy.cpp)

target_include_directories(id.tmy
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(id.tmy
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/eosio.boot.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/id.tmy.contracts.md @ONLY )

target_compile_options( id.tmy PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
