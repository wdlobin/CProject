# MeshLib 加权负偏移测试

来源：MeshLib 3.1.4.297 的 `source/MRTest/MRPointCloudVariadicOffsetTests.cpp`，
其中 `negWeightedMeshShell1`（圆环）和 `negWeightedMeshShell2`（球体）。
改为独立可执行程序，不依赖 GoogleTest；检查失败返回非零退出码。

## 三个算例

| 名称 | 基础偏移 | 顶点权重 | 体素设置 |
| --- | --- | --- | --- |
| official_torus | 0.2 | x/5 | 官方的约 1000 个体素 |
| official_sphere | 0.1 | x/5 | 官方的约 1000 个体素 |
| partial_inward_sphere | 0 | -0.15 × clamp(x/0.5, 0, 1) | 约 1000000 个体素 |

前两个保持官方的几何、权重和偏移参数：既有正权重又有负权重，且基础偏移为正，
因此不能理解成整个模型都内缩。第三个扩展算例的左半球 x<=0 权重为零，
右侧 0<x<0.5 逐渐内缩，x>=0.5 权重为 -0.15。
所有算例使用 `bidirectionalMode=false`，检查输出非空、一个连通分量、零孔洞及闭合。
第三个算例还检查权重范围和正体积减小。零权重区域的逐点偏差和自交未在本测试中验证。

## 构建及运行

在此目录运行 `build.bat`，使用本机的 VS2022 BuildTools、CMake 和 Ninja。
MeshLib 默认路径为 `E:/CLibrary/MeshLibDistVS22_v3.1.4.297`。
输出在 `out/ninja/results`，每个算例保存 original、offset 两份 STL。

本次验证：MSVC Release 编译和链接成功；CTest 启动程序时退出码为
`0xC0E90002`，尚未进入算例，没有生成 STL，几何检查尚未得到运行结果。
在受限环境外重试也出现相同退出码。

## 启动失败的已确认原因

2026-10-02 排查 Windows `Microsoft-Windows-CodeIntegrity/Operational` 日志发现：
事件 3077、3033 明确记录 Smart App Control 拦截本程序加载 `gdcmIOD.dll`，
理由是该 DLL 不满足签名要求/代码完整性策略。14:12 的 Release 和 14:22 的 Debug
运行均有相同记录；Debug 目录中该 DLL 的签名状态为 `NotSigned`。
因此当前失败发生在运行库加载阶段，不能据此判断加权负偏移算法失败。

依赖关系：`WeightedNegativeOffsetTest.exe -> MRVoxels.dll -> GDCM -> gdcmIOD.dll`。
GDCM 用于 DICOM 医学影像读写，本测试不使用此功能。
可采用满足系统签名要求的兼容依赖库；另一个构建方案是使用 MeshLib 自带的
`MRVOXELS_NO_DICOM=ON` 重新构建 MeshLib，从运行依赖中移除未使用的 DICOM 功能。
该选项必须配置在 MeshLib 源码构建工程中，不能仅在本测试工程里定义宏，
也不能直接删除 DLL，否则原有 MRVoxels.dll 仍然需要它。
本次排查没有修改 Windows 安全设置，也没有重新构建公共 MeshLib 库。

也可运行已构建的程序并指定输出目录：

```powershell
.\out\ninja\WeightedNegativeOffsetTest.exe .\out\ninja\results
```

## partialOffsetMesh 的原理

该函数强制使用无符号距离生成选区的双侧壳体，然后与完整原模型做布尔并集。
设原实体为 A、选区为 R、偏移距离为 d>0，壳体对应的实体为
`B = {p: distance(p,R) <= d}`，最终输出为 `A ∪ B` 的边界。
选区自身可以开放，因为体素距离等值面会生成围绕选区的封闭壳体，包括边缘绕回的部分。
壳体向原模型内部延伸的部分被并集吸收，向外延伸的部分成为新表面；
布尔算法处理交线、切分三角形并移除内部表面。
这不是单纯移动选区三角形，因此交界附近未选表面也可能改变。
并集只增加材料，无法直接实现局部内缩。可靠的布尔运算要求输入满足相应的闭合、无自交条件。
