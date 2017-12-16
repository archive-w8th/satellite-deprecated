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
uint hash( in uint a ) {
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    return a;
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
    uint hclk = ++randomClocks; randomClocks <<= 1;
    uint plan = (globalInvocationSMP << 10) ^ subHash;
    uint gseq = uint(superseed);
    return hrand(uvec3(hclk, plan, gseq));
}


// 2D random generators from superseed
vec2 hammersley2d( in uint N, in int superseed ) {
    uint hclk = ++randomClocks; randomClocks <<= 1;
    uint plan = (globalInvocationSMP << 10) ^ subHash;
    uint gseq = uint(superseed);
    uint comb = hash(uvec3(hclk, plan, gseq));
    return vec2(fract(float(comb%N) / float(N)), radicalInverse_VdC(comb));
}


// another 2D random generator
vec2 randf2x( in int superseed ) {
    uint hclk = ++randomClocks; randomClocks <<= 1;
    uint plan = (globalInvocationSMP << 10) ^ subHash;
    uint gseq = uint(superseed);
    uint comb = hash(uvec3(hclk, plan, gseq));
    return vec2(floatConstruct(comb), radicalInverse_VdC(comb));
}



// static aggregated randoms
float random() { return random(rayStreams[0].superseed.x); }
vec2 hammersley2d(in uint N) { return hammersley2d(N, rayStreams[0].superseed.x); }
vec2 randf2x() { return randf2x(rayStreams[0].superseed.x); }


// geometric random generators
vec3 randomCosine(in vec3 normal, in int superseed) {
    vec2 hmsm = randf2x(superseed);
    float up = sqrt(hmsm.x), over = sqrt(1.f - up * up), around = hmsm.y * TWO_PI;
    vec3 perpendicular0 = abs(normal.x) < SQRT_OF_ONE_THIRD ? vec3(1, 0, 0) : (abs(normal.y) < SQRT_OF_ONE_THIRD ? vec3(0, 1, 0) : vec3(0, 0, 1));
    vec3 perpendicular1 = normalize(cross(normal, perpendicular0));
    vec3 perpendicular2 = normalize(cross(normal, perpendicular1));
    return normalize(fma(normal, up.xxx, fma(perpendicular1, cos(around).xxx * over, perpendicular2* sin(around).xxx * over)));
}

vec3 randomDirectionInSphere() {
    vec2 hmsm = randf2x();
    float up = fma(hmsm.x, 2.0f, -1.0f), over = sqrt(1.f - up * up), around = hmsm.y * TWO_PI;
    return normalize(vec3( up, cos(around) * over, sin(around) * over ));
}

#endif
