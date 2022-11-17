cmake_minimum_required (VERSION 3.12)

project ("kvstor_test")

include_directories(doctest)
file(GLOB TESTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

message(STATUS "CMAKE_CURRENT_BINARY_DIR=" ${CMAKE_CURRENT_BINARY_DIR})

foreach(TEST_SOURCE ${TESTS})
    string(REPLACE ".cpp" "" TEST_TARGET "${TEST_SOURCE}")
    add_executable(${TEST_TARGET} ${TEST_SOURCE})
    add_test("${TEST_TARGET}" "${TEST_TARGET}" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} --verbose)
endforeach()

set_property(TARGET kvstor_test PROPERTY CXX_STANDARD 20)

if (UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif (UNIX)

