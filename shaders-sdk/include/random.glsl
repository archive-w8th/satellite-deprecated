#ifndef _RANDOM_H
#define _RANDOM_H

uint randomClocks = 0;
uint globalInvocationSMP = 0;
uint subHash = 0;


// bitfield >> float aggregators
float radicalInverse_VdC(in uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return clamp(fract(float(bits) * 2.3283064365386963e-10), 0.f, 1.f);
}

float floatConstruct( in uint m ) {
    uint ieeeMantissa = 0x007FFFFFu;
    uint ieeeOne = 0x3F800000u;
    m &= ieeeMantissa;
    m |= ieeeOne;
    return clamp(fract(uintBitsToFloat(m)), 0.f, 1.f);
}


// seeds hashers
uint hash( in uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}


// multi-dimensional seeds hashers
uint hash( in uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( in uvec3 v ) { return hash( v.x ^ hash(v.y ^ hash(v.z))             ); }
uint hash( in uvec4 v ) { return hash( v.x ^ hash(v.y ^ hash(v.z ^ hash(v.w))) ); }


// aggregated randoms from seeds
float hrand( in uint   x ) { return radicalInverse_VdC(hash(x)); }
float hrand( in uvec2  v ) { return radicalInverse_VdC(hash(v)); }
float hrand( in uvec3  v ) { return radicalInverse_VdC(hash(v)); }
float hrand( in uvec4  v ) { return radicalInverse_VdC(hash(v)); }


// 1D random generators from superseed
float random( in int superseed ) {
    uint hs = (++randomClocks); randomClocks <<= 1;
    return hrand( uvec4(globalInvocationSMP << 6, subHash, superseed, hs) );
}


// 2D random generators from superseed
vec2 hammersley2d(in uint N, in int superseed) {
    uint hs = (++randomClocks); randomClocks <<= 1;
    uint i = hash( uvec4(globalInvocationSMP << 6, subHash, superseed, hs) );
    return vec2(fract(float(i%N) / float(N)), radicalInverse_VdC(i));
}


// static aggregated randoms
float random() { return random(rayStreams[0].superseed.x); }
vec2 hammersley2d(in uint N) { return hammersley2d(N, rayStreams[0].superseed.x); }



// geometric random generators
vec3 randomCosine(in vec3 normal, in int superseed) {
    vec2 hmsm = hammersley2d(1024, superseed);
    float up = sqrt(hmsm.x), over = sqrt(1.f - up * up), around = hmsm.y * TWO_PI;
    vec3 perpendicular0 = abs(normal.x) < SQRT_OF_ONE_THIRD ? vec3(1, 0, 0) : (abs(normal.y) < SQRT_OF_ONE_THIRD ? vec3(0, 1, 0) : vec3(0, 0, 1));
    vec3 perpendicular1 = normalize(cross(normal, perpendicular0));
    vec3 perpendicular2 = normalize(cross(normal, perpendicular1));
    return normalize(fma(normal, up.xxx, fma(perpendicular1, cos(around).xxx * over, perpendicular2* sin(around).xxx * over)));
}

vec3 randomDirectionInSphere() {
    vec2 hmsm = hammersley2d(1024);
    float up = fma(hmsm.x, 2.0f, -1.0f), over = sqrt(1.f - up * up), around = hmsm.y * TWO_PI;
    return normalize(vec3( up, cos(around) * over, sin(around) * over ));
}

#endif
