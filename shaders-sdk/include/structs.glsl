#ifndef _STRUCTS_H
#define _STRUCTS_H

#include "../include/mathlib.glsl"

struct Texel {
     vec4 coord;
     vec4 color; // when collected from blocks
     vec4 p3d;
     vec4 albedo;
     vec4 normal;
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
#define UBLANEF_UNPACKED_ vec4
#else
#ifdef AMD_F16_BVH
#define UBLANEF_UNPACKED_ f16vec4
#else
#define UBLANEF_UNPACKED_ vec4
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
//#define UBOXF_ f16mat4 // AMD does not support fp16 matrices storage?
#define UBOXF_ f16vec4[4]
#else
#define UBOXF_ uvec2[4]
#endif
#endif

#ifdef USE_F32_BVH
#define UHBOXF_ mat2x4
#else 
#ifdef AMD_F16_BVH
//#define UBOXF_ f16mat4 // AMD does not support fp16 matrices storage?
#define UHBOXF_ f16vec4[2]
#else
#define UHBOXF_ uvec2[2]
#endif
#endif

#ifdef USE_F32_BVH
#define UBOXF_UNPACKED_ mat4
#else
#ifdef AMD_F16_BVH
#define UBOXF_UNPACKED_ f16mat4
#else
#define UBOXF_UNPACKED_ mat4
#endif
#endif

#ifdef USE_F32_BVH
#define UNPACK_BOX_(m)UBOXF_UNPACKED_(m[0],m[1],m[2],m[3])
//#define PACK_BOX_(m)UBOXF_(m[0],m[1],m[2],m[3])
#else 
#ifdef AMD_F16_BVH
#define UNPACK_BOX_(m)UBOXF_UNPACKED_(m[0],m[1],m[2],m[3]) // AMD does not support matrices storage?
//#define PACK_BOX_(m)UBOXF_(m[0],m[1],m[2],m[3])
#else
#define UNPACK_BOX_(m)UBOXF_UNPACKED_(unpackHalf(m[0]),unpackHalf(m[1]),unpackHalf(m[2]),unpackHalf(m[3]))
//#define PACK_BOX_(m)UBOXF_(packHalf2(m[0]),packHalf2(m[1]),packHalf2(m[2]),packHalf2(m[3]))
#endif
#endif

#ifdef USE_F32_BVH
#define UNPACK_LANE_(m)m
#define PACK_LANE_(m)m
#else 
#ifdef AMD_F16_BVH
#define UNPACK_LANE_(m)UBLANEF_UNPACKED_(m)
#define PACK_LANE_(m)m
#else
#define UNPACK_LANE_(m)unpackHalf(m)
#define PACK_LANE_(m)packHalf2(m)
#endif
#endif

#define TRANSPOSEF_(m)PACK_BOX_(transpose(UNPACK_BOX_(m)))


// ray bitfield spec (reduced to 16-bit)
// {0     }[1] - actived or not
// {1 ..2 }[2] - ray type (for example diffuse, specular, shadow)
// {3 ..7 }[5] - stream/light system id
// {8 ..10}[3] - bounce index, 3-bit (reflection, refraction)
// {11..13}[3] - bounce index, 3-bit (diffuse)



// possible structure in 32-byte presentation 
// but: 
// 1. use cartesian direction instead of 3-vec
// 2. need resolve "vec4" conflict of 16-bit encoded data in "vec3", because need reserve one of 32-bit 
// 3. decline of "final" color term 
// 4. reduce bitfield to 16-bit weight 

// possible extra solutions: 
// 1. add extra array of ray metadata/offload (but we very limited in binding registers)

struct RayRework {
     vec4 origin; vec2 cdirect; uvec2 dcolor;
};


// write color, but don't write (save) last element
uvec2 _writeColor(inout uvec2 rwby, in vec4 color){
    uint btw = BFE_HW(rwby.y, 16, 16);
    uvec2 clr = f32_f16(color);
    rwby = uvec2(clr.x, BFI_HW(clr.y, btw, 16, 16));
    return rwby;
}



struct HitRework {
     vec4 uvt; // UV, distance, triangle
     vec4 normalHeight; // normal with height mapping, will already interpolated with geometry
     vec4 tangent; // also have 4th extra slot
     vec4 bitangent; // TODO: extend structure size
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


const int B16FT = 16;
const ivec2 ACTIVED = ivec2(B16FT+0, 1);
const ivec2 TYPE = ivec2(B16FT+1, 2);
const ivec2 TARGET_LIGHT = ivec2(B16FT+3, 5);
const ivec2 BOUNCE = ivec2(B16FT+8, 3);
const ivec2 DBOUNCE = ivec2(B16FT+11, 3);


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


#define RAY_BITFIELD_ ray.dcolor.y
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




int RayType(inout RayRework ray) {
    return parameteri(TYPE, RAY_BITFIELD_);
}

void RayType(inout RayRework ray, in int type) {
    parameteri(TYPE, RAY_BITFIELD_, type);
}



// criteria-based 
BOOL_ RayDL(inout RayRework ray) {
    return BOOL_(RayType(ray) == 2);
}



int RayTargetLight(inout RayRework ray) {
    return parameteri(TARGET_LIGHT, RAY_BITFIELD_);
}

void RayTargetLight(inout RayRework ray, in int tl) {
    parameteri(TARGET_LIGHT, RAY_BITFIELD_, tl);
}


int RayBounce(inout RayRework ray) {
    return int(uint(parameteri(BOUNCE, RAY_BITFIELD_)));
}

void RayBounce(inout RayRework ray, in int bn) {
    parameteri(BOUNCE, RAY_BITFIELD_, int(uint(bn)));
}


int RayDiffBounce(inout RayRework ray) {
    return int(uint(parameteri(DBOUNCE, RAY_BITFIELD_)));
}

void RayDiffBounce(inout RayRework ray, in int bn) {
    parameteri(DBOUNCE, RAY_BITFIELD_, int(uint(bn)));
}



struct HlbvhNode {
     UHBOXF_ lbox;
     ivec4 pdata;
};



#endif
