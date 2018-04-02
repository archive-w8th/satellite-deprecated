#pragma once

#include "../../rtengine/materialSet.hpp"

namespace NSM
{
    namespace rt
    {

        /*
        void MaterialSet::init(DeviceQueueType &device)
        {
            this->device = device;
            vtextures = std::vector<glm::uvec2>(0);
            materials = std::vector<VirtualMaterial>(0);
            countBuffer = createBuffer(device, strided<uint32_t>(8), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            materialStaging = createBuffer(device, strided<VirtualMaterial>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            materialBuffer = createBuffer(device, strided<VirtualMaterial>(1024), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            vtexturesBuffer = createBuffer(device, strided<glm::uvec2>(1024), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            needUpdate = true;
        }

        // copying from material set
        MaterialSet::MaterialSet(MaterialSet &another)
        {
            device = another.device;
            vtextures = another.vtextures;
            materials = another.materials;
            countBuffer = another.countBuffer;
            materialBuffer = another.materialBuffer;
            materialStaging = another.materialStaging;
            vtexturesBuffer = another.vtexturesBuffer;
            loadOffset = another.loadOffset;
            needUpdate = true;
        }

        // directly from material set
        MaterialSet::MaterialSet(MaterialSet &&another)
        {
            device = std::move(another.device);
            vtextures = std::move(another.vtextures);
            materials = std::move(another.materials);
            countBuffer = std::move(another.countBuffer);
            materialBuffer = std::move(another.materialBuffer);
            materialStaging = std::move(another.materialStaging);
            vtexturesBuffer = std::move(another.vtexturesBuffer);
            loadOffset = std::move(another.loadOffset);
            needUpdate = true;
        }

        size_t MaterialSet::addVTexture(const glm::uvec2& vtex) {
            size_t idx = vtextures.size();
            vtextures.push_back(vtex);
            needUpdate = true;
            return idx+1;
        }

        size_t MaterialSet::addMaterial(const VirtualMaterial &submat)
        {
            size_t idx = materials.size();
            materials.push_back(submat);
            needUpdate = true;
            return idx;
        }

        void MaterialSet::setSumbat(const size_t &i, const VirtualMaterial &submat)
        {
            if (materials.size() <= i) materials.resize(i + 1);
            materials[i] = submat;
            needUpdate = true;
        }

        void MaterialSet::loadToVGA()
        {
            if (!haveMaterials()) return;

            if (needUpdate) {
                // copy materials
                {
                    auto commandBuffer = getCommandBuffer(device, true);
                    bufferSubData(commandBuffer, countBuffer, std::vector<int32_t>{int32_t(loadOffset), int32_t(materials.size())}, 0);
                    bufferSubData(commandBuffer, materialStaging, materials, 0);
                    memoryCopyCmd(commandBuffer, materialStaging, materialBuffer, { 0, 0, strided<VirtualMaterial>(materials.size()) });
                    flushCommandBuffer(device, commandBuffer, true);
                }

                // copy virtual textures
                {
                    auto commandBuffer = getCommandBuffer(device, true);
                    bufferSubData(commandBuffer, materialStaging, vtextures, 0);
                    memoryCopyCmd(commandBuffer, materialStaging, vtexturesBuffer, { 0, 0, strided<glm::uvec2>(vtextures.size()) });
                    flushCommandBuffer(device, commandBuffer, true);
                }
            }
            needUpdate = false;
        }

        size_t MaterialSet::getMaterialCount()
        {
            return materials.size();
        }

        void MaterialSet::setLoadingOffset(intptr_t loadOffset)
        {
            this->loadOffset = loadOffset;
            needUpdate = true;
        }

        BufferType &MaterialSet::getCountBuffer()
        {
            return countBuffer;
        }

        BufferType &MaterialSet::getMaterialBuffer()
        {
            return materialBuffer;
        }

        BufferType &MaterialSet::getVTextureBuffer()
        {
            return vtexturesBuffer;
        }

        bool MaterialSet::haveMaterials()
        {
            return materials.size() > 0;
        }
        */
    }
}