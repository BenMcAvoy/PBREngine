#!/usr/bin/env python3
"""
gen_cube.py

Generate cube vertex/normal/index arrays and print C++-ready constexpr std::array
- Default cube uses coordinates +/-1.0 (same as your header).
- Ensures triangle winding is consistent with face normals (fixes inverted windings).
- Optionally compute smooth normals (averaged) by setting SMOOTH_NORMALS = True.
"""

from typing import List, Tuple

SMOOTH_NORMALS = False  # set True if you want averaged vertex normals (not per-face)

Vec3 = Tuple[float, float, float]

def sub(a: Vec3, b: Vec3) -> Vec3:
    return (a[0]-b[0], a[1]-b[1], a[2]-b[2])

def cross(a: Vec3, b: Vec3) -> Vec3:
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])

def dot(a: Vec3, b: Vec3) -> float:
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]

def norm(a: Vec3) -> Vec3:
    import math
    l = math.sqrt(dot(a,a))
    if l == 0: return (0.0,0.0,0.0)
    return (a[0]/l, a[1]/l, a[2]/l)

# faces: list of (normal, [4 verts in some order])
# We'll list verts such that they are CCW when looking at the face from outside (right-hand rule).
faces = [
    # back face (z = -1), normal (0,0,-1)
    ((0.0, 0.0, -1.0), [(-1.0,-1.0,-1.0),(1.0,-1.0,-1.0),(1.0,1.0,-1.0),(-1.0,1.0,-1.0)]),
    # front face (z = +1), normal (0,0,1)
    ((0.0, 0.0, 1.0), [(-1.0,-1.0,1.0),(1.0,-1.0,1.0),(1.0,1.0,1.0),(-1.0,1.0,1.0)]),
    # left face (x = -1), normal (-1,0,0)
    ((-1.0, 0.0, 0.0), [(-1.0,-1.0,-1.0),(-1.0,1.0,-1.0),(-1.0,1.0,1.0),(-1.0,-1.0,1.0)]),
    # right face (x = +1), normal (1,0,0)
    ((1.0, 0.0, 0.0), [(1.0,-1.0,-1.0),(1.0,1.0,-1.0),(1.0,1.0,1.0),(1.0,-1.0,1.0)]),
    # bottom face (y = -1), normal (0,-1,0)
    ((0.0, -1.0, 0.0), [(-1.0,-1.0,-1.0),(1.0,-1.0,-1.0),(1.0,-1.0,1.0),(-1.0,-1.0,1.0)]),
    # top face (y = +1), normal (0,1,0)
    ((0.0, 1.0, 0.0), [(-1.0,1.0,-1.0),(1.0,1.0,-1.0),(1.0,1.0,1.0),(-1.0,1.0,1.0)]),
]

# Build vertex list (pos, normal) and indices; auto-fix triangle winding per-face.
vertices: List[Tuple[Vec3, Vec3]] = []
indices: List[int] = []

for normal, quad in faces:
    base_index = len(vertices)
    # add 4 verts with per-face normal
    for v in quad:
        vertices.append((v, normal))

    # two triangles per quad, initial index order
    tris = [(base_index+0, base_index+1, base_index+2),
            (base_index+2, base_index+3, base_index+0)]

    # for each triangle, ensure geometric normal points same direction as face normal
    fixed_tris = []
    for a,b,c in tris:
        v0 = vertices[a][0]
        v1 = vertices[b][0]
        v2 = vertices[c][0]
        e1 = sub(v1, v0)
        e2 = sub(v2, v0)
        tnormal = cross(e1, e2)
        if dot(tnormal, normal) < 0.0:
            # flip winding by swapping b and c
            fixed_tris.append((a,c,b))
        else:
            fixed_tris.append((a,b,c))
    for tri in fixed_tris:
        indices.extend(tri)

# Optionally compute averaged (smooth) normals
if SMOOTH_NORMALS:
    # accumulate
    acc = [(0.0,0.0,0.0) for _ in range(len(vertices))]
    for i in range(0, len(indices), 3):
        a,b,c = indices[i], indices[i+1], indices[i+2]
        v0 = vertices[a][0]; v1 = vertices[b][0]; v2 = vertices[c][0]
        t = norm(cross(sub(v1,v0), sub(v2,v0)))
        # add triangle normal to each vertex
        def add(i, vec):
            acc[i] = (acc[i][0]+vec[0], acc[i][1]+vec[1], acc[i][2]+vec[2])
        add(a, t); add(b, t); add(c, t)
    # normalize and replace normals
    vertices = [(pos, norm(acc_i)) for (pos,_), acc_i in zip(vertices, acc)]

# Flatten vertices to float list pos+normal
flat_vertices: List[float] = []
for pos, nrm in vertices:
    flat_vertices.extend((float(pos[0]), float(pos[1]), float(pos[2]),
                          float(nrm[0]), float(nrm[1]), float(nrm[2])))

# Helpers to format floats for C++
def fmt_f(f: float) -> str:
    # keep one decimal for ints like -1.0, add f suffix
    s = f"{f:.6f}"
    # strip trailing zeros but keep at least one decimal
    if '.' in s:
        s = s.rstrip('0').rstrip('.')
        if '.' not in s:
            s += '.0'
    return s + 'f'

def cpp_array_floats(name: str, arr: List[float], per_line: int = 6) -> str:
    out = []
    out.append(f"constexpr std::array<float, {len(arr)}> {name} = {{")
    line = "    "
    for i, v in enumerate(arr):
        line += fmt_f(v) + ", "
        if (i+1) % per_line == 0:
            out.append(line)
            line = "    "
    if line.strip():
        out.append(line)
    out.append("};")
    return "\n".join(out)

def cpp_array_uints(name: str, arr: List[int], per_line: int = 12) -> str:
    out = []
    out.append(f"constexpr std::array<unsigned int, {len(arr)}> {name} = {{")
    line = "    "
    for i, v in enumerate(arr):
        line += str(v) + ", "
        if (i+1) % per_line == 0:
            out.append(line)
            line = "    "
    if line.strip():
        out.append(line)
    out.append("};")
    return "\n".join(out)

# Print results (C++ copy/paste)
print("#pragma once")
print()
print("#include <array>")
print()
print("// Generated by gen_cube.py")
print(cpp_array_floats("cubeVertices", flat_vertices, per_line=6))
print()
print(cpp_array_uints("cubeIndices", indices, per_line=12))

# Print small summary
print()
print("// Aliases for stable names (so both generators provide the same symbols)")
print("constexpr auto sphereVertices = cubeVertices;")
print("constexpr auto sphereIndices = cubeIndices;")
print()
print(f"// Vertex count: {len(vertices)} (expected 24)")
print(f"// Index count : {len(indices)} (expected 36)")
print(f"// Smooth normals: {SMOOTH_NORMALS}")

