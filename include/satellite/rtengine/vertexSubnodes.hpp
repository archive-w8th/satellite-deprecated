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
            BufferComposer(DeviceQueueType& device, const size_t bsize = 1024 * 128);
            int32_t addElement(STRUCTURE accessorDesc);
            BufferType getBuffer(); 

            void resetStack() { data.resize(0); }

        protected:
            BufferType cache;
            BufferType stager;
            std::vector<STRUCTURE> data;
            DeviceQueueType device;
        };



        class BufferSpace {
        public:
            BufferSpace(DeviceQueueType& device, const size_t space);
            BufferType getDataBuffer();
            BufferType getRegionsBuffer();
            int32_t addRegionDesc( BufferRegion accessorDesc);

            intptr_t copyGPUBuffer( BufferType external, const size_t size);
            intptr_t copyGPUBuffer( BufferType external, const size_t size, const intptr_t offset);

            intptr_t copyHostBuffer(const uint8_t * external, const size_t size);
            intptr_t copyHostBuffer(const uint8_t * external, const size_t size, const intptr_t offset);

            template<class T>
            intptr_t copyHostBuffer(const std::vector<T> external, const intptr_t offset);

            intptr_t getLastKnownOffset();

            void resetRegionStack() { regions.resize(0); }
            void resetOffsetCounter() { lastKnownOffset = 0; }

        protected:
            std::vector<BufferRegion> regions;
            BufferType regionsStage;
            BufferType regionsBuffer;
            BufferType dataStage;
            BufferType dataBuffer;
            DeviceQueueType device;
            intptr_t lastKnownOffset = 0;
        };


        using BufferViewSet = BufferComposer<3, VirtualBufferView>;
        using DataAccessSet = BufferComposer<4, VirtualDataAccess>;
        using DataBindingSet = BufferComposer<5, VirtualBufferBinding>;
        
    }
}