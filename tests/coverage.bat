@echo off

set LLVM=C:\Qt\Tools\llvm-mingw1706_64\bin
set BUILD=D:\MF\PhotoDB\MediaBase\tests\build\Desktop_Qt_6_11_1_llvm_mingw_64bit-Debug

cd /d %BUILD%

test_settingsmanager.exe

%LLVM%\llvm-profdata.exe merge default.profraw -o default.profdata

%LLVM%\llvm-cov.exe report test_settingsmanager.exe ^
    -instr-profile=default.profdata

%LLVM%\llvm-cov.exe show test_settingsmanager.exe ^
    -instr-profile=default.profdata ^
    -format=html ^
    -output-dir=coverage

start coverage\index.html