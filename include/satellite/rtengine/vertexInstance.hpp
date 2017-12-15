#pragma once

#include "./structs.hpp"
#include "./vertexSubnodes.hpp"

namespace NSM {
    namespace rt {
        class VertexInstance {
        protected:
            friend class VertexInstance;

            DeviceQueueType device;

            void init(DeviceQueueType& device);
            void syncUniform();

            std::shared_ptr<BufferViewSet> bufferViewSet;
            std::shared_ptr<AccessorSet> accessorSet;
            std::vector<MeshUniformStruct> meshUniformData;

            BufferType meshUniformStager;
            BufferType meshUniformBuffer;
            BufferType dataBuffer;
            BufferType materialBuffer;
            BufferType indicesBuffer;

        public:
            bool index16bit = false;

            VertexInstance() {}
            VertexInstance(DeviceQueueType& device) { init(device); }
            VertexInstance(VertexInstance&& another);
            VertexInstance(VertexInstance& another);

            void setNodeCount(size_t tcount);
            void setMaterialOffset(int32_t id);
            void useIndex16bit(bool b16);
            void setTransform(glm::mat4 t);
            void setTransform(glm::dmat4 t);
            void setIndexed(const int32_t b);
            void setDataBuffer(const BufferType &buf);
            void setIndicesBuffer(const BufferType &buf);
            void setLoadingOffset(const int32_t &off);
            size_t getNodeCount();

            // getting buffer
            BufferType& getDataBuffer();
            BufferType& getIndicesBuffer();
            BufferType& getUniformBuffer();
            BufferType& getAccessorsBuffer();
            BufferType& getBufferViewsBuffer();
            BufferType& getMaterialIndicesBuffer();

            // accessors
            void setVertexAccessor(int32_t accessorID);
            void setNormalAccessor(int32_t accessorID);
            void setTexcoordAccessor(int32_t accessorID);
            void setModifierAccessor(int32_t accessorID);
            void setAccessorSet(std::shared_ptr<AccessorSet>& accessorSet);
            void setBufferViewSet(std::shared_ptr<BufferViewSet>& bufferViewSet);
        };
    }
}
