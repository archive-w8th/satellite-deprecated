#pragma once

#include "../structs.hpp"
#include "./geometryAccumulator.hpp"
#include "./hierarchyStorage.hpp"
#include "../../grlib/radixSort.hpp"

namespace NSM
{
    namespace rt
    {
        class HieararchyBuilder
        {
        protected:
            const size_t CACHED_BBOX = 128;


            // Geometry metrics
            const size_t _WIDTH = 6144;
            const size_t WARPED_WIDTH = 2048;
            const size_t ATTRIBUTE_EXTENT = 4;

            // BVH native
            const size_t _BVH_WIDTH = 2048;

            // Worker metrics
            const size_t INTENSIVITY = 4096;

            std::string shadersPathPrefix = "shaders-spv";
            std::shared_ptr<gr::RadixSort> radixSort;
            DeviceQueueType device;

            std::shared_ptr<HieararchyStorage> hierarchyStorageLink;
            std::shared_ptr<GeometryAccumulator> geometrySourceLink;

            vk::PipelineLayout pipelineLayout;
            vk::PipelineCache pipelineCache;

            ComputeContext buildBVHPpl, aabbCalculate, refitBVH, boundPrimitives, childLink;
            BufferType boundaryBufferReference, zerosBufferReference, debugOnes32BufferReference;
            BufferType bvhBoxWorking, bvhBoxWorkingResulting, leafsBuffer, countersBuffer, mortonCodesBuffer, mortonIndicesBuffer, boundaryBuffer, workingBVHNodesBuffer, leafBVHIndicesBuffer, bvhNodesFlags;
            BufferType bvhMetaWorking;
            //TextureType bvhMetaWorking;

            std::vector<vk::DescriptorSet> builderDescriptorSets;
            std::vector<vk::DescriptorSetLayout> builderDescriptorLayout; // may be single layout

            // BVH uniform
            std::vector<BVHBlockUniform> bvhBlockData;
            UniformBuffer bvhBlockUniform; // buffer of uniforms

            // streaming of buffers data
            BufferType generalStagingBuffer, generalLoadingBuffer;

        public:
            HieararchyBuilder() {}
            HieararchyBuilder(DeviceQueueType &device, std::string shadersPack)
            {
                shadersPathPrefix = shadersPack;
                init(device);
            }

            void allocateNodeReserve(size_t maxt);
            void setPrimitiveSource(std::shared_ptr<GeometryAccumulator> &geometry) { geometrySourceLink = geometry; };
            void setHieararchyOutput(std::shared_ptr<HieararchyStorage> &storage) { hierarchyStorageLink = storage; };
            void build(glm::dmat4 optproj);

        protected:
            void init(DeviceQueueType &device);
            void syncUniforms();
        };
    }
} // namespace NSM
