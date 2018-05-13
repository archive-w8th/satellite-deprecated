#pragma once

#include "../structs.hpp"

namespace NSM
{
    namespace rt
    {

        template <int BINDING, class STRUCTURE>
        class BufferComposer : public std::enable_shared_from_this<BufferComposer<BINDING, STRUCTURE>>
        {
        protected:
            friend class BufferComposer<BINDING, STRUCTURE>;
            bool needUpdateBuffer = true;

        public:
            BufferComposer(BufferComposer<BINDING, STRUCTURE> &another);
            BufferComposer(BufferComposer<BINDING, STRUCTURE> &&another);
            BufferComposer(Queue &device, const size_t bsize = 1024 * 128);
            int32_t addElement(STRUCTURE accessorDesc);
            Buffer getBuffer();
            void resetStack() { needUpdateBuffer = true; data.resize(0); }
            uint32_t getCount() { return data.size(); }
            bool haveElements() { return data.size() > 0; }
            bool needUpdateStatus() { return needUpdateBuffer; }

            // borrow edit and view
            STRUCTURE& getStructure(intptr_t ptr = 0);
            STRUCTURE getStructure(intptr_t ptr = 0) const;

        protected:
            Buffer cache;
            Buffer stager;
            std::vector<STRUCTURE> data;

            Device device;
            Queue queue;
        };

        class BufferSpace
        {
        public:
            BufferSpace(Queue &device, const size_t space);
            Buffer getDataBuffer();

            intptr_t copyGPUBuffer(Buffer external, const size_t size);
            intptr_t copyGPUBuffer(Buffer external, const size_t size, const intptr_t offset);

            intptr_t copyHostBuffer(const uint8_t *external, const size_t size);
            intptr_t copyHostBuffer(const uint8_t *external, const size_t size, const intptr_t offset);

            template <class T>
            intptr_t copyHostBuffer(const std::vector<T> external, const intptr_t offset);
            intptr_t getLastKnownOffset();

            void resetOffsetCounter() { needUpdateSpaceDescs = true; lastKnownOffset = 0; }

        protected:
            Buffer dataStage;
            Buffer dataBuffer;
            Queue queue;
            Device device;

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