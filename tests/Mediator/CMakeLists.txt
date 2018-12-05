add_executable(Test_Mediator Test_Mediator.cpp ${CMAKE_SOURCE_DIR}/src/libMediator/Mediator.cpp)
target_include_directories (Test_Mediator PUBLIC ${CMAKE_SOURCE_DIR}/tests ${CMAKE_SOURCE_DIR}/tests/Mediator/Mocks ${CMAKE_SOURCE_DIR}/src/libMediator ${CMAKE_SOURCE_DIR}/googletest-src/googletest/include/ ${CMAKE_SOURCE_DIR}/googletest-src/googlemock/include/)
target_link_libraries(Test_Mediator PUBLIC TestUtils gmock_main)
add_test(NAME Test_Mediator COMMAND Test_Mediator)
