cmake_minimum_required(VERSION 3.10)
project(PnLCalculatorMainTests)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add executable
add_executable(test_pnl_calculator_main test_pnl_calculator_main.cpp)

# Link Google Test
target_link_libraries(test_pnl_calculator_main ${GTEST_LIBRARIES} pthread)

# Enable testing
enable_testing()
add_test(NAME PnLCalculatorMainTests COMMAND test_pnl_calculator_main)