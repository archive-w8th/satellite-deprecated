
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
layout ( std430, binding = 4, set = 0 ) restrict buffer NodesBlock {
    HlbvhNode Nodes[];
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



//==============================
//BVH boxes future transcoding
//By textureGather you can get LmnRmnLmxRmx by component, also packed by f16 (and texels gives by 32-bit each)
//By fetching texels you fetching each 32-bit element (packed two 16-bit), and restore to full 4x vector by two fetch
//You need allocate 4x4 texels for each element (2x4 as 32-bit representation)
/* 
      L   L    R   R
    +================+
min | x | y || x | y |
    +================+
max | x | y || x | y |
    +================+
min | z | w || z | w |
    +================+
max | z | w || z | w |
    +================+
    
*///============================



// bvh transcoded storage
#ifdef BVH_CREATION
layout ( binding = 5, r32i, set = 1 ) uniform iimage2D bvhStorage;
layout ( binding = 6, rg16f, set = 1 ) uniform image2D bvhBoxes;
#else
layout ( binding = 5, set = 1 ) uniform sampler2D bvhStorage;
layout ( binding = 6, set = 1 ) uniform sampler2D bvhBoxes;
#endif
