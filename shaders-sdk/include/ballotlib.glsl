#ifndef _BALLOTLIB_H
#define _BALLOTLIB_H

#include "../include/mathlib.glsl"

// for constant maners
#ifndef WARP_SIZE
#ifdef ENABLE_AMD_INSTRUCTION_SET
#define WARP_SIZE 64
#else
#define WARP_SIZE 32
#endif
#endif

#define WARP_SIZE_RT gl_SubgroupSize.x

#ifndef OUR_INVOC_TERM
#define LT_IDX gl_LocalInvocationIndex.x
#define LC_IDX gl_SubgroupID.x
#define LANE_IDX gl_SubgroupInvocationID.x
#endif

#define UVEC_BALLOT_WARP uvec4
#define RL_ subgroupBroadcast
#define RLF_ subgroupBroadcastFirst


vec4 readLane(in vec4 val, in uint lane) { return RL_(val, lane); }
vec3 readLane(in vec3 val, in uint lane) { return RL_(val, lane); }
mat3 readLane(in mat3 val, in uint lane) { val[0] = readLane(val[0], lane), val[1] = readLane(val[1], lane), val[2] = readLane(val[2], lane); return val; }
float readLane(in float val, in uint lane) { return RL_(val, lane); }
uint readLane(in uint val, in uint lane) { return RL_(val, lane); }
int readLane(in int val, in uint lane) { return RL_(val, lane); }
bool readLane(in bool val, in uint lane) { return bool(RL_(int(val), lane)); }

// hack for cross-lane reading of 16-bit data (whaaat?)
#ifdef ENABLE_AMD_INSTRUCTION_SET
float16_t readLane(in float16_t val, in uint lane) { return unpackFloat2x16(RL_(packFloat2x16(f16vec2(val, 0.hf)), lane)).x; }
f16vec2 readLane(in f16vec2 val, in uint lane) { return unpackFloat2x16(RL_(packFloat2x16(val), lane)); }
#endif

float readFLane(in float val) { return RLF_(val); }
uint readFLane(in uint val) { return RLF_(val); }
int readFLane(in int val) { return RLF_(val); }

UVEC_BALLOT_WARP ballotHW() { return subgroupBallot(true); }
bool electedInvoc() { return subgroupElect(); }


// statically multiplied
#define initAtomicSubgroupIncFunction(mem, fname, by, T)\
T fname() {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint sumInOrder = subgroupBallotBitCount(bits);\
    uint idxInOrder = subgroupBallotExclusiveBitCount(bits);\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = atomicAdd(mem, T(sumInOrder) * T(by));}\
    return readFLane(gadd) + T(idxInOrder) * T(by);\
}


#define initAtomicSubgroupIncFunctionDyn(mem, fname, T)\
T fname(const T by) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint sumInOrder = subgroupBallotBitCount(bits);\
    uint idxInOrder = subgroupBallotExclusiveBitCount(bits);\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = atomicAdd(mem, T(sumInOrder) * T(by));}\
    return readFLane(gadd) + T(idxInOrder) * T(by);\
}


// statically multiplied
#define initAtomicSubgroupIncFunctionTarget(mem, fname, by, T)\
T fname(const uint WHERE) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint sumInOrder = subgroupBallotBitCount(bits);\
    uint idxInOrder = subgroupBallotExclusiveBitCount(bits);\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = atomicAdd(mem, T(sumInOrder) * T(by));}\
    return readFLane(gadd) + T(idxInOrder) * T(by);\
}

#define initAtomicSubgroupIncFunctionByTarget(mem, fname, T)\
T fname(const uint WHERE, const T by) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint sumInOrder = subgroupBallotBitCount(bits);\
    uint idxInOrder = subgroupBallotExclusiveBitCount(bits);\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = atomicAdd(mem, T(sumInOrder) * T(by));}\
    return readFLane(gadd) + T(idxInOrder) * T(by);\
}


// statically multiplied
#define initSubgroupIncFunctionTarget(mem, fname, by, T)\
T fname(const uint WHERE) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint sumInOrder = subgroupBallotBitCount(bits);\
    uint idxInOrder = subgroupBallotExclusiveBitCount(bits);\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = add(mem, T(sumInOrder) * T(by));}\
    return readFLane(gadd) + T(idxInOrder) * T(by);\
}

#define initSubgroupIncFunctionByTarget(mem, fname, T)\
T fname(const uint WHERE, const T by) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    uint sumInOrder = subgroupBallotBitCount(bits);\
    uint idxInOrder = subgroupBallotExclusiveBitCount(bits);\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = add(mem, T(sumInOrder) * T(by));}\
    return readFLane(gadd) + T(idxInOrder) * T(by);\
}


bool allInvoc(in bool bc){ return subgroupAll(bc); }
bool anyInvoc(in bool bc){ return subgroupAny(bc); }



bool allInvoc(in BOOL_ bc){ return allInvoc(SSC(bc)); }
bool anyInvoc(in BOOL_ bc){ return anyInvoc(SSC(bc)); }
#define IFALL(b)if(allInvoc(b))
#define IFANY(b)if(anyInvoc(b))

#endif

