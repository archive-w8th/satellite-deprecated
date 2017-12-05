#ifndef _RANDOM_H
#define _RANDOM_H

uint randomClocks = 0;
uint globalInvocationSMP = 0;
uint subHash = 0;

uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y ^ hash(v.z))             ); }
uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y ^ hash(v.z ^ hash(v.w))) ); }

float floatConstruct( in uint m ) {
    uint ieeeMantissa = 0x007FFFFFu;
    uint ieeeOne = 0x3F800000u;
    m &= ieeeMantissa;
    m |= ieeeOne;
    return clamp(fract(uintBitsToFloat(m)), 0.f, 1.f);
}

float random( in uint   x ) { return floatConstruct(hash(x)); }
float random( in uvec2  v ) { return floatConstruct(hash(v)); }
float random( in uvec3  v ) { return floatConstruct(hash(v)); }
float random( in uvec4  v ) { return floatConstruct(hash(v)); }

float randomSeeded( in int superseed) {
#ifdef USE_ARB_CLOCK
    return random(uvec4( globalInvocationSMP << 6, clock2x32ARB(), subHash ));
#else
    uint hs = hash(uvec2(++randomClocks, superseed)); randomClocks = hs;
    return random(uvec3( globalInvocationSMP << 6, hs, subHash ));
#endif
}

float random() {
    return randomSeeded(rayStreams[0].superseed.x);
}


vec3 randomCosine(in vec3 normal) {
     float up = sqrt(random());
     float over = sqrt(1.f - up * up);
     float around = random() * TWO_PI;

    vec3 perpendicular0 = vec3(0, 0, 1);
    if (abs(normal.x) < SQRT_OF_ONE_THIRD) {
        perpendicular0 = vec3(1, 0, 0);
    } else if (abs(normal.y) < SQRT_OF_ONE_THIRD) {
        perpendicular0 = vec3(0, 1, 0);
    }

     vec3 perpendicular1 = normalize( cross(normal, perpendicular0) );
     vec3 perpendicular2 =            cross(normal, perpendicular1);
    return normalize(
        fma(normal, vec3(up),
            fma( perpendicular1 , vec3(cos(around)) * over,
                 perpendicular2 * vec3(sin(around)) * over
            )
        )
    );
}

vec3 randomCosine(in vec3 normal, in int superseed) {
  vec3 rnd3 = vec3(randomSeeded(superseed), randomSeeded(superseed), randomSeeded(superseed));
  float r = fma(rnd3.x, 0.5f, 0.5f);
  float angle = fma(rnd3.y, PI, PI);
  float sr = sqrt(r);
  vec2 p = sr*vec2(cos(angle),sin(angle));
  vec3 ph = vec3(p.xy, sqrt(1.0f - p*p));
  vec3 tangent = normalize(rnd3);
  vec3 bitangent = cross(tangent, normal);
  tangent = cross(bitangent, normal);
  return normalize(fma(tangent, ph.xxx, fma(bitangent, ph.yyy, normal * ph.z)));


    /*
     float up = sqrt(randomSeeded(superseed));
     float over = sqrt(1.f - up * up);
     float around = randomSeeded(superseed) * TWO_PI;

    vec3 perpendicular0 = vec3(0, 0, 1);
    if (abs(normal.x) < SQRT_OF_ONE_THIRD) {
        perpendicular0 = vec3(1, 0, 0);
    } else if (abs(normal.y) < SQRT_OF_ONE_THIRD) {
        perpendicular0 = vec3(0, 1, 0);
    }

     vec3 perpendicular1 = normalize( cross(normal, perpendicular0) );
     vec3 perpendicular2 =            cross(normal, perpendicular1);
    return normalize(
        fma(normal, vec3(up),
            fma( perpendicular1 , vec3(cos(around)) * over,
                 perpendicular2 * vec3(sin(around)) * over
            )
        )
    );*/
}



vec3 randomDirectionInSphere() {
     float up = fma(random(), 2.0f, -1.0f);
     float over = sqrt(1.f - up * up);
     float around = random() * TWO_PI;
    return normalize(vec3( up, cos(around) * over, sin(around) * over ));
}

#endif
