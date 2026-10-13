@echo off
setlocal

set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Visual Studio Installer's vswhere.exe was not found.
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL=%%i"
if not defined VS_INSTALL (
    echo Visual Studio with the C++ build tools was not found.
    exit /b 1
)

call "%VS_INSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%

set "PATH=%VS_INSTALL%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
pushd "%~dp0"
cmake -S . -B out\ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b %errorlevel%

cmake --build out\ninja
set "BUILD_RESULT=%ERRORLEVEL%"
popd
exit /b %BUILD_RESULT%
