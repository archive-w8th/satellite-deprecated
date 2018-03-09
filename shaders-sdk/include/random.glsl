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
    return clamp(fract(1.f-uintBitsToFloat((m & 0x007FFFFFu) | 0x3F800000u)), 0.f, 1.f);
}


// seeds hashers
uint hash( in uint x ) {
    uint a = x;
    a += ( a << 10u );
    a ^= ( a >>  6u );
    a += ( a <<  3u );
    a ^= ( a >> 11u );
    a += ( a << 15u );
    return a;
}


// multi-dimensional seeds hashers
uint hash( in uvec2 v ) { return hash(v.x^hash(v.y)); }
uint hash( in uvec3 v ) { return hash(v.x^hash(v.yz)); }
uint hash( in uvec4 v ) { return hash(v.x^hash(v.yzw)); }


// aggregated randoms from seeds
float hrand( in uint   x ) { return floatConstruct(hash(x)); }
float hrand( in uvec2  v ) { return floatConstruct(hash(v)); }
float hrand( in uvec3  v ) { return floatConstruct(hash(v)); }
float hrand( in uvec4  v ) { return floatConstruct(hash(v)); }


// 1D random generators from superseed
float random( in int superseed ) {
    uint hclk = ++randomClocks; randomClocks <<= 1;
    uint plan = hash((globalInvocationSMP << 8) ^ subHash);
    uint gseq = hash(uint(superseed));
    return hrand(uvec3(plan, hclk, gseq));
}

// 1D random generators from superseed
float randomQnt( in int superseed ) {
    uint hclk = ++randomClocks; randomClocks <<= 1;
    uint plan = 0;
    uint gseq = hash(uint(superseed));
    return hrand(uvec3(plan, hclk, gseq));
}




// 2D random generators from superseed
vec2 hammersley2d( in uint N, in int superseed ) {
    uint hclk = ++randomClocks; randomClocks <<= 1;
    uint plan = hash((globalInvocationSMP << 8) ^ subHash);
    uint gseq = hash(uint(superseed));
    uint comb = hash(uvec3(plan, hclk, gseq));
    return vec2(fract(float(comb%N) / float(N)), radicalInverse_VdC(comb));
}

// another 2D random generator
vec2 randf2x( in int superseed ) {
    uint hclk = ++randomClocks; randomClocks <<= 1;
    uint plan = hash((globalInvocationSMP << 8) ^ subHash);
    uint gseq = hash(uint(superseed));
    uint comb = hash(uvec3(plan, hclk, gseq));
    return vec2(floatConstruct(comb), radicalInverse_VdC(comb));
}

// another 2D random generator
vec2 randf2q( in int superseed ) {
    uint hclk = ++randomClocks; randomClocks <<= 1;
    uint plan = 0;
    uint gseq = hash(uint(superseed));
    uint comb = hash(uvec3(plan, hclk, gseq));
    return vec2(floatConstruct(comb), radicalInverse_VdC(comb));
}



// static aggregated randoms
float random() { return random(rayStreams[0].superseed.x); }
float randomQnt() { return randomQnt(rayStreams[0].superseed.x); }
vec2 hammersley2d(in uint N) { return hammersley2d(N, rayStreams[0].superseed.x); }
vec2 randf2x() { return randf2x(rayStreams[0].superseed.x); }



// geometric random generators
vec3 randomCosine(in int superseed) {
    vec2 hmsm = randf2x(superseed);
    float up = sqrt(1.f-hmsm.x), over = sqrt(1.f - up * up), around = hmsm.y * TWO_PI;
    return normalize(vec3( cos(around) * over, sin(around) * over, up ));
}


vec3 randomCosineQnt(in int superseed) {
    vec2 hmsm = randf2q(superseed);
    float up = sqrt(1.f-hmsm.x), over = sqrt(1.f - up * up), around = hmsm.y * TWO_PI;
    return normalize(vec3( cos(around) * over, sin(around) * over, up ));
}


vec3 randomDirectionInSphere() {
    vec2 hmsm = randf2x();
    float up = (0.5f-hmsm.x)*2.f, over = sqrt(1.f - up * up), around = hmsm.y * TWO_PI;
    return normalize(vec3( cos(around) * over, sin(around) * over, up ));
}

#endif
