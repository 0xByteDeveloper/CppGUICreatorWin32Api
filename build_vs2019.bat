@echo off
rem ============================================================
rem  build_vs2019.bat — build with Visual Studio 2019 (MSBuild)
rem  Requires: VS2019 with "Desktop development with C++" workload
rem            (includes Windows 10 SDK; VS2019 SDK is NOT needed)
rem ============================================================
setlocal
where msbuild >nul 2>nul
if %errorlevel%==0 (
  msbuild CppGUICreatorWin32Api.vcxproj /p:Configuration=Release /p:Platform=x64 /m /v:m
  goto :done
)

rem try common VS2019 locations
for %%P in (
  "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
  "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe"
  "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
  "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
) do (
  if exist %%P (
    echo Using %%P
    %%P CppGUICreatorWin32Api.vcxproj /p:Configuration=Release /p:Platform=x64 /m /v:m
    goto :done
  )
)
echo MSBuild bulunamadi. Visual Studio 2019 kurulu mu?
echo Alternatif: build_mingw.bat (MinGW ile derle)
exit /b 1

:done
echo.
echo Cikti: bin\x64\Release\CppGUICreatorWin32Api.exe
endlocal
