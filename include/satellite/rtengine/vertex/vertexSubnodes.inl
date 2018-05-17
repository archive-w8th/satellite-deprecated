#pragma once

#include "vertexSubnodes.hpp"

namespace NSM
{
    namespace rt
    {

        template <int BINDING, class STRUCTURE>
        BufferComposer<BINDING, STRUCTURE>::BufferComposer( BufferComposer<BINDING, STRUCTURE> &another) {
            cache = another.cache;
            stager = another.stager;
            data = another.data;
            device = another.device;
            queue = another.queue;
        }

        template <int BINDING, class STRUCTURE>
        BufferComposer<BINDING, STRUCTURE>::BufferComposer(
            BufferComposer<BINDING, STRUCTURE> &&another)
        {
            cache = std::move(another.cache);
            stager = std::move(another.stager);
            data = std::move(another.data);
            device = std::move(another.device);
            queue = std::move(another.queue);
        }

        template <int BINDING, class STRUCTURE>
        BufferComposer<BINDING, STRUCTURE>::BufferComposer(Queue queue, const size_t bsize) {
            this->queue = queue;
            this->device = queue->device;

            cache = createBuffer(queue, strided<STRUCTURE>(bsize),
                vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eTransferSrc,
                VMA_MEMORY_USAGE_GPU_ONLY);
            stager = createBuffer(queue, strided<STRUCTURE>(bsize),
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
        Buffer BufferComposer<BINDING, STRUCTURE>::getBuffer()
        {
            if (data.size() > 0 && needUpdateBuffer) {
                auto commandBuffer = getCommandBuffer(queue, true);
                bufferSubData(commandBuffer, stager, data, 0);
                memoryCopyCmd(commandBuffer, stager, cache, { 0, 0, strided<STRUCTURE>(data.size()) });
                flushCommandBuffers(queue, { commandBuffer }, true);
            }
            needUpdateBuffer = false;
            return cache;
        }


        // for edit
        template <int BINDING, class STRUCTURE>
        STRUCTURE& BufferComposer<BINDING, STRUCTURE>::getStructure(intptr_t ptr) {
            needUpdateBuffer = true;
            if (data.size() <= ptr) data.resize(ptr+1);
            return data[ptr];
        }


        // for view only
        template <int BINDING, class STRUCTURE>
        STRUCTURE BufferComposer<BINDING, STRUCTURE>::getStructure(intptr_t ptr) const {
            return data[ptr];
        }

        BufferSpace::BufferSpace(Queue queue, const size_t spaceSize)
        {
            this->device = queue->device;
            this->queue = queue;

            dataBuffer = createBuffer(queue, strided<uint8_t>(tiled(spaceSize, 4) * 4),
                vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eTransferSrc,
                VMA_MEMORY_USAGE_GPU_ONLY);
            dataStage = createBuffer(queue, strided<uint8_t>(tiled(spaceSize, 4) * 4),
                vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eTransferSrc,
                VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        intptr_t BufferSpace::getLastKnownOffset() { return lastKnownOffset; }

        Buffer BufferSpace::getDataBuffer() { return dataBuffer; }

        intptr_t BufferSpace::copyGPUBuffer(Buffer external, const size_t size, const intptr_t offset) {
            if (size > 0) {
                flushCommandBuffers(queue, { createCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, external, dataBuffer, { 0, vk::DeviceSize(offset), vk::DeviceSize(size) }) }, true);
            }
            return offset;
        }

        intptr_t BufferSpace::copyGPUBuffer(Buffer external, const size_t size)
        {
            const intptr_t offset = lastKnownOffset;
            lastKnownOffset += size;
            lastKnownOffset = tiled(lastKnownOffset, 4) * 4; // correct by hardware 32-bit (for avoid errors in shaders)
            return copyGPUBuffer(external, size, offset);
        }

        intptr_t BufferSpace::copyHostBuffer(const uint8_t *external, const size_t size, const intptr_t offset) {
            if (size > 0)
            {
                auto commandBuffer = getCommandBuffer(queue, true);
                bufferSubData(commandBuffer, dataStage, external, size, 0);
                memoryCopyCmd(commandBuffer, dataStage, dataBuffer, { 0, vk::DeviceSize(offset), vk::DeviceSize(size) });
                flushCommandBuffers(queue, { commandBuffer }, true);
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