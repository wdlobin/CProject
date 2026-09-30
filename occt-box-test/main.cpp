#include <BRepPrimAPI_MakeBox.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

#include <iostream>

int main()
{
  const TopoDS_Shape box = BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape();

  if (box.IsNull() || box.ShapeType() != TopAbs_SOLID)
  {
    std::cerr << "OCCT box creation failed\n";
    return 1;
  }

  std::cout << "OCCT 8.0.1 OK: 10 x 20 x 30 box created\n";
  return 0;
}
