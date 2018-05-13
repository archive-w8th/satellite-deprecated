#pragma once

#include "../structs.hpp"
#include "vertexInstance.hpp"

namespace NSM
{
    namespace rt
    {
        class GeometryAccumulator : public std::enable_shared_from_this<GeometryAccumulator>
        {
        protected:
            // Geometry metrics
            const size_t WARPED_WIDTH = 2048;
            const size_t ATTRIBUTE_EXTENT = 4;
            const size_t _WIDTH = 6144;
            
            // Worker metrics
            const size_t INTENSIVITY = 4096;

            // current compute device
            Device device;
            Queue queue;

            std::string shadersPathPrefix = "shaders-spv";
            ComputeContext geometryLoader;

            Image attributeTexelWorking;

            Buffer vertexLinearWorking, materialIndicesWorking, orderIndicesWorking;
            Buffer zerosBufferReference, debugOnes32BufferReference, geometryCounter;

            vk::PipelineLayout pipelineLayout;

            std::vector<vk::DescriptorSet> loaderDescriptorSets;
            std::vector<vk::DescriptorSetLayout> loaderDescriptorLayout; // may be single layout
            vk::DescriptorUpdateTemplate descriptorVInstanceUpdateTemplate;


            // streaming of buffers data
            Buffer generalStagingBuffer, generalLoadingBuffer;

        public:
            GeometryAccumulator() {}
            GeometryAccumulator(Queue &queue, std::string shadersPack)
            {
                shadersPathPrefix = shadersPack;
                init(queue);
            }

            void pushGeometryMulti(std::shared_ptr<VertexInstance> vertexInstance, bool needUpdateDescriptor = true, uint32_t count = 1, uint32_t instanceConst = 0); // planned support for transformations
            void pushGeometry(std::shared_ptr<VertexInstance> vertexInstance, bool needUpdateDescriptor = true, uint32_t instanceConst = 0); // planned support for transformations
            void resetAccumulationCounter();
            void allocatePrimitiveReserve(size_t primitiveCount);

            Buffer getGeometrySourceCounterHandler() { return geometryCounter; }
            Image getAttributeTexel() { return attributeTexelWorking; }
            Buffer getVertexLinear() { return vertexLinearWorking; }
            Buffer getMaterialIndices() { return materialIndicesWorking; }
            Buffer getOrderIndices() { return orderIndicesWorking; }

        protected:
            void init(Queue &queue);
        };
    }
} // namespace NSM
