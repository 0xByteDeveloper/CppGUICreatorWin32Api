#!/bin/bash
# M2GodWin32 Faz 2.0 — mingw-w64 derleme betiği
# Gereksinim: x86_64-w64-mingw32-g++-posix ve windres (PATH'te)
set -e
cd "$(dirname "$0")/.."   # proje kökü (M2GodWin32 klasörü)
mkdir -p Bin obj
echo "[1/3] Kaynak (.rc) derleniyor..."
x86_64-w64-mingw32-windres --preprocessor x86_64-w64-mingw32-gcc-posix \
  --preprocessor-arg -E --preprocessor-arg -xc --preprocessor-arg -DRC_INVOKED \
  build/app.rc -O coff -o obj/app.res
echo "[2/3] C++ kaynakları derleniyor..."
SRCS="src/main.cpp src/utils.cpp src/json.cpp src/catalog.cpp src/model.cpp src/factory.cpp src/surface.cpp src/toolbox.cpp src/props.cpp src/codegen.cpp src/vsint.cpp src/mainwnd.cpp"
FLAGS="-std=c++17 -municode -O2 -Isrc -Wall -Wno-unused-function"
OBJS=""
for f in $SRCS; do
  o="obj/$(basename "$f" .cpp).o"
  x86_64-w64-mingw32-g++-posix $FLAGS -c "$f" -o "$o"
  OBJS="$OBJS $o"
done
echo "[3/3] Bağlanıyor..."
x86_64-w64-mingw32-g++-posix $OBJS obj/app.res -o Bin/M2GodWin32.exe \
  -municode -mwindows -static -static-libgcc -static-libstdc++ \
  -lgdi32 -lcomctl32 -lcomdlg32 -lshell32 -lshlwapi -lole32 -loleaut32 -luuid -ladvapi32
echo "OK: Bin/M2GodWin32.exe"
