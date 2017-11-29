#pragma once

#include "../utils.hpp"
#include "./vertexSubnodes.hpp"

namespace NSM {
    namespace rt {


        class VertexInstance {
        protected:
            friend class VertexInstance;

            DeviceQueueType device;

            void init(DeviceQueueType& device) {
                this->device = device;
                meshUniformData = std::vector<MeshUniformStruct>(1);
                meshUniformBuffer = createBuffer(device, strided<MeshUniformStruct>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                meshUniformStager = createBuffer(device, strided<MeshUniformStruct>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            }

            void syncUniform() {
                bufferSubData(meshUniformStager, meshUniformData, 0);
                copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, meshUniformStager, meshUniformBuffer, { 0, 0, strided<MeshUniformStruct>(1) }, true);
            }

            std::shared_ptr<BufferViewSet> bufferViewSet;
            std::shared_ptr<AccessorSet> accessorSet;
            std::vector<MeshUniformStruct> meshUniformData;

            BufferType meshUniformStager;
            BufferType meshUniformBuffer;
            BufferType dataBuffer;
            BufferType materialBuffer;
            BufferType indicesBuffer;
            //Buffer indirectBuffer;

        public:
            bool index16bit = false;

            VertexInstance() {}

            VertexInstance(DeviceQueueType& device) {
                init(device);
            }



            VertexInstance(VertexInstance&& another) {
                device = std::move(another.device);
                bufferViewSet = std::move(another.bufferViewSet);
                accessorSet = std::move(another.accessorSet);
                meshUniformData = std::move(another.meshUniformData);
                meshUniformStager = std::move(another.meshUniformStager);
                meshUniformBuffer = std::move(another.meshUniformBuffer);
                dataBuffer = std::move(another.dataBuffer);
                materialBuffer = std::move(another.materialBuffer);
                indicesBuffer = std::move(another.indicesBuffer);
            }

            VertexInstance(VertexInstance& another) {
                device = another.device;
                bufferViewSet = another.bufferViewSet;
                accessorSet = another.accessorSet;
                meshUniformData = another.meshUniformData;
                meshUniformStager = another.meshUniformStager;
                meshUniformBuffer = another.meshUniformBuffer;
                dataBuffer = another.dataBuffer;
                materialBuffer = another.materialBuffer;
                indicesBuffer = another.indicesBuffer;
            }


            void setNodeCount(size_t tcount) {
                uint32_t tiledWork = tiled(tcount, 128);
                //glNamedBufferSubData(indirect_dispatch_buffer, 0, sizeof(uint32_t), &tiledWork);
                meshUniformData[0].nodeCount = tcount;
            }

            void setMaterialOffset(int32_t id) {
                meshUniformData[0].materialID = id;
            }

            void useIndex16bit(bool b16) {
                index16bit = b16;
            }

            void setTransform(glm::mat4 t) {
                meshUniformData[0].transform = glm::transpose(t);
                meshUniformData[0].transformInv = glm::inverse(t);
            }

            void setTransform(glm::dmat4 t) {
                this->setTransform(glm::mat4(t));
            }

            void setIndexed(const int32_t b) {
                meshUniformData[0].isIndexed = b;
            }

            void setDataBuffer(const BufferType &buf) {
                dataBuffer = buf;
            };

            void setIndicesBuffer(const BufferType &buf) {
                indicesBuffer = buf;
            };

            size_t getNodeCount() {
                return meshUniformData[0].nodeCount;
            }


            // getting buffer
            BufferType& getDataBuffer() {
                return dataBuffer;
            };

            BufferType& getIndicesBuffer() {
                return indicesBuffer;
            };

            BufferType& getUniformBuffer() {
                syncUniform();
                return meshUniformBuffer;
            }


            BufferType& getAccessorsBuffer() {
                return this->accessorSet->getBuffer();
            }

            BufferType& getBufferViewsBuffer() {
                return this->bufferViewSet->getBuffer();
            }

            BufferType& getMaterialIndicesBuffer() {
                return this->materialBuffer;
            }


            void setLoadingOffset(const int32_t &off) {
                meshUniformData[0].loadingOffset = off;
            };

            // setting of accessors
            void setVertexAccessor(int32_t accessorID) {
                meshUniformData[0].vertexAccessor = accessorID;
            }

            void setNormalAccessor(int32_t accessorID) {
                meshUniformData[0].normalAccessor = accessorID;
            }

            void setTexcoordAccessor(int32_t accessorID) {
                meshUniformData[0].texcoordAccessor = accessorID;
            }

            void setModifierAccessor(int32_t accessorID) {
                meshUniformData[0].modifierAccessor = accessorID;
            }

            void setAccessorSet(std::shared_ptr<AccessorSet>& accessorSet) {
                this->accessorSet = accessorSet;
            }

            void setBufferViewSet(std::shared_ptr<BufferViewSet>& bufferViewSet) {
                this->bufferViewSet = bufferViewSet;
            }


        };

    }
}
