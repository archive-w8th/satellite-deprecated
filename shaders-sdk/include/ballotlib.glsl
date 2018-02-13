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

#ifndef ITYPE
#define ITYPE int
#endif

// customizable
#ifndef CUSTOMIZE_IDC
  #define LT_IDX ITYPE(gl_LocalInvocationIndex)
  #define LANE_IDX (LT_IDX%WARP_SIZE_RT)
  #define LC_IDX   (LT_IDX/WARP_SIZE_RT)
#endif 

#define UVEC_BALLOT_WARP uvec2




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


// filtering wrong warps
uvec2 filterBallot() {
    return lookat_lt[WARP_SIZE_RT];
}

uvec2 genLtMask() {
    return lookat_lt[LANE_IDX] & filterBallot();
}

uvec2 genGeMask() {
    return lookat_lt[LANE_IDX] ^ filterBallot();
}

vec4 readLane(in vec4 val, in int lane) {
    return readInvocationARB(val, lane);
}

vec3 readLane(in vec3 val, in int lane) {
    return readInvocationARB(val, lane);
}

mat3 readLane(in mat3 val, in int lane) {
    val[0] = readLane(val[0], lane);
    val[1] = readLane(val[1], lane);
    val[2] = readLane(val[2], lane);
    return val;
}


float readLane(in float val, in int lane) {
    return readInvocationARB(val, lane);
}

uint readLane(in uint val, in int lane) {
    return readInvocationARB(val, lane);
}

int readLane(in int val, in int lane) {
    return readInvocationARB(val, lane);
}


// hack for cross-lane reading of 16-bit data (whaaat?)
#ifdef ENABLE_AMD_INSTRUCTION_SET
float16_t readLane(in float16_t val, in int lane){
    return unpackFloat2x16(readInvocationARB(packFloat2x16(f16vec2(val, 0.hf)), lane)).x;
}

f16vec2 readLane(in f16vec2 val, in int lane){
    return unpackFloat2x16(readInvocationARB(packFloat2x16(val), lane));
}
#endif



#ifdef ENABLE_AMD_INT16
int16_t readLane(in int16_t val, in int lane){
    return unpackInt2x16(readInvocationARB(packInt2x16(i16vec2(val, 0s)), lane)).x;
}

uint16_t readLane(in uint16_t val, in int lane){
    return unpackUint2x16(readInvocationARB(packUint2x16(u16vec2(val, 0us)), lane)).x;
}

i16vec2 readLane(in i16vec2 val, in int lane){
    return unpackInt2x16(readInvocationARB(packInt2x16(val), lane));
}

u16vec2 readLane(in u16vec2 val, in int lane){
    return unpackUint2x16(readInvocationARB(packUint2x16(val), lane));
}
#endif




bool readLane(in bool val, in int lane) {
    return bool(readInvocationARB(int(val), lane));
}






uvec2 ballotHW(in BOOL_ val) {
    uint64_t plc = 0xFFFFFFFFFFFFFFFFul;
    //plc = ballotARB(bool(val));
    plc = ballotARB(true); // fix support for Intel, etc.
    return U2P(plc) & filterBallot();
}

int countInvocs(in BOOL_ val){
#ifdef ENABLE_AMD_INSTRUCTION_SET
    return addInvocationsAMD(int(val)); // WHY 16-bit not supported?!
#else
    return bitCount64(ballotHW(val));
#endif
}



float readFLane(in float val, in int lane) { return readFirstInvocationARB(val); }
uint readFLane(in uint val, in int lane) { return uint(readFirstInvocationARB(int(val)+1)-1); }
int readFLane(in int val, in int lane) { return readFirstInvocationARB(val+1)-1; }



float readFLane(in float val) { return readFLane(val, 0); }
uint readFLane(in uint val) { return readFLane(val, 0); }
int readFLane(in int val) { return readFLane(val, 0); }



  int firstActive() { return readFLane(int(LANE_IDX)); }
  int firstActive(in UVEC_BALLOT_WARP wps) { return lsb(wps); }



int mcount64(in UVEC_BALLOT_WARP bits){
#ifdef ENABLE_AMD_INSTRUCTION_SET
    return int(mbcntAMD(P2U(bits))); // AMuDe
#else
    return bitCount64(bits & genLtMask()); // some other
#endif
}


#define initAtomicIncFunction(mem, fname, T)\
T fname(in BOOL_ _value) {\
    UVEC_BALLOT_WARP bits = ballotHW(TRUE_);\
    const int activeLane = firstActive(bits);\
    T sumInOrder = T(bitCount64(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) gadd = atomicAdd(mem, sumInOrder);\
    return readLane(gadd, activeLane) + idxInOrder;\
}

#define initAtomicDecFunction(mem, fname, T)\
T fname(in BOOL_ _value) {\
    UVEC_BALLOT_WARP bits = ballotHW(TRUE_);\
    const int activeLane = firstActive(bits);\
    T sumInOrder = T(bitCount64(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) {\
    atomicMax(mem, 0); gadd = atomicAdd(mem, -sumInOrder); atomicMax(mem, 0); }\
    return readLane(gadd, activeLane) - idxInOrder;\
}

// with multiplier support
#define initAtomicIncByFunction(mem, fname, T)\
T fname(in BOOL_ _value, const int by) {\
    UVEC_BALLOT_WARP bits = ballotHW(TRUE_);\
    const int activeLane = firstActive(bits);\
    T sumInOrder = T(bitCount64(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) gadd = atomicAdd(mem, sumInOrder * by);\
    return readLane(gadd, activeLane) + idxInOrder * by;\
}

#define initAtomicIncFunctionTarget(mem, fname, T)\
T fname(in uint WHERE, in BOOL_ _value) {\
    UVEC_BALLOT_WARP bits = ballotHW(TRUE_);\
    const int activeLane = firstActive(bits);\
    T sumInOrder = T(bitCount64(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) gadd = atomicAdd(mem, sumInOrder);\
    return readLane(gadd, activeLane) + idxInOrder;\
}

#define initNonAtomicIncFunction(mem, fname, T)\
T fname(in BOOL_ _value) {\
    UVEC_BALLOT_WARP bits = ballotHW(TRUE_);\
    const int activeLane = firstActive(bits);\
    T sumInOrder = T(bitCount64(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) gadd = add(mem, sumInOrder);\
    return readLane(gadd, activeLane) + idxInOrder;\
}

#define initNonAtomicIncFunctionTarget(mem, fname, T)\
T fname(in uint WHERE, in BOOL_ _value) {\
    UVEC_BALLOT_WARP bits = ballotHW(TRUE_);\
    const int activeLane = firstActive(bits);\
    T sumInOrder = T(bitCount64(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) gadd = add(mem, sumInOrder);\
    return readLane(gadd, activeLane) + idxInOrder;\
}



bool allInvoc(in bool bc){
    //return countInvocs(bc) >= countInvocs(true);
    return allInvocations(bc);
}

bool anyInvoc(in bool bc){
    //return countInvocs(bc) > 0;
    return anyInvocation(bc);
}



bool allInvoc(in BOOL_ bc){
    //return countInvocs(bc) >= countInvocs(true);
    return allInvocations(SSC(bc));
}

bool anyInvoc(in BOOL_ bc){
    //return countInvocs(bc) > 0;
    return anyInvocation(SSC(bc));
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

