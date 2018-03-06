#pragma once

#include "./structs.hpp"
#include "./vertexSubnodes.hpp"

namespace NSM
{
    namespace rt
    {
        class VertexInstance
        {
        protected:
            friend class VertexInstance;

            DeviceQueueType device;

            void init(DeviceQueueType &device);
            void syncUniform();

            std::shared_ptr<BufferSpace> bufferSpace;
            std::shared_ptr<BufferViewSet> bufferViewSet; // pointer to buffer view
            std::shared_ptr<DataAccessSet> dataFormatSet;
            std::shared_ptr<DataBindingSet> dataBindingSet;
            std::vector<MeshUniformStruct> meshUniformData;
            BufferType meshUniformStager, meshUniformBuffer;

        public:
            VertexInstance() {}
            VertexInstance(DeviceQueueType &device) { init(device); }
            VertexInstance(VertexInstance &&another);
            VertexInstance(VertexInstance &another);

            void setNodeCount(size_t tcount);
            void setMaterialOffset(int32_t id);
            void useIndex16bit(bool b16);
            void setTransform(glm::mat4 t);
            void setTransform(glm::dmat4 t);
            void setBufferSpace(std::shared_ptr<BufferSpace> &buf);
            size_t getNodeCount();

            // data access setters
            void setMaterialBinding(int32_t bindingID);
            void setIndiceBinding(int32_t bindingID);
            void setVertexBinding(int32_t bindingID);
            void setNormalBinding(int32_t bindingID);
            void setTexcoordBinding(int32_t bindingID);
            void setModifierBinding(int32_t bindingID);
            void setBindingSet(std::shared_ptr<DataBindingSet> &bindingSet);
            void setBufferViewSet(std::shared_ptr<BufferViewSet> &bufferViewSet);
            void setDataAccessSet(std::shared_ptr<DataAccessSet> &accessDataSet);

            // getters of buffers
            BufferType getBufferSpaceBuffer() { return bufferSpace->getDataBuffer(); };
            BufferType getBufferSpaceRegions()
            {
                return bufferSpace->getRegionsBuffer();
            };
            BufferType getBufferViewsBuffer() { return bufferViewSet->getBuffer(); };
            BufferType getDataFormatBuffer() { return dataFormatSet->getBuffer(); };
            BufferType getBufferBindingBuffer() { return dataBindingSet->getBuffer(); };
            BufferType getUniformBuffer()
            {
                syncUniform();
                return meshUniformBuffer;
            };
        };
    } // namespace rt
} // namespace NSM
