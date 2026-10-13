#include <MRMesh/MRCube.h>
#include <MRMesh/MRMesh.h>
#include <MRMesh/MRMeshBoolean.h>
#include <MRMesh/MRMeshBuilder.h>
#include <MRMesh/MRMeshComponents.h>
#include <MRMesh/MRMeshSave.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{

void require( bool condition, const std::string& message )
{
    if ( !condition )
        throw std::runtime_error( message );
}

// Builds a closed subtraction tool from four explicit parts:
//   1. the inward-offset picked patch (bottom),
//   2. four boundary side walls,
//   3. the original picked patch (top cap).
//
// The top cap is extended by overlap past the source surface.  This is the
// usual robust-Boolean practice: it avoids a coincident cap on z=+0.5 while
// leaving the actual removed depth equal to `depth` inside the source solid.
MR::Mesh makePickedFaceShell( float halfWidth, float depth, float overlap )
{
    const float innerZ = 0.5f - depth;
    const float outerZ = 0.5f + overlap;

    MR::VertCoords points{
        { -halfWidth, -halfWidth, innerZ }, // inward-offset open patch
        {  halfWidth, -halfWidth, innerZ },
        {  halfWidth,  halfWidth, innerZ },
        { -halfWidth,  halfWidth, innerZ },
        { -halfWidth, -halfWidth, outerZ }, // original patch cap (+ overlap)
        {  halfWidth, -halfWidth, outerZ },
        {  halfWidth,  halfWidth, outerZ },
        { -halfWidth,  halfWidth, outerZ }
    };

    MR::Triangulation triangles{
        // Inward-offset patch, outward normal -Z.
        { MR::VertId( 0 ), MR::VertId( 2 ), MR::VertId( 1 ) },
        { MR::VertId( 0 ), MR::VertId( 3 ), MR::VertId( 2 ) },
        // Original picked-face cap, outward normal +Z.
        { MR::VertId( 4 ), MR::VertId( 5 ), MR::VertId( 6 ) },
        { MR::VertId( 4 ), MR::VertId( 6 ), MR::VertId( 7 ) },
        // Boundary side walls.
        { MR::VertId( 0 ), MR::VertId( 1 ), MR::VertId( 5 ) },
        { MR::VertId( 0 ), MR::VertId( 5 ), MR::VertId( 4 ) },
        { MR::VertId( 1 ), MR::VertId( 2 ), MR::VertId( 6 ) },
        { MR::VertId( 1 ), MR::VertId( 6 ), MR::VertId( 5 ) },
        { MR::VertId( 2 ), MR::VertId( 3 ), MR::VertId( 7 ) },
        { MR::VertId( 2 ), MR::VertId( 7 ), MR::VertId( 6 ) },
        { MR::VertId( 3 ), MR::VertId( 0 ), MR::VertId( 4 ) },
        { MR::VertId( 3 ), MR::VertId( 4 ), MR::VertId( 7 ) }
    };

    return MR::Mesh::fromTriangles( std::move( points ), triangles );
}

void save( const MR::Mesh& mesh, const std::filesystem::path& path )
{
    const auto result = MR::MeshSave::toAnySupportedFormat( mesh, path );
    require( result.has_value(), "cannot save " + path.string() + ": " +
        ( result ? std::string{} : result.error() ) );
}

} // namespace

int main( int argc, char** argv )
{
    try
    {
        const std::filesystem::path output = argc > 1 ? argv[1] :
            "output/picked_face_shell_results";
        std::filesystem::create_directories( output );

        constexpr float halfWidth = 0.30f; // selected 0.6 x 0.6 patch
        constexpr float depth = 0.15f;
        constexpr float overlap = 0.001f;

        const MR::Mesh source = MR::makeCube(); // [-0.5, 0.5]^3
        const MR::Mesh shellTool = makePickedFaceShell( halfWidth, depth, overlap );

        require( shellTool.topology.isClosed(), "shell tool is not closed" );
        require( shellTool.topology.findNumHoles() == 0, "shell tool has holes" );
        require( MR::MeshComponents::getNumComponents( shellTool ) == 1,
            "shell tool has multiple components" );

        MR::BooleanResult booleanResult = MR::boolean(
            source, shellTool, MR::BooleanOperation::DifferenceAB );
        require( booleanResult.valid(), "DifferenceAB failed: " + booleanResult.errorString );

        const MR::Mesh& result = booleanResult.mesh;
        require( result.topology.isClosed(), "difference result is not closed" );
        require( result.topology.findNumHoles() == 0, "difference result has holes" );
        require( MR::MeshComponents::getNumComponents( result ) == 1,
            "difference result has multiple components" );

        const float expectedVolume = 1.0f - 4.0f * halfWidth * halfWidth * depth;
        require( std::abs( result.volume() - expectedVolume ) < 1e-4f,
            "unexpected result volume" );

        save( source, output / "01_source_cube.stl" );
        save( shellTool, output / "02_picked_face_shell_tool.stl" );
        save( result, output / "03_cube_minus_picked_face_shell.stl" );

        std::cout << "picked-face shell test: PASS\n"
                  << "  tool: closed=" << shellTool.topology.isClosed()
                  << ", holes=" << shellTool.topology.findNumHoles()
                  << ", components=" << MR::MeshComponents::getNumComponents( shellTool )
                  << ", volume=" << shellTool.volume() << '\n'
                  << "  result: closed=" << result.topology.isClosed()
                  << ", holes=" << result.topology.findNumHoles()
                  << ", components=" << MR::MeshComponents::getNumComponents( result )
                  << ", volume=" << result.volume()
                  << ", expected=" << expectedVolume << '\n'
                  << "  files: " << std::filesystem::absolute( output ) << '\n';
        return 0;
    }
    catch ( const std::exception& e )
    {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
