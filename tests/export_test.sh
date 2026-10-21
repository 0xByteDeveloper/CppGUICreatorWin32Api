#!/usr/bin/env bash
# export_test.sh — end-to-end export pipeline verification:
#   1. generate standalone app source from assets/default_MyForm.layout
#   2. cross-compile it together with runtime/W32GuiRuntime.h
set -e
cd "$(dirname "$0")/.."
REPO="$(pwd)"
MINGW_BIN="${MINGW_BIN:-$(dirname "$MINGW_CXX")}"

echo "[1] generating export source from assets/default_MyForm.layout ..."
g++ -std=c++17 -Isrc -o /tmp/gen_export tests/gen_export_test.cpp \
    src/Util.cpp src/Model.cpp src/CodeGen.cpp
/tmp/gen_export "$REPO/assets/default_MyForm.layout" > /tmp/MyForm_w32.cpp
echo "    -> $(wc -l < /tmp/MyForm_w32.cpp) lines generated"

echo "[2] cross-compiling generated app ..."
CXX="${MINGW_CXX:-x86_64-w64-mingw32-g++}"
"$CXX" -std=c++17 -O2 -municode -mwindows -Iruntime \
    /tmp/MyForm_w32.cpp -o /tmp/MyForm_w32.exe \
    -lcomctl32 -lgdi32 -luser32
echo "[ok] generated standalone app compiled: /tmp/MyForm_w32.exe"
echo "     W32GuiRuntime.h + generated handlers + bind table: VERIFIED"
