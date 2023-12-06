add_contract(onocoin.tmy onocoin.tmy ${CMAKE_CURRENT_SOURCE_DIR}/src/onocoin.tmy.cpp)

target_include_directories(onocoin.tmy
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(onocoin.tmy
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/onocoin.tmy.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/onocoin.tmy.contracts.md @ONLY )

target_compile_options( onocoin.tmy PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
