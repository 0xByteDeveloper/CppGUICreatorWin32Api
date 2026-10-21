#!/usr/bin/env bash
# build.sh — cross-compile CppGUICreatorWin32Api from Linux (llvm-mingw / mingw-w64)
# Usage: MINGW_CXX=/path/to/x86_64-w64-mingw32-g++ ./build.sh
set -e
cd "$(dirname "$0")"

CXX="${MINGW_CXX:-x86_64-w64-mingw32-g++}"
WINDRES="${MINGW_RC:-x86_64-w64-mingw32-windres}"

SOURCES="src/main.cpp src/Util.cpp src/Model.cpp src/Theme.cpp src/Store.cpp \
src/VSDiscovery.cpp src/CodeGen.cpp src/DTEBridge.cpp src/Bars.cpp \
src/WelcomePage.cpp src/Designer.cpp src/Designer2.cpp src/Dialogs.cpp src/MainWindow.cpp"

FLAGS="-std=c++17 -O2 -municode -mwindows -static -static-libgcc -static-libstdc++ \
  -DUNICODE -D_UNICODE -DNOMINMAX -Isrc -Iruntime"
LIBS="-lgdiplus -lcomctl32 -lcomdlg32 -lshlwapi -lole32 -loleaut32 -luuid -lwinmm -lgdi32 -lcomdlg32 -lpsapi -luser32 -lshell32 -ladvapi32 -lws2_32"

RES=""
if command -v "$WINDRES" >/dev/null 2>&1; then
  "$WINDRES" mingw.rc -O coff -o mingw.res
  RES="mingw.res"
fi

echo "[build] $CXX"
"$CXX" $FLAGS $SOURCES $RES $LIBS -o CppGUICreatorWin32Api.exe
echo "[ok] CppGUICreatorWin32Api.exe"
