#ifndef _MATHLIB_H
#define _MATHLIB_H

// null of indexing in float representation
float FINT_NULL = intBitsToFloat(-1); // -1
float FINT_ZERO = intBitsToFloat( 0); //  0



// vector math utils
float sqlen(in vec3 a) { return dot(a, a); }
float sqlen(in vec2 a) { return dot(a, a); }
float sqlen(in float v) { return v * v; }
int modi(in int a, in int b) { return (a % b + b) % b; }
vec4 divW(in vec4 aw) { return aw / aw.w; }
vec3 rotate_vector( in vec4 quat, in vec3 vec ) { return vec + 2.0 * cross( cross( vec, quat.xyz ) + quat.w * vec, quat.xyz ); }
vec4 rotation_quat( in vec3 axis, in float half_angle ) { return vec4(axis * sin(half_angle), cos(half_angle)); }

#if (defined(ENABLE_AMD_INSTRUCTION_SET))
float mlength(in vec3 mcolor) { return max3(mcolor.x, mcolor.y, mcolor.z); }
#else
float mlength(in vec3 mcolor) { return max(mcolor.x, max(mcolor.y, mcolor.z)); }
#endif

// memory managment
void swap(inout int a, inout int b) { int t = a; a = b; b = t; }
uint exchange(inout uint mem, in uint v) { uint tmp = mem; mem = v; return tmp; }
int exchange(inout int mem, in int v) { int tmp = mem; mem = v; return tmp; }
int add(inout int mem, in int v) { int tmp = mem; mem += v; return tmp; }
uint add(inout uint mem, in uint ops) { uint tmp = mem; mem += ops; return tmp; }

// logical functions
bvec2 not(in bvec2 a) { return bvec2(!a.x, !a.y); }
bvec2 and(in bvec2 a, in bvec2 b) { return bvec2(a.x && b.x, a.y && b.y); }
bvec2 or(in bvec2 a, in bvec2 b) { return bvec2(a.x || b.x, a.y || b.y); }

// logical functions (bvec4)
bvec4 or(in bvec4 a, in bvec4 b) { return bvec4(a.x || b.x, a.y || b.y, a.z || b.z, a.w || b.w); }
bvec4 and(in bvec4 a, in bvec4 b) { return bvec4(a.x && b.x, a.y && b.y, a.z && b.z, a.w && b.w); }
bvec4 not(in bvec4 a) { return bvec4(!a.x, !a.y, !a.z, !a.w); }

// mixing functions
void mixed(inout float src, inout float dst, in float coef) { dst *= coef; src *= 1.0f - coef; }
void mixed(inout vec3 src, inout vec3 dst, in float coef) { dst *= coef; src *= 1.0f - coef; }
void mixed(inout vec3 src, inout vec3 dst, in vec3 coef) { dst *= coef; src *= 1.0f - coef; }
vec3 clamp01(in vec3 c) {return clamp(c,vec3(0.f),vec3(1.f));};
vec4 clamp01(in vec4 c) {return clamp(c,vec4(0.f),vec4(1.f));};
float clamp01(in float c) {return clamp(c,0.f,1.f);};

// matrix math
vec4 mult4(in vec4 vec, in mat4 tmat) { return tmat * vec; }
vec4 mult4(in mat4 tmat, in vec4 vec) { return vec * tmat; }





// 64-bit packing
#ifdef ENABLE_AMD_INSTRUCTION_SET
uvec2 U2P(in uint64_t pckg) { return unpackUint2x32(pckg); }
uint64_t P2U(in uvec2 pckg) { return packUint2x32(pckg); }
#else
uvec2 U2P(in uint64_t pckg) { return uvec2(pckg >> 0ul, pckg >> 32ul); }
uint64_t P2U(in uvec2 pckg) { return uint64_t(pckg.x) | (uint64_t(pckg.y) << 32ul); }
#endif

// 128-bit packing (2x64bit)
uvec4 U4P(in u64vec2 pckg) { return uvec4(U2P(pckg.x), U2P(pckg.y)); }
u64vec2 P4U(in uvec4 pckg) { return u64vec2(uint64_t(P2U(pckg.xy)), uint64_t(P2U(pckg.zw))); }





int btc(in uint vlc) { return bitCount(vlc); }
int lsb(in uint vlc) { return findLSB(vlc); }
int msb(in uint vlc) { return findMSB(vlc); }
int bitCount64(in uvec2 lh) { return btc(lh.x) + btc(lh.y); }



// bit measure utils
int lsb(in uint64_t vlc) {
#ifdef ENABLE_AMD_INSTRUCTION_SET
    return vlc == 0 ? -1 : findLSB(vlc); 
#else
    uvec2 pair = U2P(vlc); int lv = lsb(pair.x), hi = lsb(pair.y); return (lv >= 0) ? lv : (32 + hi);
#endif
}

int msb(in uint64_t vlc) {
#ifdef ENABLE_AMD_INSTRUCTION_SET
    return vlc == 0 ? -1 : findMSB(vlc); 
#else
    uvec2 pair = U2P(vlc); int lv = msb(pair.x), hi = msb(pair.y); return (hi >= 0) ? (32 + hi) : lv;
#endif
}

// bit insert and extract
int BFE_HW(in int base, in int offset, in int bits) { return bitfieldExtract(base, offset, bits); }
uint BFE_HW(in uint base, in int offset, in int bits) { return bitfieldExtract(base, offset, bits); }
int BFI_HW(in int base, in int inserts, in int offset, in int bits) { return bitfieldInsert(base, inserts, offset, bits); }
uint BFI_HW(in uint base, in uint inserts, in int offset, in int bits) { return bitfieldInsert(base, inserts, offset, bits); }

// int operations
int tiled(in int n, in int d) {return n <= 0 ? 0 : (n/d + sign(n%d));}



// float 16 or 32 types
#ifdef AMD_F16_BVH
#define FTYPE_ float16_t
#define FVEC3_ f16vec3
#define FVEC4_ f16vec4
#define FVEC2_ f16vec2
#define FMAT2X4_ f16mat2x4
#define FMAT4X4_ f16mat4x4
#define FMAT3X4_ f16mat3x4
#define FMAT3X2_ f16mat3x2
#define FMAT4X3_ f16mat4x3
    #ifdef USE_F32_BVH
    #define UNPACKF_(a)a
    #define PACKF_(a)a
    #else
    #define UNPACKF_(a)a
    #define PACKF_(a)FVEC4_(a)
    #endif
#else 
#define FTYPE_ float
#define FVEC2_ vec2
#define FVEC3_ vec3
#define FVEC4_ vec4
#define FMAT2X4_ mat2x4
#define FMAT4X4_ mat4x4
#define FMAT3X4_ mat3x4
#define FMAT3X2_ mat3x2
#define FMAT4X3_ mat4x3
    #ifdef USE_F32_BVH
    #define UNPACKF_(a)a
    #define PACKF_(a)a
    #else
    #define UNPACKF_ unpackHalf
    #define PACKF_ packHalf2
    #endif
#endif



// texture utils

vec4 composite(in vec4 src, in vec4 dst) {
    float oa = src.a + dst.a * (1.0f - src.a);
    return clamp(vec4((src.rgb * src.a + dst.rgb * dst.a * (1.0f - src.a)) / max(oa, 0.0001f), oa), vec4(0.0f), vec4(1.0f));
}

vec4 cubic(in float v) {
    vec4 s = vec4(1.0f,2.0f,3.0f,4.0f) - v;
    s *= s*s;
    float x = s.x;
    float y = fma(s.x,-4.f,s.y);
    float z = fma(s.y,-4.f,s.z) + 6.0f*s.x;
    float w = 6.0f-x-y-z;
    return vec4(x,y,z,w)*(1.0f/6.0f);
}

#define _FMOP fma(b,c,fma(a,-c,a)) // fma based mix/lerp

float fmix(in float a, in float b, in float c){ return _FMOP; }
vec2 fmix(in vec2 a, in vec2 b, in vec2 c){ return _FMOP; }
vec3 fmix(in vec3 a, in vec3 b, in vec3 c){ return _FMOP; }
vec4 fmix(in vec4 a, in vec4 b, in vec4 c){ return _FMOP; }
#ifdef ENABLE_AMD_INSTRUCTION_SET
float16_t fmix(in float16_t a, in float16_t b, in float16_t c){ return _FMOP; }
f16vec2 fmix(in f16vec2 a, in f16vec2 b, in f16vec2 c){ return _FMOP; }
f16vec3 fmix(in f16vec3 a, in f16vec3 b, in f16vec3 c){ return _FMOP; }
f16vec4 fmix(in f16vec4 a, in f16vec4 b, in f16vec4 c){ return _FMOP; }
#endif


vec4 textureBicubic(in sampler2D tx, in vec2 texCoords) {
    vec2 texSize = textureSize(tx, 0);
    vec2 invTexSize = 1.0f / texSize;

    texCoords *= texSize;
    vec2 fxy = fract(texCoords);
    texCoords = floor(texCoords);

    vec4 xcubic = cubic(fxy.x);
    vec4 ycubic = cubic(fxy.y);

    vec4 c = texCoords.xxyy + vec2(0.0f, 1.0f).xyxy;
    vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    vec4 offset = vec4(c + vec4(xcubic.yw, ycubic.yw) / s) * invTexSize.xxyy;

    vec4 sample0 = texture(tx, offset.xz);
    vec4 sample1 = texture(tx, offset.yz);
    vec4 sample2 = texture(tx, offset.xw);
    vec4 sample3 = texture(tx, offset.yw);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return fmix(fmix(sample3, sample2, sx.xxxx), fmix(sample1, sample0, sx.xxxx), sy.xxxx);
}




int msb(in uvec2 pair) {
    //int lv = msb(pair.x), hi = msb(pair.y);
    //return (hi >= 0) ? (32 + hi) : lv;
    return msb(P2U(pair));
}

int lsb(in uvec2 pair) {
    //int lv = lsb(pair.x), hi = lsb(pair.y);
    //return (lv >= 0) ? lv : (32 + hi);
    return lsb(P2U(pair));
}


// half float packing (64-bit)

vec4 unpackHalf(in uvec2 hilo) {
    return vec4(unpackHalf2x16(hilo.x), unpackHalf2x16(hilo.y));
}

vec4 unpackHalf(in uint64_t halfs) {
    return unpackHalf(U2P(halfs));
}

uvec2 packHalf2(in vec4 floats) {
    return uvec2(packHalf2x16(floats.xy), packHalf2x16(floats.zw));
}

// AMD based
#ifdef ENABLE_AMD_INSTRUCTION_SET
f16vec4 unpackHalf2(in uvec2 hilo) {
    return f16vec4(unpackFloat2x16(hilo.x), unpackFloat2x16(hilo.y));
}

f16vec4 unpackHalf2(in uint64_t halfs) {
    return unpackHalf2(U2P(halfs));
}

uvec2 packHalf2(in f16vec4 floats) {
    return uvec2(packFloat2x16(floats.xy), packFloat2x16(floats.zw));
}
#endif



// float packing (128-bit)

vec4 unpackFloat(in uvec4 hilo) {
    //return vec4(unpackHalf2x16(hilo.x), unpackHalf2x16(hilo.y));
    return vec4(uintBitsToFloat(hilo.x), uintBitsToFloat(hilo.y), uintBitsToFloat(hilo.z), uintBitsToFloat(hilo.w));
}

vec4 unpackFloat(in u64vec2 halfs) {
    return unpackFloat(U4P(halfs));
}

uvec4 packFloat2(in vec4 floats) {
    return uvec4(floatBitsToUint(floats.x), floatBitsToUint(floats.y), floatBitsToUint(floats.z), floatBitsToUint(floats.w));
}

// AMD based
#ifdef ENABLE_AMD_INSTRUCTION_SET
f16vec4 unpackFloat2(in uvec4 hilo) {
    return f16vec4(unpackFloat(hilo));
}

f16vec4 unpackFloat2(in u64vec2 halfs) {
    return unpackFloat2(U4P(halfs));
}

uvec4 packFloat2(in f16vec4 floats) {
    return packFloat2(vec4(floats));
}
#endif

// planned double packing (256bit and 128bit)






// reserved for future rasterizers
// just for save
vec3 barycentric2D(in vec3 p, in mat3x3 triangle) {
    mat3x3 plc = transpose(mat3x3(triangle[2] - triangle[0], triangle[1] - triangle[0], triangle[0] - p));
    vec3 u = cross(plc[0], plc[1]); // xy (2d) cross
    if (abs(u.z) < 1.f) return vec3(-1.f,1.f,1.f); 
    return vec3(u.z-(u.x+u.y), u.y, u.x)/u.z;
}





#if defined(ENABLE_AMD_INSTRUCTION_SET) || defined(ENABLE_NVIDIA_INSTRUCTION_SET)
#define INDEX16 uint16_t
#define M16(m, i) (m[i])
#else
#define INDEX16 uint
#define M16(m, i) (BFE_HW(m[(i)>>1], int(16*((i)&1)), 16))
#endif

#ifdef ENABLE_INT16_LOADING
#define INDICE_T INDEX16
#define PICK(m, i) M16(m, i)
#else
#define INDICE_T uint
#define PICK(m, i) m[i]
#endif


#ifdef ENABLE_AMD_INSTRUCTION_SET
#define BVEC3_ i16vec3
#define BVEC2_ i16vec2
#define BOOL_ int16_t
#define TRUE_ 1s
#define FALSE_ 0s
#else
#define BVEC3_ ivec3
#define BVEC2_ ivec2
#define BOOL_ int
#define TRUE_ 1
#define FALSE_ 0
#endif

#define TRUE2_ TRUE_.xx
#define FALSE2_ FALSE_.xx
BOOL_ any(in BVEC2_ b){return b.x|b.y;}
BOOL_ all(in BVEC2_ b){return b.x&b.y;}
BOOL_ not(in BOOL_ b){return TRUE_^b;}
BVEC2_ not(in BVEC2_ b){return TRUE2_^b;}

#ifdef ENABLE_AMD_INSTRUCTION_SET
bool SSC(in BOOL_ b){return (b&TRUE_)==TRUE_;}
bvec2 SSC(in BVEC2_ b){return equal(b&TRUE2_,TRUE2_);}
#else
bool SSC(in BOOL_ b){return bool(b);}
bvec2 SSC(in BVEC2_ b){return bvec2(b);}
#endif
bool SSC(in bool b){return b;}
bvec2 SSC(in bvec2 b){return b;}

#define IF(b)if(SSC(b))



// roundly comparsion functions
BOOL_ lessEqualF(in float a, in float b) { return BOOL_((b-a) > -PZERO); }
BOOL_ lessF(in float a, in float b) { return BOOL_((b-a) >= PZERO); }
BOOL_ greaterEqualF(in float a, in float b) { return BOOL_((a-b) > -PZERO); }
BOOL_ greaterF(in float a, in float b) { return BOOL_((a-b) >= PZERO); }
BOOL_ equalF(in float a, in float b) { return BOOL_(abs(a-b) < PZERO); }

/*
int mix(in int a, in int b, in BOOL_ c) { return (a*not(c)) | (b*c); }
uint mix(in uint a, in uint b, in BOOL_ c) { return (a*not(c)) | (b*c); }
float mix(in float a, in float b, in BOOL_ c) { return (a*not(c)) + (b*c); }
ivec2 mix(in ivec2 a, in ivec2 b, in BVEC2_ c) { return (a*not(c)) | (b*c); }
uvec2 mix(in uvec2 a, in uvec2 b, in BVEC2_ c) { return (a*not(c)) | (b*c); }
vec2 mix(in vec2 a, in vec2 b, in BVEC2_ c) { return (a*not(c)) + (b*c); }
#ifdef ENABLE_AMD_INSTRUCTION_SET
int16_t mix(in int16_t a, in int16_t b, in BOOL_ c) { return (a*not(c)) | (b*c); }
uint16_t mix(in uint16_t a, in uint16_t b, in BOOL_ c) { return (a*not(c)) | (b*c); }
float16_t mix(in float16_t a, in float16_t b, in BOOL_ c) { return (a*not(c)) + (b*c); }
i16vec2 mix(in i16vec2 a, in i16vec2 b, in BVEC2_ c) { return (a*not(c)) | (b*c); }
u16vec2 mix(in u16vec2 a, in u16vec2 b, in BVEC2_ c) { return (a*not(c)) | (b*c); }
f16vec2 mix(in f16vec2 a, in f16vec2 b, in BVEC2_ c) { return (a*not(c)) + (b*c); }
#endif
*/

int mix(in int a, in int b, in BOOL_ c) { return mix(a,b,SSC(c)); }
uint mix(in uint a, in uint b, in BOOL_ c) { return mix(a,b,SSC(c)); }
float mix(in float a, in float b, in BOOL_ c) { return mix(a,b,SSC(c)); }
ivec2 mix(in ivec2 a, in ivec2 b, in BVEC2_ c) { return mix(a,b,SSC(c)); }
uvec2 mix(in uvec2 a, in uvec2 b, in BVEC2_ c) { return mix(a,b,SSC(c)); }
vec2 mix(in vec2 a, in vec2 b, in BVEC2_ c) { return mix(a,b,SSC(c)); }
#ifdef ENABLE_AMD_INSTRUCTION_SET
int16_t mix(in int16_t a, in int16_t b, in BOOL_ c) { return mix(a,b,SSC(c)); }
uint16_t mix(in uint16_t a, in uint16_t b, in BOOL_ c) { return mix(a,b,SSC(c)); }
float16_t mix(in float16_t a, in float16_t b, in BOOL_ c) { return mix(a,b,SSC(c)); }
i16vec2 mix(in i16vec2 a, in i16vec2 b, in BVEC2_ c) { return mix(a,b,SSC(c)); }
u16vec2 mix(in u16vec2 a, in u16vec2 b, in BVEC2_ c) { return mix(a,b,SSC(c)); }
f16vec2 mix(in f16vec2 a, in f16vec2 b, in BVEC2_ c) { return mix(a,b,SSC(c)); }
#endif


// single float 32-bit box intersection
// some ideas been used from http://www.cs.utah.edu/~thiago/papers/robustBVH-v2.pdf
// compatible with AMD radeon min3 and max3
float intersectCubeSingle(in vec3 norig, in vec3 dr, in vec4 cubeMin, in vec4 cubeMax, inout float near, inout float far) {
    vec3 tMin = fma(cubeMin.xyz, dr, norig);
    vec3 tMax = fma(cubeMax.xyz, dr, norig);
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
#if (defined(ENABLE_AMD_INSTRUCTION_SET))
    float tNear = max3(t1.x, t1.y, t1.z);
    float tFar  = min3(t2.x, t2.y, t2.z);
#else
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar  = min(min(t2.x, t2.y), t2.z);
#endif
    float isCube = float(greaterEqualF(tFar, tNear) & greaterEqualF(tFar, 0.0f));
    const float inf = INFINITY;
    near = fmix(inf, min(tNear, tFar), isCube);
    far  = fmix(inf, max(tNear, tFar), isCube);
    return fmix(near, far, float((near + PZERO) <= 0.0f));
}


// half float 16/32-bit box intersection (claymore dual style)
// some ideas been used from http://www.cs.utah.edu/~thiago/papers/robustBVH-v2.pdf
// made by DevIL research group
// also, optimized for RPM (Rapid Packed Math) https://radeon.com/_downloads/vega-whitepaper-11.6.17.pdf
// compatible with NVidia GPU too
BVEC2_ intersectCubeDual(in FVEC3_ origin, in FVEC3_ dr, in BVEC3_ sgn, in FMAT3X4_ cubeMinMax2, inout vec2 near, inout vec2 far) {
    FMAT3X4_ tMinMax = FMAT3X4_(
        fma(SSC(sgn.x) ? cubeMinMax2[0].xyzw : cubeMinMax2[0].zwxy, dr.xxxx, origin.xxxx),
        fma(SSC(sgn.y) ? cubeMinMax2[1].xyzw : cubeMinMax2[1].zwxy, dr.yyyy, origin.yyyy),
        fma(SSC(sgn.z) ? cubeMinMax2[2].xyzw : cubeMinMax2[2].zwxy, dr.zzzz, origin.zzzz)
    );

    FVEC2_ 
#if (defined(ENABLE_AMD_INSTRUCTION_SET))
    tFar  = min3(tMinMax[0].zw, tMinMax[1].zw, tMinMax[2].zw),
    tNear = max3(tMinMax[0].xy, tMinMax[1].xy, tMinMax[2].xy);
#else
    tFar  = min(min(tMinMax[0].zw, tMinMax[1].zw), tMinMax[2].zw),
    tNear = max(max(tMinMax[0].xy, tMinMax[1].xy), tMinMax[2].xy);
#endif

    // precise error correct
    tFar += FVEC2_(PZERO.xx);

    // validate hit
    BVEC2_ isCube = BVEC2_(greaterThan(tFar, tNear)) & BVEC2_(greaterThan(tFar, FVEC2_(0.0f))) & BVEC2_(lessThanEqual(tNear, FVEC2_(INFINITY-PZERO)));

    // resolve hit
    const vec2 inf = vec2(INFINITY);
    near = mix(inf, vec2(tNear), isCube), far = mix(inf, vec2(tFar), isCube);
    //return mix(near, far, lessThanEqual(near, vec2(-PZERO)));
    return isCube;
}



uint64_t bitfieldReverse64(in uint64_t a){uvec2 p = U2P(a);p=bitfieldReverse(p);return P2U(p.yx);}
int nlz(in uint64_t x) { int vl = lsb(bitfieldReverse64(x)); return vl >= 0 ? vl : 64; }
int nlz(in uint x) { int vl = lsb(bitfieldReverse64(x)); return vl >= 0 ? vl : 64; }



const float HDR_GAMMA = 2.2f;

vec4 fromLinear(in vec4 linearRGB) {
    return pow(linearRGB, vec4(1.f/HDR_GAMMA));
    //return mix(vec4(1.055)*pow(linearRGB, vec4(1.0/2.4)) - vec4(0.055), linearRGB * vec4(12.92), lessThan(linearRGB, vec4(0.0031308)));
}

vec4 toLinear(in vec4 sRGB) {
    return pow(sRGB, vec4(HDR_GAMMA));
    //return mix(pow((sRGB + vec4(0.055))/vec4(1.055), vec4(2.4)), sRGB/vec4(12.92), lessThan(sRGB, vec4(0.04045)));
}

vec3 fromLinear(in vec3 linearRGB) {
    return pow(linearRGB, vec3(1.f/HDR_GAMMA));
    //return mix(vec3(1.055)*pow(linearRGB, vec3(1.0/2.4)) - vec3(0.055), linearRGB * vec3(12.92), lessThan(linearRGB, vec3(0.0031308)));
}

vec3 toLinear(in vec3 sRGB) {
    return pow(sRGB, vec3(HDR_GAMMA));
    //return mix(pow((sRGB + vec3(0.055))/vec3(1.055), vec3(2.4)), sRGB/vec3(12.92), lessThan(sRGB, vec3(0.04045)));
}


// also, create planar projection 
mat3 make_stream_projection(in vec3 normal){
    mat3 r;
    r[0].x = 1.f - normal.x * normal.x;
    r[0].y = -normal.x * normal.y;
    r[0].z = -normal.x * normal.z;
    r[1].x = -normal.x * normal.y;
    r[1].y = 1.f - normal.y * normal.y;
    r[1].z = -normal.y * normal.z;
    r[2].x = -normal.x * normal.z;
    r[2].y = -normal.y * normal.z;
    r[2].z = 1.f - normal.z * normal.z;
    return r;
}



#endif