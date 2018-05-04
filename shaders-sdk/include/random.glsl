#ifndef _RANDOM_H
#define _RANDOM_H

highp uint randomClocks = 0;
highp uint globalInvocationSMP = 0;
highp uint subHash = 0;


highp float floatConstruct( in uint m ) {
    return clamp(fract(uintBitsToFloat((m & 0x007FFFFFu) | 0x3F800000u)), 0.00001f, 0.99999f);
}

highp vec2 float2Construct( in uvec2 m ) {
    return clamp(vec2(floatConstruct(m.x), floatConstruct(m.y)), 0.00001f.xx, 0.99999f.xx);
}

highp vec2 half2Construct ( in uint m ) {
#ifdef ENABLE_AMD_INSTRUCTION_SET
    return clamp(vec2(fract(unpackFloat2x16((m & 0x03FF03FFu) | (0x3C003C00u)))), 0.00001f, 0.99999f);
#else
    return clamp(fract(unpackHalf2x16((m & 0x03FF03FFu) | (0x3C003C00u))), 0.00001f, 0.99999f);
#endif
}

// seeds hashers
highp uint hash ( in uint a ) {
   a = (a+0x7ed55d16) + (a<<12);
   a = (a^0xc761c23c) ^ (a>>19);
   a = (a+0x165667b1) + (a<<5);
   a = (a+0xd3a2646c) ^ (a<<9);
   a = (a+0xfd7046c5) + (a<<3);
   a = (a^0xb55a4f09) ^ (a>>16);
   return a;
}

// multi-dimensional seeds hashers
highp uint hash( in uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
highp uint hash( in uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
highp uint hash( in uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }



// aggregated randoms from seeds
highp float hrand( in uint   x ) { return floatConstruct(hash(x)); }
highp float hrand( in uvec2  v ) { return floatConstruct(hash(v)); }
highp float hrand( in uvec3  v ) { return floatConstruct(hash(v)); }
highp float hrand( in uvec4  v ) { return floatConstruct(hash(v)); }


// 1D random generators from superseed (quality)
highp float random( in uvec2 superseed ) {
    highp uint hclk = ++randomClocks;
    highp uint comb = hash(uvec3(hclk, subHash, uint(globalInvocationSMP)));
    return floatConstruct(hash(uvec3(comb, superseed)));
}

//  2D random generators from superseed (quality)
highp vec2 randf2x( in uvec2 superseed ) {
    highp uint hclk = ++randomClocks;
    highp uint comb = hash(uvec3(hclk, subHash, uint(globalInvocationSMP)));
    return half2Construct(hash(uvec3(comb, superseed)));
}

// 2D random generators from superseed 
highp vec2 randf2q( in uvec2 superseed ) {
    highp uint hclk = ++randomClocks;
    highp uint comb = hash(uvec3(hclk, subHash, uint(globalInvocationSMP)));
    return vec2(floatConstruct(hash(uvec2(comb, superseed.x))), floatConstruct(hash(uvec2(comb, superseed.y))));
}


// static aggregated randoms
highp float random() { return random(rayStreams[ rayBlock.samplerUniform.iterationCount ].superseed[0]); }
highp vec2 randf2q() { return randf2q(rayStreams[ rayBlock.samplerUniform.iterationCount ].superseed[0]); }
highp vec2 randf2x() { return randf2x(rayStreams[ rayBlock.samplerUniform.iterationCount ].superseed[0]); }


#define USE_HQUALITY_DIFFUSE


// geometric random generators
highp vec3 randomCosine(in uvec2 superseed) {
#ifdef USE_HQUALITY_DIFFUSE
    highp vec2 hmsm = randf2q(superseed);
#else
    highp vec2 hmsm = randf2x(superseed);
#endif
    highp float up = sqrt(1.f-hmsm.x), over = sqrt(1.f - up * up), around = hmsm.y * TWO_PI;
    return normalize(vec3( cos(around) * over, sin(around) * over, up ));
}


vec3 randomDirectionInSphere() {
#ifdef USE_HQUALITY_DIFFUSE
    highp vec2 hmsm = randf2q();
#else
    highp vec2 hmsm = randf2x();
#endif
    highp float up = (0.5f-hmsm.x)*2.f, over = sqrt(1.f - up * up), around = hmsm.y * TWO_PI;
    return normalize(vec3( cos(around) * over, sin(around) * over, up ));
}



highp vec3 randomCosineNormalOriented(in uvec2 superseed, in vec3 normal){
#ifdef USE_HQUALITY_DIFFUSE
    highp vec2 hmsm = randf2q(superseed);
#else
    highp vec2 hmsm = randf2x(superseed);
#endif
    highp float up = sqrt(1.f-hmsm.x), over = sqrt(1.f - up * up), around = hmsm.y * TWO_PI;

	highp vec3 directionNotNormal = vec3(0, 0, 1);
	if (abs(normal.x) < SQRT_OF_ONE_THIRD) { 
		directionNotNormal = vec3(1, 0, 0);
	} else if (abs(normal.y) < SQRT_OF_ONE_THIRD) { 
		directionNotNormal = vec3(0, 1, 0);
	}
	highp vec3 perpendicular1 = normalize( cross(normal, directionNotNormal) );
	highp vec3 perpendicular2 =            cross(normal, perpendicular1);
    return ( up * normal ) + ( cos(around) * over * perpendicular1 ) + ( sin(around) * over * perpendicular2 );
}

highp float qrand(in float r){ return random() < r ? 1.f : 0.f; }

#endif
