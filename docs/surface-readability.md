# Greybox surface material and contours

Modern height terrain, occupancy solids/ramps, slabs and ladders use an actual
normal-based bgfx surface program. The fixed light points toward
`normalize(-0.45, 0.82, 0.35)`, with directional intensity 0.80 and ambient fill
0.42 (falling to 0.22 on undersides). A height cue changes brightness by 2.5% per
tile of elevation, clamped to ±8%. Bright semantic colors retain their hue with
a bounded peak channel instead of clipping. Indoor dim colors still multiply
the material; marker and selection passes retain their existing colors.

`build_surface_visual_mesh` prepares render-only data from greybox fill quads.
It uses double arithmetic for normals, including accepted very large coordinates.
Greybox fill provides explicit outward hints for legacy slab/solid/ladder sides
and height-field walls, whose old two-sided unlit winding was inconsistent.
No vertex positions, map data, collision queries or movement rules change.

On each axis-aligned plane, rectangle partitioning cancels opposed touching faces,
including partial slab/step contacts. Contour extraction operates on polygon
boundaries, splits collinear intervals and suppresses shared coplanar edges.
Triangle diagonals and internal voxel faces therefore do not become outlines.
Oblique line matching has a visual weld tolerance of 1e-6 tile units.

Partition work and added faces are bounded. If pathological overlapping rectangles
exceed that budget, their original fill remains and their uncertain contour
contributions are omitted. This avoids multiplying an otherwise valid map into
an unbounded visual mesh. Ordinary adjacent/stacked voxels and partial contacts
use the exact partition path, covered by regressions.

Contours are depth-tested strips expanded by a vertex shader to 1.5 framebuffer
pixels, with a small depth bias. Their thickness is independent of camera zoom
and logical UI scale. The ground grid is quieter and remains separate from dark
shape contours. All passes use a defined sequential order in the world view.

The renderer build enables only the pinned bgfx `shaderc` tool, compiling our
shader sources into embedded GLSL 150 and SPIR-V arrays, plus DXBC on Windows.
The installed executable needs no runtime shader files or source-tree path.
Renderer-disabled/headless builds never fetch or build this tool chain. Native
Windows D3D11/D3D12 and Linux OpenGL/Vulkan are the supported shader targets;
local rendered acceptance is Windows WARP, not a claim of Linux execution.

`data/maps/surface_readability.json` contains steps, adjacent and stacked voxels,
an isolated cube, four ramp orientations and a thin supported bridge. Real GUI
scenarios `surface-top`, `surface-tilt` and `surface-under` capture fixed views.
PNG luminance probes verify top/side separation, slope shading, a bounded
top-down height cue, a dark underside and an actual contour pixel. Before and
after top/tilt captures use identical camera and player observations.
