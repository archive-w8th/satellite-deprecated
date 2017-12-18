#pragma once

#include "../vertexInstance.hpp"

namespace NSM {
    namespace rt {

        void VertexInstance::init(DeviceQueueType& device) {
            this->device = device;
            meshUniformData = std::vector<MeshUniformStruct>(1);
            meshUniformBuffer = createBuffer(device, strided<MeshUniformStruct>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            meshUniformStager = createBuffer(device, strided<MeshUniformStruct>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        void VertexInstance::syncUniform() {
            bufferSubData(meshUniformStager, meshUniformData, 0);
            copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, meshUniformStager, meshUniformBuffer, { 0, 0, strided<MeshUniformStruct>(1) }, true);
        }

        VertexInstance::VertexInstance(VertexInstance&& another) {
            device = std::move(another.device);
            bufferViewSet = std::move(another.bufferViewSet);
            dataBindingSet = std::move(another.dataBindingSet);
            meshUniformData = std::move(another.meshUniformData);
        }

        VertexInstance::VertexInstance(VertexInstance& another) {
            device = another.device;
            bufferViewSet = another.bufferViewSet;
            dataBindingSet = another.dataBindingSet;
            meshUniformData = another.meshUniformData;
        }


        void VertexInstance::useIndex16bit(bool b16) {
            index16bit = b16;
        }

        void VertexInstance::setBufferSpace(BufferSpace &buf) {
            bufferSpace = std::make_shared<BufferSpace>(buf);
        };


        void VertexInstance::setAccessorSet(std::shared_ptr<DataBindingSet>& accessorSet) {
            this->dataBindingSet = accessorSet;
        }

        void VertexInstance::setBufferViewSet(std::shared_ptr<BufferViewSet>& bufferViewSet) {
            this->bufferViewSet = bufferViewSet;
        }






        void VertexInstance::setNodeCount(size_t tcount) {
            meshUniformData[0].nodeCount = tcount;
        }

        void VertexInstance::setMaterialOffset(int32_t id) {
            meshUniformData[0].materialID = id;
        }

        void VertexInstance::setTransform(glm::mat4 t) {
            meshUniformData[0].transform = glm::transpose(t);
            meshUniformData[0].transformInv = glm::inverse(t);
        }

        void VertexInstance::setTransform(glm::dmat4 t) {
            this->setTransform(glm::mat4(t));
        }

        void VertexInstance::setIndexed(const int32_t b) {
            meshUniformData[0].isIndexed = b;
        }

        size_t VertexInstance::getNodeCount() {
            return meshUniformData[0].nodeCount;
        }

        // obsolete, indice or vertex index should be defined in buffer bindings 
        //void VertexInstance::setLoadingOffset(const int32_t &off) {
            //meshUniformData[0].loadingOffset = off;
        //};

        // setting of accessors
        void VertexInstance::setVertexBinding(int32_t accessorID) {
            meshUniformData[0].vertexAccessor = accessorID;
        }

        void VertexInstance::setIndiceBinding(int32_t accessorID) {
            meshUniformData[0].indiceAccessor = accessorID;
        }

        void VertexInstance::setMaterialBinding(int32_t accessorID) {
            meshUniformData[0].materialAccessor = accessorID;
        }

        void VertexInstance::setNormalBinding(int32_t accessorID) {
            meshUniformData[0].normalAccessor = accessorID;
        }

        void VertexInstance::setTexcoordBinding(int32_t accessorID) {
            meshUniformData[0].texcoordAccessor = accessorID;
        }

        void VertexInstance::setModifierBinding(int32_t accessorID) {
            meshUniformData[0].modifierAccessor = accessorID;
        }

        

    }
}
