@echo off
rem Build Rework Time with the Qt that ships on this machine (MinGW kit).
set "PATH=C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\KNOT\TOOLS\cmake\bin;C:\KNOT\TOOLS\ninja;%PATH%"

cmake -S "%~dp0." -B "%~dp0build" -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe ^
  -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
if errorlevel 1 exit /b 1

cmake --build "%~dp0build"
if errorlevel 1 exit /b 1

echo.
echo Built: %~dp0build\reworktime.exe
