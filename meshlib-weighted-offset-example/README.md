# MeshLib 加权负偏移测试

来源：MeshLib 3.1.4.297 的 `source/MRTest/MRPointCloudVariadicOffsetTests.cpp`，
其中 `negWeightedMeshShell1`（圆环）和 `negWeightedMeshShell2`（球体）。
改为独立可执行程序，不依赖 GoogleTest；检查失败返回非零退出码。

## 六个算例

| 名称 | 基础偏移 | 顶点权重 | 体素设置 |
| --- | --- | --- | --- |
| official_torus | 0.2 | x/5 | 官方的约 1000 个体素 |
| official_sphere | 0.1 | x/5 | 官方的约 1000 个体素 |
| partial_inward_sphere | 0 | -0.15 × clamp(x/0.5, 0, 1) | 约 1000000 个体素 |
| five_faces_inward_cube | 其余五面内移 0.15 | 解析平面偏移，不使用顶点权重 | 不使用体素 |
| weighted_cube_abrupt | 0 | 固定面为 0，其余顶点为 -0.15 | voxelSize=0.01 |
| weighted_cube_transition | 0 | -0.15 × clamp((x+0.5)/0.30, 0, 1) | voxelSize=0.01 |

前两个保持官方的几何、权重和偏移参数：既有正权重又有负权重，且基础偏移为正，
因此不能理解成整个模型都内缩。第三个扩展算例的左半球 x<=0 权重为零，
右侧 0<x<0.5 逐渐内缩，x>=0.5 权重为 -0.15。
前三个算例使用 `bidirectionalMode=false`，检查输出非空、一个连通分量、零孔洞及闭合。
第三个算例还检查权重范围和正体积减小。零权重区域的逐点偏差和自交未在本测试中验证。

立方体算例使用边长 1、范围 `[-0.5, 0.5]^3` 的立方体，固定 `x=-0.5` 面，
其余五面内偏移距离设为 `0.15`，通过六个偏移半空间的交集解析构造结果。
范围为 `[-0.5, 0.35] × [-0.35, 0.35] × [-0.35, 0.35]`，体积为 `0.4165`。
固定面的平面位置不变，其边界随相邻面内移而收小。
此算例不调用 WeightedShell：共用棱顶点的单一权重无法同时表示相邻面的不同偏移量。
检查结果非空、单连通分量、零孔洞、闭合、体积减小以及体积等于解析值，保存 original、offset 两份 STL。

## 构建及运行

新增两个加权立方体算例直接调用 `MR::WeightedShell::meshShell(mesh, weights, params)`，
使用单位立方体，固定面 `x=-0.5` 的顶点权重为零，基础偏移为零，`bidirectionalMode=false`。
网格先细分至最大边长 0.10，禁用改变平面几何的翻边。
`weighted_cube_abrupt` 在相邻的非固定面顶点直接赋 -0.15；三角形内部仍线性插值，
因此“突变”指节点权重跳变，并非三角形内的阶跃函数。
`weighted_cube_transition` 在相邻四面设置宽度 0.30 的线性过渡带，带外和对面权重恒为 -0.15。
固定面权重为零不保证输出该面的每个点都精确不动；结果受全局加权距离场和体素离散影响。
每个算例保存原始/偏移 STL，以及 `_weights.csv`（节点坐标与权重）。
可叠加查看结果与原模型，重点观察固定面四条棱附近，再与解析算例比较。
为了保留过渡产生的实际效果，不清理额外分量、不修补孔洞：日志报告分量数、孔洞数、闭合状态与体积；
这两个观察算例的成功仅代表算法返回非空结果且文件导出成功，并不代表闭合性验证通过。

在此目录运行 `cmake --preset msvc-release`，然后运行 `cmake --build --preset release`。
MeshLib 默认路径为 `E:/CLibrary/MeshLibDistVS22_v3.1.4.297`。
运行 `ctest --test-dir build/msvc-release -C Release --output-on-failure`，
输出在 `build/msvc-release/results`，每个算例保存 original、offset 两份 STL。

2026-10-12 验证：MSVC Release 编译和链接成功，直接运行全部六个算例退出码为零。
权重跳变版本有 16 个分量、14 个孔洞、未闭合，体积约 0.417897；
宽度 0.30 的线性过渡版本为一个分量、零孔洞、闭合，体积约 0.480058。

历史验证：CTest 启动程序时退出码为
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
.\build\msvc-release\Release\WeightedNegativeOffsetTest.exe .\build\msvc-release\results
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

## 拾取面向内抽壳并做布尔差

`picked_face_shell_test.cpp` 验证了另一条不依赖顶点权重的路线。测试在单位立方体
顶面中央选择 `0.6 x 0.6` 的方形面片，将其向内移动 `0.15`，并显式构造：

1. 向内偏移后的开口面片；
2. 连接原边界和内层边界的四组侧壁；
3. 位于原拾取面一侧的封口面。

三部分组成闭合的抽壳刀具体，再调用
`MR::boolean(source, shellTool, MR::BooleanOperation::DifferenceAB)`。
为了避免刀具封口面与原模型表面完全共面，封口向模型外延伸 `0.001`；模型内部的实际
去除深度仍为 `0.15`。

2026-10-12 本地 MSVC Release 验证通过：刀具体和差集结果均为一个连通分量、零孔洞、
闭合网格。差集体积为 `0.946`，与解析值
`1 - 0.6 * 0.6 * 0.15 = 0.946` 一致。输出文件位于
`build/msvc-release/picked_face_shell_results`：

- `01_source_cube.stl`
- `02_picked_face_shell_tool.stl`
- `03_cube_minus_picked_face_shell.stl`

此测试验证了 MeshLib 的建网格和布尔能力足以完成该方案。推广到任意拾取区域时，仍需
实现选区边界提取、内层面生成、边界环对应和自交处理；这些属于局部偏移算法本身，
不是 `WeightedShell` 能自动完成的步骤。
