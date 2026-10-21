#!/usr/bin/env bash
# run.sh — build & run portable core unit tests with the native compiler
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -Wall -Wextra -I../src -o test_core test_core.cpp \
    ../src/Util.cpp ../src/Model.cpp ../src/CodeGen.cpp \
    -DTEST_BUILD 2>&1
./test_core
