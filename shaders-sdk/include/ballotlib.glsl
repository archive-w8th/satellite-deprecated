#ifndef _BALLOTLIB_H
#define _BALLOTLIB_H

#include "../include/mathlib.glsl"



#ifndef WARP_SIZE
#ifdef ENABLE_AMD_INSTRUCTION_SET
#define WARP_SIZE 64
#else
#define WARP_SIZE 32
#endif
#endif

#ifndef LEGACY_BALLOT
#define WARP_SIZE_RT gl_SubgroupSize.x
#else
#define WARP_SIZE_RT WARP_SIZE
#endif

#ifndef OUR_INVOC_TERM
#ifndef LEGACY_BALLOT
#define LT_IDX gl_LocalInvocationIndex.x
#define LC_IDX (LT_IDX / WARP_SIZE_RT)
#define LANE_IDX gl_SubgroupInvocationID.x
#else
#define LT_IDX gl_LocalInvocationIndex.x
#define LC_IDX (LT_IDX / WARP_SIZE_RT)
#define LANE_IDX (LT_IDX % WARP_SIZE_RT)
#endif
#endif


#define UVEC_BALLOT_WARP uvec4

#ifdef LEGACY_BALLOT
#define RL_(a,b) readInvocationARB(a,int(b))
#define RLF_ readFirstInvocationARB
#else
#define RL_ subgroupBroadcast
#define RLF_ subgroupBroadcastFirst
#endif



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


#ifdef LEGACY_BALLOT
UVEC_BALLOT_WARP maskLt(const uint lid) { return UVEC_BALLOT_WARP(U2P(lid == 64 ? 0xFFFFFFFFFFFFFFFFul : (1ul << uint64_t(lid)) - 1ul), 0u.xx); }
#endif


UVEC_BALLOT_WARP ballotHW() { 
#ifdef LEGACY_BALLOT
    return UVEC_BALLOT_WARP(U2P(ballotARB(true)), 0u.xx) & maskLt(WARP_SIZE_RT);
#else
    return subgroupBallot(true); 
#endif
}


#ifdef LEGACY_BALLOT 
bool electedInvoc() { return lsb(ballotHW().xy)==LANE_IDX; }
#else
bool electedInvoc() { return subgroupElect(); }
#endif


#ifdef LEGACY_BALLOT
/*

uint bitclt(in uvec2 bits){
#ifdef ENABLE_AMD_INSTRUCTION_SET
    return mbcntAMD(P2U(bits.xy)); // AMuDe
#else
    return bitcnt((bits&maskLt(LANE_IDX)).xy); // some other
#endif
}

// statically multiplied
#define initAtomicSubgroupIncFunction(mem, fname, by, T)\
T fname() {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(bitcnt(bits.xy));\
    T idxInOrder = T(bitclt(bits.xy));\
    T gadd = 0;\
    if (lsb(bits.xy)==LANE_IDX && sumInOrder > 0) {gadd = atomicAdd(mem, sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}

#define initAtomicSubgroupIncFunctionDyn(mem, fname, T)\
T fname(const T by) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(bitcnt(bits.xy));\
    T idxInOrder = T(bitclt(bits.xy));\
    T gadd = 0;\
    if (lsb(bits.xy)==LANE_IDX && sumInOrder > 0) {gadd = atomicAdd(mem, sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}

// statically multiplied
#define initAtomicSubgroupIncFunctionTarget(mem, fname, by, T)\
T fname(const uint WHERE) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(bitcnt(bits.xy));\
    T idxInOrder = T(bitclt(bits.xy));\
    T gadd = 0;\
    if (lsb(bits.xy)==LANE_IDX && sumInOrder > 0) {gadd = atomicAdd(mem, sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}

#define initAtomicSubgroupIncFunctionByTarget(mem, fname, T)\
T fname(const uint WHERE, const T by) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(bitcnt(bits.xy));\
    T idxInOrder = T(bitclt(bits.xy));\
    T gadd = 0;\
    if (lsb(bits.xy)==LANE_IDX && sumInOrder > 0) {gadd = atomicAdd(mem, sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}

// statically multiplied
#define initSubgroupIncFunctionTarget(mem, fname, by, T)\
T fname(const uint WHERE) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(bitcnt(bits.xy));\
    T idxInOrder = T(bitcnt((bits&maskLt(LANE_IDX)).xy));\
    T gadd = 0;\
    if (lsb(bits.xy)==LANE_IDX && sumInOrder > 0) {gadd = add(mem, sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}


#define initSubgroupIncFunctionByTarget(mem, fname, T)\
T fname(const uint WHERE, const T by) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(bitcnt(bits.xy));\
    T idxInOrder = T(bitclt(bits.xy));\
    T gadd = 0;\
    if (lsb(bits.xy)==LANE_IDX && sumInOrder > 0) {gadd = add(mem, sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}

#define initSubgroupIncFunctionFunc(memfunc, fname, by, T)\
T fname(const uint WHERE) {\
    UVEC_BALLOT_WARP bits = ballotHW();\
    T sumInOrder = T(bitcnt(bits.xy));\
    T idxInOrder = T(bitclt(bits.xy));\
    T gadd = 0;\
    if (lsb(bits.xy)==LANE_IDX && sumInOrder > 0) {gadd = memfunc(T(WHERE)); memfunc(T(WHERE), gadd+sumInOrder * T(by));}\
    return readFLane(gadd) + idxInOrder * T(by);\
}

bool allInvoc(in bool bc){ return allInvocationsARB(bc); }
bool anyInvoc(in bool bc){ return anyInvocationARB(bc); }
*/

#else

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
#endif


bool allInvoc(in BOOL_ bc){ return allInvoc(SSC(bc)); }
bool anyInvoc(in BOOL_ bc){ return anyInvoc(SSC(bc)); }
#define IFALL(b)if(allInvoc(b))
#define IFANY(b)if(anyInvoc(b))

#endif

