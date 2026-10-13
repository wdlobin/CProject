#include <MRMesh/MRMakePlane.h>
#include <MRMesh/MRMesh.h>
#include <MRMesh/MRMeshCollide.h>
#include <MRMesh/MRMeshLoad.h>
#include <MRMesh/MRMeshSave.h>
#include <MRMesh/MROffsetVerts.h>
#include <MRMesh/MRRegionBoundary.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>

namespace
{

std::optional<float> parseNonnegativeFloat( const char* text, const char* name )
{
    try
    {
        std::size_t parsed = 0;
        const float value = std::stof( text, &parsed );
        if ( parsed != std::string( text ).size() || value < 0.0f )
            throw std::invalid_argument( "not a nonnegative number" );
        return value;
    }
    catch ( const std::exception& )
    {
        std::cerr << "Invalid " << name << ": " << text << '\n';
        return std::nullopt;
    }
}

bool saveMesh( const MR::Mesh& mesh, const std::filesystem::path& path )
{
    auto result = MR::MeshSave::toAnySupportedFormat( mesh, path );
    if ( result )
        return true;

    std::cerr << "Failed to save " << path << ": " << result.error() << '\n';
    return false;
}

std::string trustLabel( float trust )
{
    std::string result = std::to_string( trust );
    while ( !result.empty() && result.back() == '0' )
        result.pop_back();
    if ( !result.empty() && result.back() == '.' )
        result.pop_back();
    for ( char& c : result )
        if ( c == '.' )
            c = '_';
    return result;
}

} // namespace

int main( int argc, char** argv )
{
    if ( argc > 5 )
    {
        std::cerr
            << "Usage: MakeThickMeshExample [input-mesh] [output-directory]"
               " [outside-offset] [inside-offset]\n";
        return 2;
    }

    const std::filesystem::path inputPath = argc >= 2 ? argv[1] : "";
    const std::filesystem::path outputDirectory = argc >= 3 ? argv[2] : "output";

    float outsideOffset = 0.10f;
    float insideOffset = 0.05f;
    if ( argc >= 4 )
    {
        const auto parsed = parseNonnegativeFloat( argv[3], "outside offset" );
        if ( !parsed )
            return 2;
        outsideOffset = *parsed;
    }
    if ( argc >= 5 )
    {
        const auto parsed = parseNonnegativeFloat( argv[4], "inside offset" );
        if ( !parsed )
            return 2;
        insideOffset = *parsed;
    }

    MR::Mesh input;
    std::string inputName;
    if ( inputPath.empty() )
    {
        input = MR::makePlane();
        inputName = "plane";
        std::cout << "No input mesh supplied; using MeshLib's 1 x 1 open plane.\n";
    }
    else
    {
        auto loaded = MR::MeshLoad::fromAnySupportedFormat( inputPath );
        if ( !loaded )
        {
            std::cerr << "Failed to load " << inputPath << ": " << loaded.error() << '\n';
            return 1;
        }
        input = std::move( *loaded );
        inputName = inputPath.stem().string();
    }

    std::error_code directoryError;
    std::filesystem::create_directories( outputDirectory, directoryError );
    if ( directoryError )
    {
        std::cerr << "Failed to create output directory: " << directoryError.message() << '\n';
        return 1;
    }

    const auto originalPath = outputDirectory / ( inputName + "_original.stl" );
    if ( !saveMesh( input, originalPath ) )
        return 1;

    std::cout << "Input vertices: " << input.topology.numValidVerts() << '\n'
              << "Input faces: " << input.topology.numValidFaces() << '\n'
              << "Input boundary loops: "
              << input.topology.findHoleRepresentiveEdges().size() << '\n'
              << "Outside offset: " << outsideOffset << '\n'
              << "Inside offset: " << insideOffset << "\n\n";

    constexpr std::array<float, 3> trustFactors{ 0.15f, 1.0f, 5.0f };
    bool allSucceeded = true;

    for ( const float trust : trustFactors )
    {
        MR::ThickenParams params;
        params.outsideOffset = outsideOffset;
        params.insideOffset = insideOffset;
        params.normalsTrustFactor = trust;

        const auto start = std::chrono::steady_clock::now();
        MR::Mesh thick = MR::makeThickMesh( input, params );
        const auto elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start );

        const auto collisions = MR::findSelfCollidingTrianglesBS( MR::MeshPart{ thick } );
        if ( !collisions )
        {
            std::cerr << "Self-intersection detection failed for trust=" << trust
                      << ": " << collisions.error() << '\n';
            allSucceeded = false;
            continue;
        }

        const std::string suffix = "_thick_trust_" + trustLabel( trust );
        const auto outputPath = outputDirectory / ( inputName + suffix + ".stl" );
        if ( !saveMesh( thick, outputPath ) )
        {
            allSucceeded = false;
            continue;
        }

        std::cout << std::fixed << std::setprecision( 3 )
                  << "trust=" << trust
                  << ": " << elapsed.count() << " ms"
                  << ", vertices=" << thick.topology.numValidVerts()
                  << ", faces=" << thick.topology.numValidFaces()
                  << ", boundary loops=" << thick.topology.findHoleRepresentiveEdges().size()
                  << ", self-intersecting faces=" << collisions->count() << '\n'
                  << "  saved: " << outputPath << '\n';
    }

    std::cout << "\nOriginal reference: " << originalPath << '\n';
    return allSucceeded ? 0 : 1;
}
