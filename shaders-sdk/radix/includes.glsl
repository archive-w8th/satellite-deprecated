#include "../include/constants.glsl"
#include "../include/mathlib.glsl"

#define CUSTOMIZE_IDC
uint LC_IDX = 0;
uint LANE_IDX = 0;
uint LT_IDX = 0;

#include "../include/ballotlib.glsl"

// radices 4-bit
#define BITS_PER_PASS 4
#define RADICES 16
#define RADICES_MASK 0xf

// general work groups
#define BLOCK_SIZE (WARP_SIZE * RADICES) // how bigger block size, then more priority going to radices (i.e. BLOCK_SIZE / WARP_SIZE)
#define BLOCK_SIZE_RT (gl_WorkGroupSize.x)

#define WRK_SIZE (BLOCK_SIZE/WARP_SIZE)
#define WRK_SIZE_RT (gl_WorkGroupSize.x / WARP_SIZE_RT)

#define UVEC_WARP uint
#define BVEC_WARP bool
#define UVEC64_WARP uint64_t

#define READ_LANE(V, I) (uint(I >= 0 && I < WARP_SIZE_RT) * readLane(V, I))
#define BFE(a,o,n) ((a >> o) & ((1u << n)-1u))
//#define BFE(a,o,n) bitfieldExtract(a,o,n) // not supported 64-bit


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


struct blocks_info { uint count; uint offset; };
blocks_info get_blocks_info(in uint n) {
    //uint block_count = n > 0 ? ((n - 1) / (BLOCK_SIZE * gl_NumWorkGroups.x) + 1) : 0;
    //return blocks_info(block_count, gl_WorkGroupID.x * BLOCK_SIZE * block_count);
    uint block_count = n > 0 ? ((n - 1) / (WARP_SIZE_RT * gl_NumWorkGroups.x) + 1) : 0;
    return blocks_info(block_count, gl_WorkGroupID.x * WARP_SIZE_RT * block_count);
}

