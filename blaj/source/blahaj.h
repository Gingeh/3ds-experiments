#pragma once

typedef struct
{
    float pos[3];
    float uv[2];
    float nm[3];
} vertex;

#define vertex_count 1095
extern const vertex vertex_list[vertex_count];