
/* Atmosphere sky functions and parameters */
#define up vec3(0.0f, 1.0f, 0.0f)
#define atmosphereColor vec3(0.2, 0.5, 0.6)
#define rayleighAmmount .06*0.5
#define mieAmmount .002*0.5
#define atmosphereHeight .4*0.2
#define earthRadius 14.6*0.2
#define getThickness(a) js_getThickness(a)
#define getEarth(a) clamp( dot(up,a)*10.+1. ,0.,1.)
#define phaseRayleigh(a) (.059683104 * (1. + a * a))
#define phaseMie(a) (.0348151 / pow(1.5625 - 1.5*a,1.5))
#define absorb(a) (1. - pow(atmosphereColor, vec3(1./(a))))

layout ( binding = 0, set = 1 ) uniform sampler2D skybox[1];


vec4 readEnv(in vec2 ds) {
    vec2 tx2 = ((ds / PI - vec2(0.5f, 0.0f)) * vec2(0.5f, 1.0f));
    tx2.y = 1.f-tx2.y;
    return texture(skybox[0], tx2);
}


float js_getThickness(in vec3 rd) {
     float sr = earthRadius+atmosphereHeight;
     vec3 ro = -up*earthRadius;
     float b = dot(rd, ro);
     float c = dot(ro, ro) - sr*sr;
     float t = b*b - c;
    return b + sqrt(t);
}

vec3 saturate(in vec3 c, in float s) {
    return c*s+dot(c, vec3(.2125, .7154, .0721) )*(1 - s);
}

vec3 js_getScatter(in vec3 V, in vec3 L, in float lightStrength) {
     float thicknessV = getThickness(V), dotVL = dot(V, L);
     vec3 sunColor = lightStrength * absorb(getThickness(L)) * getEarth(L);
     vec3 rayleighGradient = atmosphereColor * rayleighAmmount * phaseRayleigh(dotVL);
     float mieGradient = mieAmmount * phaseMie(dotVL);
    vec3 skyColor = (mieGradient + rayleighGradient) * absorb(thicknessV) * sunColor * thicknessV;
    skyColor += pow(max(dotVL,0.), 7000.) * sunColor; //sun
    return saturate(skyColor,1.3)*vec3(1,1,1.35);
}

vec3 lightCenterSky(in int i) {
    vec3 playerCenter = vec3(0.0f);
    vec3 lvec = normalize(lightUniform.lightNode[i].lightVector.xyz) * 10000.0f;
    return lightUniform.lightNode[i].lightOffset.xyz + lvec + playerCenter.xyz;
}

void env(inout vec4 color, in RayRework ray) {
    //color = readEnv(dts(ray.cdirect.xy));
    color = readEnv(ray.cdirect.xy);
    //color = fromLinear(color); // HDR support (gamma correct)
    color = clamp(color, vec4(0.f.xxxx), vec4(2.f,2.f,2.f,1.f));
    
    //vec3 lcenter = lightCenterSky(0);
    //color.xyz = js_getScatter(ray.cdirect.xyz, -normalize(lcenter - ray.origin.xyz), 800.0f) +
    //            js_getScatter(ray.cdirect.xyz,  normalize(lcenter - ray.origin.xyz), 4000.0f);
    //color = clamp((1.0f - exp(-1.0f * color)), vec4(0.0f), vec4(1.0f));
}

#define EnvironmentShader env