const int NUM_STEPS = 128;
const float EPSILON	= 1e-5f;

// sea
const int ITER_GEOMETRY = 3;
const int ITER_FRAGMENT = 5;
const float SEA_HEIGHT = 0.6f;
const float SEA_CHOPPY = 4.0f;
const float SEA_SPEED = 0.8f;
const float SEA_FREQ = 0.16f;
const vec3 SEA_BASE = vec3(0.1f, 0.19f, 0.22f);
const vec3 SEA_WATER_COLOR = vec3(0.8f, 0.9f, 0.6f);
#define SEA_TIME (1.0 + (RAY_BLOCK cameraUniform.ftime * 4.f) * SEA_SPEED)
const mat2 octave_m = mat2(1.6f, 1.2f, -1.2f, 1.6f);

// math
mat3 fromEuler(vec3 ang) {
	vec2 a1 = vec2(sin(ang.x),cos(ang.x));
    vec2 a2 = vec2(sin(ang.y),cos(ang.y));
    vec2 a3 = vec2(sin(ang.z),cos(ang.z));
    mat3 m;
    m[0] = vec3(a1.y*a3.y+a1.x*a2.x*a3.x,a1.y*a2.x*a3.x+a3.y*a1.x,-a2.y*a3.x);
	m[1] = vec3(-a2.y*a1.x,a1.y*a2.y,a2.x);
	m[2] = vec3(a3.y*a1.x*a2.x+a1.y*a3.x,a1.x*a3.x-a1.y*a3.y*a2.x,a2.y*a3.y);
	return m;
}

float hash( vec2 p ) {
	float h = dot(p,vec2(127.1f, 311.7f));	
    return fract(sin(h)*43758.5453123f);
}

float noise( in vec2 p ) {
    vec2 i = floor( p );
    vec2 f = fract( p );	
	vec2 u = f*f*(3.0f - 2.0f * f);
    return -1.0+2.0*mix( mix( hash( i + vec2(0.0f, 0.0f) ), 
                     hash( i + vec2(1.0f, 0.0f) ), u.x),
                mix( hash( i + vec2(0.0f, 1.0f) ), 
                     hash( i + vec2(1.0f, 1.0f) ), u.x), u.y);
}

// sea
float sea_octave(vec2 uv, float choppy) {
    uv += noise(uv);        
    vec2 wv = 1.0-abs(sin(uv));
    vec2 swv = abs(cos(uv));    
    wv = mix(wv,swv,wv);
    return pow(1.0-pow(wv.x * wv.y, 0.65f),choppy);
}

float map(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;
    vec2 uv = p.xz; uv.x *= 0.75f;
    
    float d, h = 0.0f;    
    for(int i = 0; i < ITER_GEOMETRY; i++) {        
    	d = sea_octave((uv+SEA_TIME)*freq,choppy);
    	d += sea_octave((uv-SEA_TIME)*freq,choppy);
        h += d * amp;        
    	uv *= octave_m; freq *= 1.9f; amp *= 0.22f;
        choppy = mix(choppy, 1.0f, 0.2f);
    }
    return p.y - h;
}

float map_detailed(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;
    vec2 uv = p.xz; uv.x *= 0.75f;
    
    float d, h = 0.0;    
    for(int i = 0; i < ITER_FRAGMENT; i++) {        
    	d = sea_octave((uv+SEA_TIME)*freq,choppy);
    	d += sea_octave((uv-SEA_TIME)*freq,choppy);
        h += d * amp;        
    	uv *= octave_m; freq *= 1.9f; amp *= 0.22f;
        choppy = mix(choppy, 1.0f, 0.2f);
    }
    return p.y - h;
}

// tracing
vec3 getNormal(vec3 p, float eps) {
    vec3 n;
    n.y = map_detailed(p);    
    n.x = map_detailed(vec3(p.x+eps,p.y,p.z)) - n.y;
    n.z = map_detailed(vec3(p.x,p.y,p.z+eps)) - n.y;
    n.y = eps;
    return normalize(n);
}



float heightMapTracing(in vec3 orig, in vec3 dir) {
    float ts = 0.f;
    BOOL_ ofound = FALSE_;
    //const float TX = INFINITY;
    #define TX INFINITY
    float d = TX, od = d;
    orig.y += 0.5f;
    for (int i = 0; i < NUM_STEPS; i++) {
        od = d, d = map(orig + dir * max(abs(ts), EPSILON)); ts += d; // get distance by height
        ofound = ofound | BOOL_(abs(d) < EPSILON);
        BOOL_ parallelIssue = BOOL_(abs(d - od) < 0.0001f);
        IF ( greaterEqualF(abs(ts), TX) | ofound | parallelIssue ) break;
    }
    return SSC(lessF(abs(ts), TX) & ofound) ? max(abs(ts), EPSILON) : INFINITY;
}

void addOcean(inout bool overflow) {
    if (!overflow) {
        float dist = heightMapTracing(currentRay.origin.xyz, currentRay.direct.xyz);
        IF (lessF(dist, compositedHit[0].uvt.z)) {
            proceduralID = 0;
            wasHit = 0;
            compositedHit[0].uvt.z = dist;
            compositedHit[0].normalHeight = vec4(getNormal(currentRay.origin.xyz + currentRay.direct.xyz * dist, EPSILON), 1.f);
        }
    }
}
