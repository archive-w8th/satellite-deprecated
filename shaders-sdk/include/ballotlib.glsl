#ifndef _BALLOTLIB_H
#define _BALLOTLIB_H

#include "../include/mathlib.glsl"

#ifndef OUR_INVOC_TERM
#define LANE_IDX gl_SubgroupInvocationID.x
#define WARP_SIZE_RT gl_SubgroupSize.x
#define LT_IDX gl_LocalInvocationIndex.x
#define LC_IDX (LT_IDX / WARP_SIZE_RT)
#endif


#ifndef WARP_SIZE
#ifdef ENABLE_AMD_INSTRUCTION_SET
#define WARP_SIZE 64
#else
#define WARP_SIZE 32
#endif
#endif






#define UVEC_BALLOT_WARP uvec4


vec4 readLane(in vec4 val, in uint lane) { return subgroupBroadcast(val, lane); }
vec3 readLane(in vec3 val, in uint lane) { return subgroupBroadcast(val, lane); }
mat3 readLane(in mat3 val, in uint lane) { val[0] = readLane(val[0], lane), val[1] = readLane(val[1], lane), val[2] = readLane(val[2], lane); return val; }

float readLane(in float val, in uint lane) { return subgroupBroadcast(val, lane); }
uint readLane(in uint val, in uint lane) { return subgroupBroadcast(val, lane); }
int readLane(in int val, in uint lane) { return subgroupBroadcast(val, lane); }



// hack for cross-lane reading of 16-bit data (whaaat?)
#ifdef ENABLE_AMD_INSTRUCTION_SET
float16_t readLane(in float16_t val, in uint lane) { return unpackFloat2x16(subgroupBroadcast(packFloat2x16(f16vec2(val, 0.hf)), lane)).x; }
f16vec2 readLane(in f16vec2 val, in uint lane) { return unpackFloat2x16(subgroupBroadcast(packFloat2x16(val), lane)); }
#endif



#ifdef ENABLE_AMD_INT16
int16_t readLane(in int16_t val, in uint lane) { return unpackInt2x16(subgroupBroadcast(packInt2x16(i16vec2(val, 0s)), lane)).x; }
uint16_t readLane(in uint16_t val, in uint lane) { return unpackUint2x16(subgroupBroadcast(packUint2x16(u16vec2(val, 0us)), lane)).x; }
i16vec2 readLane(in i16vec2 val, in uint lane) { return unpackInt2x16(subgroupBroadcast(packInt2x16(val), lane)); }
u16vec2 readLane(in u16vec2 val, in uint lane) { return unpackUint2x16(subgroupBroadcast(packUint2x16(val), lane)); }
#endif


bool readLane(in bool val, in uint lane) { return bool(subgroupBroadcast(int(val), lane)); }




UVEC_BALLOT_WARP ballotHW() { return subgroupBallot(true); }

float readFLane(in float val) { return subgroupBroadcastFirst(val); }
uint readFLane(in uint val) { return subgroupBroadcastFirst(val); }
int readFLane(in int val) { return subgroupBroadcastFirst(val); }


// statically multiplied
#define initAtomicSubgroupIncFunction(mem, fname, by, T)\
T fname() {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(subgroupBallotBitCount(bits));\
    T idxInOrder = T(subgroupBallotExclusiveBitCount(bits));\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = atomicAdd(mem, sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}


#define initAtomicSubgroupIncFunctionDyn(mem, fname, T)\
T fname(const T by) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(subgroupBallotBitCount(bits));\
    T idxInOrder = T(subgroupBallotExclusiveBitCount(bits));\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = atomicAdd(mem, sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}


// statically multiplied
#define initAtomicSubgroupIncFunctionTarget(mem, fname, by, T)\
T fname(const uint WHERE) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(subgroupBallotBitCount(bits));\
    T idxInOrder = T(subgroupBallotExclusiveBitCount(bits));\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = atomicAdd(mem, sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}



#define initAtomicSubgroupIncFunctionByTarget(mem, fname, T)\
T fname(const uint WHERE, const T by) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(subgroupBallotBitCount(bits));\
    T idxInOrder = T(subgroupBallotExclusiveBitCount(bits));\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = atomicAdd(mem, sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}



// statically multiplied
#define initSubgroupIncFunctionTarget(mem, fname, by, T)\
T fname(const uint WHERE) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(subgroupBallotBitCount(bits));\
    T idxInOrder = T(subgroupBallotExclusiveBitCount(bits));\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = add(mem, sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}


#define initSubgroupIncFunctionByTarget(mem, fname, T)\
T fname(const uint WHERE, const T by) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(subgroupBallotBitCount(bits));\
    T idxInOrder = T(subgroupBallotExclusiveBitCount(bits));\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = add(mem, sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}



#define initSubgroupIncFunctionFunc(memfunc, fname, by, T)\
T fname(const uint WHERE) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(subgroupBallotBitCount(bits));\
    T idxInOrder = T(subgroupBallotExclusiveBitCount(bits));\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = memfunc(T(WHERE)); memfunc(T(WHERE), gadd+sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}




bool allInvoc(in bool bc){ return subgroupAll(bc); }
bool anyInvoc(in bool bc){ return subgroupAny(bc); }
bool allInvoc(in BOOL_ bc){ return allInvoc(SSC(bc)); }
bool anyInvoc(in BOOL_ bc){ return anyInvoc(SSC(bc)); }
#define IFALL(b)if(allInvoc(b))
#define IFANY(b)if(anyInvoc(b))

#endif

