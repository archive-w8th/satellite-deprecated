#pragma once

#include "../vertexSubnodes.hpp"

namespace NSM {
    namespace rt {

        template<int BINDING, class STRUCTURE>
        BufferComposer<BINDING,STRUCTURE>::BufferComposer(BufferComposer<BINDING, STRUCTURE>& another) {
            cache = another.cache;
            stager = another.stager;
            data = another.data;
            device = another.device;
        }

        template<int BINDING, class STRUCTURE>
        BufferComposer<BINDING,STRUCTURE>::BufferComposer(BufferComposer<BINDING, STRUCTURE>&& another) {
            cache = std::move(another.cache);
            stager = std::move(another.stager);
            data = std::move(another.data);
            device = std::move(another.device);
        }

        template<int BINDING, class STRUCTURE>
        BufferComposer<BINDING,STRUCTURE>::BufferComposer(DeviceQueueType& device, const size_t bsize) {
            this->device = device;
            cache = createBuffer(device, strided<STRUCTURE>(bsize), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            stager = createBuffer(device, strided<STRUCTURE>(bsize), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }


        template<int BINDING, class STRUCTURE>
        int32_t BufferComposer<BINDING,STRUCTURE>::addElement(STRUCTURE accessorDesc) {
            int32_t ptr = data.size();
            data.push_back(accessorDesc);
            return ptr;
        };

        template<int BINDING, class STRUCTURE>
        BufferType BufferComposer<BINDING,STRUCTURE>::getBuffer() {
            if (data.size() > 0) {
                bufferSubData(stager, data, 0);
                copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, stager, cache, { 0, 0, strided<STRUCTURE>(data.size()) }, true);
            }
            return cache;
        }




        BufferSpace::BufferSpace(DeviceQueueType& device, const size_t spaceSize) {
            this->device = device;

            regionsBuffer = createBuffer(device, strided<BufferRegion>(1024 * 64), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            regionsStage = createBuffer(device, strided<BufferRegion>(1024 * 64), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

            dataBuffer = createBuffer(device, strided<uint8_t>(tiled(spaceSize, 4) * 4), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            dataStage = createBuffer(device, strided<uint8_t>(tiled(spaceSize, 4) * 4), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }


        intptr_t BufferSpace::getLastKnownOffset() {
            return lastKnownOffset;
        }

        BufferType BufferSpace::getDataBuffer() {
            return dataBuffer;
        }

        BufferType BufferSpace::getRegionsBuffer() {
            if (regions.size() > 0) {
                bufferSubData(regionsStage, regions, 0);
                copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, regionsStage, regionsBuffer, { 0, 0, strided<BufferRegion>(regions.size()) }, true);
            }
            return regionsBuffer;
        }


        intptr_t BufferSpace::copyGPUBuffer( BufferType external, const size_t size, const intptr_t offset) {
            if (size > 0) {
                copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, external, dataBuffer, { 0, vk::DeviceSize(offset), vk::DeviceSize(size) }, true);
            }
            return offset;
        }

        intptr_t BufferSpace::copyGPUBuffer( BufferType external, const size_t size) {
            const intptr_t offset = lastKnownOffset; lastKnownOffset += size;
            lastKnownOffset = tiled(lastKnownOffset, 4) * 4; // correct by hardware 32-bit (for avoid errors in shaders) 
            return copyGPUBuffer(external, size, offset);
        }


        intptr_t BufferSpace::copyHostBuffer(const uint8_t * external, const size_t size, const intptr_t offset) {
            if (size > 0) {
                bufferSubData(dataStage, external, size, 0);
            }
            return copyGPUBuffer(dataStage, size, offset);
        }

        intptr_t BufferSpace::copyHostBuffer(const uint8_t * external, const size_t size) {
            const intptr_t offset = lastKnownOffset; lastKnownOffset += size;
            lastKnownOffset = tiled(lastKnownOffset, 4) * 4; // correct by hardware 32-bit (for avoid errors in shaders) 
            return copyHostBuffer(external, size, offset);
        }

        template<class T>
        intptr_t BufferSpace::copyHostBuffer(const std::vector<T> external, const intptr_t offset) {
            return copyHostBuffer((const uint8_t *)external.data(), external.size()*sizeof(T), offset);
        }


        int32_t BufferSpace::addRegionDesc(BufferRegion region) {
            int32_t ptr = regions.size();
            regions.push_back(region);
            return ptr;
        }

    }
}