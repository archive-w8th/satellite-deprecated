#pragma once

#include "textureSet.hpp"

namespace NSM {
    namespace rt {

        void TextureSet::init(DeviceQueueType& device) {
            this->device = device;

            textures = std::vector<ImageType>(0);
            freedomTextures = std::vector<size_t>(0);
        }

        TextureSet::TextureSet(TextureSet& another) {
            device = another.device;
            freedomTextures = another.freedomTextures;
            textures = another.textures;
        }

        TextureSet::TextureSet(TextureSet&& another) {
            device = std::move(another.device);
            freedomTextures = std::move(another.freedomTextures);
            textures = std::move(another.textures);
        }

        void TextureSet::freeTexture(const int32_t& idx) {
            freedomTextures.push_back(idx);
        }

        void TextureSet::clearTextures() {
            freedomTextures.resize(0);
            textures.resize(0);
        }

        void TextureSet::setTexture(const int32_t &idx, const ImageType& texture) {
            for (int i = 0; i < freedomTextures.size(); i++) {
                if (freedomTextures[i] == idx) freedomTextures.erase(freedomTextures.begin() + i);
            }
            if (textures.size() <= idx) textures.resize(idx+1);
            textures[idx] = texture;
        }

        int32_t TextureSet::loadTexture(const ImageType& texture) {
            int32_t idx = -1;
            if (freedomTextures.size() > 0) {
                idx = freedomTextures[freedomTextures.size() - 1];
                freedomTextures.pop_back();
                textures[idx] = texture;
            } else {
                idx = textures.size();
                textures.push_back(texture);
            }
            return idx;
        };

        bool TextureSet::haveTextures() {
            return textures.size() > 0;
        }

        std::vector<ImageType>& TextureSet::getTextures() {
            return textures;
        }

        // planned merge to externaled
#ifdef EXPERIMENTAL_GLTF
        int32_t TextureSet::loadTexture(tinygltf::Image* image, bool force_write) {
            if (!image) return -1;

            // TODO: grayscale support
            auto format = vk::Format::eR8G8B8A8Unorm;
            if (image->component == 3) format = vk::Format::eR8G8B8Unorm;

            // create texture
            auto texture = createTexture(device, vk::ImageViewType::e2D, { uint32_t(image->width), uint32_t(image->height), 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, format, 1);
            auto tstage = createBuffer(device, image->image.size() * sizeof(uint8_t), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

            // purple-black square
            {
                auto command = getCommandBuffer(device, true);
                bufferSubData(command, tstage, (const uint8_t *)image->image.data(), image->image.size(), 0);
                memoryCopyCmd(command, tstage, texture, vk::BufferImageCopy()
                    .setImageExtent({ uint32_t(image->width), uint32_t(image->height), 1 })
                    .setImageOffset({ 0, 0, 0 })
                    .setBufferOffset(0)
                    .setBufferRowLength(image->width)
                    .setBufferImageHeight(image->height)
                    .setImageSubresource(texture->subresourceLayers));
                flushCommandBuffer(device, command, [&]() { destroyBuffer(tstage); });
            }

            return this->loadTexture(texture);
        }
#endif

    }
}