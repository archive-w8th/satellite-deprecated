#pragma once

#include "../../../rtengine/geometry/hierarchyStorage.hpp"

namespace NSM
{
namespace rt
{

void HieararchyStorage::init(DeviceQueueType &device)
{
    this->device = device;

    {
        // descriptor set layout of foreign storage (planned to sharing this structure by C++17)
        std::vector<vk::DescriptorSetLayoutBinding> clientDescriptorSetLayoutDesc = {
            vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // attributed data (alpha footage)
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // BVH boxes
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // materials
            vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // orders
            vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // geometryUniform
            vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // BVH metadata
            vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // reserved
            vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // vertex linear buffer
        };
        clientDescriptorLayout = std::vector<vk::DescriptorSetLayout>{device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setBindingCount(clientDescriptorSetLayoutDesc.size()).setPBindings(clientDescriptorSetLayoutDesc.data()))};
        clientDescriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(1).setPSetLayouts(&clientDescriptorLayout[0]));
    }

    {
        zerosBufferReference = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
        debugOnes32BufferReference = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

        // minmaxes
        std::vector<glm::vec4> minmaxes(64);
        for (int i = 0; i < 32; i++)
        {
            minmaxes[i * 2 + 0] = glm::vec4(10000.f), minmaxes[i * 2 + 1] = glm::vec4(-10000.f);
        }

        // zeros
        std::vector<uint32_t> zeros(1024);
        std::vector<uint32_t> ones(1024);
        for (int i = 0; i < 1024; i++)
        {
            zeros[i] = 0, ones[i] = 1;
        }

        // make reference buffers
        auto command = getCommandBuffer(device, true);
        bufferSubData(command, zerosBufferReference, zeros, 0); // make reference of zeros
        bufferSubData(command, debugOnes32BufferReference, ones, 0);
        flushCommandBuffer(device, command, true);
    }

    {
        // create client geometry uniform buffer
        geometryBlockData = std::vector<GeometryBlockUniform>(1);
        geometryBlockUniform.buffer = createBuffer(device, strided<GeometryBlockUniform>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
        geometryBlockUniform.staging = createBuffer(device, strided<GeometryBlockUniform>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

        // descriptor templates
        auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
        device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                                                 vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(3).setPBufferInfo(&geometryBlockUniform.buffer->descriptorInfo)},
                                             nullptr);
    }
}

void HieararchyStorage::allocatePrimitiveReserve(size_t primitiveCount)
{
    // storage
    size_t _MAX_HEIGHT = std::max(primitiveCount > 0 ? (primitiveCount - 1) / _BVH_WIDTH + 1 : 0, _BVH_WIDTH) + 1;
    attributeTexelStorage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{uint32_t(_WIDTH), uint32_t(_MAX_HEIGHT), 1}, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
    materialIndicesStorage = createBuffer(device, strided<uint32_t>(primitiveCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
    orderIndicesStorage = createBuffer(device, strided<uint32_t>(primitiveCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
    vertexLinearStorage = createBuffer(device, strided<float>(primitiveCount * 2 * 9), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

    // descriptor templates
    auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
    device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
        vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eSampledImage).setDstBinding(10).setPBufferInfo(&attributeTexelStorage->descriptorInfo),
        vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(1).setPBufferInfo(&materialIndicesStorage->descriptorInfo),
        vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(2).setPBufferInfo(&orderIndicesStorage->descriptorInfo), 
        vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(7).setPBufferInfo(&vertexLinearStorage->descriptorInfo), 
    nullptr);
}

void HieararchyStorage::allocateNodeReserve(size_t nodeCount)
{
        // bvh storage (32-bits elements)
        size_t _MAX_HEIGHT = std::max(nodeCount > 0 ? (nodeCount - 1) / _BVH_WIDTH + 1 : 0, _BVH_WIDTH) + 1;
        bvhMetaStorage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{uint32_t(_BVH_WIDTH), uint32_t(_MAX_HEIGHT * 2), 1}, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sint);
        bvhBoxStorage = createBuffer(device, strided<glm::mat4>(nodeCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

        // descriptor templates
        auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
        device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
            vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(0).setPBufferInfo(&bvhBoxStorage->descriptorInfo),
            vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eSampledImage).setDstBinding(5).setPBufferInfo(&bvhMetaStorage->descriptorInfo),
        nullptr);
}

void HieararchyStorage::syncUniforms()
{
            // insafe at now, no support loading from hosts
            //auto command = getCommandBuffer(device, true);
            //bufferSubData(command, geometryBlockUniform.staging, geometryBlockData, 0);
            //memoryCopyCmd(command, geometryBlockUniform.staging, geometryBlockUniform.buffer, {0, 0, strided<GeometryBlockUniform>(1)});
            //flushCommandBuffer(device, command, true);
}
}
} // namespace NSM
