#pragma once

#include "../utils.hpp"

namespace NSM {
    namespace rt {

        struct bbox {
            glm::vec4 mn = glm::vec4(10000.f);
            glm::vec4 mx = glm::vec4(-10000.f);
        };

        struct RayRework {
            glm::vec4 a, b;
        };

        struct HitRework {
            // getting under intersections
            glm::vec4 uvt; // UV, distance, triangle (base data)

            // surface shaders and interpolators
            glm::vec4 normalHeight; // normal with height mapping, will already interpolated with geometry
            glm::vec4 tangent; // 
            glm::vec4 bitangent; // 
            glm::vec4 texcoord; // critical texcoords 

            // textured data
            glm::uvec2 metallicRoughness, unk16;
            glm::uvec2 emission, albedo;

            // integer metadata
            glm::uint bitfield;
            int ray; // ray index
            int materialID; // may not necessary 
            int next;
        };

        struct Texel {
            glm::vec4 coord;
            glm::vec4 color; // when collected from blocks
            glm::vec4 p3d;
            glm::vec4 albedo;
            glm::vec4 normal;
        };

        struct HlbvhNode {
            glm::uvec4 lbox;
            glm::uvec4 rbox;
            glm::ivec4 pdata;
        };



        struct BVHBlockUniform {
            glm::mat4 transform = glm::mat4(1.f);
            glm::mat4 transformInv = glm::mat4(1.f);
            glm::mat4 projection = glm::mat4(1.f);
            glm::mat4 projectionInv = glm::mat4(1.f);
            int leafCount = 0;
        };

        struct MaterialUniformStruct {
            int materialOffset = 0;
            int materialCount = 0;
            int time = 0;
            int lightcount = 0;
        };

        struct SamplerUniformStruct {
            glm::vec2 sceneRes;
            int iterationCount;
            int blockCount;
            int blockBinCount;
            int texelCount;
            int hitCount;
            int traverseCount;
            glm::mat4 transform = glm::mat4(1.f);
        };

        struct LightUniformStruct {
            glm::vec4 lightVector;
            glm::vec4 lightColor;
            glm::vec4 lightOffset;
            glm::vec4 lightAmbient;
            glm::vec4 lightRandomizedOrigin;
        };

        struct GeometryUniformStruct {
            glm::mat4 transform = glm::mat4(1.f);
            glm::mat4 transformInv = glm::mat4(1.f);
            glm::mat4 projection = glm::mat4(1.f);
            glm::mat4 projectionInv = glm::mat4(1.f);

            int materialID = 0;
            int triangleCount = 1;
            int triangleOffset = 0;
            int clearDepth = 0;
        };

        struct CameraUniformStruct {
            glm::mat4 projInv = glm::mat4(1.f);
            glm::mat4 camInv = glm::mat4(1.f);
            glm::mat4 prevCamInv = glm::mat4(1.f);

            float ftime = 0.f;
            int enable360 = 0;
            int interlace = 0;
            int interlaceStage = 0;
        };

        struct RayStream {
            glm::vec4 diffuseStream;
            glm::ivec4 superseed;
            glm::vec4 frand4;
        };



        struct GeometryBlockUniform {
            GeometryUniformStruct geometryUniform = GeometryUniformStruct();
        };

        struct RayBlockUniform {
            SamplerUniformStruct samplerUniform = SamplerUniformStruct();
            CameraUniformStruct cameraUniform = CameraUniformStruct();
            MaterialUniformStruct materialUniform = MaterialUniformStruct();
        };



        // low level buffer region, that means just "buffer"
        struct BufferRegion {
            uint32_t byteOffset;
            uint32_t byteSize;
        };



        struct MeshUniformStruct {
            int vertexAccessor = -1;
            int normalAccessor = -1;
            int texcoordAccessor = -1;
            int modifierAccessor = -1;
            int materialAccessor = -1;
            int indiceAccessor = -1;
            int r0 = -1, r1 = -1;

            glm::mat4 transform = glm::mat4(1.f);
            glm::mat4 transformInv = glm::mat4(1.f);

            int materialID = 0;
            int int16bit = 0;
            int nodeCount = 1;
            int primitiveType = 0;
        };

        // subdata structuring in buffer region
        struct VirtualBufferView {
            int byteOffset = 0;
            int byteStride = 1;
            int bufferID = 0;
        };

        // structuring swizzle
        struct VirtualDataAccess {
            int bufferView = -1; // buffer-view structure
            int byteOffset = 0; // in structure offset
            uint32_t components : 2, type : 4, normalized : 1;
        };

        // structure accessors 
        struct VirtualBufferBinding {
            int bufferID = -1; // buffer region PTR, override
            int dataAccess = -1; // structure accessor 
            int indexOffset = 0; // structure index offset (where should be counting), so offset calculates as   (bv.byteOffset + indexOffset*bv.byteStride + ac.byteOffset)
        };


        struct VirtualMaterial {
            glm::vec4 diffuse = glm::vec4(0.0f);
            glm::vec4 specular = glm::vec4(0.0f);
            glm::vec4 transmission = glm::vec4(0.0f);
            glm::vec4 emissive = glm::vec4(0.0f);

            float ior = 1.0f;
            float roughness = 0.0001f;
            float alpharef = 0.0f;
            float unk0f = 0.0f;

            uint32_t diffuseTexture = 0, diffuseSampler = 0;
            uint32_t specularTexture = 0, specularSampler = 0;
            uint32_t bumpTexture = 0, bumpSampler = 0;
            uint32_t emissiveTexture = 0, emissiveSampler = 0;

            int32_t flags = 0;
            int32_t alphafunc = 0;
            int32_t binding = 0;
            int32_t bitfield = 0;
        };



        struct BlockCacheInfo {
            int indiceCount; // count of indices in traverse
            int triangleCount; // count of triangle pairs
            glm::ivec2 padding0;
        };

        struct NodeCacheInfo {
            int bakedStackCount;
            int chainID;
            int prevHt;
            int nextHt;

            glm::vec4 horigin;
            glm::vec4 hdirect;
            //glm::mat3x4 rayAffine = glm::mat3x4(1.f);

            //int rayAxis;
            float reserved;
            float toffset;
            float nearhit;
            float hcorrection;
            int validBox;
            int bvhPtr;
            int deferredPtr;
            int esc;
        };

    }
}