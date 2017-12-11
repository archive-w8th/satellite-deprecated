#ifndef _VERTEX_H
#define _VERTEX_H

#include "../include/mathlib.glsl"

#ifdef VERTEX_FILLING
layout ( std430, binding = 10, set = 0 ) restrict buffer GeomMaterialsSSBO { int materials[]; };
layout ( binding = 0, rgba32f, set = 1 ) uniform image2D vertex_texture_out;
layout ( binding = 1, rgba32f, set = 1 ) uniform image2D normal_texture_out;
layout ( binding = 2, rgba32f, set = 1 ) uniform image2D texcoords_texture_out;
layout ( binding = 3, rgba32f, set = 1 ) uniform image2D modifiers_texture_out;
#else
layout ( std430, binding = 10, set = 0 ) readonly buffer GeomMaterialsSSBO { int materials[]; };
layout ( binding = 0, set = 1 ) uniform sampler2D vertex_texture;
layout ( binding = 1, set = 1 ) uniform sampler2D normal_texture;
layout ( binding = 2, set = 1 ) uniform sampler2D texcoords_texture;
layout ( binding = 3, set = 1 ) uniform sampler2D modifiers_texture;
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

vec2 intersectTriangle2(inout vec3 orig, inout vec3 dir, inout ivec2 tri, inout vec4 UV, inout bvec2 valid) {
    UV = vec4(0.f);

    vec2 t2 = vec2(INFINITY);

    valid = and(valid, notEqual(tri, ivec2(LONGEST)));
    if (any(valid)) {
        ivec2 tri0 = gatherMosaic(getUniformCoord(tri.x));
        ivec2 tri1 = gatherMosaic(getUniformCoord(tri.y));

        mat3x2 v012x, v012y, v012z;
        {
            vec2 sz = 1.f / textureSize(vertex_texture, 0), hs = sz * 0.9999f;
            vec2 ntri0 = fma(vec2(tri0), sz, hs), ntri1 = fma(vec2(tri1), sz, hs);
            v012x = transpose(mat2x3(
                SGATHER(vertex_texture, ntri0, 0)._SWIZV,
                SGATHER(vertex_texture, ntri1, 0)._SWIZV
            )), 
            v012y = transpose(mat2x3(
                SGATHER(vertex_texture, ntri0, 1)._SWIZV,
                SGATHER(vertex_texture, ntri1, 1)._SWIZV
            )), 
            v012z = transpose(mat2x3(
                SGATHER(vertex_texture, ntri0, 2)._SWIZV,
                SGATHER(vertex_texture, ntri1, 2)._SWIZV
            ));
        }

        mat3x2 e1 = mat3x2(v012x[1] - v012x[0], v012y[1] - v012y[0], v012z[1] - v012z[0]);
        mat3x2 e2 = mat3x2(v012x[2] - v012x[0], v012y[2] - v012y[0], v012z[2] - v012z[0]);
        mat3x2 dir2 = mat3x2(dir.xx, dir.yy, dir.zz);
        mat3x2 orig2 = mat3x2(orig.xx, orig.yy, orig.zz);

        mat3x2 pvec = mat3x2(
            fma(dir2[1], e2[2], - dir2[2] * e2[1]), 
            fma(dir2[2], e2[0], - dir2[0] * e2[2]), 
            fma(dir2[0], e2[1], - dir2[1] * e2[0])
        );

        vec2 det = dot2(pvec, e1);
        valid = and(valid, greaterThan(abs(det), vec2(0.f)));
        if (any(valid)) {
            vec2 invDev = 1.f / (max(abs(det), 0.0001f) * sign(det));
            mat3x2 tvec = orig2; // reuse
            tvec = mat3x2(
                tvec[0] - v012x[0],
                tvec[1] - v012y[0], 
                tvec[2] - v012z[0]
            );

            vec2 u = vec2(0.f);
            u = dot2(tvec, pvec) * invDev;
            valid = and(valid, and(greaterThanEqual(u, vec2(-0.0001f)), lessThan(u, vec2(1.0001f))));
            if (any(valid)) {
                mat3x2 qvec = tvec;
                qvec = mat3x2(
                    fma(qvec[1], e1[2], -qvec[2] * e1[1]),
                    fma(qvec[2], e1[0], -qvec[0] * e1[2]),
                    fma(qvec[0], e1[1], -qvec[1] * e1[0])
                );

                vec2 v = vec2(0.f);
                v = dot2(dir2, qvec) * invDev;
                valid = and(valid, and(greaterThanEqual(v, vec2(-0.0001f)), lessThan(u+v, vec2(1.0001f))));
                if (any(valid)) {
                    // distance
                    t2 = dot2(e2, qvec) * invDev;
                    valid = and(valid, lessThan(t2, vec2(INFINITY - PZERO)));
                    valid = and(valid, greaterThan(t2, vec2(0.0f - PZERO)));

                    UV = vec4(u, v);
                }
            }
        }
    }

    return mix(vec2(INFINITY), t2, valid);
}






const vec3 D[3] = {vec3(1.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f)};

// 
float intersectTriangle(inout vec3 orig, inout mat3 M, inout int axis, inout int tri, inout vec2 UV, inout BOOL_ _valid) {
    float T = INFINITY; BOOL_ valid = tri < 0 ? FALSE_ : _valid; // pre-define
    IF (valid) {
        // gather patterns (may have papers, I didn't know)
        vec2 sz = 1.f / textureSize(vertex_texture, 0), hs = sz * 0.9999f;
        vec2 ntri = fma(vec2(gatherMosaic(getUniformCoord(tri))), sz, hs);
        mat3 ABC = mat3(
            SGATHER(vertex_texture, ntri, 0)._SWIZV - orig.x,
            SGATHER(vertex_texture, ntri, 1)._SWIZV - orig.y,
            SGATHER(vertex_texture, ntri, 2)._SWIZV - orig.z
        ) * M;

        // PURE watertight triangle intersection (our, GPU-GLSL adapted version)
        // http://jcgt.org/published/0002/01/05/paper.pdf
        vec3 UVW_ = D[axis] * inverse(ABC);
        valid &= BOOL_(all(greaterThanEqual(UVW_, vec3(0.f)))) | BOOL_(all(lessThanEqual(UVW_, vec3(0.f))));
        IF (valid) {
            float det = dot(UVW_,vec3(1)); UVW_ *= 1.f/(max(abs(det),0.000001f)*(det>=0.f?1:-1));
            UV = vec2(UVW_.yz), UVW_ *= ABC; // calculate axis distances
            T = mix(mix(UVW_.z, UVW_.y, axis == 1), UVW_.x, axis == 0);
            T = mix(INFINITY, T, greaterEqualF(T, 0.0f) & valid);
        }
    }
    return T;
}

// 
float intersectTriangle(inout vec3 orig, inout vec3 direct, inout int tri, inout vec2 UV, inout BOOL_ _valid) {
    float T = INFINITY; BOOL_ valid = tri < 0 ? FALSE_ : _valid; // pre-define
    IF (valid) {
        ivec2 ntri = gatherMosaic(getUniformCoord(tri));
        mat3 ABC = mat3(
            orig.xyz - fetchMosaic(vertex_texture, ntri, 0).xyz,
            orig.xyz - fetchMosaic(vertex_texture, ntri, 1).xyz,
            orig.xyz - fetchMosaic(vertex_texture, ntri, 2).xyz
        );
        ABC[1] = ABC[0] - ABC[1], ABC[2] = ABC[0] - ABC[2];
        
        vec3 pvec = cross(direct, ABC[2]);
        float idet = dot(ABC[1], pvec); idet = 1.f/(max(abs(idet),0.000001f)*(idet >= 0.f?1:-1));
        float u = dot(ABC[0], pvec) * idet;
        //vec3 qvec = cross(ABC[0], ABC[1]);
        //float v = dot(direct, qvec) * idet;
        //float t = dot(ABC[2], qvec) * idet;
        pvec = cross(ABC[0], ABC[1]); // reuse variable
        float v = dot(direct, pvec) * idet;
        float t = dot(ABC[2], pvec) * idet;

        valid &= greaterEqualF(t, 0.f) & greaterEqualF(u, 0.f) & greaterEqualF(v, 0.f) & lessEqualF(u+v, 1.f) & BOOL_(abs(idet) < 100000.f), UV = vec2(u,v);
        return mix(INFINITY, t, valid);
    }
    return T;
}



#endif


//==============================
// Current layout 
/* 
      mn  mn  mn  mn  mx  mx  mx  mx
    +===============================+
 L  | x | y | z | w | x | y | z | w | 128-bit (8x16-bit elements)
    +===============================+
 R  | x | y | z | w | x | y | z | w | 128-bit (8x16-bit elements)
    +===============================+
    
*///============================


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
layout ( binding = 5, rgba32i, set = 1 ) uniform iimage2D bvhStorage;
//layout ( binding = 6, rgba32ui, set = 1 ) uniform uimage2D bvhBoxes;
#else
layout ( binding = 5, set = 1 ) uniform isampler2D bvhStorage;
//layout ( binding = 6, set = 1 ) uniform usampler2D bvhBoxes;
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
