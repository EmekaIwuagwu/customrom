cmake_minimum_required(VERSION 3.10)
project(SatordTests)

set(CMAKE_CXX_STANDARD 17)

# Find GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(../../vendor/google/services/satord)

# Add the test executable
add_executable(satord_test satord_test.cpp ../../vendor/google/services/satord/satord_utils.cpp)

# Link GTest
target_link_libraries(satord_test ${GTEST_LIBRARIES} pthread)
