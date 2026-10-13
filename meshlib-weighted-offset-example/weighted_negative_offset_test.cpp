// Adapted from MeshLib 3.1.4.297 source/MRTest/MRPointCloudVariadicOffsetTests.cpp:
// negWeightedMeshShell1 and negWeightedMeshShell2.
// The third case extends the official sphere case to zero/negative weights only.
#include <MRMesh/MRMesh.h>
#include <MRMesh/MRCube.h>
#include <MRMesh/MRMeshSubdivide.h>
#include <MRMesh/MRMeshComponents.h>
#include <MRMesh/MRMeshSave.h>
#include <MRMesh/MRTorus.h>
#include <MRMesh/MRMakeSphereMesh.h>
#include <MRVoxels/MROffset.h>
#include <MRVoxels/MRWeightedPointsShell.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

void require(bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

void runCase(const MR::Mesh& mesh, const std::string& name, float baseOffset,
             bool partialShrink, const std::filesystem::path& output)
{
    MR::VertScalars weights(mesh.topology.vertSize(), 0.0f);
    for (auto v : mesh.topology.getValidVerts())
    {
        const float x = mesh.points[v].x;
        // Official cases: x/5, with a positive common offset.
        // Extension: x<=0 stays at zero; 0<x<0.5 is a transition;
        // x>=0.5 gets the full -0.15 inward offset.
        weights[v] = partialShrink ? -0.15f * std::clamp(x / 0.5f, 0.0f, 1.0f)
                                  : x / 5.0f;
    }
    auto minmax = std::minmax_element(begin(weights), end(weights));
    MR::WeightedShell::ParametersMetric params;
    params.voxelSize = MR::suggestVoxelSize(mesh, partialShrink ? 1'000'000.0f : 1000.0f);
    params.offset = baseOffset;
    params.dist.minWeight = *minmax.first;
    params.dist.maxWeight = *minmax.second;
    params.dist.bidirectionalMode = false;

    auto res = MR::WeightedShell::meshShell(mesh, weights, params);
    require(res.has_value(), name + ": offset failed" + (res ? "" : ": " + res.error()));
    const int components = MR::MeshComponents::getNumComponents(*res);
    const auto holes = res->topology.findNumHoles();
    require(components == 1, name + ": expected one component");
    require(holes == 0 && res->topology.isClosed(), name + ": expected closed mesh");
    require(res->topology.numValidFaces() > 0, name + ": empty result");
    if (partialShrink)
    {
        require(*minmax.first < 0 && *minmax.second == 0, name + ": invalid weights");
        require(res->volume() > 0 && res->volume() < mesh.volume(), name + ": volume did not shrink");
    }
    auto saved = MR::MeshSave::toAnySupportedFormat(mesh, output / (name + "_original.stl"));
    require(saved.has_value(), name + ": cannot save original");
    saved = MR::MeshSave::toAnySupportedFormat(*res, output / (name + "_offset.stl"));
    require(saved.has_value(), name + ": cannot save result");
    std::cout << name << ": PASS; base=" << params.offset
              << ", weights=[" << params.dist.minWeight << ", " << params.dist.maxWeight
              << "], voxel=" << params.voxelSize << ", components=" << components
              << ", holes=" << holes << ", volume=" << mesh.volume() << " -> " << res->volume() << '\n';
}

void runWeightedCubeCase(const std::filesystem::path& output, float transitionWidth)
{
    const std::string name = transitionWidth == 0.0f
        ? "weighted_cube_abrupt" : "weighted_cube_transition";
    constexpr float inwardDistance = 0.15f;
    auto mesh = MR::makeCube(); // [-0.5, 0.5]^3; fixed face: xmin
    MR::SubdivideSettings settings;
    settings.maxEdgeLen = 0.10f;
    settings.maxEdgeSplits = 100000;
    settings.maxDeviationAfterFlip = 0.0f;
    settings.maxAngleChangeAfterFlip = 0.0f;
    MR::subdivideMesh(mesh, settings);

    MR::VertScalars weights(mesh.topology.vertSize(), 0.0f);
    std::ofstream nodes(output / (name + "_weights.csv"));
    require(nodes.is_open(), name + ": cannot open weights file");
    nodes << "vertex_id,x,y,z,weight\n";
    for (auto v : mesh.topology.getValidVerts())
    {
        const auto& p = mesh.points[v];
        const float distanceFromFixedFace = p.x + 0.5f;
        // Shared vertices on xmin have zero weight, including its edges.
        // Compare an abrupt change at the next row of vertices with a
        // linear transition on the four adjacent faces over transitionWidth.
        const float factor = transitionWidth > 0.0f
            ? std::clamp(distanceFromFixedFace / transitionWidth, 0.0f, 1.0f)
            : (distanceFromFixedFace <= 1e-6f ? 0.0f : 1.0f);
        weights[v] = -inwardDistance * factor;
        nodes << int(v) << ',' << p.x << ',' << p.y << ',' << p.z
              << ',' << weights[v] << '\n';
    }
    nodes.close();
    require(bool(nodes), name + ": cannot write weights file");

    MR::WeightedShell::ParametersMetric params;
    params.offset = 0.0f;
    params.voxelSize = 0.01f;
    params.dist.minWeight = -inwardDistance;
    params.dist.maxWeight = 0.0f;
    params.dist.bidirectionalMode = false;
    std::cout << name << ": computing meshShell..." << std::endl;
    auto res = MR::WeightedShell::meshShell(mesh, weights, params);
    require(res.has_value(), name + ": offset failed" + (res ? "" : ": " + res.error()));
    require(res->topology.numValidFaces() > 0, name + ": empty result");
    auto saved = MR::MeshSave::toAnySupportedFormat(mesh, output / (name + "_original.stl"));
    require(saved.has_value(), name + ": cannot save original");
    // Preserve the raw result, including any extra components or holes,
    // because this example is intended to inspect transition artifacts.
    saved = MR::MeshSave::toAnySupportedFormat(*res, output / (name + "_offset.stl"));
    require(saved.has_value(), name + ": cannot save result");
    std::cout << name << ": EXPORTED; inward=" << inwardDistance
              << ", transition width=" << transitionWidth
              << ", voxel=" << params.voxelSize
              << ", components=" << MR::MeshComponents::getNumComponents(*res)
              << ", holes=" << res->topology.findNumHoles()
              << ", closed=" << res->topology.isClosed()
              << ", volume=" << mesh.volume() << " -> " << res->volume() << '\n';
}

void runCubeCase(const std::filesystem::path& output)
{
    const std::string name = "five_faces_inward_cube";
    const float inwardDistance = 0.15f;
    const auto cube = MR::makeCube(); // [-0.5, 0.5]^3
    // Exact intersection of the six offset half-spaces. xmin stays fixed.
    // Vertex weights cannot express different offsets on shared face edges.
    const MR::Vector3f size{1 - inwardDistance, 1 - 2 * inwardDistance,
                           1 - 2 * inwardDistance};
    const auto result = MR::makeCube(size,
        {-0.5f, -0.5f + inwardDistance, -0.5f + inwardDistance});
    require(MR::MeshComponents::getNumComponents(result) == 1,
            name + ": expected one component");
    require(result.topology.isClosed() && result.topology.findNumHoles() == 0,
            name + ": expected closed mesh");
    require(result.topology.numValidFaces() > 0, name + ": empty result");
    require(std::abs(result.volume() - size.x * size.y * size.z) < 1e-6f,
            name + ": unexpected volume");
    require(result.volume() > 0 && result.volume() < cube.volume(),
            name + ": volume did not shrink");
    auto saved = MR::MeshSave::toAnySupportedFormat(
        cube, output / (name + "_original.stl"));
    require(saved.has_value(), name + ": cannot save original");
    saved = MR::MeshSave::toAnySupportedFormat(result, output / (name + "_offset.stl"));
    require(saved.has_value(), name + ": cannot save result");
    std::cout << name << ": PASS; fixed face=xmin, inward distance="
              << inwardDistance << ", volume=" << cube.volume()
              << " -> " << result.volume() << '\n';
}

int main(int argc, char** argv)
{
    try
    {
        const std::filesystem::path output = argc > 1 ? argv[1] : "output/weighted_offset_results";
        std::filesystem::create_directories(output);
        // runCase(MR::makeTorus(), "official_torus", 0.2f, false, output);
        // runCase(MR::makeUVSphere(1, 16, 16), "official_sphere", 0.1f, false, output);
        // runCase(MR::makeUVSphere(1, 16, 16), "partial_inward_sphere", 0.0f, true, output);
        // runCubeCase(output);
        runWeightedCubeCase(output, 0.0f);
        // runWeightedCubeCase(output, 0.30f);
        std::cout << "Results: " << std::filesystem::absolute(output) << '\n';
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
