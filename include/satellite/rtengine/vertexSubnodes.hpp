#pragma once

#include "../utils.hpp"
#include "./structs.hpp"

namespace NSM {
    namespace rt {

        template<int BINDING, class STRUCTURE>
        class BufferComposer {
        protected:
            friend class BufferComposer<BINDING, STRUCTURE>;

        public:
            BufferComposer(BufferComposer<BINDING, STRUCTURE>& another) {
                cache = another.cache;
                stager = another.stager;
                data = another.data;
                device = another.device;
            }

            BufferComposer(BufferComposer<BINDING, STRUCTURE>&& another) {
                cache = std::move(another.cache);
                stager = std::move(another.stager);
                data = std::move(another.data);
                device = std::move(another.device);
            }

            BufferComposer(DeviceQueueType& device) {
                this->device = device;
                cache = createBuffer(device, strided<STRUCTURE>(64), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                stager = createBuffer(device, strided<STRUCTURE>(64), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            }

            int32_t addElement(STRUCTURE accessorDesc) {
                int32_t ptr = data.size();
                data.push_back(accessorDesc);
                return ptr;
            };

            BufferType& getBuffer() {
                bufferSubData(stager, data, 0);
                copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, stager, cache, { 0, 0, strided<STRUCTURE>(data.size()) }, true);
                return cache;
            }

        protected:
            BufferType cache;
            BufferType stager;
            std::vector<STRUCTURE> data;
            DeviceQueueType device;
        };

        using AccessorSet = BufferComposer<7, VirtualAccessor>;
        using BufferViewSet = BufferComposer<8, VirtualBufferView>;
    }
}