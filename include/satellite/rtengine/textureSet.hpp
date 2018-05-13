#pragma once

#include "./structs.hpp"

#ifdef EXPERIMENTAL_GLTF
#include "tiny_gltf.h"
#endif

namespace NSM
{
    namespace rt
    {
        class TextureSet
        {
        protected:
            friend class TextureSet;

            DeviceQueueType device;
            void init(DeviceQueueType &device);

            std::vector<size_t> freedomTextures;
            std::vector<ImageType> textures;

        public:
            TextureSet() {}
            TextureSet(DeviceQueueType &device) { init(device); }
            TextureSet(TextureSet &another);
            TextureSet(TextureSet &&another);

            void freeTexture(const int32_t &idx);
            void setTexture(const int32_t &idx, const ImageType &texture);

            void clearTextures();
            bool haveTextures();
            std::vector<ImageType> &getTextures();

            int32_t loadTexture(const ImageType &texture);

            // planned merge to externaled
#ifdef EXPERIMENTAL_GLTF
            int32_t loadTexture(tinygltf::Image* image, bool force_write = false);
#endif
        };
    } // namespace rt
} // namespace NSM