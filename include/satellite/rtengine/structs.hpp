#pragma once

// Readme license https://github.com/AwokenGraphics/prismarine-core/blob/master/LICENSE.md

#include "../utils.hpp"

namespace NSM {

    namespace rt {

        struct bbox {
            glm::vec4 mn = glm::vec4( 10000.f);
            glm::vec4 mx = glm::vec4(-10000.f);
        };

        struct RayRework {
            //glm::vec3 origin = glm::vec3(0.f); int hit = -1; // index of hit chain
            //glm::vec3 direct = glm::vec3(0.f); int bitfield = 0; // up to 32-bits
            glm::vec4 origin = glm::vec4(0.f);
            glm::vec4 direct = glm::vec4(0.f);
            glm::vec4 color = glm::vec4(0.f);
            glm::vec4 final = glm::vec4(0.f);
        };

        struct HitRework {
            glm::vec4 uvt; // UV, distance, triangle
            glm::vec4 normalHeight; // normal with height mapping, will already interpolated with geometry
            glm::vec4 tangent; // also have 4th extra slot
            glm::vec4 texcoord; // critical texcoords 

            glm::uvec2 metallicRoughness;
            glm::uvec2 unk16;
            glm::uvec2 emission;
            glm::uvec2 albedo;

            // integer metadata
            int bitfield;
            int ray; // ray index
            int materialID;
            int next;
        };


        struct Texel {
            glm::vec4 coord;
            glm::vec4 color; // when collected from blocks
        };

        struct HlbvhNode {
            glm::uvec4 lbox;
            glm::uvec4 rbox;
            glm::ivec4 pdata;
        };

        struct VboDataStride {
            glm::vec4 vertex;
            glm::vec4  normal;
            glm::vec4  texcoord;
            glm::vec4  color;
            glm::vec4  modifiers;
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
        };

        struct GeometryUniformStruct {
            glm::mat4 transform = glm::mat4(1.f);
            glm::mat4 transformInv = glm::mat4(1.f);

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
        };



        struct GeometryBlockUniform {
            GeometryUniformStruct geometryUniform = GeometryUniformStruct();
        };

        struct RayBlockUniform {
            SamplerUniformStruct samplerUniform = SamplerUniformStruct();
            CameraUniformStruct cameraUniform = CameraUniformStruct();
            MaterialUniformStruct materialUniform = MaterialUniformStruct();
        };





        struct MeshUniformStruct {
            int vertexAccessor = -1;
            int normalAccessor = -1;
            int texcoordAccessor = -1;
            int modifierAccessor = -1;

            glm::mat4 transform = glm::mat4(1.f);
            glm::mat4 transformInv = glm::mat4(1.f);

            int materialID = 0;
            int isIndexed = 0;
            int nodeCount = 1;
            int primitiveType = 0;

            int loadingOffset = 0;
            int storingOffset = 0;
            int _reserved0 = 1;
            int _reserved1 = 2;
        };


        struct VirtualBufferView {
            int offset4 = 0;
            int stride4 = 1;
        };

        struct VirtualAccessor {
            int offset4 = 0;
            int components : 2, type : 4, normalized : 1;
            int bufferView = -1;
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

            uint32_t diffusePart = 0;
            uint32_t specularPart = 0;
            uint32_t bumpPart = 0;
            uint32_t emissivePart = 0;

            int32_t flags = 0;
            int32_t alphafunc = 0;
            int32_t binding = 0;
            int32_t bitfield = 0;

            glm::ivec4 iModifiers0 = glm::ivec4(0);
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