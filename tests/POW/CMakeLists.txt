

add_executable (Test_POW test_POW.cpp)
target_include_directories (Test_POW PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(Test_POW PUBLIC ethash POW DirectoryService Lookup Node Server Utils Crypto Boost::unit_test_framework Boost::filesystem)
add_test(NAME Test_POW COMMAND Test_POW)


add_executable (Test_RemoteMine test_RemoteMine.cpp)
target_link_libraries(Test_RemoteMine PUBLIC ethash POW DirectoryService Lookup Node Server Utils Crypto Boost::unit_test_framework Boost::filesystem)
target_include_directories (Test_RemoteMine PUBLIC ${PROJECT_SOURCE_DIR}/src)
