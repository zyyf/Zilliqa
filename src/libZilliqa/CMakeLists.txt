add_library (Zilliqa Zilliqa.cpp)
add_dependencies(Zilliqa jsonrpc-project)
target_include_directories (Zilliqa PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries (Zilliqa PUBLIC Consensus Crypto Lookup Mediator Network Node Server)
