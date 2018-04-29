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
            const size_t INTENSIVITY = 4096;

            std::string shadersPathPrefix = "shaders-spv";
            DeviceQueueType device;
            ComputeContext geometryLoader;

            TextureType attributeTexelWorking;

            BufferType vertexLinearWorking, materialIndicesWorking, orderIndicesWorking;
            BufferType zerosBufferReference, debugOnes32BufferReference, geometryCounter;

            vk::PipelineLayout pipelineLayout;

            std::vector<vk::DescriptorSet> loaderDescriptorSets;
            std::vector<vk::DescriptorSetLayout> loaderDescriptorLayout; // may be single layout
            vk::DescriptorUpdateTemplate descriptorVInstanceUpdateTemplate;


            // streaming of buffers data
            BufferType generalStagingBuffer, generalLoadingBuffer;

        public:
            GeometryAccumulator() {}
            GeometryAccumulator(DeviceQueueType &device, std::string shadersPack)
            {
                shadersPathPrefix = shadersPack;
                init(device);
            }

            void pushGeometryMulti(std::shared_ptr<VertexInstance> vertexInstance, bool needUpdateDescriptor = true, uint32_t count = 1, uint32_t instanceConst = 0); // planned support for transformations
            void pushGeometry(std::shared_ptr<VertexInstance> vertexInstance, bool needUpdateDescriptor = true, uint32_t instanceConst = 0); // planned support for transformations
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
