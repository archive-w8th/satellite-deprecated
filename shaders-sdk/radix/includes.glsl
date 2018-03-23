#include "../include/constants.glsl"
#include "../include/mathlib.glsl"

#define OUR_INVOC_TERM
uint LC_IDX = 0;
uint LANE_IDX = 0;
uint LT_IDX = 0;
uint LF_IDX = 0;

#include "../include/ballotlib.glsl"

// radices 4-bit
#define BITS_PER_PASS 4
#define RADICES 16
#define RADICES_MASK 0xf
#define AFFINITION 1

// general work groups
#define BLOCK_SIZE (WARP_SIZE * RADICES / AFFINITION) // how bigger block size, then more priority going to radices (i.e. BLOCK_SIZE / WARP_SIZE)
#define BLOCK_SIZE_RT (gl_WorkGroupSize.x)
#define WRK_SIZE_RT (gl_WorkGroupSize.x / WARP_SIZE_RT * gl_WorkGroupSize.y)

#define UVEC_WARP uint
#define BVEC_WARP bool
#define UVEC64_WARP uvec2
#define BVEC2_WARP bvec2

#ifdef ENABLE_AMD_INSTRUCTION_SET
//#define URDC_WARP uint16_t
//#define URDC_WARP_LCM u16vec2
//#define URDC_WARP_DUAL u16vec2
#define URDC_WARP uint
#define URDC_WARP_LCM uvec2
#define URDC_WARP_DUAL uvec2
#else
#define URDC_WARP uint
#define URDC_WARP_LCM uint
#define URDC_WARP_DUAL uvec2
#endif

// pointer of...
#define WPTR uint
#define WPTR2 uvec2

#define READ_LANE(V, I) (uint(I >= 0 && I < WARP_SIZE_RT) * readLane(V, I))

uint BFE(in uint ua, in uint o, in uint n) {
    return BFE_HW(ua, int(o), int(n));
}

//planned extended support
//uint64_t BFE(inout uint64_t ua, in uint64_t o, in uint64_t n) {
uint BFE(in uvec2 ua, in uint o, in uint n) {
    return uint(o >= 32u ? BFE_HW(ua.y, int(o-32u), int(n)) : BFE_HW(ua.x, int(o), int(n)));
}



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


struct blocks_info { uint count; uint offset; uint limit; uint r0; };
blocks_info get_blocks_info(in uint n) {
    uint block_tile = WARP_SIZE_RT;
    uint block_size = tiled(n, gl_NumWorkGroups.x);
    uint block_count = tiled(n, block_tile * gl_NumWorkGroups.x);
    uint block_offset = gl_WorkGroupID.x * block_tile * block_count;
    return blocks_info(block_count, block_offset, min(block_offset + tiled(block_size, block_tile)*block_tile, n), 0);
}

