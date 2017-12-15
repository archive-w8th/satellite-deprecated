#pragma once

#include "./structs.hpp"
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
            void init(DeviceQueueType& device);

        public:
            MaterialSet() {}
            MaterialSet(DeviceQueueType& device) {
                init(device);
            }

            // copying from material set
            MaterialSet(MaterialSet& another);

            // directly from material set
            MaterialSet(MaterialSet&& another);

            size_t addMaterial(const VirtualMaterial& submat);
            void setSumbat(const size_t& i, const VirtualMaterial &submat);
            void loadToVGA();
            size_t getMaterialCount();
            void setLoadingOffset(intptr_t loadOffset);
            BufferType& getCountBuffer();
            BufferType& getMaterialBuffer();
            bool haveMaterials();
        };

    }
}