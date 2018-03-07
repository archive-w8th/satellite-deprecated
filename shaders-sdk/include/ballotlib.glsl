#ifndef _BALLOTLIB_H
#define _BALLOTLIB_H

#include "../include/mathlib.glsl"

// ordered increment
#if (!defined(FRAGMENT_SHADER) && !defined(ORDERING_NOT_REQUIRED))

#ifndef WARP_SIZE
#ifdef ENABLE_AMD_INSTRUCTION_SET
#define WARP_SIZE 64
#else
#define WARP_SIZE 32
#endif
#endif


#ifndef ITYPE
#define ITYPE uint
#endif

//#define WARP_SIZE_RT ITYPE(gl_SubGroupSizeARB)
  #define WARP_SIZE_RT ITYPE(WARP_SIZE)



// planned dynamic support
#if (WARP_SIZE == 64)
#define WARP_FILTER_SHIFT 6
#define WARP_FILTER_MASK 63
#elif (WARP_SIZE == 32)
#define WARP_FILTER_SHIFT 5
#define WARP_FILTER_MASK 31
#endif

// customizable
#ifndef CUSTOMIZE_IDC
  #define LT_IDX ITYPE(gl_LocalInvocationIndex)
  #define LANE_IDX (LT_IDX%WARP_SIZE_RT)
  #define LC_IDX   (LT_IDX/WARP_SIZE_RT)
#endif 

//#define UVEC_BALLOT_WARP uvec2
#define UVEC_BALLOT_WARP uint64_t


// intel hd graphics may not support const arrays
#ifndef INTEL_PLATFORM

const uvec2 lookat_lt[65] = {
    uvec2(0x0u, 0x0u),

    uvec2(0x1u, 0x0u),
    uvec2(0x3u, 0x0u),
    uvec2(0x7u, 0x0u),
    uvec2(0xFu, 0x0u),

    uvec2(0x1Fu, 0x0u),
    uvec2(0x3Fu, 0x0u),
    uvec2(0x7Fu, 0x0u),
    uvec2(0xFFu, 0x0u),

    uvec2(0x1FFu, 0x0u),
    uvec2(0x3FFu, 0x0u),
    uvec2(0x7FFu, 0x0u),
    uvec2(0xFFFu, 0x0u),

    uvec2(0x1FFFu, 0x0u),
    uvec2(0x3FFFu, 0x0u),
    uvec2(0x7FFFu, 0x0u),
    uvec2(0xFFFFu, 0x0u),

    uvec2(0x1FFFFu, 0x0u),
    uvec2(0x3FFFFu, 0x0u),
    uvec2(0x7FFFFu, 0x0u),
    uvec2(0xFFFFFu, 0x0u),

    uvec2(0x1FFFFFu, 0x0u),
    uvec2(0x3FFFFFu, 0x0u),
    uvec2(0x7FFFFFu, 0x0u),
    uvec2(0xFFFFFFu, 0x0u),

    uvec2(0x1FFFFFFu, 0x0u),
    uvec2(0x3FFFFFFu, 0x0u),
    uvec2(0x7FFFFFFu, 0x0u),
    uvec2(0xFFFFFFFu, 0x0u),

    uvec2(0x1FFFFFFFu, 0x0u),
    uvec2(0x3FFFFFFFu, 0x0u),
    uvec2(0x7FFFFFFFu, 0x0u),
    uvec2(0xFFFFFFFFu, 0x0u),

    uvec2(0xFFFFFFFFu, 0x1u),
    uvec2(0xFFFFFFFFu, 0x3u),
    uvec2(0xFFFFFFFFu, 0x7u),
    uvec2(0xFFFFFFFFu, 0xFu),

    uvec2(0xFFFFFFFFu, 0x1Fu),
    uvec2(0xFFFFFFFFu, 0x3Fu),
    uvec2(0xFFFFFFFFu, 0x7Fu),
    uvec2(0xFFFFFFFFu, 0xFFu),

    uvec2(0xFFFFFFFFu, 0x1FFu),
    uvec2(0xFFFFFFFFu, 0x3FFu),
    uvec2(0xFFFFFFFFu, 0x7FFu),
    uvec2(0xFFFFFFFFu, 0xFFFu),

    uvec2(0xFFFFFFFFu, 0x1FFFu),
    uvec2(0xFFFFFFFFu, 0x3FFFu),
    uvec2(0xFFFFFFFFu, 0x7FFFu),
    uvec2(0xFFFFFFFFu, 0xFFFFu),

    uvec2(0xFFFFFFFFu, 0x1FFFFu),
    uvec2(0xFFFFFFFFu, 0x3FFFFu),
    uvec2(0xFFFFFFFFu, 0x7FFFFu),
    uvec2(0xFFFFFFFFu, 0xFFFFFu),

    uvec2(0xFFFFFFFFu, 0x1FFFFFu),
    uvec2(0xFFFFFFFFu, 0x3FFFFFu),
    uvec2(0xFFFFFFFFu, 0x7FFFFFu),
    uvec2(0xFFFFFFFFu, 0xFFFFFFu),

    uvec2(0xFFFFFFFFu, 0x1FFFFFFu),
    uvec2(0xFFFFFFFFu, 0x3FFFFFFu),
    uvec2(0xFFFFFFFFu, 0x7FFFFFFu),
    uvec2(0xFFFFFFFFu, 0xFFFFFFFu),

    uvec2(0xFFFFFFFFu, 0x1FFFFFFFu),
    uvec2(0xFFFFFFFFu, 0x3FFFFFFFu),
    uvec2(0xFFFFFFFFu, 0x7FFFFFFFu),
    uvec2(0xFFFFFFFFu, 0xFFFFFFFFu)
};

uint64_t lookatLt(const uint lid) { return P2U(lookat_lt[lid]); }

#else 

uint64_t lookatLt(const uint lid) { return (1ul << uint64_t(lid)) - 1ul; }

#endif



// filtering wrong warps
uint64_t filterBallot() {
    return lookatLt(WARP_SIZE_RT);
}

uint64_t genLtMask() {
    return lookatLt(LANE_IDX) & filterBallot();
}

uint64_t genGeMask() {
    return lookatLt(LANE_IDX) ^ filterBallot();
}

vec4 readLane(in vec4 val, in uint lane) {
    return readInvocationARB(val, lane);
}

vec3 readLane(in vec3 val, in uint lane) {
    return readInvocationARB(val, lane);
}

mat3 readLane(in mat3 val, in uint lane) {
    val[0] = readLane(val[0], lane);
    val[1] = readLane(val[1], lane);
    val[2] = readLane(val[2], lane);
    return val;
}


float readLane(in float val, in uint lane) {
    return readInvocationARB(val, lane);
}

uint readLane(in uint val, in uint lane) {
    return readInvocationARB(val, lane);
}

int readLane(in int val, in uint lane) {
    return readInvocationARB(val, lane);
}


// hack for cross-lane reading of 16-bit data (whaaat?)
#ifdef ENABLE_AMD_INSTRUCTION_SET
float16_t readLane(in float16_t val, in uint lane){
    return unpackFloat2x16(readInvocationARB(packFloat2x16(f16vec2(val, 0.hf)), lane)).x;
}

f16vec2 readLane(in f16vec2 val, in uint lane){
    return unpackFloat2x16(readInvocationARB(packFloat2x16(val), lane));
}
#endif



#ifdef ENABLE_AMD_INT16
int16_t readLane(in int16_t val, in uint lane){
    return unpackInt2x16(readInvocationARB(packInt2x16(i16vec2(val, 0s)), lane)).x;
}

uint16_t readLane(in uint16_t val, in uint lane){
    return unpackUint2x16(readInvocationARB(packUint2x16(u16vec2(val, 0us)), lane)).x;
}

i16vec2 readLane(in i16vec2 val, in uint lane){
    return unpackInt2x16(readInvocationARB(packInt2x16(val), lane));
}

u16vec2 readLane(in u16vec2 val, in uint lane){
    return unpackUint2x16(readInvocationARB(packUint2x16(val), lane));
}
#endif




bool readLane(in bool val, in uint lane) {
    return bool(readInvocationARB(int(val), lane));
}


uint64_t ballotHW() {
    return ballotARB(true) & filterBallot();
}


float readFLane(in float val, in uint lane) { return readFirstInvocationARB(val); }
uint readFLane(in uint val, in uint lane) { return readFirstInvocationARB(val+1u)-1u; }
int readFLane(in int val, in uint lane) { return int(readFirstInvocationARB(uint(val+1)))-1; }


float readFLane(in float val) { return readFLane(val, 0u); }
uint readFLane(in uint val) { return readFLane(val, 0u); }
int readFLane(in int val) { return readFLane(val, 0u); }


  uint firstActive() { return readFLane(LANE_IDX); }
  uint firstActive(in UVEC_BALLOT_WARP wps) { return uint(lsb(wps)); }
  //uint firstActive(in UVEC_BALLOT_WARP wps) { return readFLane(LANE_IDX); }


uint mcount64(in UVEC_BALLOT_WARP bits){
#ifdef ENABLE_AMD_INSTRUCTION_SET
    return mbcntAMD(bits); // AMuDe
#else
    return bitcnt(bits & genLtMask()); // some other
#endif
}


#define initAtomicIncFunction(mem, fname, T)\
T fname() {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint activeLane = firstActive(bits);\
    T sumInOrder = T(bitcnt(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) {gadd = atomicAdd(mem, sumInOrder);}\
    return readFLane(gadd, activeLane) + idxInOrder;\
}

#define initAtomicDecFunction(mem, fname, T)\
T fname() {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint activeLane = firstActive(bits);\
    T sumInOrder = T(bitcnt(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) {\
    atomicMax(mem, 0); gadd = atomicAdd(mem, -sumInOrder); atomicMax(mem, 0); }\
    return readFLane(gadd, activeLane) - idxInOrder;\
}

// with multiplier support
#define initAtomicIncByFunction(mem, fname, T)\
T fname(const T by) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint activeLane = firstActive(bits);\
    T sumInOrder = T(bitcnt(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) {gadd = atomicAdd(mem, sumInOrder * by);}\
    return readFLane(gadd, activeLane) + idxInOrder * by;\
}

#define initAtomicIncFunctionTarget(mem, fname, T)\
T fname(in uint WHERE) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint activeLane = firstActive(bits);\
    T sumInOrder = T(bitcnt(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) {gadd = atomicAdd(mem, sumInOrder);}\
    return readFLane(gadd, activeLane) + idxInOrder;\
}

#define initNonAtomicIncFunction(mem, fname, T)\
T fname() {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint activeLane = firstActive(bits);\
    T sumInOrder = T(bitcnt(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) gadd = add(mem, sumInOrder);\
    return readFLane(gadd, activeLane) + idxInOrder;\
}

#define initNonAtomicIncFunctionTarget(mem, fname, T)\
T fname(const uint WHERE) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint activeLane = firstActive(bits);\
    T sumInOrder = T(bitcnt(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) {gadd = add(mem, sumInOrder);}\
    return readFLane(gadd, activeLane) + idxInOrder;\
}

// for bitfield based layouts (not for global usages!)
#define initNonAtomicIncFunctionTargetFunc(funcinout, fname, T)\
T fname(const uint WHERE) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint activeLane = firstActive(bits);\
    T sumInOrder = T(bitcnt(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) { gadd = funcinout(WHERE); funcinout(WHERE, gadd + sumInOrder); }\
    return readFLane(gadd, activeLane) + idxInOrder;\
}





bool allInvoc(in BOOL_ bc){
    return allInvocations(SSC(bc));
}

bool anyInvoc(in BOOL_ bc){
    return anyInvocation(SSC(bc));
}



bool allInvoc(in bool bc){
    return allInvoc(BOOL_(bc));
}

bool anyInvoc(in bool bc){
    return anyInvoc(BOOL_(bc));
}



#define IFALL(b)if(allInvoc(b))
#define IFANY(b)if(anyInvoc(b))

#else

#define initAtomicIncFunction(mem, fname, T)\
T fname(in bool value) { \
    return atomicAdd(mem, T(value)); \
}

#define initAtomicDecFunction(mem, fname, T)\
T fname(in bool value) { \
    return atomicAdd(mem, -T(value)); \
}

#endif

#endif

