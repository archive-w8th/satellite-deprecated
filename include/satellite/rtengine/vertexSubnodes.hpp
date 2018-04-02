#pragma once

#include "./structs.hpp"

namespace NSM
{
    namespace rt
    {

        template <int BINDING, class STRUCTURE>
        class BufferComposer
        {
        protected:
            friend class BufferComposer<BINDING, STRUCTURE>;
            bool needUpdateBuffer = true;

        public:
            BufferComposer(BufferComposer<BINDING, STRUCTURE> &another);
            BufferComposer(BufferComposer<BINDING, STRUCTURE> &&another);
            BufferComposer(DeviceQueueType &device, const size_t bsize = 1024 * 128);
            int32_t addElement(STRUCTURE accessorDesc);
            BufferType getBuffer();
            void resetStack() { needUpdateBuffer = true; data.resize(0); }
            uint32_t getCount() { return data.size(); }
            bool haveElements() { return data.size() > 0; }
            bool needUpdateStatus() { return needUpdateBuffer; }

            // borrow edit and view
            STRUCTURE& getStructure(intptr_t ptr = 0);
            STRUCTURE getStructure(intptr_t ptr = 0) const;

        protected:
            BufferType cache;
            BufferType stager;
            std::vector<STRUCTURE> data;
            DeviceQueueType device;
        };

        class BufferSpace
        {
        public:
            BufferSpace(DeviceQueueType &device, const size_t space);
            BufferType getDataBuffer();

            intptr_t copyGPUBuffer(BufferType external, const size_t size);
            intptr_t copyGPUBuffer(BufferType external, const size_t size, const intptr_t offset);

            intptr_t copyHostBuffer(const uint8_t *external, const size_t size);
            intptr_t copyHostBuffer(const uint8_t *external, const size_t size, const intptr_t offset);

            template <class T>
            intptr_t copyHostBuffer(const std::vector<T> external, const intptr_t offset);
            intptr_t getLastKnownOffset();

            void resetOffsetCounter() { needUpdateSpaceDescs = true; lastKnownOffset = 0; }

        protected:
            BufferType dataStage;
            BufferType dataBuffer;
            DeviceQueueType device;
            intptr_t lastKnownOffset = 0;

            bool needUpdateSpaceDescs = true;
        };

        using BufferRegionSet = BufferComposer<1, VirtualBufferRegion>;
        using BufferViewSet = BufferComposer<2, VirtualBufferView>;
        using DataAccessSet = BufferComposer<3, VirtualDataAccess>;
        using DataBindingSet = BufferComposer<4, VirtualBufferBinding>;
        using MeshUniformSet = BufferComposer<5, MeshUniformStruct>;
    } // namespace rt
} // namespace NSM