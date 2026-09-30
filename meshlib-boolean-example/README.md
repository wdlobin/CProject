# MeshLib Boolean Example

This project reproduces MeshLib's official C++ mesh-boolean example. It creates
two overlapping spheres, computes their intersection, prints topology-mapping
information, and writes `out_boolean.stl`.

## Build

Run from a Visual Studio developer PowerShell:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

## Run

```powershell
cd build/Release
./MeshBooleanExample.exe
```

The generated `out_boolean.stl` is written to the current working directory.

To select a different operation, change `MR::BooleanOperation::Intersection`
in `main.cpp` to `Union`, `DifferenceAB`, or `DifferenceBA`.
