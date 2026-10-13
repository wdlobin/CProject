#include <MRMesh/MRMesh.h>
#include <MRMesh/MRMeshBoolean.h>
#include <MRMesh/MRMeshSave.h>
#include <MRMesh/MRUVSphere.h>
#include <MRMesh/MRMeshLoad.h>
#include <MRMesh/MRMatrix3.h>
#include <MRMesh/MRAffineXf3.h>
#include <chrono>

#include <iostream>

int main()
{
    // --- 自带测试几何 ----
    // Create the first sphere with a radius of 1 unit.
    // MR::Mesh sphere1 = MR::makeUVSphere( 1.0f, 64, 64 );

    // // Clone the first sphere and move the copy along the X axis.
    // MR::Mesh sphere2 = sphere1;

    // const MR::AffineXf3f xf =
    //     MR::AffineXf3f::translation( MR::Vector3f( 0.7f, 0.0f, 0.0f ) );
    // sphere2.transform( xf );
    // ---------------------------------------------------------------------

    // --- 本地几何 ---
    // Change only this path when testing another STL, OBJ or OFF mesh.
    const std::filesystem::path inputPath =
        R"(D:\Downloads\libigl_tutorial_data-src\libigl_tutorial_data-src\bunny.off)";

    constexpr float offsetRatio = 0.05f;
    constexpr float approximateVoxelCount = 10'000'000.0f;

    std::cout << "Input: " << inputPath << '\n';
    auto loadResult = MR::MeshLoad::fromAnySupportedFormat(inputPath);
    if (!loadResult.has_value())
    {
        std::cerr << "Load failed: " << loadResult.error() << '\n';
        return 1;
    }
    MR::Mesh mesh1 = std::move(*loadResult);
    std::cout << "Input vertices: " << mesh1.topology.numValidVerts() << '\n';
    std::cout << "Input faces: " << mesh1.topology.numValidFaces() << '\n';
    MR::Mesh mesh2 = mesh1;

    const MR::AffineXf3f xf1 = MR::AffineXf3f::translation( MR::Vector3f( 0.0f, 0.25f, 0.0f ) );
    // mesh2.transform( xf );

    // 角度转弧度
    constexpr float pi = 3.14159265358979323846f;
    const float angle = 180.0f * pi / 180.0f;

    const MR::AffineXf3f xf2 = MR::AffineXf3f::linear(
        MR::Matrix3f::rotation(
            MR::Vector3f( 1.0f, 0.0f, 0.0f ), angle
        )
    );

    mesh2.transform( xf2 );
    mesh2.transform(xf1);

    const std::filesystem::path outputDirectory = "output";
    const auto baseName = inputPath.stem().string();

    const auto originalPath = outputDirectory / (baseName + "_original.stl");
    const auto originalPath2 = outputDirectory / (baseName + "_original2.stl");
    const auto booleanPath = outputDirectory / (baseName + "_boolean.stl");

    // 确保输出目录存在。
    std::filesystem::create_directories(outputDirectory);

    const auto saveResult1 = MR::MeshSave::toAnySupportedFormat(mesh1, originalPath);

    if (!saveResult1)
    {
        std::cerr << "Failed to save mesh1: "
                << saveResult1.error() << '\n';
        return 1;
    }

    const auto saveResult2 = MR::MeshSave::toAnySupportedFormat(mesh2, originalPath2);

    if (!saveResult2)
    {
        std::cerr << "Failed to save mesh2: "
                << saveResult2.error() << '\n';
        return 1;
    }
    // ---------------------------------------------------------
    

    // Track how input topology maps to the boolean result.
    MR::BooleanResultMapper mapper;

    const auto start = std::chrono::steady_clock::now();

    // Compute the intersection of the two spheres.
    MR::BooleanResult result = MR::boolean(
        mesh1,
        mesh2,
        MR::BooleanOperation::Intersection, // 交集
        // MR::BooleanOperation::DifferenceAB,    // 差集
        { .mapper = &mapper }
    );

    const auto end = std::chrono::steady_clock::now();

    const double elapsedMs = std::chrono::duration<double, std::milli>( end - start ).count();

    std::cout << "耗时：" << elapsedMs << " ms\n";

    if ( !result.valid() )
    {
        std::cerr << "Boolean operation failed: " << result.errorString << '\n';
        return 1;
    }

    MR::Mesh resultMesh = std::move( result.mesh );

    // Inspect where the result faces came from.
    using MapObject = MR::BooleanResultMapper::MapObject;
    const MR::FaceBitSet facesOfSphere1 =
        mapper.map( mesh1.topology.getValidFaces(), MapObject::A );
    const MR::FaceBitSet facesOfSphere2 =
        mapper.map( mesh2.topology.getValidFaces(), MapObject::B );
    const MR::FaceBitSet newFaces = mapper.newFaces();

    std::cout << "Faces from mesh 1: " << facesOfSphere1.count() << '\n'
              << "Faces from mesh 2: " << facesOfSphere2.count() << '\n'
              << "Faces created by the cut: " << newFaces.count() << '\n';

    // Map one input face forward to the output mesh.
    const MR::FaceId inputFace( 793 );
    MR::FaceBitSet oneFace;
    oneFace.autoResizeSet( inputFace );
    const MR::FaceBitSet producedFaces = mapper.map( oneFace, MapObject::A );

    std::cout << "Face " << inputFace << " of sphere 1 produced "
              << producedFaces.count() << " result face(s):";
    for ( const MR::FaceId face : producedFaces )
        std::cout << ' ' << face;
    std::cout << '\n';

    // Map a result face back to its original face in sphere 1.
    if ( producedFaces.any() )
    {
        const MR::FaceMap newToOldFaces = mapper.getNew2OldFaceMap( MapObject::A );
        const MR::FaceId resultFace = producedFaces.find_first();
        std::cout << "Result face " << resultFace << " came from sphere 1 face "
                  << newToOldFaces[resultFace] << '\n';
    }

    const char* outputPath = "output/out_boolean.stl";
    if ( const auto saveResult =
             MR::MeshSave::toAnySupportedFormat( resultMesh, outputPath );
         !saveResult )
    {
        std::cerr << "Failed to save result: " << saveResult.error() << '\n';
        return 1;
    }

    std::cout << "Boolean result saved to: " << outputPath << '\n';
    return 0;
}
