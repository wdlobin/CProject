@echo off
setlocal

set "MESHLIB_ROOT=E:\CLibrary\MeshLib-3.1.4.297-msvc2022-x64"
set "PROJECT_ROOT=%~dp0"
set "OUTPUT_DIR=%PROJECT_ROOT%build"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

cl /nologo /EHsc /MD /std:c++20 /bigobj /utf-8 /Zi /Od /D_ITERATOR_DEBUG_LEVEL=0 ^
  /I"%MESHLIB_ROOT%\install\include" ^
  "%PROJECT_ROOT%main.cpp" ^
  /Fo:"%OUTPUT_DIR%\main.obj" ^
  /Fe:"%OUTPUT_DIR%\MeshOffsetExample.exe" ^
  /Fd:"%OUTPUT_DIR%\MeshOffsetExample.pdb" ^
  /link /DEBUG /LIBPATH:"%MESHLIB_ROOT%\install\lib\Release" MRVoxels.lib MRMesh.lib

exit /b %errorlevel%
