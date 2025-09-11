@echo off
setlocal ENABLEDELAYEDEXPANSION

rem === Script folder and config file ===
set "SCRIPT_DIR=%~dp0"
set "CFG_FILE=%SCRIPT_DIR%llvm-path.txt"

rem === Load path from file if it exists ===
if exist "%CFG_FILE%" (
    set /p TOOLCHAIN=<"%CFG_FILE%"
)

:CHECK_PATH
if not defined TOOLCHAIN (
    set /p "TOOLCHAIN=Please enter the path to llvm-mingw: "
)

if not exist "%TOOLCHAIN%" (
    echo [Error] Path "%TOOLCHAIN%" does not exist.
    set "TOOLCHAIN="
    goto CHECK_PATH
)

rem Save the valid path into file
echo %TOOLCHAIN%>"%CFG_FILE%"

rem === Set bin folder ===
set "BIN=%TOOLCHAIN%\bin"
echo Using LLVM MinGW toolchain at: %TOOLCHAIN%
echo.




echo Using toolchain: %TOOLCHAIN%
if not exist "%BIN%" (
	echo ERROR: Toolchain bin directory not found: %BIN%
	exit /b 1
)

set "CC="
set "CC_NEEDS_TARGET=0"
set "CC_IS_GNU=0"
set "RC=%BIN%\llvm-rc.exe"

if exist "%BIN%\clang++.exe" (
	set "CC=%BIN%\clang++.exe"
	set "CC_NEEDS_TARGET=1"
) else if exist "%BIN%\aarch64-w64-mingw32-clang++.exe" (
	set "CC=%BIN%\aarch64-w64-mingw32-clang++.exe"
) else if exist "%BIN%\aarch64-w64-mingw32-g++.exe" (
	set "CC=%BIN%\aarch64-w64-mingw32-g++.exe"
	set "CC_IS_GNU=1"
)

if not defined CC (
	echo ERROR: No suitable C++ compiler found in %BIN% ^(clang++, aarch64-w64-mingw32-clang++, or aarch64-w64-mingw32-g++^)
	exit /b 1
)

if not exist "%RC%" (
	echo NOTE: llvm-rc not found at %RC%
	echo Falling back to windres if available.
	set "RC=%BIN%\windres.exe"
	if not exist "%RC%" (
		set "RC=%BIN%\aarch64-w64-mingw32-windres.exe"
	)
	if not exist "%RC%" (
		echo ERROR: Neither llvm-rc nor windres found in %BIN%.
		exit /b 1
	)
)

set SRC=Auto_Modificator_019_C++.cpp
set RES=Auto_Modificator_019_C++.rc
set OUT=Auto_Modificator_llvm_019_arm64.exe

rem Compile resources -> build.res (llvm-rc) or build.o (windres)
set "RESOBJ=build.res"
for %%F in ("%RC%") do set RCBASENAME=%%~nF
if /I "%RCBASENAME%"=="windres" (
	set "RESOBJ=build.res.o"
	"%RC%" -I . -O coff -i "%RES" -o "%RESOBJ%"
) else (
	"%RC%" /nologo /FO "%RESOBJ%" "%RES%"
)
if errorlevel 1 exit /b 1

rem Build with clang++/g++ for aarch64-w64-windows-gnu target
set "TARGET_FLAG="
if "%CC_NEEDS_TARGET%"=="1" set "TARGET_FLAG=--target=aarch64-w64-windows-gnu"

"%CC%" ^
	%TARGET_FLAG% ^
	-std=c++17 -O2 -DNOMINMAX -DUNICODE -D_UNICODE -municode ^
	-mwindows ^
	-static -static-libstdc++ -static-libgcc -fuse-ld=lld ^
	"%SRC%" "%RESOBJ%" ^
	-lcomctl32 -lshell32 -luser32 -lgdi32 -lwinmm ^
	-Wl,-s ^
	-o "%OUT%"

if errorlevel 1 (
	echo Build failed.
	exit /b 1
)

echo Build succeeded: %OUT%
endlocal 