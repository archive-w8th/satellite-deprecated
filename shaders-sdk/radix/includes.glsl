#extension GL_ARB_gpu_shader_int64 : enable
//#extension GL_AMD_gpu_shader_int64 : enable
#extension GL_ARB_shader_ballot : enable
#extension GL_ARB_shader_group_vote : enable
#extension GL_GOOGLE_include_directive : enable

// enable required GAPI extensions
#ifdef ENABLE_AMD_INSTRUCTION_SET
#extension GL_AMD_gcn_shader : enable
#extension GL_AMD_gpu_shader_half_float : enable
//#extension GL_AMD_gpu_shader_half_float2 : enable
#extension GL_AMD_gpu_shader_int16 : enable
#extension GL_AMD_shader_trinary_minmax : enable
#extension GL_AMD_texture_gather_bias_lod : enable
#extension GL_AMD_shader_ballot : enable
#endif


// radices 2-bit
//#define BITS_PER_PASS 2
//#define RADICES 4
//#define RADICES_MASK 0x3

// radices 4-bit
#define BITS_PER_PASS 4
#define RADICES 16
#define RADICES_MASK 0xf

// radices 8-bit
//#define BITS_PER_PASS 8
//#define RADICES 256
//#define RADICES_MASK 0xff


// warp size (used for process data)
#ifndef WARP_SIZE
#ifdef ENABLE_AMD_INSTRUCTION_SET
#define WARP_SIZE 64
#else
#define WARP_SIZE 32
#endif
#endif

// general work groups
#define BLOCK_SIZE (WARP_SIZE * RADICES) // how bigger block size, then more priority going to radices (i.e. BLOCK_SIZE / WARP_SIZE)
#define BLOCK_SIZE_RT (gl_WorkGroupSize.x)

// force ballot
//#define WARP_SIZE_RT gl_SubGroupSizeARB
#define WARP_SIZE_RT WARP_SIZE 


#define WORK_SIZE (BLOCK_SIZE/WARP_SIZE)
#define WORK_SIZE_RT (gl_WorkGroupSize.x / WARP_SIZE_RT)

uint LC_IDX = 0;
uint LANE_IDX = 0;
uint LT_IDX = 0;

#define UVEC_WARP uint
#define BVEC_WARP bool
#define UVEC64_WARP uint64_t

#define READ_LANE(V, I) (uint(I >= 0 && I < WARP_SIZE_RT) * readLane(V, I))
#define BFE(a,o,n) ((a >> o) & ((1u << n)-1u))
//#define BFE(a,o,n) bitfieldExtract(a,o,n)

#define KEYTYPE UVEC64_WARP
//#define KEYTYPE UVEC_WARP
layout (std430, binding = 20, set = 0 ) coherent buffer KeyInBlock {KEYTYPE KeyIn[]; };
layout (std430, binding = 21, set = 0 ) coherent buffer ValueInBlock {uint ValueIn[]; };
layout (std430, binding = 24, set = 0 ) readonly buffer VarsBlock {
    uint NumKeys;
    uint Shift;
    uint Descending;
    uint IsSigned;
};
layout (std430, binding = 25, set = 0 ) coherent buffer KeyTmpBlock {KEYTYPE KeyTmp[]; };
layout (std430, binding = 26, set = 0 ) coherent buffer ValueTmpBlock {uint ValueTmp[]; };
layout (std430, binding = 27, set = 0 ) restrict buffer HistogramBlock {uint Histogram[]; };
layout (std430, binding = 28, set = 0 ) restrict buffer PrefixBlock {uint PrefixSum[]; };




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



#ifdef ENABLE_AMD_INSTRUCTION_SET
uvec2 U2P(in uint64_t pckg) { return unpackUint2x32(pckg); }
uint64_t P2U(in uvec2 pckg) { return packUint2x32(pckg); }
#else
uvec2 U2P(in uint64_t pckg) { return uvec2(pckg >> 0ul, pckg >> 32ul); }
uint64_t P2U(in uvec2 pckg) { return uint64_t(pckg.x) | (uint64_t(pckg.y) << 32ul); }
#endif


struct blocks_info { uint count; uint offset; };
blocks_info get_blocks_info(in uint n) {
    //uint block_count = n > 0 ? ((n - 1) / (BLOCK_SIZE * gl_NumWorkGroups.x) + 1) : 0;
    //return blocks_info(block_count, gl_WorkGroupID.x * BLOCK_SIZE * block_count);
    uint block_count = n > 0 ? ((n - 1) / (WARP_SIZE_RT * gl_NumWorkGroups.x) + 1) : 0;
    return blocks_info(block_count, gl_WorkGroupID.x * WARP_SIZE_RT * block_count);
}

//#define UVEC_BALLOT_WARP UVEC64_WARP
#define UVEC_BALLOT_WARP uvec2


// filtering wrong warps
uvec2 filterBallot() {
    return lookat_lt[WARP_SIZE_RT];
}

uvec2 genLtMask() {
    return lookat_lt[LANE_IDX] & filterBallot();
}


int btc(in uint vlc) { return bitCount(vlc); }
int lsb(in uint vlc) { return findLSB(vlc); }
int msb(in uint vlc) { return findMSB(vlc); }


// bit measure utils
int lsb(in uint64_t vlc) {
#ifdef ENABLE_AMD_INSTRUCTION_SET
    return findLSB(vlc); 
#else
    uvec2 pair = U2P(vlc); int lv = lsb(pair.x), hi = lsb(pair.y); return (lv >= 0) ? lv : (32 + hi);
#endif
}

int msb(in uint64_t vlc) {
#ifdef ENABLE_AMD_INSTRUCTION_SET
    return findMSB(vlc); 
#else
    uvec2 pair = U2P(vlc); int lv = msb(pair.x), hi = msb(pair.y); return (hi >= 0) ? (32 + hi) : lv;
#endif
}


int msb(in uvec2 pair) {
    return msb(P2U(pair));
}

int lsb(in uvec2 pair) {
    return lsb(P2U(pair));
}



int bitCount64(in uvec2 lh) {
    return btc(lh.x) + btc(lh.y);
}

uint readLane(in uint val, in int lane) {
    return readInvocationARB(val, lane);
}

uint readLane(in uint val, in uint lane) {
    return readInvocationARB(val, lane);
}

uvec2 ballotHW(in bool val) {
    return U2P(ballotARB(val)) & filterBallot();
}


int firstActive() {
    return lsb(ballotHW(true));
}


int mcount64(in UVEC_BALLOT_WARP bits){
#ifdef ENABLE_AMD_INSTRUCTION_SET
    return int(mbcntAMD(P2U(bits))); // AMuDe
#else
    return bitCount64(bits & genLtMask()); // some other
#endif
}



#define initAtomicIncFunctionDynamic(mem, fname, T)\
T fname(in uint clm, in bool value) { \
    int activeLane = firstActive();\
    UVEC_BALLOT_WARP bits = ballotHW(value);\
    T sumInOrder = T(bitCount64(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) gadd = atomicAdd(mem[clm], sumInOrder * T(LANE_IDX == activeLane));\
    return readLane(gadd, activeLane) + idxInOrder; \
}

#define initNonAtomicIncFunctionDynamic(mem, fname, T)\
T fname(in uint clm, in bool value) { \
    int activeLane = firstActive();\
    UVEC_BALLOT_WARP bits = ballotHW(value);\
    T sumInOrder = T(bitCount64(bits));\
    T idxInOrder = T(mcount64(bits));\
    T gadd = 0;\
    if (sumInOrder > 0 && LANE_IDX == activeLane) {gadd = mem[clm]; mem[clm] += sumInOrder * T(LANE_IDX == activeLane); }\
    return readLane(gadd, activeLane) + idxInOrder; \
}

uint countInvocs(in bool val){
#ifdef ENABLE_AMD_INSTRUCTION_SET
    return addInvocationsAMD(int(val));
#else
    return bitCount64(ballotHW(val));
#endif
}

bool allInvoc(in bool bc){
    //return countInvocs(bc) >= countInvocs(true);
    return allInvocations(bc);
}

bool anyInvoc(in bool bc){
    //return countInvocs(bc) > 0;
    return anyInvocation(bc);
}

#define IFALL(b)if(allInvoc(b))
#define IFANY(b)if(anyInvoc(b))
