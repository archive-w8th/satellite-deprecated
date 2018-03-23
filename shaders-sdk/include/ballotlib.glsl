#ifndef _BALLOTLIB_H
#define _BALLOTLIB_H

#include "../include/mathlib.glsl"

// for constant maners
#ifndef WARP_SIZE
#ifndef NVIDIA_PLATFORM
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


// universal aliases
#define readFLane RLF_
#define readLane RL_

UVEC_BALLOT_WARP ballotHW(in bool i) { return subgroupBallot(i); }
UVEC_BALLOT_WARP ballotHW() { return subgroupBallot(true); }
bool electedInvoc() { return subgroupElect(); }


// statically multiplied
#define initAtomicSubgroupIncFunction(mem, fname, by, T)\
T fname() {\
    const UVEC_BALLOT_WARP bits = ballotHW();\
    const uint sumInOrder = subgroupBallotBitCount(bits), idxInOrder = subgroupBallotExclusiveBitCount(bits);\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = atomicAdd(mem, T(sumInOrder) * T(by));}\
    return readFLane(gadd) + T(idxInOrder) * T(by);\
}

#define initAtomicSubgroupIncFunctionDyn(mem, fname, T)\
T fname(const T by) {\
    const UVEC_BALLOT_WARP bits = ballotHW();\
    const uint sumInOrder = subgroupBallotBitCount(bits), idxInOrder = subgroupBallotExclusiveBitCount(bits);\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = atomicAdd(mem, T(sumInOrder) * T(by));}\
    return readFLane(gadd) + T(idxInOrder) * T(by);\
}


// statically multiplied
#define initAtomicSubgroupIncFunctionTarget(mem, fname, by, T)\
T fname(const uint WHERE) {\
    const UVEC_BALLOT_WARP bits = ballotHW();\
    const uint sumInOrder = subgroupBallotBitCount(bits), idxInOrder = subgroupBallotExclusiveBitCount(bits);\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = atomicAdd(mem, T(sumInOrder) * T(by));}\
    return readFLane(gadd) + T(idxInOrder) * T(by);\
}

#define initAtomicSubgroupIncFunctionByTarget(mem, fname, T)\
T fname(const uint WHERE, const T by) {\
    const UVEC_BALLOT_WARP bits = ballotHW();\
    const uint sumInOrder = subgroupBallotBitCount(bits), idxInOrder = subgroupBallotExclusiveBitCount(bits);\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = atomicAdd(mem, T(sumInOrder) * T(by));}\
    return readFLane(gadd) + T(idxInOrder) * T(by);\
}


// statically multiplied
#define initSubgroupIncFunctionTarget(mem, fname, by, T)\
T fname(const uint WHERE) {\
    const UVEC_BALLOT_WARP bits = ballotHW();\
    const uint sumInOrder = subgroupBallotBitCount(bits), idxInOrder = subgroupBallotExclusiveBitCount(bits);\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = add(mem, T(sumInOrder) * T(by));}\
    return readFLane(gadd) + T(idxInOrder) * T(by);\
}

#define initSubgroupIncFunctionByTarget(mem, fname, T)\
T fname(const uint WHERE, const T by) {\
    const UVEC_BALLOT_WARP bits = ballotHW();\
    const uint sumInOrder = subgroupBallotBitCount(bits), idxInOrder = subgroupBallotExclusiveBitCount(bits);\
    T gadd = 0;\
    if (subgroupElect() && sumInOrder > 0) {gadd = add(mem, T(sumInOrder) * T(by));}\
    return readFLane(gadd) + T(idxInOrder) * T(by);\
}



// statically multiplied
#define initSubgroupIncFunctionTargetDual(mem, fname, by, T, T2)\
T2 fname(const uint WHERE, in bvec2 a) {\
    const UVEC_BALLOT_WARP bitsx = ballotHW(a.x), bitsy = ballotHW(a.y);\
    const uvec2 \
        sumInOrder = uvec2(subgroupBallotBitCount(bitsx), subgroupBallotBitCount(bitsy)),\
        idxInOrder = uvec2(subgroupBallotExclusiveBitCount(bitsx), subgroupBallotExclusiveBitCount(bitsy));\
    T gadd = 0;\
    if (subgroupElect() && any(greaterThan(sumInOrder, (0u).xx))) {gadd = add(mem, T(sumInOrder.x+sumInOrder.y)*T(by));}\
    return readFLane(gadd).xx + T2(idxInOrder.x, sumInOrder.x+idxInOrder.y) * T(by);\
}



// invoc vote
bool allInvoc(in bool bc){ return subgroupAll(bc); }
bool anyInvoc(in bool bc){ return subgroupAny(bc); }


// aliases
bool allInvoc(in BOOL_ bc){ return allInvoc(SSC(bc)); }
bool anyInvoc(in BOOL_ bc){ return anyInvoc(SSC(bc)); }
#define IFALL(b)if(allInvoc(b))
#define IFANY(b)if(anyInvoc(b))


// subgroup barriers
#define SB_BARRIER subgroupMemoryBarrier(),subgroupBarrier();


#endif

