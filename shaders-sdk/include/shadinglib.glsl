#ifndef _SHADINGLIB_H
#define _SHADINGLIB_H
#define GAP (PZERO*2.f)


float computeFresnel(in vec3 normal, in vec3 indc, in float n1, in float n2) {
    float cosi = dot(normal,  normalize(indc));
    float cost = dot(normal,  normalize(refract(indc, normal, n1 / n2)));
    float Rs = fma(n1, cosi, - n2 * cost) / fma(n1, cosi, n2 * cost);
    float Rp = fma(n1, cost, - n2 * cosi) / fma(n1, cost, n2 * cosi);
    return sqrt(clamp(sqlen(vec2(Rs, Rp)) * 0.5f, 0.0f, 1.0f));
}


vec3 lightCenter(in int i) {
    return fma(normalize(lightUniform.lightNode[i].lightVector.xyz), lightUniform.lightNode[i].lightVector.www, lightUniform.lightNode[i].lightOffset.xyz);
}


vec3 sLight(in int i) {
    return fma(randomDirectionInSphere(), vec3(lightUniform.lightNode[i].lightColor.w - 0.0001f), lightCenter(i));
}


float intersectSphere(in vec3 origin, in vec3 ray, in vec3 sphereCenter, in float sphereRadius) {
    vec3 toSphere = origin - sphereCenter;
    float a = dot(ray, ray);
    float b = 2.0f * dot(toSphere, ray);
    float c = dot(toSphere, toSphere) - sphereRadius*sphereRadius;
    float discriminant = fma(b,b,-4.0f*a*c);
    float t = INFINITY;
    if (discriminant > 0.0f) {
        float da = 0.5f / a;
        float t1 = (-b - sqrt(discriminant)) * da;
        float t2 = (-b + sqrt(discriminant)) * da;
        float mn = min(t1, t2);
        float mx = max(t1, t2);
        t = mx >= 0.0f ? (mn >= 0.0f ? mn : mx) : t;
    }
    return t;
}


float modularize(in float f) {
    return 1.0f-sqrt(max(1.0f - f, 0.f));
}


float samplingWeight(in vec3 ldir, in vec3 ndir, in float radius, in float dist) {
    return modularize(max(dot(ldir, ndir), 0.f) * pow(radius / dist, 2.f)) * 2.f;
}


RayRework directLight(in int i, in RayRework directRay, in vec3 color, in mat3 tbn) {
    RayActived(directRay, RayType(directRay) == 2 ? FALSE_ : RayActived(directRay));
    RayTargetLight(directRay, i);
    RayDiffBounce(directRay, min(1, max(RayDiffBounce(directRay)-1,0)));
    RayBounce(directRay, min(RayBounce(directRay), 1)); // incompatible with reflections and diffuses
    RayType(directRay, 2);

    vec3 lpath = sLight(i) - directRay.origin.xyz;
    vec3 ldirect = normalize(lpath);
    float dist = length(lightCenter(i).xyz - directRay.origin.xyz);
    float weight = samplingWeight(ldirect, tbn[2], lightUniform.lightNode[i].lightColor.w, dist);

    directRay.cdirect.xy = lcts(ldirect);
    directRay.origin.xyz = fma(ldirect.xyz, vec3(GAP), directRay.origin.xyz);
    _writeColor(directRay.dcolor, f16_f32(directRay.dcolor) * vec4(color,1.f) * vec4(weight.xxx,1.f));

    IF (lessF(dot(ldirect.xyz, tbn[2]), 0.f)) {
        RayActived(directRay, FALSE_); // wrong direction, so invalid
    }

    // any trying will fail when flag not enabled
#ifndef DIRECT_LIGHT_ENABLED
    RayActived(directRay, FALSE_);
#endif

    // inactived can't be shaded
    IF (not(RayActived(directRay))) _writeColor(directRay.dcolor, 0.f.xxxx);

    return directRay;
}


vec3 normalOrient(in vec3 runit, in mat3 tbn){
    //vec3 btn = cross(runit, tbn[2]), tng = cross(btn, tbn[2]); btn = cross(tng, tbn[2]);
    //return normalize(mat3(tng, btn, tbn[2]) * runit);
    return normalize(tbn * runit);
}


RayRework diffuse(in RayRework ray, in vec3 color, in mat3 tbn) {
    _writeColor(ray.dcolor, f16_f32(ray.dcolor) * vec4(color,1.f));

    const int diffuse_reflections = 2;
    RayActived(ray, RayType(ray) == 2 ? FALSE_ : RayActived(ray));
    RayDiffBounce(ray, min(diffuse_reflections, max(RayDiffBounce(ray)-1,0)));

    vec3 sdr = normalOrient(randomCosine(rayStreams[RayBounce(ray)].superseed.x), tbn);
    sdr = faceforward(sdr, sdr, -tbn[2]);
    ray.cdirect.xy = lcts(sdr);
    ray.origin.xyz = fma(sdr, vec3(GAP), ray.origin.xyz);

    if (RayType(ray) != 2) RayType(ray, 1);

    // inactived can't be shaded
    IF (not(RayActived(ray))) _writeColor(ray.dcolor, 0.f.xxxx);

    return ray;
}


RayRework promised(in RayRework ray, in mat3 tbn) {
    ray.origin.xyz = fma(dcts(ray.cdirect.xy), vec3(GAP), ray.origin.xyz);
    return ray;
}


RayRework emissive(in RayRework ray, in vec3 color, in mat3 tbn) {
    _writeColor(ray.dcolor, max(f16_f32(ray.dcolor) * vec4(color,1.f), vec4(0.0f)));
    _writeColor(ray.dcolor, RayType(ray) == 2 ? 0.0f.xxxx : f16_f32(ray.dcolor));
    ray.origin.xyz = fma(dcts(ray.cdirect.xy), vec3(GAP), ray.origin.xyz);
    RayBounce(ray, 0);
    RayActived(ray, FALSE_);
    return ray;
}


RayRework reflection(in RayRework ray, in vec3 color, in mat3 tbn, in float refly) {
    _writeColor(ray.dcolor, f16_f32(ray.dcolor) * vec4(color, 1.f));

    // bounce mini-config
#ifdef USE_SIMPLIFIED_MODE
    const int caustics_bounces = 0, reflection_bounces = 1;
#else
    const int caustics_bounces = 0, reflection_bounces = 2;
#endif

    if (RayType(ray) != 2) RayType(ray, 0); // reflection ray transfer (primary)
    RayBounce(ray, min(RayType(ray) == 1 ? caustics_bounces : reflection_bounces, max(RayBounce(ray)-1, 0)));

    vec3 sdr = normalOrient(randomCosine(rayStreams[RayBounce(ray)].superseed.x), tbn);
    sdr = faceforward(sdr, sdr, -tbn[2]);
    sdr = normalize(fmix(reflect(dcts(ray.cdirect.xy), tbn[2]), sdr, clamp(sqrt(random()) * (refly), 0.0f, 1.0f).xxx));
    sdr = faceforward(sdr, sdr, -tbn[2]);

    ray.cdirect.xy = lcts(sdr);
    ray.origin.xyz = fma(sdr, vec3(GAP), ray.origin.xyz);
    RayActived(ray, RayType(ray) == 2 ? FALSE_ : RayActived(ray));

    // inactived can't be shaded
    IF (not(RayActived(ray))) _writeColor(ray.dcolor, 0.f.xxxx);

    return ray;
}


// for future purpose
RayRework refraction(in RayRework ray, in vec3 color, in mat3 tbn, in float inior, in float outior, in float glossiness) {
    vec3 refrDir = normalize(  refract( dcts(ray.cdirect.xy) , tbn[2], inior / outior)  );
    BOOL_ directDirc = equalF(inior, outior), isShadowed = BOOL_(RayType(ray) == 2);

#ifdef REFRACTION_SKIP_SUN
    IF (not(isShadowed | directDirc)) {
        ray.cdirect.xy = lcts(refrDir);
        RayBounce(ray, max(RayBounce(ray)-1, 0));
    }
#else
    IF (not(directDirc)) { // if can't be directed
        ray.cdirect.xy = lcts(refrDir);
        RayBounce(ray, max(RayBounce(ray)-1, 0));
        if ( isShadowed) RayActived(ray, FALSE_); // trying to refract shadows will fails
        if (!isShadowed) RayType(ray, 0); // can be lighted by direct
    }
#endif

    ray.origin.xyz = fma(refrDir, vec3(GAP), ray.origin.xyz);
    _writeColor(ray.dcolor, f16_f32(ray.dcolor) * vec4(color, 1.f));

    // inactived can't be shaded
    IF (not(RayActived(ray))) _writeColor(ray.dcolor, 0.f.xxxx);

    return ray;
}


vec3 getLightColor(in int lc) {
    return max(lightUniform.lightNode[lc].lightColor.xyz, vec3(0.f));
}

#endif
