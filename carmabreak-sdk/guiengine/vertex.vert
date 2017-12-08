#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "../include/constants.glsl"
#include "../include/mathlib.glsl"

layout ( std430, binding = 0, set = 0 ) readonly buffer GuiEngineUniform { ivec4 mdata; mat4 gproj; };
layout ( std430, binding = 1, set = 0 ) readonly buffer GuiEngineVBuffer { float verts[]; };
layout ( std430, binding = 2, set = 0 ) readonly buffer GuiEngineVIndice { INDICE_T indices[]; };
layout ( location = 0 ) out vec2 texcoord;
layout ( location = 1 ) out vec4 color;
void main() {
    const uint stride4 = 5;
    uint vid = gl_VertexIndex + mdata.y;
    uint idc = mdata.x + uint(PICK(indices, vid));
    texcoord = vec2(verts[idc*stride4+2], verts[idc*stride4+3]), color = unpackUnorm4x8(floatBitsToUint(verts[idc*stride4+4]));
    gl_Position = vec4(verts[idc*stride4+0], verts[idc*stride4+1], 0.f, 1.f) * gproj;
}
