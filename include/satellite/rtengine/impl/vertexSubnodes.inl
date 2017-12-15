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
        BufferComposer<BINDING,STRUCTURE>::BufferComposer(DeviceQueueType& device) {
            this->device = device;
            cache = createBuffer(device, strided<STRUCTURE>(64), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            stager = createBuffer(device, strided<STRUCTURE>(64), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        template<int BINDING, class STRUCTURE>
        int32_t BufferComposer<BINDING,STRUCTURE>::addElement(STRUCTURE accessorDesc) {
            int32_t ptr = data.size();
            data.push_back(accessorDesc);
            return ptr;
        };

        template<int BINDING, class STRUCTURE>
        BufferType& BufferComposer<BINDING,STRUCTURE>::getBuffer() {
            bufferSubData(stager, data, 0);
            copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, stager, cache, { 0, 0, strided<STRUCTURE>(data.size()) }, true);
            return cache;
        }

    }
}