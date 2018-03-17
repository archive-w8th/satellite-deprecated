#pragma once

#include "../../rtengine/vertexInstance.hpp"

namespace NSM
{
    namespace rt
    {

        void VertexInstance::init(DeviceQueueType &_device)
        {
            this->device = _device;

            std::vector<vk::DescriptorSetLayoutBinding> vertexInstanceDescreiptorBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // buffer data space
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // buffer regions
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // buffer views
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // data formats
                vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // data bindings (with buffer views)
                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // mesh uniforms
            };

            meshUniformData = std::vector<MeshUniformStruct>{ MeshUniformStruct() };
            meshUniformBuffer = createBuffer(device, strided<MeshUniformStruct>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            meshUniformStager = createBuffer(device, strided<MeshUniformStruct>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        void VertexInstance::syncUniform()
        {
            auto commandBuffer = getCommandBuffer(device, true);
            bufferSubData(commandBuffer, meshUniformStager, meshUniformData, 0);
            memoryCopyCmd(commandBuffer, meshUniformStager, meshUniformBuffer, { 0, 0, strided<MeshUniformStruct>(1) });
            flushCommandBuffer(device, commandBuffer, true);
        }

        VertexInstance::VertexInstance(VertexInstance &&another)
        {
            device = std::move(another.device);
            bufferViewSet = std::move(another.bufferViewSet);
            dataBindingSet = std::move(another.dataBindingSet);
            meshUniformData = std::move(another.meshUniformData);
        }

        VertexInstance::VertexInstance(VertexInstance &another)
        {
            device = another.device;
            bufferViewSet = another.bufferViewSet;
            dataBindingSet = another.dataBindingSet;
            meshUniformData = another.meshUniformData;
        }


        void VertexInstance::setBufferSpace(std::shared_ptr<BufferSpace> &buf)
        {
            this->bufferSpace = buf;
        }

        void VertexInstance::setBufferViewSet(std::shared_ptr<BufferViewSet> &bufferViewSet)
        {
            this->bufferViewSet = bufferViewSet;
        }

        void VertexInstance::setDataAccessSet(std::shared_ptr<DataAccessSet> &accessDataSet)
        {
            this->dataFormatSet = accessDataSet;
        }

        void VertexInstance::setBindingSet(std::shared_ptr<DataBindingSet> &bindingSet)
        {
            this->dataBindingSet = bindingSet;
        }


        VertexInstanceViews VertexInstance::getDescViewData(bool needUpdate)
        {
            if (needUpdate) {
                descViews.vInstanceBufferInfos[0] = this->getBufferSpaceBuffer()->descriptorInfo,
                descViews.vInstanceBufferInfos[1] = this->getBufferSpaceRegions()->descriptorInfo,
                descViews.vInstanceBufferInfos[2] = this->getBufferViewsBuffer()->descriptorInfo,
                descViews.vInstanceBufferInfos[3] = this->getDataFormatBuffer()->descriptorInfo,
                descViews.vInstanceBufferInfos[4] = this->getBufferBindingBuffer()->descriptorInfo,
                descViews.vInstanceBufferInfos[5] = this->getUniformBuffer()->descriptorInfo;
            }
            return descViews;
        };

        size_t VertexInstance::getNodeCount()
        {
            return meshUniformData[0].nodeCount;
        }

        // deprecated, indice or vertex index should be defined in buffer bindings
        //void VertexInstance::setLoadingOffset(const int32_t &off) {
        //meshUniformData[0].loadingOffset = off;
        //};

        // setting of accessors
        void VertexInstance::useIndex16bit(bool b16)
        {
            meshUniformData[0].int16bit = int(b16);
        }

        void VertexInstance::setVertexBinding(int32_t bindingID)
        {
            meshUniformData[0].vertexAccessor = bindingID;
        }

        void VertexInstance::setIndiceBinding(int32_t bindingID)
        {
            meshUniformData[0].indiceAccessor = bindingID;
        }

        void VertexInstance::setMaterialBinding(int32_t bindingID)
        {
            meshUniformData[0].materialAccessor = bindingID;
        }

        void VertexInstance::setNormalBinding(int32_t bindingID)
        {
            meshUniformData[0].normalAccessor = bindingID;
        }

        void VertexInstance::setTexcoordBinding(int32_t bindingID)
        {
            meshUniformData[0].texcoordAccessor = bindingID;
        }

        void VertexInstance::setModifierBinding(int32_t bindingID)
        {
            meshUniformData[0].modifierAccessor = bindingID;
        }

        void VertexInstance::setTransform(glm::mat4 t)
        {
            meshUniformData[0].transform = glm::transpose(t);
            meshUniformData[0].transformInv = glm::inverse(t);
        }

        void VertexInstance::setTransform(glm::dmat4 t)
        {
            this->setTransform(glm::mat4(t));
        }

        // getting of user defined
        void VertexInstance::setNodeCount(size_t tcount)
        {
            meshUniformData[0].nodeCount = tcount;
        }

        void VertexInstance::setMaterialOffset(int32_t id)
        {
            meshUniformData[0].materialID = id;
        }
    }
}
