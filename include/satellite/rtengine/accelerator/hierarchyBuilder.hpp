#pragma once

#include "../structs.hpp"
#include "./vertex/geometryAccumulator.hpp"
#include "./hierarchyStorage.hpp"
#include "../../grlib/radixSort.hpp"

namespace NSM
{
    namespace rt
    {
        class HieararchyBuilder : public std::enable_shared_from_this<HieararchyBuilder> {
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

            Queue queue;
            Device device;

            std::shared_ptr<HieararchyStorage> hierarchyStorageLink;
            std::shared_ptr<GeometryAccumulator> geometrySourceLink;

            vk::PipelineLayout pipelineLayout;

            ComputeContext buildBVHPpl, buildBVHLargePpl, aabbCalculate, refitBVH, boundPrimitives, childLink;
            Buffer boundaryBufferReference, zerosBufferReference, debugOnes32BufferReference;
            Buffer bvhBoxWorking, bvhBoxWorkingResulting, leafsBuffer, countersBuffer, mortonCodesBuffer, mortonIndicesBuffer, boundaryBuffer, workingBVHNodesBuffer, leafBVHIndicesBuffer, bvhNodesFlags;
            Buffer bvhMetaWorking;
            //Image bvhMetaWorking;

            std::vector<vk::DescriptorSet> builderDescriptorSets;
            std::vector<vk::DescriptorSetLayout> builderDescriptorLayout; // may be single layout

            // BVH uniform
            std::vector<BVHBlockUniform> bvhBlockData;
            UniformBuffer bvhBlockUniform; // buffer of uniforms

            // streaming of buffers data
            Buffer generalStagingBuffer, generalLoadingBuffer;

        public:
            HieararchyBuilder() {}
            HieararchyBuilder(Queue queue, std::string shadersPack)
            {
                shadersPathPrefix = shadersPack;
                init(queue);
            }

            void allocateNodeReserve(size_t maxt);
            void setPrimitiveSource(std::shared_ptr<GeometryAccumulator> &geometry) { geometrySourceLink = geometry; };
            void setHieararchyOutput(std::shared_ptr<HieararchyStorage> &storage) { hierarchyStorageLink = storage; };
            void build(glm::dmat4 optproj);

        protected:
            void init(Queue queue);
            void syncUniforms();
        };
    }
} // namespace NSM
