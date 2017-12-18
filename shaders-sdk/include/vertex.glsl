#ifndef _VERTEX_H
#define _VERTEX_H

#include "../include/mathlib.glsl"

#ifdef VERTEX_FILLING
layout ( std430, binding = 7, set = 0 ) restrict buffer GeomMaterialsSSBO { int materials[]; };
layout ( std430, binding = 8, set = 0 ) restrict buffer OrderIdxSSBO { int vorders[]; };
layout ( binding = 10, rgba32f, set = 0 ) uniform image2D vertex_texture_out;
layout ( binding = 11, rgba32f, set = 0 ) uniform image2D normal_texture_out;
layout ( binding = 12, rgba32f, set = 0 ) uniform image2D texcoords_texture_out;
layout ( binding = 13, rgba32f, set = 0 ) uniform image2D modifiers_texture_out;
#else
layout ( binding = 10, set = 1 ) uniform sampler2D vertex_texture;
layout ( binding = 11, set = 1 ) uniform sampler2D normal_texture;
layout ( binding = 12, set = 1 ) uniform sampler2D texcoords_texture;
layout ( binding = 13, set = 1 ) uniform sampler2D modifiers_texture;
layout ( std430, binding = 0, set = 1 ) readonly buffer BVHBlock { UBLANEF_ bvhBoxes[][4]; };
layout ( std430, binding = 1, set = 1 ) readonly buffer GeomMaterialsSSBO { int materials[]; };
layout ( std430, binding = 2, set = 1 ) readonly buffer OrderIdxSSBO { int vorders[]; };
layout ( std430, binding = 3, set = 1 ) readonly buffer GeometryBlockUniform { GeometryUniformStruct geometryUniform;} geometryBlock;
#endif

#ifdef ENABLE_AMD_INSTRUCTION_SET
//#define ISTORE(img, crd, data) imageStoreLodAMD(img, crd, 0, data)
//#define SGATHER(smp, crd, chnl) textureGatherLodAMD(smp, crd, 0, chnl)
#else
//#define ISTORE(img, crd, data) imageStore(img, crd, data)
//#define SGATHER(smp, crd, chnl) textureGather(smp, crd, chnl)
#endif

#define ISTORE(img, crd, data) imageStore(img, crd, data)
#define SGATHER(smp, crd, chnl) textureGather(smp, crd, chnl)

//#define _SWIZV wzx
#define _SWIZV xyz

const int WARPED_WIDTH = 2048;
//const ivec2 mit[3] = {ivec2(0,0), ivec2(1,0), ivec2(0,1)};
const ivec2 mit[3] = {ivec2(0,1), ivec2(1,1), ivec2(1,0)};

ivec2 mosaicIdc(in ivec2 mosaicCoord, in int idc) {
    return mosaicCoord + mit[idc];
}

ivec2 gatherMosaic(in ivec2 uniformCoord) {
    return ivec2(uniformCoord.x * 3 + uniformCoord.y % 3, uniformCoord.y);
}

vec4 fetchMosaic(in sampler2D vertices, in ivec2 mosaicCoord, in uint idc) {
    //return texelFetch(vertices, mosaicCoord + mit[idc], 0);
    return textureLod(vertices, (vec2(mosaicCoord + mit[idc]) + 0.49999f) / textureSize(vertices, 0), 0);
}

ivec2 getUniformCoord(in int indice) {
    return ivec2(indice % WARPED_WIDTH, indice / WARPED_WIDTH);
}

ivec2 getUniformCoord(in uint indice) {
    return ivec2(indice % WARPED_WIDTH, indice / WARPED_WIDTH);
}



vec2 dot2(in mat3x2 a, in mat3x2 b) {
    return fma(a[0],b[0], fma(a[1],b[1], a[2]*b[2]));
    //mat2x3 at = transpose(a);
    //mat2x3 bt = transpose(b);
    //return vec2(dot(at[0], bt[0]), dot(at[1], bt[1]));
}

#ifndef VERTEX_FILLING
float intersectTriangle(inout vec3 orig, inout mat3 M, inout int axis, inout int tri, inout vec2 UV, inout BOOL_ _valid, in float testdist) {
    float T = INFINITY; BOOL_ valid = tri < 0 ? FALSE_ : _valid; // pre-define
    const vec3 D[3] = {vec3(1.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f)};
    const vec2 sz = 1.f / textureSize(vertex_texture, 0), hs = sz * 0.9999f;
    IF (valid) {
        // gather patterns
        vec2 ntri = fma(vec2(gatherMosaic(getUniformCoord(tri))), sz, hs);
        mat3 ABC = mat3(
            SGATHER(vertex_texture, ntri, 0)._SWIZV - orig.x,
            SGATHER(vertex_texture, ntri, 1)._SWIZV - orig.y,
            SGATHER(vertex_texture, ntri, 2)._SWIZV - orig.z
        ) * M;

        // PURE watertight triangle intersection (our, GPU-GLSL adapted version)
        // http://jcgt.org/published/0002/01/05/paper.pdf
        vec3 UVW_ = D[axis] * inverse(ABC);
        valid &= BOOL_(all(greaterThanEqual(UVW_, vec3(0.f))) || all(lessThanEqual(UVW_, vec3(0.f))));
        IF (valid) {
            float det = dot(UVW_,vec3(1)); UVW_ *= 1.f/(max(abs(det),0.00001f)*(det>=0.f?1:-1));
            UV = vec2(UVW_.yz), UVW_ *= ABC; // calculate axis distances
            T = mix(mix(UVW_.z, UVW_.y, axis == 1), UVW_.x, axis == 0);
            T = mix(INFINITY, T, greaterEqualF(T, 0.0f) & valid);
        }
    }
    return T;
}
#endif


//==============================
//BVH data future transcoding (each by 32-bit only)
//By textureGather you can get siblings
/* 
        Sib     P    T
    +==================+
L   | x || y || z || w |
    +==================+
R   | x || y || z || w |
    +==================+
    
*///============================


// bvh transcoded storage
#ifdef BVH_CREATION
layout ( binding = 11, rgba32i, set = 0 ) uniform iimage2D bvhStorage;
#else
layout ( binding = 5, set = 1 ) uniform isampler2D bvhStorage;
#endif


#ifdef USE_F32_BVH // planned full support
#define UNPACK_TX_(m)unpackHalf(m)
#define PACK_TX_(m)packHalf2(m)
#else 
#ifdef AMD_F16_BVH
#define UNPACK_TX_(m)unpackHalf2(m)
#define PACK_TX_(m)packHalf2(m)
#else
#define UNPACK_TX_(m)unpackHalf(m)
#define PACK_TX_(m)packHalf2(m)
#endif
#endif


const int _BVH_WIDTH = 2048;

ivec2 bvhLinear2D(in int linear) {
    int md = linear & 1; linear >>= 1;
    return ivec2(linear % _BVH_WIDTH, ((linear / _BVH_WIDTH) << 1) + md);
}

#ifndef BVH_CREATION
vec2 bvhGatherifyStorage(in ivec2 ipt){
    const vec2 sz = 1.f / textureSize(bvhStorage, 0), hs = sz * 0.9999f;
    return fma(vec2(ipt), sz, hs);
}

#endif


#endif
