#version 460 core

// here will generation of initial vertices from buffers


#include "../include/constants.glsl"
#include "../include/structs.glsl"
#include "../include/vertex.glsl"
#include "../include/mathlib.glsl"
#include "../include/ballotlib.glsl"

layout (location = 0) out vec2 v_g_texCoord;
void main(void) {
	int vid = int(gl_VertexIndex), tid = vid / 3, lid = vid % 3;
	ivec2 td2 = gatherMosaic(getUniformCoord(tid));
	v_g_texCoord = fetchMosaic(texcoord_texture, td2, lid).xy;
	gl_Position = vec4(fetchMosaic(vertex_texture, td2, lid).xyz,1.f);
}

