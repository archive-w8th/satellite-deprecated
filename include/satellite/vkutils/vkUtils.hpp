#pragma once

#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#define DEFAULT_FENCE_TIMEOUT 100000000000

#include <half.hpp> // force include half's
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <fstream>
#include <chrono>
#include <future>

#include "./vkStructures.hpp"


namespace NSM {

    // create command pool function
    vk::CommandPool createCommandPool(DeviceQueueType& deviceQueue) {
        return deviceQueue->logical.createCommandPool(vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer),
            deviceQueue->familyIndex
        ));
    }

    // get or create command buffer
    auto getCommandBuffer(DeviceQueueType& deviceQueue, bool begin = true) {
        vk::CommandBuffer cmdBuffer = deviceQueue->logical.allocateCommandBuffers(vk::CommandBufferAllocateInfo(deviceQueue->commandPool, vk::CommandBufferLevel::ePrimary, 1))[0];
        if (begin) cmdBuffer.begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse));
        cmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eBottomOfPipe | vk::PipelineStageFlagBits::eTransfer, 
            vk::PipelineStageFlagBits::eTopOfPipe | vk::PipelineStageFlagBits::eTransfer, 
            {}, nullptr, nullptr, nullptr);
        return cmdBuffer;
    };






    // finish temporary command buffer function 
    auto flushCommandBuffer(DeviceQueueType& deviceQueue, vk::CommandBuffer& commandBuffer, bool async = false) {
        commandBuffer.end();

        std::vector<vk::SubmitInfo> submitInfos = {
            vk::SubmitInfo()
            .setWaitSemaphoreCount(0)
            .setCommandBufferCount(1)
            .setPCommandBuffers(&commandBuffer)
        };

        //vk::PipelineStageFlags stageMasks = vk::PipelineStageFlagBits::eAllCommands;
        //if (!deviceQueue->executed || !deviceQueue->currentSemaphore) {
        //    deviceQueue->currentSemaphore = deviceQueue->logical.createSemaphore(vk::SemaphoreCreateInfo());
        //    deviceQueue->executed = true;
        //}

        if (async) {
            std::async([=]() { // async submit and await for destruction command buffers
                vk::Fence fence = deviceQueue->logical.createFence(vk::FenceCreateInfo());
                deviceQueue->queue.submit(submitInfos, fence);
                deviceQueue->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
                deviceQueue->logical.destroyFence(fence);
                deviceQueue->logical.freeCommandBuffers(deviceQueue->commandPool, 1, &commandBuffer);
            });
        }
        else {
            auto fence = deviceQueue->fence;
            deviceQueue->queue.submit(submitInfos, fence);
            deviceQueue->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
            deviceQueue->logical.resetFences(1, &fence);
            deviceQueue->logical.freeCommandBuffers(deviceQueue->commandPool, 1, &commandBuffer);
        }
    };

    // finish temporary command buffer function
    auto flushCommandBuffer(DeviceQueueType& deviceQueue, vk::CommandBuffer& commandBuffer, const std::function<void()>& asyncCallback) {
        commandBuffer.end();

        std::vector<vk::SubmitInfo> submitInfos = {
            vk::SubmitInfo()
            .setWaitSemaphoreCount(0)
            .setCommandBufferCount(1)
            .setPCommandBuffers(&commandBuffer)
        };

        //vk::PipelineStageFlags stageMasks = vk::PipelineStageFlagBits::eAllCommands;
        //if (!deviceQueue->executed || !deviceQueue->currentSemaphore) {
        //    deviceQueue->currentSemaphore = deviceQueue->logical.createSemaphore(vk::SemaphoreCreateInfo());
        //    deviceQueue->executed = true;
        //}

        std::async([=]() { // async submit and await for destruction command buffers
            vk::Fence fence = deviceQueue->logical.createFence(vk::FenceCreateInfo());
            deviceQueue->queue.submit(submitInfos, fence);
            deviceQueue->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
            asyncCallback();
            deviceQueue->logical.destroyFence(fence);
            deviceQueue->logical.freeCommandBuffers(deviceQueue->commandPool, 1, &commandBuffer);
        });
    };

    // transition texture layout
    void imageBarrier(vk::CommandBuffer& cmd, TextureType& image, vk::ImageLayout oldLayout) {
        // image memory barrier transfer
        vk::ImageMemoryBarrier srcImb;
        srcImb.oldLayout = oldLayout;
        srcImb.newLayout = image->layout;
        srcImb.subresourceRange = image->subresourceRange;
        srcImb.image = image->image;
        srcImb.srcAccessMask = {};

        // barrier
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, std::array<vk::ImageMemoryBarrier, 1> {srcImb});
        image->initialLayout = srcImb.newLayout;
    }

    // transition texture layout
    void imageBarrier(vk::CommandBuffer& cmd, TextureType& image) {
        imageBarrier(cmd, image, image->initialLayout);
    }

    // create texture object
    auto createTexture(DeviceQueueType& deviceQueue, vk::ImageType imageType, vk::ImageViewType viewType, vk::ImageLayout layout, vk::Extent3D size, vk::ImageUsageFlags usage, vk::Format format = vk::Format::eR8G8B8A8Unorm, uint32_t mipLevels = 1, VmaMemoryUsage usageType = VMA_MEMORY_USAGE_GPU_ONLY) {
        std::shared_ptr<Texture> texture(new Texture);
        
        // link device
        texture->device = deviceQueue;

        // use this layout
        texture->layout = layout;
        texture->initialLayout = vk::ImageLayout::ePreinitialized;//vk::ImageLayout::eUndefined;

        // create logical image
        auto imageInfo = vk::ImageCreateInfo();
        imageInfo.initialLayout = texture->initialLayout;
        imageInfo.imageType = imageType;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.arrayLayers = 1;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        //imageInfo.tiling = vk::ImageTiling::eLinear;
        imageInfo.extent = {size.width, size.height, size.depth };
        imageInfo.format = format;
        imageInfo.mipLevels = mipLevels;
        imageInfo.pQueueFamilyIndices = &deviceQueue->familyIndex;
        imageInfo.queueFamilyIndexCount = 1;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.usage = usage;


        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usageType;
        
        VkImage _image;
        vmaCreateImage(deviceQueue->allocator, &(VkImageCreateInfo)imageInfo, &allocCreateInfo, &_image, &texture->allocation, &texture->allocationInfo);
        texture->image = _image;

        texture->format = format;

        // subresource range
        texture->subresourceRange.levelCount = 1;
        texture->subresourceRange.levelCount = 1;
        texture->subresourceRange.layerCount = 1;
        texture->subresourceRange.baseMipLevel = 0;
        texture->subresourceRange.baseArrayLayer = 0;
        texture->subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

        // subresource layers
        texture->subresourceLayers.layerCount = texture->subresourceRange.levelCount;
        texture->subresourceLayers.baseArrayLayer = texture->subresourceRange.baseArrayLayer;
        texture->subresourceLayers.aspectMask = texture->subresourceRange.aspectMask;
        texture->subresourceLayers.mipLevel = texture->subresourceRange.baseMipLevel;

        // create image view
        auto imageViewInfo = vk::ImageViewCreateInfo();
        imageViewInfo.subresourceRange = texture->subresourceRange;
        imageViewInfo.viewType = viewType;
        imageViewInfo.components = vk::ComponentMapping();
        imageViewInfo.image = texture->image;
        imageViewInfo.format = format;
        imageViewInfo.components = vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
        texture->view = deviceQueue->logical.createImageView(imageViewInfo);

        // descriptor for usage
        texture->descriptorInfo = vk::DescriptorImageInfo(nullptr, texture->view, texture->layout);
        texture->initialized = true;

        auto commandBuffer = getCommandBuffer(deviceQueue, true);
        imageBarrier(commandBuffer, texture); // transit to new layouts
        flushCommandBuffer(deviceQueue, commandBuffer, true);
        return std::move(texture);
    }

    auto createTexture(DeviceQueueType& deviceQueue, vk::ImageType imageType, vk::ImageViewType viewType, vk::Extent3D size, vk::ImageUsageFlags usage, vk::Format format = vk::Format::eR8G8B8A8Unorm, uint32_t mipLevels = 1, VmaMemoryUsage usageType = VMA_MEMORY_USAGE_GPU_ONLY) {
        return createTexture(deviceQueue, imageType, viewType, vk::ImageLayout::eGeneral, size, usage, format, mipLevels, usageType);
    }
    






    // create buffer function
    auto createBuffer(DeviceQueueType& deviceQueue, size_t bufferSize, vk::BufferUsageFlags usageBits, VmaMemoryUsage usageType, vk::SharingMode sharingMode = vk::SharingMode::eExclusive) {
        //Buffer buffer;
        std::shared_ptr<Buffer> buffer(new Buffer);

        // link with device
        buffer->device = deviceQueue;

        auto binfo = vk::BufferCreateInfo(
            vk::BufferCreateFlags(),
            bufferSize,
            usageBits,
            sharingMode,
            1, &deviceQueue->familyIndex
        );

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usageType;
        if (usageType != VMA_MEMORY_USAGE_GPU_ONLY) allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer _buffer;
        vmaCreateBuffer(deviceQueue->allocator, &(VkBufferCreateInfo)binfo, &allocCreateInfo, &_buffer, &buffer->allocation, &buffer->allocationInfo);
        buffer->buffer = _buffer; // risen back a buffer

        // add state descriptor info
        buffer->descriptorInfo = vk::DescriptorBufferInfo(buffer->buffer, 0, bufferSize);

        buffer->initialized = true;

        return std::move(buffer);
    }







    // fill buffer function
    template<class T>
    void bufferSubData(BufferType& buffer, const std::vector<T>& hostdata, intptr_t offset = 0) {
        size_t bufferSize = hostdata.size() * sizeof(T);
        uint8_t * data = (uint8_t *)buffer->allocationInfo.pMappedData + offset;
        memcpy(data, hostdata.data(), bufferSize);
    }


    void bufferSubData(BufferType& buffer, const uint8_t * hostdata, const size_t bufferSize, intptr_t offset = 0) {
        uint8_t * data = (uint8_t *)buffer->allocationInfo.pMappedData + offset;
        memcpy(data, hostdata, bufferSize);
    }

    // get buffer data function
    template<class T>
    void getBufferSubData(BufferType& buffer, std::vector<T>& hostdata, intptr_t offset = 0) {
        size_t bufferSize = hostdata.size() * sizeof(T);
        uint8_t * data = (uint8_t *)buffer->allocationInfo.pMappedData + offset;
        memcpy(hostdata.data(), data, bufferSize);
    }

    // get buffer data function
    void getBufferSubData(BufferType& buffer, uint8_t * hostdata, const size_t bufferSize, intptr_t offset = 0) {
        uint8_t * data = (uint8_t *)buffer->allocationInfo.pMappedData + offset;
        memcpy(hostdata, data, bufferSize);
    }






    // copy buffer command by region
    void memoryCopyCmd(vk::CommandBuffer& cmd, BufferType& src, BufferType& dst, vk::BufferCopy region) {
        cmd.copyBuffer(src->buffer, dst->buffer, 1, &region);
    }

    // store buffer data to subimage
    void memoryCopyCmd(vk::CommandBuffer& cmd, BufferType& src, TextureType& dst, vk::BufferImageCopy region, vk::ImageLayout oldLayout) {
        cmd.copyBufferToImage(src->buffer, dst->image, dst->layout, 1, &region);
    }

    // load image subdata to buffer
    void memoryCopyCmd(vk::CommandBuffer& cmd, TextureType& src, BufferType& dst, vk::BufferImageCopy region, vk::ImageLayout oldLayout) {
        cmd.copyImageToBuffer(src->image, src->layout, dst->buffer, 1, &region);
    }

    // copy image to image
    void memoryCopyCmd(vk::CommandBuffer& cmd, TextureType& src, TextureType& dst, vk::ImageCopy region, vk::ImageLayout srcOldLayout, vk::ImageLayout dstOldLayout) {
        cmd.copyImage(src->image, src->layout, dst->image, dst->layout, 1, &region);    
    }

    // store buffer data to subimage
    void memoryCopyCmd(vk::CommandBuffer& cmd, BufferType& src, TextureType& dst, vk::BufferImageCopy region) {
        memoryCopyCmd(cmd, src, dst, region, dst->initialLayout);
    }

    // store buffer data to subimage
    void memoryCopyCmd(vk::CommandBuffer& cmd, TextureType& src, TextureType& dst, vk::ImageCopy region) {
        memoryCopyCmd(cmd, src, dst, region, src->initialLayout, dst->initialLayout);
    }

    // load image subdata to buffer
    void memoryCopyCmd(vk::CommandBuffer& cmd, TextureType& src, BufferType& dst, vk::BufferImageCopy region) {
        memoryCopyCmd(cmd, src, dst, region, src->initialLayout);
    }


    template<class ...T>
    void copyMemoryProxy(DeviceQueueType& deviceQueue, T... args, bool async = true) {
        // copy staging buffers
        vk::CommandBuffer copyCmd = getCommandBuffer(deviceQueue, true);
        memoryCopyCmd(copyCmd, args...);
        flushCommandBuffer(deviceQueue, copyCmd, async);
    }

    template<class ...T>
    void copyMemoryProxy(DeviceQueueType& deviceQueue, T... args, const std::function<void()>& asyncCallback) {
        // copy staging buffers
        vk::CommandBuffer copyCmd = getCommandBuffer(deviceQueue, true);
        memoryCopyCmd(copyCmd, args...);
        flushCommandBuffer(deviceQueue, copyCmd, asyncCallback);
    }



    // create fence function
    vk::Fence createFence(DeviceQueueType& device, bool signaled = true) {
        vk::FenceCreateInfo info;
        if (signaled) info.setFlags(vk::FenceCreateFlagBits::eSignaled);
        return device->logical.createFence(info);
    }

    // create command buffer function
    vk::CommandBuffer createCommandBuffer(DeviceQueueType& deviceQueue) {
        return deviceQueue->logical.allocateCommandBuffers(vk::CommandBufferAllocateInfo(deviceQueue->commandPool, vk::CommandBufferLevel::ePrimary, 1))[0];
    }

    // create wait fences
    std::vector<vk::Fence> createFences(DeviceQueueType& device, uint32_t fenceCount) {
        std::vector<vk::Fence> waitFences;
        for (int i = 0; i < fenceCount; i++) {
            waitFences.push_back(device->logical.createFence(vk::FenceCreateInfo()));
        }
        return waitFences;
    }

    // create command buffers
    std::vector<vk::CommandBuffer> createCommandBuffers(DeviceQueueType& deviceQueue, uint32_t bcount = 1) {
        return deviceQueue->logical.allocateCommandBuffers(vk::CommandBufferAllocateInfo(deviceQueue->commandPool, vk::CommandBufferLevel::ePrimary, bcount));
    }

    // destroy buffer function (by linked device)
    void destroyBuffer(BufferType& buffer) {
        if (buffer && buffer->initialized) {
            vmaDestroyBuffer(buffer->device->allocator, buffer->buffer, buffer->allocation);
            buffer->initialized = false;
        }
    }

    void destroyTexture(TextureType& image) {
        if (image && image->initialized) {
            vmaDestroyImage(image->device->allocator, image->image, image->allocation);
            image->initialized = false;
        }
    }

    void destroyBuffer(Buffer& buffer) {
        if (buffer.initialized) {
            vmaDestroyBuffer(buffer.device->allocator, buffer.buffer, buffer.allocation);
            buffer.initialized = false;
        }
    }


    // read source (unused)
    std::string readSource(const std::string &filePath, const bool& lineDirective = false) {
        std::string content;
        std::ifstream fileStream(filePath, std::ios::in);
        if (!fileStream.is_open()) {
            std::cerr << "Could not read file " << filePath << ". File does not exist." << std::endl;
            return "";
        }
        std::string line = "";
        while (!fileStream.eof()) {
            std::getline(fileStream, line);
            if (lineDirective || line.find("#line") == std::string::npos) content.append(line + "\n");
        }
        fileStream.close();
        return content;
    }

    // read binary (for SPIR-V)
    std::vector<char> readBinary(const std::string &filePath) {
        std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);
        std::vector<char> data;
        if (file.is_open()) {
            std::streampos size = file.tellg();
            data.resize(size);
            file.seekg(0, std::ios::beg);
            file.read(&data[0], size);
            file.close();
        }
        else {
            std::cerr << "Failure to open " + filePath << std::endl;
        }
        return data;
    };

    // load module for Vulkan device
    vk::ShaderModule loadAndCreateShaderModule(DeviceQueueType& device, std::string path) {
        auto code = readBinary(path);
        return device->logical.createShaderModule(vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),
            code.size(), (uint32_t*)code.data()
        ));
    }


    // create compute pipeline
    vk::Pipeline createCompute(DeviceQueueType& device, std::string path, vk::PipelineLayout& layout, vk::PipelineCache& cache) {
        vk::ComputePipelineCreateInfo cmpi;
        cmpi.setStage(vk::PipelineShaderStageCreateInfo().setModule(loadAndCreateShaderModule(device, path)).setPName("main").setStage(vk::ShaderStageFlagBits::eCompute));;
        cmpi.setLayout(layout);
        return device->logical.createComputePipeline(cache, cmpi);
    }




};