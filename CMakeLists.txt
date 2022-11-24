cmake_minimum_required (VERSION 3.12)

project ("kvstor")

include_directories(include)

enable_testing()

add_library(kvstor INTERFACE)
target_include_directories(kvstor INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

add_subdirectory(tests)

set_property(TARGET kvstor PROPERTY CXX_STANDARD 17)


