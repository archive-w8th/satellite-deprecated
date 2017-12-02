#ifndef _UNIFORMS_H
#define _UNIFORMS_H

#include "../include/structs.glsl"

#define RAY_BLOCK rayBlock. 
#define GEOMETRY_BLOCK geometryBlock.  

struct MaterialUniformStruct {
    int materialOffset;
    int materialCount;
    int phase;
    int lightcount;
};

struct SamplerUniformStruct {
    vec2 sceneRes;
    int iterationCount;
    int blockCount;
    int blockBinCount;
    int texelCount;
    int hitCount;
    int traverseCount;
    mat4 transform;
};

struct LightUniformStruct {
    vec4 lightVector;
    vec4 lightColor;
    vec4 lightOffset;
    vec4 lightAmbient;
};

struct GeometryUniformStruct {
    mat4x4 transform;
    mat4x4 transformInv;

    int materialID;
    int triangleCount;
    int triangleOffset;
    int clearDepth;
};

struct CameraUniformStruct {
    mat4x4 projInv;
    mat4x4 camInv;
    mat4x4 prevCamInv;

    float ftime;
    int enable360;
    int interlace;
    int interlaceStage;
};


struct RayStream {
    vec4 diffuseStream;
    ivec4 superseed;
};


layout ( std430, binding = 12, set = 0 ) readonly buffer LightUniform {
    LightUniformStruct lightNode[];
} lightUniform;

layout ( std430, binding = 13, set = 0 ) readonly buffer RayBlockUniform {
    SamplerUniformStruct samplerUniform;
    CameraUniformStruct cameraUniform;
    MaterialUniformStruct materialUniform;
} rayBlock; 

layout ( std430, binding = 14, set = 0 ) readonly buffer GeometryBlockUniform {
    GeometryUniformStruct geometryUniform;
} geometryBlock;

layout ( std430, binding = 15, set = 0 ) readonly buffer StreamsBlockUniform {
    RayStream rayStreams[];
};

#endif
