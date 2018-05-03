
vec2 intersectCube(in vec3 origin, in vec3 ray, in vec4 cubeMin, in vec4 cubeMax) {
    vec3 dr = 1.0f / ray;
    vec3 tMin = (cubeMin.xyz - origin) * dr;
    vec3 tMax = (cubeMax.xyz - origin) * dr;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max3_wrap(t1.x, t1.y, t1.z);
    float tFar  = min3_wrap(t2.x, t2.y, t2.z);
    bool_ isCube = lessEqualF(tNear, tFar);
    return SSC(isCube) ? vec2(min(tNear, tFar), max(tNear, tFar)) : vec2(INFINITY);
}

vec4 points[6];

// check if point is containing
bool_ isContain(in vec4 point, in bbox abox){
    return (
        lessEqualF(point.x, abox.mx.x) & greaterEqualF(point.x, abox.mn.x) & 
        lessEqualF(point.y, abox.mx.y) & greaterEqualF(point.y, abox.mn.y) & 
        lessEqualF(point.z, abox.mx.z) & greaterEqualF(point.z, abox.mn.z)
    );
}

// compaction box by triangle
bbox compactBoxByTriangle(in bbox abox, in mat3x4 triverts){
    // triangle vectors
    vec3 e0 = normalize(triverts[1].xyz - triverts[0].xyz);
    vec3 e1 = normalize(triverts[2].xyz - triverts[1].xyz);
    vec3 e2 = normalize(triverts[0].xyz - triverts[2].xyz);

    float l0 = length(triverts[1].xyz - triverts[0].xyz);
    float l1 = length(triverts[2].xyz - triverts[1].xyz);
    float l2 = length(triverts[0].xyz - triverts[2].xyz);

    // box distances
    vec2 d0 = intersectCube(triverts[0].xyz, e0.xyz, abox.mn, abox.mx);
    vec2 d1 = intersectCube(triverts[1].xyz, e1.xyz, abox.mn, abox.mx);
    vec2 d2 = intersectCube(triverts[2].xyz, e2.xyz, abox.mn, abox.mx);

    uint pcount = 0;

    if (d0.x < INFINITY) {
        vec4 p0 = vec4(triverts[0].xyz + e0.xyz * sign(d0.x) * min(abs(d0.x), l0), 1.0f);
        vec4 p1 = vec4(triverts[0].xyz + e0.xyz * sign(d0.y) * min(abs(d0.y), l0), 1.0f);

        IF (isContain(p0, abox) & greaterEqualF(d0.x, 0.f)) points[pcount++] = p0;
        IF (isContain(p1, abox) & greaterEqualF(d0.y, 0.f)) points[pcount++] = p1;
    }

    if (d1.x < INFINITY) {
        vec4 p0 = vec4(triverts[1].xyz + e1.xyz * sign(d1.x) * min(abs(d1.x), l1), 1.0f);
        vec4 p1 = vec4(triverts[1].xyz + e1.xyz * sign(d1.y) * min(abs(d1.y), l1), 1.0f);

        IF (isContain(p0, abox) & greaterEqualF(d1.x, 0.f)) points[pcount++] = p0;
        IF (isContain(p1, abox) & greaterEqualF(d1.y, 0.f)) points[pcount++] = p1;
    }

    if (d2.x < INFINITY) {
        vec4 p0 = vec4(triverts[2].xyz + e2.xyz * sign(d2.x) * min(abs(d2.x), l2), 1.0f);
        vec4 p1 = vec4(triverts[2].xyz + e2.xyz * sign(d2.y) * min(abs(d2.y), l2), 1.0f);

        IF (isContain(p0, abox) & greaterEqualF(d2.x, 0.f)) points[pcount++] = p0;
        IF (isContain(p1, abox) & greaterEqualF(d2.y, 0.f)) points[pcount++] = p1;
    }

    bbox result;
    result.mn =  vec4(100000.f);
    result.mx = -vec4(100000.f);

    for (int i=0;i<pcount;i++) {
        result.mn = min(points[i], result.mn);
        result.mx = max(points[i], result.mx);
    }

    // clip box by original 
    result.mn = max(result.mn, abox.mn);
    result.mx = min(result.mx, abox.mx);
    
    return result;
}