#version 460 core
#extension GL_GOOGLE_include_directive : enable

#define FRAGMENT_SHADER
#define SIMPLIFIED_RAY_MANAGMENT

#include "../include/constants.glsl"
#include "../include/structs.glsl"
#include "../include/uniforms.glsl"

layout ( location = 0 ) out vec4 outFragColor;
layout ( location = 0 ) in vec2 texcoord;
layout ( binding = 0 ) uniform sampler2D samples;

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


float rcp(in float a) { return 1.f/a; }
vec3 Tonemap(in vec3 c) { return c * rcp(mlength(c.xyz) + 1.0); }
vec3 TonemapWithWeight(in vec3 c, in float w) { return c * (w * rcp(mlength(c.xyz) + 1.0)); }
vec3 TonemapInvert(in vec3 c) { return c * rcp(1.0 - mlength(c.xyz)); }


void main() {
    vec2 ctx = texcoord * vec2(1.f,0.5f) + vec2(0.f,0.5f);
    outFragColor = vec4(fromLinear(filtered(ctx).xyz), 1.0f);
}
