#pragma once

#include "./structs.hpp"

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
            std::vector<TextureType> textures;

        public:
            TextureSet() {}
            TextureSet(DeviceQueueType &device) { init(device); }
            TextureSet(TextureSet &another);
            TextureSet(TextureSet &&another);

            void freeTexture(const int32_t &idx);
            void setTexture(const int32_t &idx, const TextureType &texture);

            void clearTextures();
            bool haveTextures();
            std::vector<TextureType> &getTextures();

            int32_t loadTexture(const TextureType &texture);
            int32_t loadTexture(std::string tex, bool force_write = false);
        };
    } // namespace rt
} // namespace NSM