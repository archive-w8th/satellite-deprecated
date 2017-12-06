#ifndef _STRUCTS_H
#define _STRUCTS_H

#include "../include/mathlib.glsl"

struct Texel {
     vec4 coord;
     vec4 color; // when collected from blocks
};

struct bbox {
     vec4 mn;
     vec4 mx;
};



#ifdef USE_F32_BVH
#define UBLANEF_ vec4
#else
#ifdef AMD_F16_BVH
#define UBLANEF_ f16vec4
#else
#define UBLANEF_ uvec2
#endif
#endif


#ifdef USE_F32_BVH
#define UBHALFLANEF_ vec2
#else
#ifdef AMD_F16_BVH
#define UBHALFLANEF_ f16vec2
#else
#define UBHALFLANEF_ uint
#endif
#endif


#ifdef USE_F32_BVH
#define UBOXF_ mat4
#else 
#ifdef AMD_F16_BVH
//#define UBOXF_ f16mat4 // AMD does not support matrices storage?
#define UBOXF_ UBLANEF_[4]
#else
#define UBOXF_ uvec2[4]
#endif
#endif


#ifdef USE_F32_BVH
#define TRANSPOSEF_ transpose
#else 
#ifdef AMD_F16_BVH
//#define TRANSPOSEF_ transpose
#define TRANSPOSEF_(m) UBOXF_(UBLANEF_(m[0].x,m[1].x,m[2].x,m[3].x),UBLANEF_(m[0].y,m[1].y,m[2].y,m[3].y),UBLANEF_(m[0].z,m[1].z,m[2].z,m[3].z),UBLANEF_(m[0].w,m[1].w,m[2].w,m[3].w))
#else
#define TRANSPOSEF_(m) (m) // TODO support for uint packed half floats
#endif
#endif





/*
// 128-bit bandwidth
struct bboxf16 {
#ifdef AMD_F16_BVH
    f16vec4 mn;
    f16vec4 mx;
#else
    uvec2 mn;
    uvec2 mx;
#endif
};

// 256-bit bandwidth
struct bboxf32 {
    uvec4 mn;
    uvec4 mx;
};
*/


// ray bitfield spec
// {0     }[1] - actived or not
// {1 ..2 }[2] - ray type (for example diffuse, specular, shadow)
// {3     }[1] - applyable direct light (can intersect with light data or not)
// {4 ..7 }[4] - target light index (for shadow type)
// {8 ..11}[4] - bounce index
// {11..15}[5] - stream directional order (up to 31 streams)

// should be compacted to 64 byte, for storing on 64kb blocks, fast HBM fetching and 1024 lanes dispatch
struct RayRework {
     //vec3 origin; int hit; // index of hit chain
     //vec3 direct; uint bitfield; // up to 32-bits
     vec4 origin;
     vec4 direct;
     vec4 color;
     vec4 final;
};


struct HitRework {
     vec4 uvt; // UV, distance, triangle
     vec4 normalHeight; // normal with height mapping, will already interpolated with geometry
     vec4 tangent; // also have 4th extra slot
     vec4 texcoord; // critical texcoords 

    uvec2 metallicRoughness;
    uvec2 unk16;
    uvec2 emission;
    uvec2 albedo;

    // integer metadata
    uint bitfield; 
     int ray; // ray index
     int materialID;
     int next;
};


const ivec2 ACTIVED = ivec2(0, 1);
const ivec2 TYPE = ivec2(1, 2);
const ivec2 DIRECT_LIGHT = ivec2(3, 1);
const ivec2 TARGET_LIGHT = ivec2(4, 4);
const ivec2 BOUNCE = ivec2(8, 4);
const ivec2 SDO = ivec2(11, 5);


int parameteri(const ivec2 parameter, inout uint bitfield) {
    return int(BFE_HW(bitfield, parameter.x, parameter.y));
}

void parameteri(const ivec2 parameter, inout uint bitfield, in int value) {
    bitfield = BFI_HW(bitfield, uint(value), parameter.x, parameter.y);
}

BOOL_ parameterb(const ivec2 parameter, inout uint bitfield) {
    return BOOL_(BFE_HW(bitfield, parameter.x, 1));
}

void parameterb(const ivec2 parameter, inout uint bitfield, in BOOL_ value) {
    bitfield = BFI_HW(bitfield, uint(value), parameter.x, 1);
}


int parameteri(const ivec2 parameter, inout float bitfield) {
    return int(BFE_HW(floatBitsToUint(bitfield), parameter.x, parameter.y));
}

void parameteri(const ivec2 parameter, inout float bitfield, in int value) {
    bitfield = uintBitsToFloat(BFI_HW(floatBitsToUint(bitfield), uint(value), parameter.x, parameter.y));
}

BOOL_ parameterb(const ivec2 parameter, inout float bitfield) {
    return BOOL_(BFE_HW(floatBitsToUint(bitfield), parameter.x, 1));
}

void parameterb(const ivec2 parameter, inout float bitfield, in BOOL_ value) {
    bitfield = uintBitsToFloat(BFI_HW(floatBitsToUint(bitfield), uint(value), parameter.x, 1));
}





/* // under consideration parameter based system, this code should be rewrite
int parameteri(const ivec2 parameter, inout HitRework hit) {
    return BFE(hit.bitfield, parameter.x, parameter.y);
}

int parameteri(const ivec2 parameter, inout RayRework ray) {
    return BFE(floatBitsToInt(ray.direct.w), parameter.x, parameter.y);
}

void parameteri(const ivec2 parameter, inout RayRework ray, in int value) {
    ray.direct.w = intBitsToFloat(BFI_HW(floatBitsToInt(ray.direct.w), value, parameter.x, parameter.y));
}

void parameterb(const ivec2 parameter, inout RayRework ray, in BOOL_ value) {
    ray.direct.w = intBitsToFloat(BFI_HW(floatBitsToInt(ray.direct.w), int(value), parameter.x, parameter.y));
}

void parameteri(const ivec2 parameter, inout HitRework hit, in int value) {
    hit.bitfield = BFI_HW(hit.bitfield, value, parameter.x, parameter.y);
}

void parameterb(const ivec2 parameter, inout HitRework hit, in BOOL_ value) {
    hit.bitfield = BFI_HW(hit.bitfield, int(value), parameter.x, parameter.y);
}
*/



#define RAY_BITFIELD_ ray.direct.w
#define HIT_BITFIELD_ hit.bitfield


BOOL_ HitActived(inout HitRework hit) {
    return parameterb(ACTIVED, HIT_BITFIELD_);
}

void HitActived(inout HitRework hit, in BOOL_ actived) {
    parameterb(ACTIVED, HIT_BITFIELD_, actived);
}

BOOL_ RayActived(inout RayRework ray) {
    return parameterb(ACTIVED, RAY_BITFIELD_);
}

void RayActived(inout RayRework ray, in BOOL_ actived) {
    parameterb(ACTIVED, RAY_BITFIELD_, actived);
}

BOOL_ RayDL(inout RayRework ray) {
    return parameterb(DIRECT_LIGHT, RAY_BITFIELD_);
}

void RayDL(inout RayRework ray, in BOOL_ dl) {
    parameterb(DIRECT_LIGHT, RAY_BITFIELD_, dl);
}


int RayType(inout RayRework ray) {
    return parameteri(TYPE, RAY_BITFIELD_);
}

void RayType(inout RayRework ray, in int type) {
    parameteri(TYPE, RAY_BITFIELD_, type);
}

int RayTargetLight(inout RayRework ray) {
    return parameteri(TARGET_LIGHT, RAY_BITFIELD_);
}

void RayTargetLight(inout RayRework ray, in int tl) {
    parameteri(TARGET_LIGHT, RAY_BITFIELD_, tl);
}

int RayBounce(inout RayRework ray) {
    return parameteri(BOUNCE, RAY_BITFIELD_);
}

void RayBounce(inout RayRework ray, in int bn) {
    parameteri(BOUNCE, RAY_BITFIELD_, bn);
}



struct HlbvhNode {
     UBOXF_ lrbox;
     ivec4 pdata;
};


struct VboDataStride {
     vec4 vertex;
     vec4 normal;
     vec4 texcoord;
     vec4 color;
     vec4 modifiers;
};

struct ColorChain {
     vec4 color;
     ivec4 cdata;
};

struct MeshUniformStruct {
     int vertexAccessor;
     int normalAccessor;
     int texcoordAccessor;
     int modifierAccessor;

     mat4 transform;
     mat4 transformInv;

     int materialID;
     int isIndexed;
     int nodeCount;
     int primitiveType;

     int loadingOffset;
     int storingOffset;
     int _reserved0;
     int _reserved1;
};

struct VirtualBufferView {
     int offset4;
     int stride4;
};

struct VirtualAccessor {
     int offset4;
     uint bitfield;
     int bufferView;
};

const ivec2 COMPONENTS = ivec2(0, 2);
const ivec2 ATYPE = ivec2(2, 4);
const ivec2 NORMALIZED = ivec2(6, 1);

int aComponents(inout VirtualAccessor vac) {
    return parameteri(COMPONENTS, vac.bitfield);
}

int aType(inout VirtualAccessor vac) {
    return parameteri(ATYPE, vac.bitfield);
}

int aNormalized(inout VirtualAccessor vac) {
    return parameteri(NORMALIZED, vac.bitfield);
}

#endif
