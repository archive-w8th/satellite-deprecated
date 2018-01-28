#version 460 core
#extension GL_GOOGLE_include_directive : enable

const vec2 cpositions[4] = { vec2(-1.f), vec2(1.f, -1.f), vec2(-1.f, 1.f), vec2(1.f) };

layout ( location = 0 ) out vec2 vcoord;

void main() {
    gl_Position = vec4(cpositions[gl_VertexIndex].xy, 0.0f, 1.0f);
    vcoord = cpositions[gl_VertexIndex].xy*0.5f.xx+0.5f.xx;
}
