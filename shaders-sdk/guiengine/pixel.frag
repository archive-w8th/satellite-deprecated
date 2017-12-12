#version 460 core
#extension GL_GOOGLE_include_directive : enable

#include "../include/constants.glsl"
#include "../include/mathlib.glsl"

layout ( location = 0 ) out vec4 outFragColor;
layout ( location = 0 ) in vec2 texcoord;
layout ( location = 1 ) in vec4 color;
//layout ( binding = 0, set = 1 ) uniform sampler2D guiAtlas;
layout ( binding = 3, set = 0 ) uniform sampler2D guiAtlas;
void main() { 
    outFragColor = color * texture(guiAtlas, texcoord); 
}
