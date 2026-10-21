cmake_minimum_required(VERSION 3.15)
project(CppGUICreatorWin32Api CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT WIN32)
    message(FATAL_ERROR "CppGUICreatorWin32Api builds only on Windows (Win32 API).")
endif()

add_executable(CppGUICreatorWin32Api WIN32
    src/main.cpp
    src/Util.cpp
    src/Model.cpp
    src/Theme.cpp
    src/Store.cpp
    src/VSDiscovery.cpp
    src/CodeGen.cpp
    src/DTEBridge.cpp
    src/Bars.cpp
    src/WelcomePage.cpp
    src/Designer.cpp
    src/Designer2.cpp
    src/Dialogs.cpp
    src/MainWindow.cpp
    app.rc
    app.manifest
)

target_include_directories(CppGUICreatorWin32Api PRIVATE src runtime)
target_compile_definitions(CppGUICreatorWin32Api PRIVATE UNICODE _UNICODE NOMINMAX)
if (MSVC)
    target_compile_options(CppGUICreatorWin32Api PRIVATE /utf-8 /W4 /permissive-)
    target_link_libraries(CppGUICreatorWin32Api PRIVATE
        user32 gdi32 gdiplus comctl32 comdlg32 shell32 advapi32 ole32 oleaut32 uuid shlwapi)
else()
    # MinGW / clang-mingw
    target_link_libraries(CppGUICreatorWin32Api PRIVATE
        gdiplus comctl32 comdlg32 shlwapi ole32 oleaut32 uuid dwrite)
endif()

# copy assets next to the binary for dev runs
add_custom_command(TARGET CppGUICreatorWin32Api POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:CppGUICreatorWin32Api>/assets)
