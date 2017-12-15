#pragma once

#include "./structs.hpp"

namespace NSM {
    namespace rt {
        template<int BINDING, class STRUCTURE>
        class BufferComposer {
        protected:
            friend class BufferComposer<BINDING, STRUCTURE>;

        public:
            BufferComposer(BufferComposer<BINDING, STRUCTURE>& another);
            BufferComposer(BufferComposer<BINDING, STRUCTURE>&& another);
            BufferComposer(DeviceQueueType& device);
            int32_t addElement(STRUCTURE accessorDesc);
            BufferType& getBuffer(); 

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