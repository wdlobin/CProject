# OCCT irregular-boundary honeycomb test

This small program generates a concave planar polygon, creates an oversized
periodic hexagonal grid, and clips the grid with the polygon using OCCT Boolean
operations. It also subtracts the same grid from the polygon to produce a
perforated sheet.

Run from PowerShell:

```powershell
.\build-release.ps1
```

The `build` directory receives four BREP files:

- `01_irregular_region.brep`
- `02_unclipped_hex_grid.brep`
- `03_clipped_hex_cells.brep`
- `04_perforated_region.brep`

The executable checks that the clipped cells have zero area outside the target
boundary and that `region area = clipped hole area + remaining material area`
within numerical tolerance.
