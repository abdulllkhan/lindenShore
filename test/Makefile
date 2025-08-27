# Makefile for Google Test with pnl_calculator_main tests

CXX = g++
CXXFLAGS = -Wall -g -std=c++98
GTEST_FLAGS = -lgtest -lgtest_main -lpthread

# Default target
all: test_pnl_calculator_main

# Build test executable
test_pnl_calculator_main: test_pnl_calculator_main.cpp
	$(CXX) $(CXXFLAGS) -o test_pnl_calculator_main test_pnl_calculator_main.cpp $(GTEST_FLAGS)

# Run tests
test: test_pnl_calculator_main
	./test_pnl_calculator_main

# Clean
clean:
	rm -f test_pnl_calculator_main test_parse.csv test_empty.csv

.PHONY: all test clean