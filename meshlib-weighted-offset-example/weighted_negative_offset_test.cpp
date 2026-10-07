// Adapted from MeshLib 3.1.4.297 source/MRTest/MRPointCloudVariadicOffsetTests.cpp:
// negWeightedMeshShell1 and negWeightedMeshShell2.
// The third case extends the official sphere case to zero/negative weights only.
#include <MRMesh/MRMesh.h>
#include <MRMesh/MRMeshComponents.h>
#include <MRMesh/MRMeshSave.h>
#include <MRMesh/MRTorus.h>
#include <MRMesh/MRMakeSphereMesh.h>
#include <MRVoxels/MROffset.h>
#include <MRVoxels/MRWeightedPointsShell.h>
#include <algorithm>
#include <filesystem>
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

int main(int argc, char** argv)
{
    try
    {
        const std::filesystem::path output = argc > 1 ? argv[1] : "output/weighted_offset_results";
        std::filesystem::create_directories(output);
        runCase(MR::makeTorus(), "official_torus", 0.2f, false, output);
        runCase(MR::makeUVSphere(1, 16, 16), "official_sphere", 0.1f, false, output);
        runCase(MR::makeUVSphere(1, 16, 16), "partial_inward_sphere", 0.0f, true, output);
        std::cout << "Results: " << std::filesystem::absolute(output) << '\n';
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
