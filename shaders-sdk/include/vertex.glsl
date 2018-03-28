#ifndef _VERTEX_H
#define _VERTEX_H

#include "../include/mathlib.glsl"

// enable this data for interpolation meshes
#ifdef ENABLE_VERTEX_INTERPOLATOR
#ifndef ENABLE_VSTORAGE_DATA
#define ENABLE_VSTORAGE_DATA
#endif
#endif

// for geometry accumulators
#ifdef VERTEX_FILLING
    layout ( std430, binding = 0, set = 0 ) restrict buffer BuildCounters { int tcounter[1]; };
    layout ( std430, binding = 1, set = 0 ) restrict buffer GeomMaterialsSSBO { int materials[]; };
    layout ( std430, binding = 2, set = 0 ) restrict buffer OrderIdxSSBO { int vorders[]; };
    layout ( std430, binding = 3, set = 0 ) restrict buffer VertexLinearSSBO { float lvtx[]; };
    layout ( rgba32f, binding = 4, set = 0 ) uniform image2D attrib_texture_out;
#else
    layout ( std430, binding = 1, set = 1 ) readonly buffer GeomMaterialsSSBO { int materials[]; };

    #ifdef ENABLE_VERTEX_INTERPOLATOR
        layout ( binding = 10, set = 1 ) uniform sampler2D attrib_texture;
        layout ( std430, binding = 2, set = 1 ) readonly buffer OrderIdxSSBO { int vorders[]; };
    #endif

    #ifdef ENABLE_VSTORAGE_DATA
        #ifdef ENABLE_TRAVERSE_DATA
        #ifndef BVH_CREATION
            #ifdef USE_F32_BVH
            layout ( std430, binding = 0, set = 1 ) readonly buffer BVHBoxBlock { vec4 bvhBoxes[][4]; };
            #else
            layout ( std430, binding = 0, set = 1 ) readonly buffer BVHBoxBlock { uvec2 bvhBoxes[][4]; }; 
            #endif
            layout ( binding = 5, set = 1 ) uniform isampler2D bvhStorage;
        #endif
        #endif
        
        layout ( std430, binding = 3, set = 1 ) readonly buffer GeometryBlockUniform { GeometryUniformStruct geometryUniform;} geometryBlock;
        #ifdef VTX_TRANSPLIT // for leaf gens
            layout ( std430, binding = 7, set = 1 ) restrict buffer VertexLinearSSBO { float lvtx[]; };
        #else
            layout ( std430, binding = 7, set = 1 ) readonly buffer VertexLinearSSBO { float lvtx[]; };
        #endif
    #endif
#endif

const int ATTRIB_EXTENT = 4;

// attribute formating
const int NORMAL_TID = 0;
const int TEXCOORD_TID = 1;
const int COLOR_TID = 2; // unused
const int MODF_TID = 3; // at now no supported 

#ifdef ENABLE_AMD_INSTRUCTION_SET
#define ISTORE(img, crd, data) imageStoreLodAMD(img, crd, 0, data)
#define SGATHER(smp, crd, chnl) textureGatherLodAMD(smp, crd, 0, chnl)
#else
#define ISTORE(img, crd, data) imageStore(img, crd, data)
#define SGATHER(smp, crd, chnl) textureGather(smp, crd, chnl)
#endif

//#define _SWIZV wzx
#define _SWIZV xyz

const int WARPED_WIDTH = 2048;
//const ivec2 mit[3] = {ivec2(0,0), ivec2(1,0), ivec2(0,1)};
const ivec2 mit[3] = {ivec2(0,1), ivec2(1,1), ivec2(1,0)};

ivec2 mosaicIdc(in ivec2 mosaicCoord, in int idc) {
#ifdef VERTEX_FILLING
    mosaicCoord.x %= int(imageSize(attrib_texture_out).x);
#endif
    return mosaicCoord + mit[idc];
}

ivec2 gatherMosaic(in ivec2 uniformCoord) {
    return ivec2(uniformCoord.x * 3 + uniformCoord.y % 3, uniformCoord.y);
}

vec4 fetchMosaic(in sampler2D vertices, in ivec2 mosaicCoord, in uint idc) {
    //return texelFetch(vertices, mosaicCoord + mit[idc], 0);
    return textureLod(vertices, (vec2(mosaicCoord + mit[idc]) + 0.49999f) / textureSize(vertices, 0), 0); // supper native warping
}

ivec2 getUniformCoord(in int indice) {
    return ivec2(indice % WARPED_WIDTH, indice / WARPED_WIDTH);
}

ivec2 getUniformCoord(in uint indice) {
    return ivec2(indice % WARPED_WIDTH, indice / WARPED_WIDTH);
}


#ifndef VERTEX_FILLING
#ifndef BVH_CREATION
#ifdef ENABLE_VSTORAGE_DATA
/*
float intersectTriangle(const vec3 orig, const mat3 M, const int axis, const int tri, inout vec2 UV, inout bool_ _valid, const float testdist) {
    float T = INFINITY;
    //IFANY (_valid) {
        bool_ valid = tri < 0 ? false_ : _valid; // pre-define
        const vec3 D[3] = {vec3(1.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f), vec3(0.f, 0.f, 1.f)};
        //const vec2 sz = 1.f / textureSize(vertex_texture, 0), hs = sz * 0.9999f;
        IFANY (valid) {
            // gather patterns
            const int itri = tri*9;
            const mat3 ABC = (mat3(
                vec3(lvtx[itri+0], lvtx[itri+1], lvtx[itri+2]),
                vec3(lvtx[itri+3], lvtx[itri+4], lvtx[itri+5]),
                vec3(lvtx[itri+6], lvtx[itri+7], lvtx[itri+8])
            )-mat3(orig.xxx, orig.yyy, orig.zzz))*M;

            // PURE watertight triangle intersection (our, GPU-GLSL adapted version)
            // http://jcgt.org/published/0002/01/05/paper.pdf
            vec3 UVW_ = D[axis] * inverse(ABC);
            valid &= bool_(all(greaterThanEqual(UVW_, vec3(0.f))) || all(lessThanEqual(UVW_, vec3(0.f))));
            IFANY (valid) {
                float det = dot(UVW_,vec3(1)); UVW_ *= 1.f/(max(abs(det),0.00001f)*(det>=0.f?1:-1));
                UV = vec2(UVW_.yz), UVW_ *= ABC; // calculate axis distances
                T = mix(mix(UVW_.z, UVW_.y, axis == 1), UVW_.x, axis == 0);
                T = mix(INFINITY, T, greaterEqualF(T, 0.0f) & valid);
            }
        }
    //}
    return T;
}*/

float intersectTriangle(const vec3 orig, const vec3 dir, const int tri, inout vec2 uv, inout bool_ _valid, const float testdist) {
    const int itri = tri*9;
    const mat3 vT = mat3(
        vec3(lvtx[itri+0], lvtx[itri+1], lvtx[itri+2]),
        vec3(lvtx[itri+3], lvtx[itri+4], lvtx[itri+5]),
        vec3(lvtx[itri+6], lvtx[itri+7], lvtx[itri+8])
    );
    const vec3 e1 = vT[1]-vT[0], e2 = vT[2]-vT[0];
    const vec3 h = cross(dir, e2);
    const float a = dot(e1,h);
    if (abs(a) < 1e-5f) { _valid = false_; }

    const float f = 1.f/a;
    const vec3 s = orig - vT[0], q = cross(s, e1);
    uv = f * vec2(dot(s,h),dot(dir,q));

    if (uv.x < 0.f || uv.y < 0.f || (uv.x+uv.y) > 1.f) { _valid = false_; }

    float T = f * dot(e2,q);
    if (T >= INFINITY || T < 0.f) { _valid = false_; } 
    IF (not(_valid)) T = INFINITY;
    return T;
}


#endif
#endif
#endif



const int _BVH_WIDTH = 2048;


#define bvhT_ptr ivec2
bvhT_ptr mk_bvhT_ptr(in int linear) {
    //int md = linear & 1; linear >>= 1;
    //return bvhT_ptr(linear % _BVH_WIDTH, ((linear / _BVH_WIDTH) << 1) + md);
    return bvhT_ptr(linear % _BVH_WIDTH, linear / _BVH_WIDTH); // just make linear (gather by tops of...)
}


#ifdef ENABLE_VSTORAGE_DATA
#ifdef ENABLE_VERTEX_INTERPOLATOR
// barycentric map (for corrections tangents in POM)
const mat3 uvwMap = mat3(vec3(1.f,0.f,0.f),vec3(0.f,1.f,0.f),vec3(0.f,0.f,1.f));

//HitPayload interpolateMeshData(in HitData ht, inout HitPayload res) {
HitData interpolateMeshData(inout HitData ht) {
    const int tri = floatBitsToInt(ht.uvt.w), itri = tri*9; vec2 trig = vec2(0.f.xx);
    bool_ validInterpolant = greaterEqualF(ht.uvt.z, 0.0f) & lessF(ht.uvt.z, INFINITY) & bool_(tri != LONGEST) & bool_(materials[tri] == ht.materialID);
    
    IFANY (validInterpolant) {
        // pre-calculate interpolators
        const vec3 vs = vec3(1.0f - ht.uvt.x - ht.uvt.y, ht.uvt.xy);
        const vec2 sz = 1.f / textureSize(attrib_texture, 0);

        // gather normal 
        trig = fma(vec2(gatherMosaic(getUniformCoord(tri*ATTRIB_EXTENT+NORMAL_TID))), sz, sz * 0.9999f);
        const vec3 normal = normalize(vs * mat3x3(SGATHER(attrib_texture, trig, 0)._SWIZV, SGATHER(attrib_texture, trig, 1)._SWIZV, SGATHER(attrib_texture, trig, 2)._SWIZV));

        // gather texcoord 
        trig = fma(vec2(gatherMosaic(getUniformCoord(tri*ATTRIB_EXTENT+TEXCOORD_TID))), sz, sz * 0.9999f);
        const mat2x3 texcoords = mat2x3(SGATHER(attrib_texture, trig, 0)._SWIZV, SGATHER(attrib_texture, trig, 1)._SWIZV);
        const vec2 texcoord = vs * texcoords;

        // get delta vertex
        mat3x2 dlts = transpose(mat2x3(texcoords[0], 1.f-texcoords[1]));
        mat3x3 dlps = mat3x3(
            lvtx[itri+0], lvtx[itri+1], lvtx[itri+2],
            lvtx[itri+3], lvtx[itri+4], lvtx[itri+5],
            lvtx[itri+6], lvtx[itri+7], lvtx[itri+8]
        );

        // deltas of positions and texcoords
        dlps[1] -= dlps[0], dlps[2] -= dlps[0], dlts[1] -= dlts[0], dlts[2] -= dlts[0];

        // calc raw TBN 
        float idet = 1.f/precIssue(determinant(mat2(dlts[1],dlts[2]))); // inv determinant
        vec3 t = fma(dlts[2].yyy, dlps[1], -dlts[1].y * dlps[2]), b = fma(dlts[1].xxx, dlps[2], -dlts[2].x * dlps[1]), n = normal; // pre-tbn

        // if texcoord not found or incorrect, calculate by axis
        if (
            all(lessThanEqual(abs(dlts[1]), 1e-5f.xx)) || 
            all(lessThanEqual(abs(dlts[2]), 1e-5f.xx)) || 
            all(lessThanEqual(abs(dlps[1]), 1e-5f.xxx)) || 
            all(lessThanEqual(abs(dlps[2]), 1e-5f.xxx))
        ) {
            vec3 c0 = cross(n, vec3(0.f, 0.f, 1.f)), c1 = cross(n, vec3(0.f, 1.f, 0.f));
            t = length(c0) >= length(c1) ? c0 : c1, b = cross(t, n);
            idet = 1.f;
        }

        { // orthonormalization process
            t -= n * dot( t, n );
            b -= n * dot( b, n );
            b -= t * dot( b, t );
        }

        IF (validInterpolant) {
            ht.normal      = vec4( normalize(n), 0.0f);
            ht.tangent     = vec4( normalize(t*idet), 0.0f);
            ht.bitangent   = vec4( normalize(b*idet), 0.0f);
            ht.texcoord.xy = texcoord;
        }
    }
    return ht;
}
#endif
#endif

#endif
