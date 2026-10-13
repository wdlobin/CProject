#include <MRMesh/MRBox.h>
#include <MRMesh/MRMesh.h>
#include <MRMesh/MRMeshComponents.h>
#include <MRMesh/MRMeshLoad.h>
#include <MRMesh/MRMeshSave.h>
#include <MRMesh/MRRegionBoundary.h>
#include <MRVoxels/MROffset.h>

#include <filesystem>
#include <iostream>

int main()
{
    // Camel head from libigl's tutorial data. It has one open boundary at the neck.
    const std::filesystem::path inputPath =
        LR"(E:\resource\网格模型\libigl_tutorial_data-src\camelhead.off)";

    constexpr float inwardOffsetRatio = 0.05f;
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

    const std::filesystem::path outputDirectory = "output";
    const auto baseName = inputPath.stem().string();
    const auto originalPath = outputDirectory / "offset"  / (baseName + "_original.stl");
    const auto rawOffsetPath = outputDirectory / "offset" / (baseName + "_inward_1pct_raw.stl");
    const auto offsetPath = outputDirectory / "offset" / (baseName + "_inward_1pct_clean.stl");

    std::error_code directoryError;
    std::filesystem::create_directories(originalPath.parent_path(), directoryError);
    if (directoryError)
    {
        std::cerr << "Output directory creation failed: " << directoryError.message() << '\n';
        return 1;
    }

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
    const float offset = -diagonal * inwardOffsetRatio;

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

    auto rawSaveResult = MR::MeshSave::toAnySupportedFormat(*offsetResult, rawOffsetPath);
    if (!rawSaveResult.has_value())
    {
        std::cerr << "Raw offset mesh save failed: " << rawSaveResult.error() << '\n';
        return 1;
    }

    std::cout << "Output vertices: " << offsetResult->topology.numValidVerts() << '\n';
    std::cout << "Output faces: " << offsetResult->topology.numValidFaces() << '\n';
    std::cout << "Output open boundary: "
              << (!MR::findRightBoundary(offsetResult->topology).empty() ? "yes" : "no") << '\n';
    const auto outputBox = offsetResult->computeBoundingBox();
    const auto outputSize = outputBox.size();
    std::cout << "Output size: " << outputSize.x << " x " << outputSize.y << " x " << outputSize.z << '\n';
    const auto components = MR::MeshComponents::getAllComponents(*offsetResult);
    std::cout << "Output connected components: " << components.size() << '\n';
    for (std::size_t i = 0; i < components.size(); ++i)
        std::cout << "  Component " << (i + 1) << " faces: " << components[i].count() << '\n';
    std::cout << "Output signed volume: " << offsetResult->volume() << '\n';

    auto largestComponent = MR::MeshComponents::getLargestComponent(*offsetResult);
    auto facesToDelete = largestComponent;
    offsetResult->topology.flip(facesToDelete);
    offsetResult->deleteFaces(facesToDelete);
    offsetResult->pack();

    auto offsetSaveResult = MR::MeshSave::toAnySupportedFormat(*offsetResult, offsetPath);
    if (!offsetSaveResult.has_value())
    {
        std::cerr << "Clean offset mesh save failed: " << offsetSaveResult.error() << '\n';
        return 1;
    }

    std::cout << "Clean vertices: " << offsetResult->topology.numValidVerts() << '\n';
    std::cout << "Clean faces: " << offsetResult->topology.numValidFaces() << '\n';
    std::cout << "Clean connected components: "
              << MR::MeshComponents::getNumComponents(*offsetResult) << '\n';
    std::cout << "Original reference: " << originalPath << '\n';
    std::cout << "Raw offset result: " << rawOffsetPath << '\n';
    std::cout << "Clean offset result: " << offsetPath << '\n';
    return 0;
}
