#pragma once

#include "../structs.hpp"
#include "../../grlib/radixSort.hpp"
#include "../vertex/vertexInstance.hpp"

namespace NSM
{
    namespace rt
    {
        class HieararchyStorage : public std::enable_shared_from_this<HieararchyStorage>
        {
        protected:

            // Geometry metrics
            const size_t _WIDTH = 6144;

            // BVH metrics
            const size_t _BVH_WIDTH = 2048;
            const size_t ATTRIBUTE_EXTENT = 4;

            // for traversing
            const size_t INTENSIVITY = 4096;

            std::string shadersPathPrefix = "shaders-spv";

            Queue queue;
            Device device;

            ComputeContext bvhTraverse, vertexInterpolator;

            Buffer boundaryBufferReference, zerosBufferReference, debugOnes32BufferReference;
            Buffer traverseBlockData, traverseCacheData;

            // BVH storage
            Buffer bvhBoxStorage;
            Buffer bvhMetaStorage;

            // vertex data storage
            Image attributeTexelStorage;
            Buffer vertexLinearStorage, materialIndicesStorage, orderIndicesStorage;

            std::vector<GeometryBlockUniform> geometryBlockData;
            UniformBuffer geometryBlockUniform; // buffer of uniforms

            // will shared for
            vk::PipelineLayout rayTraversePipelineLayout;
            std::vector<vk::DescriptorSet> clientDescriptorSets;
            std::vector<vk::DescriptorSetLayout> clientDescriptorLayout; // may be single layout

            // streaming of buffers data
            Buffer generalStagingBuffer, generalLoadingBuffer;

        public:
            HieararchyStorage() {}
            HieararchyStorage(Queue queue, std::string shadersPack)
            {
                shadersPathPrefix = shadersPack;
                init(queue);
            }

            void resetInternalGeometryCount()
            {
                // confirm geometry accumulation by counter (planned locking ops)
                auto geometrySourceCounterHandler = zerosBufferReference;
                flushCommandBuffers(queue, { makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, geometrySourceCounterHandler, geometryBlockUniform.buffer, { strided<uint32_t>(0), offsetof(GeometryBlockUniform, geometryUniform) + offsetof(GeometryUniformStruct, triangleCount), strided<uint32_t>(1) }) }, true); //
            };

            // static allocation
            void allocatePrimitiveReserve(size_t primitiveCount);
            void allocateNodeReserve(size_t maxt);

            // share for client state
            UniformBuffer &getGeometryBlockUniform() { return geometryBlockUniform; }; // here will confirmation and copying counters

            // for bvh building (loads from geometry accumulator and builder)
            Buffer getBvhBox() { return bvhBoxStorage; }
            Buffer getBvhMeta() { return bvhMetaStorage; }

            // geometry related buffers
            Image getAttributeTexel() { return attributeTexelStorage; }
            Buffer getVertexLinear() { return vertexLinearStorage; }
            Buffer getMaterialIndices() { return materialIndicesStorage; }
            Buffer getOrderIndices() { return orderIndicesStorage; }

            // getter for hierarchy builder
            vk::DescriptorSet& getStorageDescSec() { return clientDescriptorSets[1]; };

            // traversing with shared ray-tracer data
            void queryTraverse(TraversibleData& tbsData);

        protected:
            void init(Queue queue);
        };
    }
} // namespace NSM
