# MeshLib `makeThickMesh` 算例

这个独立 C++20 示例使用 MeshLib 3.1.4.297 的非体素厚壳算法
`MR::makeThickMesh`。它不会调用 `generalOffsetMesh`，也不需要链接 `MRVoxels`。

程序默认使用 MeshLib 自带的 1 x 1 开放平面，分别以
`normalsTrustFactor = 0.15、1、5` 生成三个厚壳，用来比较平滑方向与原始伪法向方向。
每个结果都会报告耗时、顶点数、面数、开放边界数量以及自交面数量，并保存 STL。

## 构建

在此目录直接运行：

```powershell
.\build.bat
```

它使用本机 VS2022 Build Tools、CMake 和附带的 Ninja，输出位于 `out/ninja`。

默认 MeshLib 路径：

```text
E:/CLibrary/MeshLibDistVS22_v3.1.4.297
```

如需覆盖：

```powershell
cmake -S . -B out/ninja -G Ninja `
  -DMESHLIB_ROOT="D:/path/to/MeshLibDist"
```

## 运行默认平面算例

```powershell
.\out\ninja\MakeThickMeshExample.exe
```

结果保存在当前目录的 `output` 文件夹：

```text
plane_original.stl
plane_thick_trust_0_15.stl
plane_thick_trust_1.stl
plane_thick_trust_5.stl
```

## 测试自己的模型

```powershell
.\out\ninja\MakeThickMeshExample.exe `
  "D:\models\input.stl" `
  ".\output-my-model" `
  2.0 `
  1.0
```

四个参数依次是：输入网格、输出目录、外侧偏移、内侧偏移。偏移单位与输入模型一致。
输入路径留空字符串时使用默认平面：

```powershell
.\out\ninja\MakeThickMeshExample.exe "" ".\output" 0.10 0.05
```

`normalsTrustFactor` 越小，移动方向通常越平滑；越大则越接近原始顶点伪法向，
但发生翻折或自交的风险也更高。`makeThickMesh` 不会自动修复自交，
所以复杂凹面、大偏移和窄间隙模型应重点查看程序报告的 `self-intersecting faces`。
