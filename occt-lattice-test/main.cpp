#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kTolerance = 1.0e-7;

TopoDS_Face makeFace(const std::vector<gp_Pnt>& points)
{
  BRepBuilderAPI_MakePolygon polygon;
  for (const gp_Pnt& point : points)
    polygon.Add(point);
  polygon.Close();

  if (!polygon.IsDone())
    throw std::runtime_error("Failed to create polygon wire");

  BRepBuilderAPI_MakeFace faceMaker(polygon.Wire());
  if (!faceMaker.IsDone())
    throw std::runtime_error("Failed to create polygon face");
  return faceMaker.Face();
}

TopoDS_Face makeIrregularRegion()
{
  // A deliberately concave simple polygon, all coordinates in millimetres.
  return makeFace({
    {0.0, 2.0, 0.0}, {5.0, 0.0, 0.0}, {12.0, 1.2, 0.0},
    {18.0, 0.3, 0.0}, {25.0, 3.5, 0.0}, {23.0, 8.0, 0.0},
    {26.0, 13.0, 0.0}, {20.0, 18.0, 0.0}, {14.0, 16.2, 0.0},
    {9.0, 19.0, 0.0}, {4.5, 15.0, 0.0}, {6.5, 10.0, 0.0},
    {1.0, 8.0, 0.0}
  });
}

TopoDS_Face makePointyHexagon(double cx, double cy, double radius)
{
  std::vector<gp_Pnt> points;
  points.reserve(6);
  for (int i = 0; i < 6; ++i)
  {
    const double angle = (30.0 + 60.0 * i) * kPi / 180.0;
    points.emplace_back(cx + radius * std::cos(angle),
                        cy + radius * std::sin(angle), 0.0);
  }
  return makeFace(points);
}

double areaOf(const TopoDS_Shape& shape)
{
  GProp_GProps properties;
  BRepGProp::SurfaceProperties(shape, properties);
  return properties.Mass();
}

int countFaces(const TopoDS_Shape& shape)
{
  int count = 0;
  for (TopExp_Explorer explorer(shape, TopAbs_FACE); explorer.More(); explorer.Next())
    ++count;
  return count;
}

void writeBrep(const TopoDS_Shape& shape, const std::filesystem::path& path)
{
  if (!BRepTools::Write(shape, path.string().c_str()))
    throw std::runtime_error("Failed to write " + path.string());
}
}

int main(int argc, char** argv)
{
  try
  {
    const std::filesystem::path outputDir =
      argc > 1 ? std::filesystem::path(argv[1]) : std::filesystem::current_path();
    std::filesystem::create_directories(outputDir);

    const TopoDS_Face region = makeIrregularRegion();
    if (!BRepCheck_Analyzer(region).IsValid())
      throw std::runtime_error("The generated irregular region is invalid");

    // Hole geometry: radius is centre-to-vertex. Pitch is independently enlarged
    // to leave material between neighbouring holes.
    const double radius = 1.15;
    const double wall = 0.55;
    const double naturalFlatWidth = std::sqrt(3.0) * radius;
    const double scale = (naturalFlatWidth + wall) / naturalFlatWidth;
    const double pitchX = std::sqrt(3.0) * radius * scale;
    const double pitchY = 1.5 * radius * scale;

    // The chosen region lies inside [0, 26] x [0, 19]. Expand by two radii so
    // cells whose centres are outside the region can still reach its boundary.
    const double xmin = -2.0 * radius;
    const double xmax = 26.0 + 2.0 * radius;
    const double ymin = -2.0 * radius;
    const double ymax = 19.0 + 2.0 * radius;

    BRep_Builder builder;
    TopoDS_Compound allHexagons;
    builder.MakeCompound(allHexagons);

    int generatedCount = 0;
    int row = 0;
    for (double cy = ymin; cy <= ymax; cy += pitchY, ++row)
    {
      const double rowOffset = (row % 2 == 0) ? 0.0 : 0.5 * pitchX;
      for (double cx = xmin + rowOffset; cx <= xmax; cx += pitchX)
      {
        builder.Add(allHexagons, makePointyHexagon(cx, cy, radius));
        ++generatedCount;
      }
    }

    BRepAlgoAPI_Common clipper(allHexagons, region);
    clipper.SetFuzzyValue(kTolerance);
    clipper.Build();
    if (!clipper.IsDone())
      throw std::runtime_error("Common operation failed");
    const TopoDS_Shape clippedCells = clipper.Shape();

    BRepAlgoAPI_Cut cutter(region, allHexagons);
    cutter.SetFuzzyValue(kTolerance);
    cutter.Build();
    if (!cutter.IsDone())
      throw std::runtime_error("Cut operation failed");
    const TopoDS_Shape perforatedRegion = cutter.Shape();

    if (!BRepCheck_Analyzer(clippedCells).IsValid())
      throw std::runtime_error("Clipped honeycomb cells are invalid");
    if (!BRepCheck_Analyzer(perforatedRegion).IsValid())
      throw std::runtime_error("Perforated region is invalid");

    // A second subtraction must leave practically no area. This directly checks
    // that the clipped cells do not extend beyond the irregular boundary.
    BRepAlgoAPI_Cut outsideCheck(clippedCells, region);
    outsideCheck.SetFuzzyValue(kTolerance);
    outsideCheck.Build();
    if (!outsideCheck.IsDone())
      throw std::runtime_error("Outside-boundary verification failed");

    const double regionArea = areaOf(region);
    const double holeArea = areaOf(clippedCells);
    const double materialArea = areaOf(perforatedRegion);
    const double outsideArea = areaOf(outsideCheck.Shape());
    const double areaBalanceError = std::abs(regionArea - holeArea - materialArea);

    writeBrep(region, outputDir / "01_irregular_region.brep");
    writeBrep(allHexagons, outputDir / "02_unclipped_hex_grid.brep");
    writeBrep(clippedCells, outputDir / "03_clipped_hex_cells.brep");
    writeBrep(perforatedRegion, outputDir / "04_perforated_region.brep");

    std::cout << std::fixed << std::setprecision(9)
              << "Generated candidate cells : " << generatedCount << '\n'
              << "Clipped result faces      : " << countFaces(clippedCells) << '\n'
              << "Region area               : " << regionArea << '\n'
              << "Clipped hole area         : " << holeArea << '\n'
              << "Remaining material area   : " << materialArea << '\n'
              << "Area balance error        : " << areaBalanceError << '\n'
              << "Area outside boundary     : " << outsideArea << '\n';

    const double allowedError = std::max(1.0e-6, regionArea * 1.0e-8);
    if (outsideArea > allowedError || areaBalanceError > allowedError)
    {
      std::cerr << "Verification failed; allowed error = " << allowedError << '\n';
      return 2;
    }

    std::cout << "PASS: honeycomb reaches the boundary without crossing it.\n"
              << "BREP files written to: " << outputDir.string() << '\n';
    return 0;
  }
  catch (const std::exception& exception)
  {
    std::cerr << "ERROR: " << exception.what() << '\n';
    return 1;
  }
}

