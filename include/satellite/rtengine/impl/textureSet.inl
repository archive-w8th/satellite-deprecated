//#pragma once

#include "./textureSet.hpp"

namespace NSM {
    namespace rt {

        void TextureSet::init(DeviceQueueType& device) {
            this->device = device;

            textures = std::vector<TextureType>(0);
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

        void TextureSet::freeTexture(const uint32_t& idx) {
            freedomTextures.push_back(idx-1);
        }

        void TextureSet::clearTextures() {
            //for (int i = 1; i <= textures.size(); i++) { this->freeTexture(i); }
            freedomTextures.resize(0);
            textures.resize(0);
        }

        void TextureSet::setTexture(uint32_t location, const TextureType& texture) {
            for (int i = 0; i < freedomTextures.size();i++) {
                if (freedomTextures[i] == location-1) freedomTextures.erase(freedomTextures.begin() + i);
            }
            if (textures.size() < location) textures.resize(location);
            textures[location-1] = texture;
        }

        uint32_t TextureSet::loadTexture(const TextureType& texture) {
            //if (idx && idx >= 0 && idx != -1) return idx;
            uint32_t idx = 0;
            if (freedomTextures.size() > 0) {
                idx = freedomTextures[freedomTextures.size() - 1];
                freedomTextures.pop_back();
                textures[idx] = texture;
            }
            else {
                textures.push_back(texture);
                idx = textures.size();
            }
            return idx;
        };

        bool TextureSet::haveTextures() {
            return textures.size() > 0;
        }

        std::vector<TextureType>& TextureSet::getTextures() {
            return textures;
        }

#ifdef USE_CIMG
        uint32_t TextureSet::loadTexture(std::string tex, bool force_write) {
            //struct uint8_rgba { uint8_t r, g, b, a; };

            cil::CImg<uint8_t> image(tex.c_str());
            uint32_t width = image.width(), height = image.height(), spectrum = image.spectrum();
            image.channels(0, 3);
            if (spectrum == 3) image.get_shared_channel(3).fill(255); // if RGB, will alpha channel
            //image.mirror("y");
            image.permute_axes("cxyz");
            

            // create texture
            auto texture = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, { width, height, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR8G8B8A8Unorm, 1);
            auto tstage = createBuffer(device, image.size() * sizeof(uint8_t), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

            auto command = getCommandBuffer(device, true);
            imageBarrier(command, texture);
            flushCommandBuffer(device, command, true);

            // purple-black square
            bufferSubData(tstage, (const uint8_t *)image.data(), image.size() * sizeof(uint8_t), 0);

            {
                auto bufferImageCopy = vk::BufferImageCopy()
                    .setImageExtent({ width, height, 1 })
                    .setImageOffset({ 0, 0, 0 })
                    .setBufferOffset(0)
                    .setBufferRowLength(width)
                    .setBufferImageHeight(height)
                    .setImageSubresource(texture->subresourceLayers);

                copyMemoryProxy<BufferType&, TextureType&, vk::BufferImageCopy>(device, tstage, texture, bufferImageCopy, [&]() {
                    destroyBuffer(tstage);
                });
            }

            return this->loadTexture(texture);


        }
#endif

#ifdef USE_FREEIMAGE
        uint32_t TextureSet::loadTexture(std::string tex, bool force_write) {
            FREE_IMAGE_FORMAT formato = FreeImage_GetFileType(tex.c_str(), 0);
            if (formato == FIF_UNKNOWN) {
                return 0;
            }
            FIBITMAP* imagen = FreeImage_Load(formato, tex.c_str());
            if (!imagen) {
                return 0;
            }

            FIBITMAP* temp = FreeImage_ConvertTo32Bits(imagen);
            FreeImage_Unload(imagen);
            imagen = temp;

            uint32_t width = FreeImage_GetWidth(imagen);
            uint32_t height = FreeImage_GetHeight(imagen);
            uint8_t * pixelsPtr = FreeImage_GetBits(imagen);

            // create compatible imageData
            std::vector<uint8_t> imageData(width*height*4);
            memcpy(imageData.data(), pixelsPtr, imageData.size() * sizeof(uint8_t));

            // create texture
            auto texture = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, { width, height, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eB8G8R8A8Unorm, 1);
            auto tstage = createBuffer(device, imageData.size() * sizeof(uint8_t), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

            auto command = getCommandBuffer(device, true);
            imageBarrier(command, texture);
            flushCommandBuffer(device, command, true);

            // purple-black square
            bufferSubData(tstage, imageData);

            {
                auto bufferImageCopy = vk::BufferImageCopy()
                    .setImageExtent({ width, height, 1 })
                    .setImageOffset({ 0, 0, 0 })
                    .setBufferOffset(0)
                    .setBufferRowLength(width)
                    .setBufferImageHeight(height)
                    .setImageSubresource(texture->subresourceLayers);

                copyMemoryProxy<BufferType&, TextureType&, vk::BufferImageCopy>(device, tstage, texture, bufferImageCopy, [&]() {
                    destroyBuffer(tstage);
                });
            }

            return this->loadTexture(texture);
        }
#endif

    }
}