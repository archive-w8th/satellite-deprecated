#version 460 core
#extension GL_GOOGLE_include_directive : enable

const vec2 cpositions[4] = { vec2(-1.f), vec2(1.f, -1.f), vec2(-1.f, 1.f), vec2(1.f) };
layout ( location = 0 ) out vec2 texcoord;

void main() {
    vec2 position = cpositions[gl_VertexIndex];
    texcoord.xy = fma(position.xy, vec2(0.5f), vec2(0.5f));
    texcoord.y = 1.f - texcoord.y;
    gl_Position = vec4(position.xy, 0.0f, 1.0f);
}
