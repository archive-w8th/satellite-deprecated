#pragma once

#include "../utils.hpp"
#include "./textureSet.hpp"

namespace NSM {

    namespace rt {

        class MaterialSet {
        protected:
            DeviceQueueType device;
            std::vector<VirtualMaterial> materials;
            std::shared_ptr<TextureSet> textureSet;
            BufferType countBuffer;
            BufferType materialBuffer;
            BufferType materialStaging;
            intptr_t loadOffset = 0;

            void init(DeviceQueueType& device) {
                this->device = device;

                materials = std::vector<VirtualMaterial>(0);
                countBuffer = createBuffer(device, strided<uint32_t>(8), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
                materialStaging = createBuffer(device, strided<VirtualMaterial>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
                materialBuffer = createBuffer(device, strided<VirtualMaterial>(1024), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            }

        public:
            MaterialSet() {}
            MaterialSet(DeviceQueueType& device) {
                init(device);
            }


            // copying from material set
            MaterialSet(MaterialSet& another) {
                device = another.device;
                materials = another.materials;
                textureSet = another.textureSet;
                countBuffer = another.countBuffer;
                materialBuffer = another.materialBuffer;
                materialStaging = another.materialStaging;
                loadOffset = another.loadOffset;
            }


            // directly from material set
            MaterialSet(MaterialSet&& another) {
                device = std::move(another.device);
                materials = std::move(another.materials);
                textureSet = std::move(another.textureSet);
                countBuffer = std::move(another.countBuffer);
                materialBuffer = std::move(another.materialBuffer);
                materialStaging = std::move(another.materialStaging);
                loadOffset = std::move(another.loadOffset);
            }



            size_t addMaterial(const VirtualMaterial& submat) {
                size_t idx = materials.size();
                materials.push_back(submat);
                return idx;
            }

            void setSumbat(const size_t& i, const VirtualMaterial &submat) {
                if (materials.size() <= i) materials.resize(i + 1);
                materials[i] = submat;
            }

            void loadToVGA() {
                if (!haveMaterials()) return;

                std::vector<int32_t> offsetSize = { int32_t(loadOffset), int32_t(materials.size()) };
                bufferSubData(countBuffer, offsetSize, 0);
                bufferSubData(materialStaging, materials, 0);
                copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, materialStaging, materialBuffer, { 0, 0, strided<VirtualMaterial>(materials.size()) }, true);
            }

            size_t getMaterialCount() {
                return materials.size();
            }

            void setLoadingOffset(intptr_t loadOffset) {
                this->loadOffset = loadOffset;
            }

            BufferType& getCountBuffer() {
                return countBuffer;
            }

            BufferType& getMaterialBuffer() {
                return materialBuffer;
            }

            bool haveMaterials() {
                return materials.size() > 0;
            }
        };

    }

}