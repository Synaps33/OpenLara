@echo off
REM ========================================
REM  FrogUI Bare-Metal Build Script for OpenLara
REM ========================================

set "MULTICORE_PATH=D:\sf2000_multicore"

echo ========================================
echo  FrogUI - OpenLara Build
echo ========================================
echo.

REM Copy the OpenLara source files into the core directory
xcopy /E /I /Y "%~dp0repos\OpenLara\src" "%~dp0cores\openlara\src" >nul
wsl bash -c "sed -i 's/#define OS_PTHREAD_MT/\/\/#define OS_PTHREAD_MT/g' /mnt/d/froguimod/cores/openlara/src/core.h"

REM Copy the openlara core to multicore path
xcopy /E /I /Y "%~dp0cores\openlara" "%MULTICORE_PATH%\cores\openlara" >nul

REM Convert multicore path to WSL path
set "WSL_PATH=%MULTICORE_PATH:\=/%"
set "WSL_PATH=%WSL_PATH:C:=/mnt/c%"
set "WSL_PATH=%WSL_PATH:D:=/mnt/d%"

echo Building SF2000 from: %WSL_PATH%
echo.

wsl bash -c "cd '%WSL_PATH%' && make clean CORE=cores/openlara FROGGY_TYPE=SF2000 && make CORE=cores/openlara FROGGY_TYPE=SF2000 CONSOLE=openlara"

if %ERRORLEVEL% EQU 0 (
    copy "%MULTICORE_PATH%\core_87000000" "%~dp0openlara_sf2000.sf2k" >nul
    echo  SF2000 BUILD SUCCESSFUL!
    echo  Core Output: %~dp0openlara_sf2000.sf2k
) else (
    echo OpenLara SF2000 build failed!
    exit /b 1
)

echo.
echo Building GB300 from: %WSL_PATH%
echo.

wsl bash -c "cd '%WSL_PATH%' && make clean CORE=cores/openlara FROGGY_TYPE=GB300V2 && make CORE=cores/openlara FROGGY_TYPE=GB300V2 CONSOLE=openlara"

if %ERRORLEVEL% EQU 0 (
    copy "%MULTICORE_PATH%\core_87000000" "%~dp0openlara_gb300.sf2k" >nul
    echo  GB300 BUILD SUCCESSFUL!
    echo  Core Output: %~dp0openlara_gb300.sf2k
) else (
    echo OpenLara GB300 build failed!
    exit /b 1
)

echo ========================================
echo  All builds completed successfully!
echo ========================================
