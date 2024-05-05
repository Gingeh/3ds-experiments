import sys

name = sys.argv[1]

poss = []
uvs = []
nms = []
ids = []

with open(f"{name}.obj", "r") as f:
    lines = f.readlines()
    for line in lines:
        line = line.split(' ')
        if line[0] == "v":
            poss.append((line[1],line[2],line[3]))
        elif line[0] == "vt":
            uvs.append((line[1],line[2]))
        elif line[0] == "vn":
            nms.append((line[1],line[2],line[3]))
        elif line[0] == "f":
            ids.append(line[1].split('/'))
            ids.append(line[2].split('/'))
            ids.append(line[3].split('/'))

verts = []

for vert in ids:
    pos = poss[int(vert[0])-1]
    uv = uvs[int(vert[1])-1]
    nm = nms[int(vert[2])-1]
    verts.append((pos, uv, nm))

count = len(verts)

with open(f"{name}.h", "w+") as f:
    f.write(f"""#pragma once

typedef struct
{{
    float pos[3];
    float uv[2];
    float nm[3];
}} vertex;

#define vertex_count {count}
extern const vertex vertex_list[vertex_count];"""
    )

with open(f"{name}.c", "w+") as f:
    f.write(f"""#include "{name}.h"

const vertex vertex_list[vertex_count] = 
{{"""
    )
    for vert in verts:
        f.write(f"    {{ {{{vert[0][0]}, {vert[0][1]}, {vert[0][2][:-1]}}}, {{{vert[1][0]}, {vert[1][1][:-1]}}}, {{{vert[2][0]}, {vert[2][1]}, {vert[2][2][:-1]}}}}},\n")
    f.write("};")