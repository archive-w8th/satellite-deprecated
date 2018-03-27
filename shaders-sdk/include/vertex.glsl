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
    #ifdef ENABLE_VERTEX_INTERPOLATOR
        layout ( binding = 10, set = 1 ) uniform sampler2D attrib_texture;
        layout ( std430, binding = 1, set = 1 ) readonly buffer GeomMaterialsSSBO { int materials[]; };
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

HitRework interpolateMeshData(inout HitRework res) {
    int tri = floatBitsToInt(res.uvt.w);
    bool_ validInterpolant = greaterEqualF(res.uvt.z, 0.0f) & lessF(res.uvt.z, INFINITY) & bool_(tri != LONGEST);
    
    IFANY (validInterpolant & not(HitInterpolated(res))) {
        // pre-calculate interpolators
        const vec3 vs = vec3(1.0f - res.uvt.x - res.uvt.y, res.uvt.xy);
        const vec2 sz = 1.f / textureSize(attrib_texture, 0);

        // gather normal 
        vec2 trig = fma(vec2(gatherMosaic(getUniformCoord(tri*ATTRIB_EXTENT+NORMAL_TID))), sz, sz * 0.9999f);
        const vec3 normal = normalize(vs * mat3x3(SGATHER(attrib_texture, trig, 0)._SWIZV, SGATHER(attrib_texture, trig, 1)._SWIZV, SGATHER(attrib_texture, trig, 2)._SWIZV));

        // gather texcoord 
        trig = fma(vec2(gatherMosaic(getUniformCoord(tri*ATTRIB_EXTENT+TEXCOORD_TID))), sz, sz * 0.9999f);
        const mat2x3 texcoords = mat2x3(SGATHER(attrib_texture, trig, 0)._SWIZV, SGATHER(attrib_texture, trig, 1)._SWIZV);

        // calculate texcoord transposed 
        const mat3x2 txds = transpose(mat2x3(texcoords[0], 1.f-texcoords[1]));

        // get delta vertex 
        const int itri = tri*9;
        const mat3x3 triverts = transpose(mat3x3(
            lvtx[itri+0], lvtx[itri+1], lvtx[itri+2],
            lvtx[itri+3], lvtx[itri+4], lvtx[itri+5],
            lvtx[itri+6], lvtx[itri+7], lvtx[itri+8]
        ));

        // calc deltas
        const mat2x2 dlts = mat2x2(txds[1]-txds[0], txds[2]-txds[0]);
        const mat2x3 dlps = mat2x3(triverts[1]-triverts[0], triverts[2]-triverts[0]);
        float idet = 1.f/precIssue(determinant(dlts));

        // pre-tbn
        vec3 t = fma(dlts[1].yyy, dlps[0], -dlts[0].y * dlps[1]);
        vec3 b = fma(dlts[0].xxx, dlps[1], -dlts[1].x * dlps[0]);
        vec3 n = normal;

        // if texcoord not found or incorrect, calculate by axis
        if (
            all(lessThanEqual(abs(dlts[0]), 1e-5f.xx)) || 
            all(lessThanEqual(abs(dlts[1]), 1e-5f.xx)) || 
            all(lessThanEqual(abs(dlps[0]), 1e-5f.xxx)) || 
            all(lessThanEqual(abs(dlps[1]), 1e-5f.xxx))
        ) {
            vec3 c0 = cross(n, vec3(0.f, 0.f, 1.f));
            vec3 c1 = cross(n, vec3(0.f, 1.f, 0.f));
            t = length(c0) >= length(c1) ? c0 : c1, b = cross(t, n);
            idet = 1.f;
        }

        {
            t -= n * dot( t, n ); // orthonormalization ot the tangent vectors
            b -= n * dot( b, n ); // orthonormalization of the binormal vectors to the normal vector 
            b -= t * dot( b, t ); // orthonormalization of the binormal vectors to the tangent vector
        }

        IF (validInterpolant & not(HitInterpolated(res))) {
            res.normalHeight = vec4( normalize(n), 0.0f);
            res.tangent =      vec4( normalize(t*idet), 0.0f);
            res.bitangent =    vec4( normalize(b*idet), 0.0f);
            res.texcoord.xy = vs * texcoords; // mult matrix
            res.materialID = materials[tri];
            HitActived(res, true_); // temporary enable
            HitInterpolated(res, true_);
        }
    }
    return res;
}
#endif
#endif

#endif
