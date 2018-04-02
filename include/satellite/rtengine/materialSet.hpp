#pragma once

#include "./structs.hpp"
#include "./vertexSubnodes.hpp"

namespace NSM
{
    namespace rt
    {
        using MaterialSet = BufferComposer<2, VirtualMaterial>;
        using VirtualTextureSet = BufferComposer<3, VirtualTexture>;


        /*
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
            bool needUpdate = true;

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
            void resetMaterialSet() { materials.resize(0); needUpdate = true; }
            void resetVirtualTextures() { vtextures.resize(0); needUpdate = true; }
            
        };
        */

    } // namespace rt
} // namespace NSM
