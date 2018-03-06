#pragma once

#include "../structs.hpp"
#include "../../grlib/radixSort.hpp"
#include "../vertexInstance.hpp"

namespace NSM
{
namespace rt
{
class HieararchyStorage
{
  protected:
    const size_t INTENSIVITY = 512;

    std::string shadersPathPrefix = "shaders-spv";

    DeviceQueueType device;
    ComputeContext buildBVHPpl, aabbCalculate, refitBVH, boundPrimitives, childLink;

    BufferType boundaryBufferReference, zerosBufferReference, debugOnes32BufferReference;

    // BVH storage
    BufferType bvhBoxStorage;
    TextureType bvhMetaStorage;

    // vertex data storage
    TextureType attributeTexelStorage;
    BufferType vertexLinearStorage, materialIndicesStorage, orderIndicesStorage;

    std::vector<GeometryBlockUniform> geometryBlockData;
    UniformBuffer geometryBlockUniform; // buffer of uniforms

    // will shared for
    std::vector<vk::DescriptorSet> clientDescriptorSets;
    std::vector<vk::DescriptorSetLayout> clientDescriptorLayout; // may be single layout

    // streaming of buffers data
    BufferType generalStagingBuffer, generalLoadingBuffer;

  public:
    HieararchyStorage() {}
    HieararchyStorage(DeviceQueueType &device, std::string shadersPack)
    {
        shadersPathPrefix = shadersPack;
        init(device);
    }

    void resetInternalGeometryCount()
    {
        // confirm geometry accumulation by counter (planned locking ops)
        auto geometrySourceCounterHandler = zerosBufferReference;
        auto geometryBlockUniform = this->getGeometryBlockUniform();
        flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, geometrySourceCounterHandler, geometryBlockUniform.buffer, {strided<uint32_t>(0), offsetof(GeometryBlockUniform, geometryUniform) + offsetof(GeometryUniformStruct, triangleCount), strided<uint32_t>(1)}), true); //
    };

    // static allocation
    void allocatePrimitiveReserve(size_t primitiveCount);
    void allocateNodeReserve(size_t maxt);

    // share for client state
    vk::DescriptorSet &getClientDescriptorSet();
    UniformBuffer &getGeometryBlockUniform() { return geometryBlockUniform; }; // here will confirmation and copying counters

    // for bvh building (loads from geometry accumulator and builder)
    BufferType getBvhBox() { return bvhBoxStorage; }
    TextureType getBvhMeta() { return bvhMetaStorage; }

    // geometry related buffers
    TextureType getAttributeTexel() { return attributeTexelStorage; }
    BufferType getVertexLinear() { return vertexLinearStorage; }
    BufferType getMaterialIndices() { return materialIndicesStorage; }
    BufferType getOrderIndices() { return orderIndicesStorage; }

  protected:
    void init(DeviceQueueType &device);
};
}
} // namespace NSM
