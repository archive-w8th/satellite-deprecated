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

            Queue queue;
            Device device;

            void init(Queue &device);
            //void syncUniform();

            std::shared_ptr<MeshUniformSet> meshUniformSet;
            std::shared_ptr<BufferSpace> bufferSpace;
            std::shared_ptr<BufferViewSet> bufferViewSet; // pointer to buffer view
            std::shared_ptr<DataAccessSet> dataFormatSet;
            std::shared_ptr<DataBindingSet> dataBindingSet;
            std::shared_ptr<BufferRegionSet> bufferRegions;

            std::vector<MeshUniformStruct> meshUniformData;
            Buffer meshUniformStager, meshUniformBuffer;

            VertexInstanceViews descViews;
            bool needUpdateUniform = true;
            size_t ucount = 1;
            size_t uptr = 0;

        public:
            VertexInstance() {}
            VertexInstance(Queue &device) { init(device); }
            VertexInstance(VertexInstance &&another);
            VertexInstance(VertexInstance &another);

            /*
            size_t getNodeCount();
            void setNodeCount(size_t tcount);
            void setMaterialOffset(int32_t id);
            void useIndex16bit(bool b16);
            void setTransform(glm::mat4 t);
            void setTransform(glm::dmat4 t);
            void setMaterialBinding(int32_t bindingID);
            void setIndiceBinding(int32_t bindingID);
            void setVertexBinding(int32_t bindingID);
            void setNormalBinding(int32_t bindingID);
            void setTexcoordBinding(int32_t bindingID);
            void setModifierBinding(int32_t bindingID);
            */

            // data access setters
            void setBufferSpace(std::shared_ptr<BufferSpace> &buf);
            void setBindingSet(std::shared_ptr<DataBindingSet> &bindingSet);
            void setBufferViewSet(std::shared_ptr<BufferViewSet> &bufferViewSet);
            void setDataAccessSet(std::shared_ptr<DataAccessSet> &accessDataSet);
            void setBufferRegionSet(std::shared_ptr<BufferRegionSet>& regionSet);
            void setUniformSet(std::shared_ptr<MeshUniformSet>& uniforms);
            VertexInstanceViews getDescViewData(bool needUpdate = true);

            std::shared_ptr<MeshUniformSet>& getUniformSet() { return meshUniformSet; };

            //VertexInstance& setUPtr(size_t p) { this->uptr = p; return *this; }
            //void makeMultiVersion(size_t ucount);

        protected:
            // getters of buffers
            Buffer getBufferSpaceBuffer() { return bufferSpace->getDataBuffer(); };
            Buffer getBufferSpaceRegions(){ return bufferRegions->getBuffer(); };
            Buffer getBufferViewsBuffer() { return bufferViewSet->getBuffer(); };
            Buffer getDataFormatBuffer() { return dataFormatSet->getBuffer(); };
            Buffer getBufferBindingBuffer() { return dataBindingSet->getBuffer(); };
            Buffer getUniformBuffer() { return meshUniformSet->getBuffer(); };
        };
    } // namespace rt
} // namespace NSM
