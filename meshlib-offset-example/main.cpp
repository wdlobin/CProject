#include <MRMesh/MRBox.h>
#include <MRMesh/MRMesh.h>
#include <MRMesh/MRMeshLoad.h>
#include <MRMesh/MRMeshSave.h>
#include <MRMesh/MRRegionBoundary.h>
#include <MRVoxels/MROffset.h>

#include <filesystem>
#include <iostream>

int main()
{
    // Change only this path when testing another STL, OBJ or OFF mesh.
    const std::filesystem::path inputPath =
        R"(D:\Downloads\glass_fix.stl)";

    constexpr float offsetRatio = 0.05f;
    constexpr float approximateVoxelCount = 10'000'000.0f;

    std::cout << "Input: " << inputPath << '\n';
    auto loadResult = MR::MeshLoad::fromAnySupportedFormat(inputPath);
    if (!loadResult.has_value())
    {
        std::cerr << "Load failed: " << loadResult.error() << '\n';
        return 1;
    }

    MR::Mesh mesh = std::move(*loadResult);
    std::cout << "Input vertices: " << mesh.topology.numValidVerts() << '\n';
    std::cout << "Input faces: " << mesh.topology.numValidFaces() << '\n';

    const auto outputDirectory = inputPath.parent_path();
    const auto baseName = inputPath.stem().string();
    const auto originalPath = outputDirectory / "offset"  / (baseName + "_original.stl");
    const auto offsetPath = outputDirectory / "offset" / (baseName + "_offset.stl");

    auto originalSaveResult = MR::MeshSave::toAnySupportedFormat(mesh, originalPath);
    if (!originalSaveResult.has_value())
    {
        std::cerr << "Original mesh save failed: " << originalSaveResult.error() << '\n';
        return 1;
    }

    MR::GeneralOffsetParameters params;
    params.voxelSize = MR::suggestVoxelSize(mesh, approximateVoxelCount);

    const bool hasOpenBoundary = !MR::findRightBoundary(mesh.topology).empty();
    if (hasOpenBoundary)
        params.signDetectionMode = MR::SignDetectionMode::HoleWindingRule;

    const float diagonal = mesh.computeBoundingBox().diagonal();
    const float offset = -0.2;

    std::cout << "Bounding-box diagonal: " << diagonal << '\n';
    std::cout << "Offset distance: " << offset << '\n';
    std::cout << "Voxel size: " << params.voxelSize << '\n';
    std::cout << "Open boundary detected: " << (hasOpenBoundary ? "yes" : "no") << '\n';

    auto offsetResult = MR::generalOffsetMesh(mesh, offset, params);
    if (!offsetResult.has_value())
    {
        std::cerr << "Offset failed: " << offsetResult.error() << '\n';
        return 1;
    }

    auto offsetSaveResult = MR::MeshSave::toAnySupportedFormat(*offsetResult, offsetPath);
    if (!offsetSaveResult.has_value())
    {
        std::cerr << "Offset mesh save failed: " << offsetSaveResult.error() << '\n';
        return 1;
    }

    std::cout << "Output vertices: " << offsetResult->topology.numValidVerts() << '\n';
    std::cout << "Output faces: " << offsetResult->topology.numValidFaces() << '\n';
    std::cout << "Original reference: " << originalPath << '\n';
    std::cout << "Offset result: " << offsetPath << '\n';
    return 0;
}
