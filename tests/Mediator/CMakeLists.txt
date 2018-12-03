link_directories(${CMAKE_BINARY_DIR}/lib)
configure_file(${CMAKE_SOURCE_DIR}/tests/Mediator/constants.xml constants.xml COPYONLY)

add_executable(Test_Mediator Test_Mediator.cpp)
target_include_directories (Test_Mediator PUBLIC ${CMAKE_BINARY_DIR}/src ${CMAKE_BINARY_DIR}/googletest-src/googletest/include/ ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(Test_Mediator PUBLIC AccountData Message Utils TestUtils Mediator Validator gmock_main)
gtest_add_tests(Test_Mediator)
#add_test(NAME Test_Mediator COMMAND Test_Mediator)
