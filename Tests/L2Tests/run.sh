#!/bin/bash 
# This script will build and run the L2 tests

rm -rf build 

mkdir build

cd build

export GTEST_OUTPUT="json"

cmake ..

make

./L2Test

#Generate coverage report
if [ "$1" != "" ] && [ $1 = "-c" ]; then
	lcov --capture --directory . --output-file coverage.info
	lcov --remove coverage.info '/usr/*' --output-file coverage.info
	lcov --list coverage.info
	genhtml coverage.info --output-directory coverage_report
fi


