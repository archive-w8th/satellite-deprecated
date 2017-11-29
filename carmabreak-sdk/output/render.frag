#version 460 core

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
    ivec2 center_pix = ivec2(tx * textureSize(samples, 0));
    vec4 center_pix_cache = texelFetch(samples, center_pix, 0);
    return center_pix_cache;
}

void main() {
    outFragColor = vec4(fromLinear(filtered(texcoord).xyz), 1.0f);
}
