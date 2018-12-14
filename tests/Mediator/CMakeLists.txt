link_directories(${CMAKE_BINARY_DIR}/lib)

add_executable(Test_Mediator Mocks/libDirectoryService/DirectoryService.cpp Test_Mediator.cpp ${CMAKE_SOURCE_DIR}/src/libMediator/Mediator.cpp ${CMAKE_SOURCE_DIR}/src/libUtils/Logger.cpp ${CMAKE_SOURCE_DIR}/src/libUtils/TimeUtils.cpp ${CMAKE_SOURCE_DIR}/src/libUtils/DataConversion.cpp)
target_include_directories (Test_Mediator PUBLIC ${G3LOG_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/tests/Mediator/Mocks ${CMAKE_SOURCE_DIR}/tests/ ${CMAKE_SOURCE_DIR}/src/libMediator ${CMAKE_SOURCE_DIR}/googletest-src/googletest/include ${CMAKE_SOURCE_DIR}/googletest-src/googlemock/include ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(Test_Mediator PUBLIC g3logger gmock_main TestUtils)
add_test(NAME Test_Mediator COMMAND Test_Mediator)
