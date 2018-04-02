#pragma once

#include "../../rtengine/vertexSubnodes.hpp"

namespace NSM
{
    namespace rt
    {

        template <int BINDING, class STRUCTURE>
        BufferComposer<BINDING, STRUCTURE>::BufferComposer(
            BufferComposer<BINDING, STRUCTURE> &another)
        {
            cache = another.cache;
            stager = another.stager;
            data = another.data;
            device = another.device;
        }

        template <int BINDING, class STRUCTURE>
        BufferComposer<BINDING, STRUCTURE>::BufferComposer(
            BufferComposer<BINDING, STRUCTURE> &&another)
        {
            cache = std::move(another.cache);
            stager = std::move(another.stager);
            data = std::move(another.data);
            device = std::move(another.device);
        }

        template <int BINDING, class STRUCTURE>
        BufferComposer<BINDING, STRUCTURE>::BufferComposer(DeviceQueueType &device,
            const size_t bsize)
        {
            this->device = device;
            cache = createBuffer(device, strided<STRUCTURE>(bsize),
                vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eTransferSrc,
                VMA_MEMORY_USAGE_GPU_ONLY);
            stager = createBuffer(device, strided<STRUCTURE>(bsize),
                vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eTransferSrc,
                VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        template <int BINDING, class STRUCTURE>
        int32_t BufferComposer<BINDING, STRUCTURE>::addElement(STRUCTURE accessorDesc)
        {
            needUpdateBuffer = true;
            int32_t ptr = data.size();
            data.push_back(accessorDesc);
            return ptr;
        };

        template <int BINDING, class STRUCTURE>
        BufferType BufferComposer<BINDING, STRUCTURE>::getBuffer()
        {
            if (data.size() > 0 && needUpdateBuffer) {
                auto commandBuffer = getCommandBuffer(device, true);
                bufferSubData(commandBuffer, stager, data, 0);
                memoryCopyCmd(commandBuffer, stager, cache, { 0, 0, strided<STRUCTURE>(data.size()) });
                flushCommandBuffer(device, commandBuffer, true);
            }
            needUpdateBuffer = false;
            return cache;
        }

        BufferSpace::BufferSpace(DeviceQueueType &device, const size_t spaceSize)
        {
            this->device = device;
            dataBuffer = createBuffer(device, strided<uint8_t>(tiled(spaceSize, 4) * 4),
                vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eTransferSrc,
                VMA_MEMORY_USAGE_GPU_ONLY);
            dataStage = createBuffer(device, strided<uint8_t>(tiled(spaceSize, 4) * 4),
                vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eTransferSrc,
                VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        intptr_t BufferSpace::getLastKnownOffset() { return lastKnownOffset; }

        BufferType BufferSpace::getDataBuffer() { return dataBuffer; }

        intptr_t BufferSpace::copyGPUBuffer(BufferType external, const size_t size, const intptr_t offset) {
            if (size > 0) {
                flushCommandBuffer( device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>( device, external, dataBuffer, { 0, vk::DeviceSize(offset), vk::DeviceSize(size) }), true);
            }
            return offset;
        }

        intptr_t BufferSpace::copyGPUBuffer(BufferType external, const size_t size)
        {
            const intptr_t offset = lastKnownOffset;
            lastKnownOffset += size;
            lastKnownOffset = tiled(lastKnownOffset, 4) * 4; // correct by hardware 32-bit (for avoid errors in shaders)
            return copyGPUBuffer(external, size, offset);
        }

        intptr_t BufferSpace::copyHostBuffer(const uint8_t *external, const size_t size, const intptr_t offset) {
            if (size > 0)
            {
                auto commandBuffer = getCommandBuffer(device, true);
                bufferSubData(commandBuffer, dataStage, external, size, 0);
                memoryCopyCmd(commandBuffer, dataStage, dataBuffer, { 0, vk::DeviceSize(offset), vk::DeviceSize(size) });
                flushCommandBuffer(device, commandBuffer, true);
            }
            return offset;
        }

        intptr_t BufferSpace::copyHostBuffer(const uint8_t *external, const size_t size) {
            const intptr_t offset = lastKnownOffset;
            lastKnownOffset += size;
            lastKnownOffset = tiled(lastKnownOffset, 4) * 4; // correct by hardware 32-bit (for avoid errors in shaders)
            return copyHostBuffer(external, size, offset);
        }

        template <class T>
        intptr_t BufferSpace::copyHostBuffer(const std::vector<T> external, const intptr_t offset)
        {
            return copyHostBuffer((const uint8_t *)external.data(), external.size() * sizeof(T), offset);
        }

    } // namespace rt
} // namespace NSM