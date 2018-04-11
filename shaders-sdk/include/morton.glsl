#ifndef _MORTON_H
#define _MORTON_H

// method to seperate bits from a given integer 3 positions apart
uint64_t splitBy3(in uint a){
    uint64_t x = uint64_t(a) & 0x1ffffful; // we only look at the first 21 bits
    x = (x | x << 32ul) & 0x1f00000000fffful;  // shift left 32 bits, OR with self, and 00011111000000000000000000000000000000001111111111111111
    x = (x | x << 16ul) & 0x1f0000ff0000fful;  // shift left 32 bits, OR with self, and 00011111000000000000000011111111000000000000000011111111
    x = (x | x << 8ul) & 0x100f00f00f00f00ful; // shift left 32 bits, OR with self, and 0001000000001111000000001111000000001111000000001111000000000000
    x = (x | x << 4ul) & 0x10c30c30c30c30c3ul; // shift left 32 bits, OR with self, and 0001000011000011000011000011000011000011000011000011000100000000
    x = (x | x << 2ul) & 0x1249249249249249ul;
    return x;
}

uvec2 encodeMorton3_64(in uvec3 a) {
    return U2P((splitBy3(a.x) | (splitBy3(a.y) << 1) | (splitBy3(a.z) << 2)));
}

#endif
