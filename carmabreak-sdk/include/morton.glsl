#ifndef _MORTON_H
#define _MORTON_H

#if (defined(INT64_MORTON))

uint64_t part1By2_64(in uint a) {
    uint64_t x = a & 0x1ffffful;
    x = (x | x << 32) & 0x1f00000000fffful;
    x = (x | x << 16) & 0x1f0000ff0000fful;
    x = (x | x << 8) & 0x100f00f00f00f00ful;
    x = (x | x << 4) & 0x10c30c30c30c30c3ul;
    x = (x | x << 2) & 0x1249249249249249ul;
    return x;
}

uint64_t encodeMorton3_64(in uvec3 a)
{
    return part1By2_64(a.x) | (part1By2_64(a.y) << 1) | (part1By2_64(a.z) << 2);
    //return MortonToHilbert3D(part1By2_64(a.x) | (part1By2_64(a.y) << 1) | (part1By2_64(a.z) << 2), 21);
}

#else

uint part1By2_64(in uint a) {
    uint x = a;
    x &= 0x000003ff;
    x = (x ^ (x << 16)) & 0x030000ff;
    x = (x ^ (x <<  8)) & 0x0300f00f;
    x = (x ^ (x <<  4)) & 0x030c30c3;
    x = (x ^ (x <<  2)) & 0x09249249;
    return x;
}

uint encodeMorton3_64(in uvec3 a) {
    return part1By2_64(a.x) | (part1By2_64(a.y) << 1) | (part1By2_64(a.z) << 2);
    //return MortonToHilbert3D(part1By2_64(a.x) | (part1By2_64(a.y) << 1) | (part1By2_64(a.z) << 2), 10);
}

#endif


uint part1By2(in uint a) {
    uint x = a;
    x &= 0x000003ff;
    x = (x ^ (x << 16)) & 0x030000ff;
    x = (x ^ (x <<  8)) & 0x0300f00f;
    x = (x ^ (x <<  4)) & 0x030c30c3;
    x = (x ^ (x <<  2)) & 0x09249249;
    return x;
}

uint encodeMorton3(in uvec3 a) {
    return part1By2(a.x) | (part1By2(a.y) << 1) | (part1By2(a.z) << 2);
}



uint64_t partBy6(in uint x){
    uint64_t answer = 0;
    for (int i = 0; i < 10; ++i) {
        answer |= ((x & (1ul << uint64_t(i))) << 6ul*i);
    }
    return answer;
}

uint64_t encodeMorton6(in uvec3 a, in uvec3 b){
    return 
        (partBy6(a.x) << 0ul) | (partBy6(a.y) << 1ul) | (partBy6(a.z) << 2ul) |
        (partBy6(b.y) << 3ul) | (partBy6(b.y) << 4ul) | (partBy6(b.z) << 5ul);

        //(partBy6(a.x) << 0ul) | (partBy6(b.x) << 1ul) | 
        //(partBy6(a.y) << 2ul) | (partBy6(b.y) << 3ul) | 
        //(partBy6(a.z) << 4ul) | (partBy6(b.z) << 5ul);
}




#endif
