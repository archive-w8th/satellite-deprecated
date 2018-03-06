#pragma once

#include "./structs.hpp"
#include "../grlib/radixSort.hpp"
#include "./vertexInstance.hpp"
#include "./geometry/geometryAccumulator.hpp"
#include "./geometry/hierarchyBuilder.hpp"
#include "./geometry/hierarchyStorage.hpp"

namespace NSM
{
    namespace rt
    {

        /*
        class TriangleHierarchy
        {
          protected:
            std::shared_ptr<gr::RadixSort> radixSort;
            const size_t INTENSIVITY = 512;
            const size_t WARPED_WIDTH = 2048;
            const size_t _WIDTH = 6144;
            const size_t _BVH_WIDTH = 2048;
            const size_t ATTRIBUTE_EXTENT = 4;

            DeviceQueueType device;

            // compute pipelines
            ComputeContext buildBVHPpl, aabbCalculate, refitBVH, boundPrimitives,
                childLink, geometryLoader;

            // static buffers for resets
            BufferType boundaryBufferReference, zerosBufferReference,
                debugOnes32BufferReference;

            // worktable buffers
            BufferType bvhBoxStorage, bvhBoxWorking, leafsBuffer, countersBuffer,
                mortonCodesBuffer, leafsIndicesBuffer, boundaryBuffer,
                workingBVHNodesBuffer, leafBVHIndicesBuffer, bvhNodesFlags;
            TextureType bvhMetaStorage, bvhMetaWorking;

            // where will upload/loading data
            BufferType generalStagingBuffer, generalLoadingBuffer;

            // texel storage of geometry
            TextureType
                attributeTexelStorage; // vertexTexelStorage, texcoordTexelStorage,
                                       // normalsTexelStorage, modsTexelStorage;
            BufferType vertexLinearStorage, materialIndicesStorage, orderIndicesStorage;

            // where will loading geometry data
            TextureType
                attributeTexelWorking; // vertexTexelWorking, texcoordTexelWorking,
                                       // normalsTexelWorking, modsTexelWorking;
            BufferType vertexLinearWorking, materialIndicesWorking, orderIndicesWorking;

            std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
            std::vector<vk::DescriptorSet> descriptorSets, clientDescriptorSets,
                loaderDescriptorSets;

            vk::PipelineCache pipelineCache;
            vk::PipelineLayout pipelineLayout, loaderPipelineLayout;

            size_t triangleCount = 1;
            size_t maxTriangles = 128 * 1024;
            // size_t workingTriangleCount = 1;
            bool isDirty = false;
            std::string shadersPathPrefix = "shaders-spv";

            std::vector<GeometryBlockUniform> geometryBlockData;
            UniformBuffer geometryBlockUniform; // buffer of uniforms

            std::vector<BVHBlockUniform> bvhBlockData;
            UniformBuffer bvhBlockUniform; // buffer of uniforms

            void init(DeviceQueueType &_device);

          public:
            void allocate(size_t maxt);
            void clearTribuffer();
            void loadGeometry(std::shared_ptr<VertexInstance> &vertexInstance);
            void syncUniforms();
            void markDirty();
            void buildBVH(glm::dmat4 optproj);

            vk::DescriptorSet getClientDescriptorSet();

          public:
            TriangleHierarchy() {}
            TriangleHierarchy(DeviceQueueType &device, std::string shadersPack)
            {
                shadersPathPrefix = shadersPack;
                init(device);
            }
        };
        */

    } // namespace rt
} // namespace NSM