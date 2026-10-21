@echo off
rem ============================================================
rem  build_mingw.bat — build with MinGW-w64 (no Visual Studio)
rem  Requires: x86_64-w64-mingw32-g++ or g++ (MinGW) in PATH
rem ============================================================
setlocal
set SOURCES=src\main.cpp src\Util.cpp src\Model.cpp src\Theme.cpp src\Store.cpp ^
 src\VSDiscovery.cpp src\CodeGen.cpp src\DTEBridge.cpp src\Bars.cpp ^
 src\WelcomePage.cpp src\Designer.cpp src\Designer2.cpp src\Dialogs.cpp src\MainWindow.cpp
set LIBS=-lgdiplus -lcomctl32 -lcomdlg32 -lshlwapi -lole32 -loleaut32 -luuid -lstdc++fs
set CXXFLAGS=-std=c++17 -O2 -municode -mwindows -DUNICODE -D_UNICODE -DNOMINMAX -Isrc -Iruntime

windres mingw.rc -O coff -o mingw.res 2>nul
if exist mingw.res (set RES=mingw.res) else (set RES=)

where x86_64-w64-mingw32-g++ >nul 2>nul
if %errorlevel%==0 (
  x86_64-w64-mingw32-g++ %CXXFLAGS% %SOURCES% %RES% %LIBS% -o CppGUICreatorWin32Api.exe
  goto :done
)
where g++ >nul 2>nul
if %errorlevel%==0 (
  g++ %CXXFLAGS% %SOURCES% %RES% %LIBS% -o CppGUICreatorWin32Api.exe
  goto :done
)
echo Derleyici bulunamadi: MinGW-w64 kurun (veya VS2019 + build_vs2019.bat)
exit /b 1

:done
echo Cikti: CppGUICreatorWin32Api.exe
endlocal
