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
}

#endif
#endif
