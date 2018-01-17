#version 460 core
#extension GL_GOOGLE_include_directive : enable

#define FRAGMENT_SHADER
#define SIMPLIFIED_RAY_MANAGMENT

#include "../include/constants.glsl"
#include "../include/structs.glsl"
#include "../include/uniforms.glsl"

layout ( location = 0 ) out vec4 outFragColor;
layout ( binding = 0 ) uniform sampler2D samples;
layout ( location = 0 ) in vec2 vcoord;

#define NEIGHBOURS 8
#define AXES 4
#define POW2(a) ((a)*(a))
#define GEN_METRIC(before, center, after) POW2((center) * vec4(2.0f) - (before) - (after))
#define BAIL_CONDITION(new,original) (lessThanEqual(new, original))
#define SYMMETRY(a) (-a)
#define O(u,v) (ivec2(u, v))

const ivec2 axes[AXES] = {O(-1, -1), O( 0, -1), O( 1, -1), O(-1,  0)};

vec4 filtered(in vec2 tx) {
    return textureLod(samples, tx, 0);
}

void main() {
    vec2 ctx = vcoord.xy * vec2(1.f,0.5f) + vec2(0.f,0.5f);

    vec3 color = filtered(ctx).xyz;
    color = fromLinear(color);
    outFragColor = vec4(color.xyz, 1.0f);
}
