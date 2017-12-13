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

vec3 glossy(in vec3 dir, in vec3 normal, in float refli) {
    return normalize(fmix(normalize(dir), randomCosine(normal), clamp(sqrt(random()) * refli, 0.0f, 1.0f).xxx));
}

vec3 lightCenter(in int i) {
    vec3 playerCenter = vec3(0.0f);
    vec3 lvec = normalize(lightUniform.lightNode[i].lightVector.xyz) * (lightUniform.lightNode[i].lightVector.y < 0.0f ? -1.0f : 1.0f);
    return fma(lvec, vec3(lightUniform.lightNode[i].lightVector.w), (lightUniform.lightNode[i].lightOffset.xyz + playerCenter.xyz));
}

vec3 sLight(in int i) {
    //return fma(randomDirectionInSphere(), vec3(lightUniform.lightNode[i].lightColor.w - 0.0001f), lightCenter(i));
    return lightUniform.lightNode[i].lightRandomizedOrigin.xyz;
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

RayRework directLight(in int i, in RayRework directRay, in vec3 color, in vec3 normal) {
    RayActived(directRay, RayType(directRay) == 2 ? FALSE_ : RayActived(directRay));
    RayDL(directRay, TRUE_);
    RayTargetLight(directRay, i);
    RayBounce(directRay, min(1, max(RayBounce(directRay) - (RayType(directRay) == 0 ? 0 : 1), 0)));
    RayType(directRay, 2);

    vec3 lpath = sLight(i) - directRay.origin.xyz;
    vec3 ldirect = normalize(lpath);
    float dist = length(lightCenter(i).xyz - directRay.origin.xyz);
    float weight = samplingWeight(ldirect, normal, lightUniform.lightNode[i].lightColor.w, dist);

    directRay.direct.xyz = ldirect;
    directRay.color.xyz *= color * weight;
    directRay.final.xyz *= 0.f;
    directRay.origin.xyz = fma(directRay.direct.xyz, vec3(GAP), directRay.origin.xyz);

    IF (lessF(dot(directRay.direct.xyz, normal), 0.f)) RayActived(directRay, FALSE_); // wrong direction, so invalid

    // any trying will fail when flag not enabled
#ifndef DIRECT_LIGHT_ENABLED
    RayActived(directRay, FALSE_);
#endif

    return directRay;
}

RayRework diffuse(in RayRework ray, in vec3 color, in vec3 normal) {
    ray.color.xyz *= color;
    ray.final.xyz *= 0.f;

    const int diffuse_reflections = 1; // i.e. support only emissions, even no support caustics 
    RayActived(ray, RayType(ray) == 2 ? FALSE_ : RayActived(ray));
    RayBounce(ray, min(diffuse_reflections, max(RayBounce(ray) - (RayType(ray) == 0 ? 0 : 1), 0)));

    //vec3 sdr = rayStreams[RayBounce(ray)].diffuseStream.xyz;
    vec3 sdr = rayStreams[int(floor(16.f * random(rayStreams[RayBounce(ray)].superseed.x)))].diffuseStream.xyz; // experimental random choiced selection
    //vec3 sdr = randomCosine(normal, rayStreams[RayBounce(ray)].superseed.x);
    ray.direct.xyz = faceforward(sdr, sdr, -normal);
    ray.origin.xyz = fma(ray.direct.xyz, vec3(GAP), ray.origin.xyz);
    ray.color.xyz *= fmix(0.f, 1.f, max(dot(normal, ray.direct.xyz), 0.f)) * 2.f;

    if (RayType(ray) != 2) RayType(ray, 1);
#ifdef DIRECT_LIGHT_ENABLED
    RayDL(ray, FALSE_);
#else
    RayDL(ray, TRUE_);
#endif
    return ray;
}

RayRework promised(in RayRework ray, in vec3 normal) {
    ray.origin.xyz = fma(ray.direct.xyz, vec3(GAP), ray.origin.xyz);
    return ray;
}

RayRework emissive(in RayRework ray, in vec3 color, in vec3 normal) {
    ray.final.xyz = max(ray.color.xyz * color, vec3(0.0f));
    ray.final = RayType(ray) == 2 ? vec4(0.0f) : ray.final;
    ray.color.xyz *= 0.0f;
    ray.direct.xyz = normalize(randomCosine(normal));
    ray.origin.xyz = fma(ray.direct.xyz, vec3(GAP), ray.origin.xyz);
    RayBounce(ray, 0);
    RayActived(ray, FALSE_);
    RayDL(ray, FALSE_);
    return ray;
}

RayRework reflection(in RayRework ray, in vec3 color, in vec3 normal, in float refly) {
    ray.color.xyz *= color;
    ray.final.xyz *= 0.f;

    // bounce mini-config
    const int caustics_bounces = 1, reflection_bounces = 1;

    //if (RayType(ray) == 3) RayDL(ray, TRUE_); // specular color
    if (RayType(ray) == 1) RayDL(ray, BOOL_(SUNLIGHT_CAUSTICS)); // caustics
    if (RayType(ray) != 2) RayType(ray, 0); // reflection ray transfer (primary)
    RayBounce(ray, min(RayType(ray) == 1 ? caustics_bounces : reflection_bounces, max(RayBounce(ray)-1, 0)));

    //vec3 sdr = rayStreams[RayBounce(ray)].diffuseStream.xyz;
    vec3 sdr = rayStreams[int(floor(16.f * random(rayStreams[RayBounce(ray)].superseed.x)))].diffuseStream.xyz; // experimental random choiced selection
    //vec3 sdr = randomCosine(normal, rayStreams[RayBounce(ray)].superseed.x);

    //ray.direct.xyz = normalize(fmix(reflect(ray.direct.xyz, normal), faceforward(sdr, sdr, -normal), clamp(refly * sqrt(random()), 0.0f, 1.0f)));
    //ray.direct.xyz = normalize(fmix(reflect(ray.direct.xyz, normal), faceforward(sdr, sdr, -normal), clamp(refly * random(), 0.0f, 1.0f)));
    sdr = normalize(fmix(reflect(ray.direct.xyz, normal), sdr, clamp(random() * modularize(refly), 0.0f, 1.0f).xxx));
    //sdr = normalize(fmix(reflect(ray.direct.xyz, normal), sdr, clamp(random() * (refly), 0.0f, 1.0f).xxx));
    ray.direct.xyz = faceforward(sdr, sdr, -normal);
    ray.origin.xyz = fma(ray.direct.xyz, vec3(GAP), ray.origin.xyz);


    RayActived(ray, RayType(ray) == 2 ? FALSE_ : RayActived(ray));
    return ray;
}

// for future purpose
RayRework refraction(in RayRework ray, in vec3 color, in vec3 normal, in float inior, in float outior, in float glossiness) {
    vec3 refrDir = normalize(  refract(ray.direct.xyz, normal, inior / outior)  );
    BOOL_ directDirc = equalF(inior, outior), isShadowed = BOOL_(RayType(ray) == 2);

#ifdef REFRACTION_SKIP_SUN
    IF (not(isShadowed | directDirc)) {
        ray.direct.xyz = refrDir;
        RayBounce(ray, max(RayBounce(ray)-1, 0));
    }
#else
    IF (not(directDirc)) { // if can't be directed
        ray.direct.xyz = refrDir;
        RayBounce(ray, max(RayBounce(ray)-1, 0));
        //if (RayType(ray) == 3) RayDL(ray, TRUE_); // specular color
        if (RayType(ray) == 1) RayDL(ray, BOOL_(SUNLIGHT_CAUSTICS)); // caustics
        if ( isShadowed) RayActived(ray, FALSE_); // trying to refract shadows will fails
        if (!isShadowed) RayType(ray, 0); // can be lighted by direct
    }
#endif

    ray.origin.xyz = fma(ray.direct.xyz, vec3(GAP), ray.origin.xyz);
    ray.color.xyz *= color;
    return ray;
}

vec3 getLightColor(in int lc) {
    return max(lightUniform.lightNode[lc].lightColor.xyz, vec3(0.f));
}

#endif
