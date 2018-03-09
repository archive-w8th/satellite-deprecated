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
    return clamp(fract(uintBitsToFloat((m & 0x007FFFFFu) | 0x3F800000u)-1.f), 0.f, 1.f);
}

vec2 float2Construct( in uvec2 m ) {
    return vec2(floatConstruct(m.x), floatConstruct(m.y));
}

vec2 half2Construct ( in uint m ) {
    return clamp(fract(unpackHalf2x16((m & 0x03FF03FFu) | (0x3C003C00u))-1.f.xx), 0.f.xx, 1.f.xx);
}


// seeds hashers
uint hash2( in uint a ) {
   a = (a+0x7ed55d16) + (a<<12);
   a = (a^0xc761c23c) ^ (a>>19);
   a = (a+0x165667b1) + (a<<5);
   a = (a+0xd3a2646c) ^ (a<<9);
   a = (a+0xfd7046c5) + (a<<3);
   a = (a^0xb55a4f09) ^ (a>>16);
   return a;
}


uint hash( in uint x ) {
    uint a = hash2(x);
    a += ( a << 10u );
    a ^= ( a >>  6u );
    a += ( a <<  3u );
    a ^= ( a >> 11u );
    a += ( a << 15u );
    return a;
}



// multi-dimensional seeds hashers
uint hash( in uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( in uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( in uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }



// aggregated randoms from seeds
float hrand( in uint   x ) { return floatConstruct(hash(x)); }
float hrand( in uvec2  v ) { return floatConstruct(hash(v)); }
float hrand( in uvec3  v ) { return floatConstruct(hash(v)); }
float hrand( in uvec4  v ) { return floatConstruct(hash(v)); }


// 1D random generators from superseed
float random( in uvec2 superseed ) {
    uint hclk = ++randomClocks;
    uint plan = uint(globalInvocationSMP);
    uint comb = hash(uvec4(hclk, plan, subHash, hash(superseed) ));
    return floatConstruct(comb);
}


// 1D random generators from superseed
float urandom( in uvec2 superseed ) {
    uint hclk = ++randomClocks;
    uint plan = uint(0);
    uint comb = hash(uvec4(hclk, plan, subHash, hash(superseed) ));
    return floatConstruct(comb);
}




// another 2D random generator
vec2 randf2q( in uvec2 superseed ) {
    uint hclk = ++randomClocks;
    uint plan = uint(globalInvocationSMP);
    uint comb = hash(uvec4(hclk, plan, subHash, hash(superseed) ));
    return vec2(floatConstruct(comb), radicalInverse_VdC(comb));
}

// another 2D random generator
vec2 randf2x( in uvec2 superseed ) {
    uint hclk = ++randomClocks;
    uint plan = uint(globalInvocationSMP);
    uint comb = hash(uvec4(hclk, plan, subHash, hash(superseed) ));
    return vec2(half2Construct(comb));
}



// static aggregated randoms
float random() { return random(rayStreams[0].superseed[0]); }
vec2 randf2q() { return randf2q(rayStreams[0].superseed[0]); }
vec2 randf2x() { return randf2x(rayStreams[0].superseed[0]); }


// geometric random generators
vec3 randomCosine(in uvec2 superseed) {
    vec2 hmsm = randf2x(superseed);
    float up = sqrt(1.f-hmsm.x), over = sqrt(1.f - up * up), around = hmsm.y * TWO_PI;
    return normalize(vec3( cos(around) * over, sin(around) * over, up ));
}


vec3 randomDirectionInSphere() {
    vec2 hmsm = randf2x();
    float up = (0.5f-hmsm.x)*2.f, over = sqrt(1.f - up * up), around = hmsm.y * TWO_PI;
    return normalize(vec3( cos(around) * over, sin(around) * over, up ));
}

#endif
