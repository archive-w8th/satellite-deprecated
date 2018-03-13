#pragma once

#include "./structs.hpp"

namespace NSM
{
    namespace rt
    {

        class MaterialSet
        {
        protected:
            DeviceQueueType device;

            std::vector<VirtualMaterial> materials;
            std::vector<glm::uvec2> vtextures;

            BufferType countBuffer;
            BufferType materialBuffer;
            BufferType materialStaging;
            BufferType vtexturesBuffer;


            intptr_t loadOffset = 0;
            void init(DeviceQueueType &device);

            

        public:
            MaterialSet() {}
            MaterialSet(DeviceQueueType &device) { init(device); }

            // copying from material set
            MaterialSet(MaterialSet &another);

            // directly from material set
            MaterialSet(MaterialSet &&another);

            size_t addVTexture(const glm::uvec2& vtex);
            size_t addMaterial(const VirtualMaterial &submat);
            void setSumbat(const size_t &i, const VirtualMaterial &submat);
            void loadToVGA();
            size_t getMaterialCount();
            void setLoadingOffset(intptr_t loadOffset);

            // planned descriptor template instead of...
            BufferType &getCountBuffer();
            BufferType &getMaterialBuffer();
            BufferType &getVTextureBuffer();

            bool haveMaterials();

            // resets
            void resetMaterialSet() { materials.resize(0); }
            void resetVirtualTextures() { vtextures.resize(0); }
            
        };

    } // namespace rt
} // namespace NSM
