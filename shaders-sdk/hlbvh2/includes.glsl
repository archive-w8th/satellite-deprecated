
// Morton codes and geometry counters

layout ( std430, binding = 0, set = 0 ) restrict buffer MortoncodesBlock {
    uvec2 Mortoncodes[];
};

layout ( std430, binding = 1, set = 0 ) restrict buffer IndicesBlock {
    int MortoncodesIndices[];
};

layout ( std430, binding = 3, set = 0 ) restrict buffer LeafBlock {
    HlbvhNode Leafs[];
};

layout ( std430, binding = 4, set = 0 ) restrict buffer BVHBoxBlockWorking { 
    vec4 bvhBoxesWork[][4];
};

layout ( std430, binding = 5, set = 0 ) restrict buffer FlagsBlock {
    int Flags[];
};

layout ( std430, binding = 6, set = 0 ) restrict buffer ActivesBlock {
    int Actives[][2];
};

layout ( std430, binding = 7, set = 0 ) restrict buffer ChildBuffer {
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

#ifdef USE_F32_BVH
layout ( std430, binding = 12, set = 0 ) restrict buffer BVHBoxBlockResulting { vec4 bvhBoxesResulting[][4]; };
#else
layout ( std430, binding = 12, set = 0 ) restrict buffer BVHBoxBlockResulting { uvec2 bvhBoxesResulting[][4]; }; 
#endif

layout ( std430, binding = 11, set = 0 ) restrict buffer BVHMetaBlock { ivec4 bvhMeta[]; };


struct BVHCreatorUniformStruct {
    mat4x4 transform;
    mat4x4 transformInv;
    mat4x4 projection;
    mat4x4 projectionInv;
    int leafCount;
};

layout ( std430, binding = 10, set = 0 ) restrict readonly buffer BVHCreatorBlockUniform { BVHCreatorUniformStruct creatorUniform;} bvhBlock;



bbox calcTriBox(in mat3x4 triverts) {
    bbox result;
    result.mn = min3_wrap(triverts[0], triverts[1], triverts[2]);
    result.mx = max3_wrap(triverts[0], triverts[1], triverts[2]);
    result.mn -= 1e-5f;
    result.mx += 1e-5f;
    return result;
}
