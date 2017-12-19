
// Morton codes and geometry counters
layout ( std430, binding = 0, set = 0 ) coherent buffer MortoncodesBlock {
    MORTONTYPE Mortoncodes[];
};

layout ( std430, binding = 1, set = 0 ) coherent buffer IndicesBlock {
    int MortoncodesIndices[];
};

layout ( std430, binding = 3, set = 0 ) coherent buffer LeafBlock {
    HlbvhNode Leafs[];
};

// BVH nodes
layout ( std430, binding = 4, set = 0 ) restrict buffer BVHBoxBlock {
    UBLANEF_ bvhBoxes[][2];
};

layout ( std430, binding = 5, set = 0 ) restrict buffer FlagsBlock {
    int Flags[];
};

layout ( std430, binding = 6, set = 0 ) coherent buffer ActivesBlock {
    int Actives[];
};

layout ( std430, binding = 7, set = 0 ) coherent buffer ChildBuffer {
    int LeafIndices[];
};

layout ( std430, binding = 8, set = 0 ) restrict buffer BuildCounters {
    int cCounter;
    int lCounter;
    int aCounter;
    int bvhLevel;
    int aRange[2];
    int aabbcount[1];
};





struct BVHCreatorUniformStruct {
    mat4 transform;
    mat4 transformInv;
    int leafCount;
};

layout ( std430, binding = 10, set = 0 ) readonly buffer BVHCreatorBlockUniform { BVHCreatorUniformStruct creatorUniform;} bvhBlock;



bbox calcTriBox(in mat3x4 triverts) {
    bbox result;
#if (defined(ENABLE_AMD_INSTRUCTION_SET))
    result.mn = min3(triverts[0], triverts[1], triverts[2]);
    result.mx = max3(triverts[0], triverts[1], triverts[2]);
#else
    result.mn = min(min(triverts[0], triverts[1]), triverts[2]);
    result.mx = max(max(triverts[0], triverts[1]), triverts[2]);
#endif
    return result;
}
