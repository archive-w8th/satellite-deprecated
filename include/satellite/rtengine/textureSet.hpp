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

            Device device;
            Queue queue;

            void init(Queue &queue);

            std::vector<size_t> freedomTextures;
            std::vector<Image> textures;

        public:
            TextureSet() {}
            TextureSet(Queue &device) { init(device); }
            TextureSet(TextureSet &another);
            TextureSet(TextureSet &&another);

            void freeTexture(const int32_t &idx);
            void setTexture(const int32_t &idx, const Image &texture);

            void clearTextures();
            bool haveTextures();
            std::vector<Image> &getTextures();

            int32_t loadTexture(const Image &texture);

            // planned merge to externaled
#ifdef EXPERIMENTAL_GLTF
            int32_t loadTexture(tinygltf::Image* image, bool force_write = false);
#endif
        };
    } // namespace rt
} // namespace NSM