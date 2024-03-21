add_contract(vestng.token vestng.token ${CMAKE_CURRENT_SOURCE_DIR}/src/vestng.token.cpp)

target_include_directories(vestng.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(vestng.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/vestng.token.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/vestng.token.contracts.md @ONLY )

target_compile_options( vestng.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
