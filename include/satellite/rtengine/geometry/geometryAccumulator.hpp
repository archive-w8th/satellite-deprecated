#pragma once

#include "../structs.hpp"
#include ".././vertexInstance.hpp"

namespace NSM
{
    namespace rt
    {
        class GeometryAccumulator
        {
        protected:
            // Geometry metrics
            const size_t WARPED_WIDTH = 2048;
            const size_t ATTRIBUTE_EXTENT = 4;
            const size_t _WIDTH = 6144;
            
            // Worker metrics
            const size_t INTENSIVITY = 512;

            std::string shadersPathPrefix = "shaders-spv";
            DeviceQueueType device;
            ComputeContext geometryLoader;

            TextureType attributeTexelWorking;

            BufferType vertexLinearWorking, materialIndicesWorking, orderIndicesWorking;
            BufferType zerosBufferReference, debugOnes32BufferReference, geometryCounter;

            vk::PipelineLayout pipelineLayout;
            vk::PipelineCache pipelineCache;

            std::vector<vk::DescriptorSet> loaderDescriptorSets;
            std::vector<vk::DescriptorSetLayout> loaderDescriptorLayout; // may be single layout

            // streaming of buffers data
            BufferType generalStagingBuffer, generalLoadingBuffer;

        public:
            GeometryAccumulator() {}
            GeometryAccumulator(DeviceQueueType &device, std::string shadersPack)
            {
                shadersPathPrefix = shadersPack;
                init(device);
            }

            void pushGeometry(std::shared_ptr<VertexInstance> vertexInstance); // planned support for transformations
            void resetAccumulationCounter();
            void allocatePrimitiveReserve(size_t primitiveCount);

            BufferType getGeometrySourceCounterHandler() { return geometryCounter; }
            TextureType getAttributeTexel() { return attributeTexelWorking; }
            BufferType getVertexLinear() { return vertexLinearWorking; }
            BufferType getMaterialIndices() { return materialIndicesWorking; }
            BufferType getOrderIndices() { return orderIndicesWorking; }

        protected:
            void init(DeviceQueueType &device);
        };
    }
} // namespace NSM
