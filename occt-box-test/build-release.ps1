$ErrorActionPreference = 'Stop'

$mingwBin = 'D:\mingw64\bin'
$cmakeBin = 'D:\Program Files\CMake\bin'
$occtRoot = 'E:\CLibrary\OCCT-8.0.1-mingw64'
$projectDir = $PSScriptRoot
$buildDir = Join-Path $projectDir 'build'

$env:Path = "$mingwBin;$cmakeBin;$env:Path"

& "$cmakeBin\cmake.exe" -S $projectDir -B $buildDir -G 'MinGW Makefiles' `
  -DCMAKE_BUILD_TYPE=Release `
  "-DCMAKE_PREFIX_PATH=$occtRoot" `
  "-DCMAKE_CXX_COMPILER=$mingwBin\g++.exe" `
  "-DCMAKE_MAKE_PROGRAM=$mingwBin\mingw32-make.exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& "$cmakeBin\cmake.exe" --build $buildDir --parallel 8
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$buildDir\occt_box.exe"
exit $LASTEXITCODE
