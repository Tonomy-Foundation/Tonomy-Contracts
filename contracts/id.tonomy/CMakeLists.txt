add_contract(id.tonomy id.tonomy ${CMAKE_CURRENT_SOURCE_DIR}/src/id.tonomy.cpp)

target_include_directories(id.tonomy
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(id.tonomy
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/eosio.boot.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/id.tonomy.contracts.md @ONLY )

target_compile_options( id.tonomy PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
