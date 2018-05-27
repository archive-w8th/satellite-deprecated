#pragma once

#include "./vkStructures.hpp"

namespace NSM {

    void commandBarrier(const vk::CommandBuffer& cmdBuffer) {

        auto writeMask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eMemoryWrite;
        auto readMask = vk::AccessFlags{};//vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eUniformRead;
        auto memoryBarriers = std::vector<vk::MemoryBarrier>{ vk::MemoryBarrier().setSrcAccessMask(writeMask).setDstAccessMask(readMask) };
        cmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eGeometryShader | vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eVertexShader   | vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eTransfer,
            {}, memoryBarriers, {}, {});
        
    };

    // get or create command buffer
    auto createCommandBuffer(const Queue deviceQueue, bool begin = true, bool secondary = false) {
        vk::CommandBuffer cmdBuffer = deviceQueue->device->logical.allocateCommandBuffers(vk::CommandBufferAllocateInfo(deviceQueue->commandPool, secondary ? vk::CommandBufferLevel::eSecondary : vk::CommandBufferLevel::ePrimary, 1))[0];
        auto inheritanceInfo = vk::CommandBufferInheritanceInfo().setPipelineStatistics(vk::QueryPipelineStatisticFlagBits::eComputeShaderInvocations);
        if (begin) {
            cmdBuffer.begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse).setPInheritanceInfo(secondary ? &inheritanceInfo : nullptr));
            commandBarrier(cmdBuffer);
        }
        return cmdBuffer;
    };
    
    auto cmdSubmission(const vk::CommandBuffer& cmdBuffer, const std::vector<vk::CommandBuffer>& commandBuffers, bool needEnd = true) {
        if (needEnd) {
            for (auto &cmdf : commandBuffers) cmdf.end(); // end cmd buffers
        }
        cmdBuffer.executeCommands(commandBuffers);
        //commandBarrier(cmdBuffer);
    };



    // should be done by ".end()", we at now have no "autoend" mechanism for Vulkan API
    // usefull for dispatch command, that can be reused
    void executeCommands(const Queue deviceQueue, const std::vector<vk::CommandBuffer>& commandBuffers, bool async = true) {
        std::vector<vk::SubmitInfo> submitInfos = {
            vk::SubmitInfo().setWaitSemaphoreCount(0).setCommandBufferCount(commandBuffers.size()).setPCommandBuffers(commandBuffers.data())
        };

        auto fence = async ? deviceQueue->device->logical.createFence(vk::FenceCreateInfo()) : deviceQueue->fence;
        deviceQueue->queue.submit(submitInfos, fence);

        if (async) {
            std::async(std::launch::async | std::launch::deferred, [=]() { // async submit and await for destruction command buffers
                deviceQueue->device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
                deviceQueue->device->logical.destroyFence(fence);
            });
        } else {
            deviceQueue->device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
            std::async(std::launch::async | std::launch::deferred, [=]() {
                deviceQueue->device->logical.resetFences(1, &fence);
            });
        }
    };



    // finish temporary command buffer function
    void flushCommandBuffers(const Queue deviceQueue, const std::vector<vk::CommandBuffer>& commandBuffers, bool async = true, bool needEnd = true) {
        std::vector<vk::SubmitInfo> submitInfos = {
            vk::SubmitInfo().setWaitSemaphoreCount(0).setCommandBufferCount(commandBuffers.size()).setPCommandBuffers(commandBuffers.data()) 
        };

        // submit and don't disagree sequences
        if (needEnd) {
            for (auto &cmdf : commandBuffers) cmdf.end(); // end cmd buffers
        }
        auto fence = async ? deviceQueue->device->logical.createFence(vk::FenceCreateInfo()) : deviceQueue->fence;
        deviceQueue->queue.submit(submitInfos, fence);

        if (async)
        {
            std::async(std::launch::async | std::launch::deferred, [=]() { // async submit and await for destruction command buffers
                deviceQueue->device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
                deviceQueue->device->logical.destroyFence(fence);
                deviceQueue->device->logical.freeCommandBuffers(deviceQueue->commandPool, commandBuffers);
            });
        }
        else
        {
            deviceQueue->device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
            std::async(std::launch::async | std::launch::deferred, [=]() {
                deviceQueue->device->logical.resetFences(1, &fence);
                deviceQueue->device->logical.freeCommandBuffers(deviceQueue->commandPool, commandBuffers);
            });
        }
    };

    // finish temporary command buffer function
    void flushCommandBuffers(const Queue deviceQueue, const std::vector<vk::CommandBuffer>& commandBuffers, const std::function<void()> &asyncCallback) {
        std::vector<vk::SubmitInfo> submitInfos = { vk::SubmitInfo().setWaitSemaphoreCount(0).setCommandBufferCount(commandBuffers.size()).setPCommandBuffers(commandBuffers.data()) };

        // submit and don't disagree sequences
        for (auto &cmdf : commandBuffers) cmdf.end(); // end cmd buffers
        vk::Fence fence = deviceQueue->device->logical.createFence(vk::FenceCreateInfo());
        deviceQueue->queue.submit(submitInfos, fence);

        std::async(std::launch::async | std::launch::deferred, [=]() { // async submit and await for destruction command buffers
            deviceQueue->device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
            std::async(std::launch::async | std::launch::deferred, [=]() {
                deviceQueue->device->logical.destroyFence(fence);
                deviceQueue->device->logical.freeCommandBuffers(deviceQueue->commandPool, commandBuffers);
            });
            asyncCallback();
        });
    };


    // finish temporary command buffer function
    void flushCommandBuffers(const Queue deviceQueue, const std::vector<vk::CommandBuffer>& commandBuffers, vk::SubmitInfo sinfo) {
        std::vector<vk::SubmitInfo> submitInfos = { sinfo.setCommandBufferCount(commandBuffers.size()).setPCommandBuffers(commandBuffers.data()) };

        // submit and don't disagree sequences
        for (auto &cmdf : commandBuffers) cmdf.end(); // end cmd buffers
        vk::Fence fence = deviceQueue->device->logical.createFence(vk::FenceCreateInfo());
        deviceQueue->queue.submit(submitInfos, fence);

        std::async(std::launch::async | std::launch::deferred, [=]() { // async submit and await for destruction command buffers
            deviceQueue->device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
            std::async(std::launch::async | std::launch::deferred, [=]() {
                deviceQueue->device->logical.destroyFence(fence);
                deviceQueue->device->logical.freeCommandBuffers(deviceQueue->commandPool, commandBuffers);
            });
        });
    };


    /*
    // finish temporary command buffer function
    void flushCommandBuffer(Queue deviceQueue, const vk::CommandBuffer &commandBuffer, bool async = true)
    {
        commandBuffer.end();

        std::vector<vk::SubmitInfo> submitInfos = { vk::SubmitInfo().setWaitSemaphoreCount(0).setCommandBufferCount(1).setPCommandBuffers(&commandBuffer) };

        // submit and don't disagree sequences
        auto fence = async ? deviceQueue->device->logical.createFence(vk::FenceCreateInfo()) : deviceQueue->fence;
        deviceQueue->queue.submit(submitInfos, fence);

        if (async)
        {
            std::async(std::launch::async | std::launch::deferred, [=]() { // async submit and await for destruction command buffers
                deviceQueue->device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
                deviceQueue->device->logical.destroyFence(fence);
                deviceQueue->device->logical.freeCommandBuffers(deviceQueue->commandPool, 1, &commandBuffer);
            });
        }
        else
        {
            deviceQueue->device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
            std::async(std::launch::async | std::launch::deferred, [=]() {
                deviceQueue->device->logical.resetFences(1, &fence);
                deviceQueue->device->logical.freeCommandBuffers(deviceQueue->commandPool, 1, &commandBuffer);
            });
        }
    };

    // finish temporary command buffer function
    void flushCommandBuffer(Queue deviceQueue, const vk::CommandBuffer &commandBuffer, const std::function<void()> &asyncCallback) {
        commandBuffer.end();

        std::vector<vk::SubmitInfo> submitInfos = { vk::SubmitInfo().setWaitSemaphoreCount(0).setCommandBufferCount(1).setPCommandBuffers(&commandBuffer) };
        auto fence = deviceQueue->device->logical.createFence(vk::FenceCreateInfo());
        deviceQueue->queue.submit(submitInfos, fence);

        std::async(std::launch::async | std::launch::deferred, [=]() { // async submit and await for destruction command buffers
            deviceQueue->device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
            std::async(std::launch::async | std::launch::deferred, [=]() {
                deviceQueue->device->logical.destroyFence(fence);
                deviceQueue->device->logical.freeCommandBuffers(deviceQueue->commandPool, 1, &commandBuffer);
            });
            asyncCallback();
        });
    };

    // flush command for rendering
    void flushCommandBuffer(Queue deviceQueue, const vk::CommandBuffer &commandBuffer, vk::SubmitInfo kernel, const std::function<void()> &asyncCallback) {
        commandBuffer.end();

        kernel.setCommandBufferCount(1).setPCommandBuffers(&commandBuffer);
        auto fence = deviceQueue->device->logical.createFence(vk::FenceCreateInfo());
        deviceQueue->queue.submit(1, &kernel, fence);

        std::async(std::launch::async | std::launch::deferred, [=]() { // async submit and await for destruction command buffers
            deviceQueue->device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
            std::async(std::launch::async | std::launch::deferred, [=]() {
                deviceQueue->device->logical.destroyFence(fence);
                deviceQueue->device->logical.freeCommandBuffers(deviceQueue->commandPool, 1, &commandBuffer);
            });
            asyncCallback();
        });
    };
    */

    // transition texture layout
    void imageBarrier(const vk::CommandBuffer &cmd, Image image, vk::ImageLayout oldLayout) {
        vk::ImageMemoryBarrier imageMemoryBarriers = {};
        imageMemoryBarriers.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarriers.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarriers.oldLayout = oldLayout;
        imageMemoryBarriers.newLayout = image->layout;
        imageMemoryBarriers.image = image->image;
        imageMemoryBarriers.subresourceRange = image->subresourceRange;

        // Put barrier on top
        vk::PipelineStageFlags srcStageMask{ vk::PipelineStageFlagBits::eTopOfPipe };
        vk::PipelineStageFlags dstStageMask{ vk::PipelineStageFlagBits::eTopOfPipe };
        vk::DependencyFlags dependencyFlags{};
        vk::AccessFlags srcMask{};
        vk::AccessFlags dstMask{};

        typedef vk::ImageLayout il;
        typedef vk::AccessFlagBits afb;

        // Is it me, or are these the same?
        switch (oldLayout) {
            case il::eUndefined: break;
            case il::eGeneral: srcMask = afb::eTransferWrite; break;
            case il::eColorAttachmentOptimal: srcMask = afb::eColorAttachmentWrite; break;
            case il::eDepthStencilAttachmentOptimal: srcMask = afb::eDepthStencilAttachmentWrite; break;
            case il::eDepthStencilReadOnlyOptimal: srcMask = afb::eDepthStencilAttachmentRead; break;
            case il::eShaderReadOnlyOptimal: srcMask = afb::eShaderRead; break;
            case il::eTransferSrcOptimal: srcMask = afb::eTransferRead; break;
            case il::eTransferDstOptimal: srcMask = afb::eTransferWrite; break;
            case il::ePreinitialized: srcMask = afb::eTransferWrite | afb::eHostWrite; break;
            case il::ePresentSrcKHR: srcMask = afb::eMemoryRead; break;
        }

        switch (image->layout) {
            case il::eUndefined: break;
            case il::eGeneral: dstMask = afb::eTransferWrite; break;
            case il::eColorAttachmentOptimal: dstMask = afb::eColorAttachmentWrite; break;
            case il::eDepthStencilAttachmentOptimal: dstMask = afb::eDepthStencilAttachmentWrite; break;
            case il::eDepthStencilReadOnlyOptimal: dstMask = afb::eDepthStencilAttachmentRead; break;
            case il::eShaderReadOnlyOptimal: dstMask = afb::eShaderRead; break;
            case il::eTransferSrcOptimal: dstMask = afb::eTransferRead; break;
            case il::eTransferDstOptimal: dstMask = afb::eTransferWrite; break;
            case il::ePreinitialized: dstMask = afb::eTransferWrite; break;
            case il::ePresentSrcKHR: dstMask = afb::eMemoryRead; break;
        }

        imageMemoryBarriers.srcAccessMask = srcMask;
        imageMemoryBarriers.dstAccessMask = dstMask;

        // barrier
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, std::array<vk::ImageMemoryBarrier, 1>{imageMemoryBarriers});
        image->initialLayout = imageMemoryBarriers.newLayout;
    }

    // transition texture layout
    void imageBarrier(const vk::CommandBuffer &cmd, Image image)
    {
        imageBarrier(cmd, image, image->initialLayout);
    }

    // create texture object
    auto createImage(Queue deviceQueue,
        vk::ImageViewType imageViewType, vk::ImageLayout layout,
        vk::Extent3D size, vk::ImageUsageFlags usage,
        vk::Format format = vk::Format::eR8G8B8A8Unorm,
        uint32_t mipLevels = 1,
        VmaMemoryUsage usageType = VMA_MEMORY_USAGE_GPU_ONLY)
    {
        Image texture = std::make_shared<ImageType>();
        texture->queue = deviceQueue;
        texture->layout = layout;
        texture->initialLayout = vk::ImageLayout::eUndefined;

        // init image dimensional type
        vk::ImageType imageType = vk::ImageType::e2D;
        bool isCubemap = false;
        switch (imageViewType)
        {
        case vk::ImageViewType::e1D:
            imageType = vk::ImageType::e1D;
            break;
        case vk::ImageViewType::e1DArray:
            imageType = vk::ImageType::e2D;
            break;
        case vk::ImageViewType::e2D:
            imageType = vk::ImageType::e2D;
            break;
        case vk::ImageViewType::e2DArray:
            imageType = vk::ImageType::e3D;
            break;
        case vk::ImageViewType::e3D:
            imageType = vk::ImageType::e3D;
            break;
        case vk::ImageViewType::eCube:
            imageType = vk::ImageType::e3D;
            isCubemap = true;
            break;
        case vk::ImageViewType::eCubeArray:
            imageType = vk::ImageType::e3D;
            isCubemap = true;
            break;
        };

        // image memory descriptor
        auto imageInfo = vk::ImageCreateInfo();
        imageInfo.initialLayout = texture->initialLayout;
        imageInfo.imageType = imageType;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.arrayLayers = 1; // unsupported
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.extent = { size.width, size.height, size.depth * (isCubemap ? 6 : 1) };
        imageInfo.format = format;
        imageInfo.mipLevels = mipLevels;
        imageInfo.pQueueFamilyIndices = &deviceQueue->familyIndex;
        imageInfo.queueFamilyIndexCount = 1;
        imageInfo.samples = vk::SampleCountFlagBits::e1; // at now not supported MSAA
        imageInfo.usage = usage;

        // create image with allocation
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usageType;
        vmaCreateImage(deviceQueue->device->allocator, &(VkImageCreateInfo)imageInfo,
            &allocCreateInfo, (VkImage *)&texture->image,
            &texture->allocation, &texture->allocationInfo);

        // subresource range
        texture->subresourceRange.levelCount = 1;
        texture->subresourceRange.layerCount = 1;
        texture->subresourceRange.baseMipLevel = 0;
        texture->subresourceRange.baseArrayLayer = 0;
        texture->subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

        // subresource layers
        texture->subresourceLayers.layerCount = texture->subresourceRange.layerCount;
        texture->subresourceLayers.baseArrayLayer = texture->subresourceRange.baseArrayLayer;
        texture->subresourceLayers.aspectMask = texture->subresourceRange.aspectMask;
        texture->subresourceLayers.mipLevel = texture->subresourceRange.baseMipLevel;

        // descriptor for usage
        texture->view = deviceQueue->device->logical.createImageView(
            vk::ImageViewCreateInfo()
            .setSubresourceRange(texture->subresourceRange)
            .setViewType(imageViewType)
            .setComponents(vk::ComponentMapping(
                vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA))
            .setImage(texture->image)
            .setFormat(format));
        texture->descriptorInfo = vk::DescriptorImageInfo(nullptr, texture->view, texture->layout);
        texture->initialized = true;

        // do layout transition
        auto commandBuffer = createCommandBuffer(deviceQueue, true);
        imageBarrier(commandBuffer, texture); // transit to new layouts
        flushCommandBuffers(deviceQueue, { commandBuffer }, true);
        return std::move(texture);
    }

    //
    auto createImage(Queue deviceQueue,
        vk::ImageViewType viewType, vk::Extent3D size,
        vk::ImageUsageFlags usage,
        vk::Format format = vk::Format::eR8G8B8A8Unorm,
        uint32_t mipLevels = 1,
        VmaMemoryUsage usageType = VMA_MEMORY_USAGE_GPU_ONLY) {
        return createImage(deviceQueue, viewType, vk::ImageLayout::eGeneral, size, usage, format, mipLevels, usageType);
    }

    // create buffer function
    auto createBuffer(Queue deviceQueue, size_t bufferSize, vk::BufferUsageFlags usageBits, VmaMemoryUsage usageType) {
        Buffer buffer = std::make_shared<BufferType>();

        // link with device
        buffer->queue = deviceQueue;

        auto binfo = vk::BufferCreateInfo(vk::BufferCreateFlags(), bufferSize,
            usageBits, vk::SharingMode::eExclusive, 1,
            &deviceQueue->familyIndex);

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usageType;
        if (usageType != VMA_MEMORY_USAGE_GPU_ONLY)
        {
            allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }

        vmaCreateBuffer(deviceQueue->device->allocator, &(VkBufferCreateInfo)binfo,
            &allocCreateInfo, (VkBuffer *)&buffer->buffer,
            &buffer->allocation, &buffer->allocationInfo);

        // add state descriptor info
        buffer->descriptorInfo =
            vk::DescriptorBufferInfo(buffer->buffer, 0, bufferSize);
        buffer->initialized = true;
        return std::move(buffer);
    }


    auto createBufferView(Buffer buffer, vk::Format format) {
        if (!buffer->bufferView) {
            buffer->bufferView = buffer->queue->device->logical.createBufferView(vk::BufferViewCreateInfo().setBuffer(buffer->buffer).setFormat(format).setOffset(0).setRange(buffer->descriptorInfo.range));
        }
        return buffer->bufferView;
    }


    // fill buffer function
    template <class T>
    void bufferSubData(const vk::CommandBuffer &cmd, Buffer buffer, const std::vector<T> &hostdata, intptr_t offset = 0)
    {
        const size_t bufferSize = hostdata.size() * sizeof(T);
        if (bufferSize > 0)
            memcpy((uint8_t *)buffer->allocationInfo.pMappedData + offset,
                hostdata.data(), bufferSize);
    }

    void bufferSubData(const vk::CommandBuffer &cmd, Buffer buffer, const uint8_t *hostdata, const size_t bufferSize, intptr_t offset = 0)
    {
        if (bufferSize > 0)
            memcpy((uint8_t *)buffer->allocationInfo.pMappedData + offset, hostdata, bufferSize);
    }

    // get buffer data function
    template <class T>
    void getBufferSubData(const Buffer &buffer, std::vector<T> &hostdata, intptr_t offset = 0)
    {
        memcpy(hostdata.data(),
            (const uint8_t *)buffer->allocationInfo.pMappedData + offset,
            hostdata.size() * sizeof(T));
    }

    // get buffer data function
    void getBufferSubData(const Buffer &buffer, uint8_t *hostdata, const size_t bufferSize, intptr_t offset = 0) {
        memcpy(hostdata, (const uint8_t *)buffer->allocationInfo.pMappedData + offset, bufferSize);
    }

    // copy buffer command by region
    void memoryCopyCmd(vk::CommandBuffer &cmd, const Buffer &src, const Buffer &dst, vk::BufferCopy region)
    {
        cmd.copyBuffer(src->buffer, dst->buffer, 1, &region);
    }

    // store buffer data to subimage
    void memoryCopyCmd(vk::CommandBuffer &cmd, const Buffer &src, const Image &dst, vk::BufferImageCopy region, vk::ImageLayout oldLayout)
    {
        cmd.copyBufferToImage(src->buffer, dst->image, dst->layout, 1, &region);
    }

    // load image subdata to buffer
    void memoryCopyCmd(vk::CommandBuffer &cmd, const Image &src, const Buffer &dst, vk::BufferImageCopy region, vk::ImageLayout oldLayout)
    {
        cmd.copyImageToBuffer(src->image, src->layout, dst->buffer, 1, &region);
    }

    // copy image to image
    void memoryCopyCmd(vk::CommandBuffer &cmd, const Image &src, const Image &dst, vk::ImageCopy region, vk::ImageLayout srcOldLayout, vk::ImageLayout dstOldLayout)
    {
        cmd.copyImage(src->image, src->layout, dst->image, dst->layout, 1, &region);
    }

    // store buffer data to subimage
    void memoryCopyCmd(vk::CommandBuffer &cmd, const Buffer &src, const Image &dst, vk::BufferImageCopy region)
    {
        memoryCopyCmd(cmd, src, dst, region, dst->initialLayout);
    }

    // store buffer data to subimage
    void memoryCopyCmd(vk::CommandBuffer &cmd, const Image &src, const Image &dst, vk::ImageCopy region)
    {
        memoryCopyCmd(cmd, src, dst, region, src->initialLayout, dst->initialLayout);
    }

    // load image subdata to buffer
    void memoryCopyCmd(vk::CommandBuffer &cmd, const Image &src, const Buffer &dst, vk::BufferImageCopy region)
    {
        memoryCopyCmd(cmd, src, dst, region, src->initialLayout);
    }

    /*
    template<class ...T>
    void copyMemoryProxy(const Queue deviceQueue, T... args, bool async =
    true) { // copy staging buffers vk::CommandBuffer copyCmd =
    createCommandBuffer(deviceQueue, true); memoryCopyCmd(copyCmd, args...);
    flushCommandBuffer(deviceQueue, copyCmd, async);
    }

    template<class ...T>
    void copyMemoryProxy(const Queue deviceQueue, T... args, const
    std::function<void()>& asyncCallback) { // copy staging buffers
        vk::CommandBuffer copyCmd = createCommandBuffer(deviceQueue, true);
        memoryCopyCmd(copyCmd, args...); flushCommandBuffer(deviceQueue, copyCmd,
    asyncCallback);
    }
    */

    template <class... T>
    vk::CommandBuffer makeCopyCmd(const Queue deviceQueue, T... args, bool end = false)
    { // copy staging buffers
        vk::CommandBuffer copyCmd = createCommandBuffer(deviceQueue, true);
        memoryCopyCmd(copyCmd, args...);
        if (end) copyCmd.end();
        return copyCmd;
    }

    // create fence function
    vk::Fence createFence(Device &device, bool signaled = true) {
        vk::FenceCreateInfo info;
        if (signaled) info.setFlags(vk::FenceCreateFlagBits::eSignaled);
        return device->logical.createFence(info);
    }

    // create command buffer function
    vk::CommandBuffer createCommandBuffer(Queue deviceQueue) {
        return deviceQueue->device->logical.allocateCommandBuffers( vk::CommandBufferAllocateInfo(deviceQueue->commandPool, vk::CommandBufferLevel::ePrimary, 1))[0];
    }

    // create wait fences
    std::vector<vk::Fence> createFences(Device &device, uint32_t fenceCount) {
        std::vector<vk::Fence> waitFences;
        for (int i = 0; i < fenceCount; i++) {
            waitFences.push_back(device->logical.createFence(vk::FenceCreateInfo()));
        }
        return waitFences;
    }

    // create command buffers
    std::vector<vk::CommandBuffer> createCommandBuffers(Queue deviceQueue, uint32_t bcount = 1) {
        return deviceQueue->device->logical.allocateCommandBuffers( vk::CommandBufferAllocateInfo(deviceQueue->commandPool, vk::CommandBufferLevel::ePrimary, bcount));
    }


    // destructors for shared pointer system
    BufferType::~BufferType() {
        this->queue->device->logical.waitIdle();
        auto buffer = this;
        auto queue = this->queue;
        std::async(std::launch::async | std::launch::deferred, [=]() {
            if (buffer && buffer->initialized) {
                queue->device->logical.waitIdle();
                vmaDestroyBuffer(queue->device->allocator, buffer->buffer, buffer->allocation);
                buffer->initialized = false;
            }
        });
    }

    // destructors for shared pointer system
    ImageType::~ImageType() {
        this->queue->device->logical.waitIdle();
        auto image = this;
        auto queue = this->queue;
        std::async(std::launch::async | std::launch::deferred, [=]() {
            if (image && image->initialized) {
                queue->device->logical.waitIdle();
                vmaDestroyImage(queue->device->allocator, image->image, image->allocation);
                image->initialized = false;
            }
        });
    }



    // read source (unused)
    std::string readSource(const std::string &filePath, const bool &lineDirective = false) {
        std::string content;
        std::ifstream fileStream(filePath, std::ios::in);
        if (!fileStream.is_open())
        {
            std::cerr << "Could not read file " << filePath << ". File does not exist."
                << std::endl;
            return "";
        }
        std::string line = "";
        while (!fileStream.eof())
        {
            std::getline(fileStream, line);
            if (lineDirective || line.find("#line") == std::string::npos)
                content.append(line + "\n");
        }
        fileStream.close();
        return content;
    }

    // read binary (for SPIR-V)
    std::vector<char> readBinary(const std::string &filePath) {
        std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);
        std::vector<char> data;
        if (file.is_open())
        {
            std::streampos size = file.tellg();
            data.resize(size);
            file.seekg(0, std::ios::beg);
            file.read(&data[0], size);
            file.close();
        }
        else
        {
            std::cerr << "Failure to open " + filePath << std::endl;
        }
        return data;
    };

    // load module for Vulkan device
    vk::ShaderModule loadAndCreateShaderModule(Device device, std::string path) {
        auto code = readBinary(path);
        return device->logical.createShaderModule(vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), code.size(), (uint32_t *)code.data()));
    }

    // create compute pipeline
    auto createCompute(Queue queue, std::string path, vk::PipelineLayout &layout) {
        auto cmpi = vk::ComputePipelineCreateInfo().setStage(vk::PipelineShaderStageCreateInfo().setModule(loadAndCreateShaderModule(queue->device, path)).setPName("main").setStage(vk::ShaderStageFlagBits::eCompute)).setLayout(layout);

        vk::Pipeline pipeline;
        try {
            pipeline = queue->device->logical.createComputePipeline(queue->device->pipelineCache, cmpi);
        } catch (vk::Result const &e) {
            std::cout << "Something wrong (" << path << ")" << std::endl;
        }

        ComputeContext cmpx = std::make_shared<ComputeContextType>();
        cmpx->queue = queue;
        cmpx->pipelineLayout = layout;
        cmpx->pipelineCache = queue->device->pipelineCache;
        cmpx->pipeline = pipeline;
        return cmpx;
    }





    auto makeDispatchCmd(ComputeContext compute, glm::uvec3 workGroups, const std::vector<vk::DescriptorSet>& sets, bool end = true, bool secondary = false) {
        auto commandBuffer = createCommandBuffer(compute->queue, true, secondary);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, compute->pipelineLayout, 0, sets, nullptr);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, compute->pipeline);
        commandBuffer.dispatch(workGroups.x, workGroups.y, workGroups.z);
        //commandBarrier(commandBuffer);
        if (end) commandBuffer.end();
        return commandBuffer;
    }

    // make dispatch with push constant
    template<class T = uint32_t>
    auto makeDispatchCmd(ComputeContext compute, glm::uvec3 workGroups, const std::vector<vk::DescriptorSet>& sets, const T * instanceConst, bool end = true) {
        auto commandBuffer = createCommandBuffer(compute->queue, true);
        commandBuffer.pushConstants(compute->pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(T), instanceConst);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, compute->pipelineLayout, 0, sets, nullptr);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, compute->pipeline);
        commandBuffer.dispatch(workGroups.x, workGroups.y, workGroups.z);
        //commandBarrier(commandBuffer);
        if (end) commandBuffer.end();
        return commandBuffer;
    }
    
    // may used with push constants
    template<class T = uint32_t>
    auto dispatchCompute(ComputeContext compute, glm::uvec3 workGroups, const std::vector<vk::DescriptorSet>& sets, const T * instanceConst) {
        auto commandBuffer = createCommandBuffer(compute->queue, true);
        commandBuffer.pushConstants(compute->pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(T), instanceConst);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, compute->pipelineLayout, 0, sets, nullptr);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, compute->pipeline);
        commandBuffer.dispatch(workGroups.x, workGroups.y, workGroups.z);
        //commandBarrier(commandBuffer);
        flushCommandBuffers(compute->queue, { commandBuffer }, true);
    }

    auto dispatchCompute(ComputeContext compute, glm::uvec3 workGroups, const std::vector<vk::DescriptorSet>& sets) {
        auto commandBuffer = createCommandBuffer(compute->queue, true);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, compute->pipelineLayout, 0, sets, nullptr);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, compute->pipeline);
        commandBuffer.dispatch(workGroups.x, workGroups.y, workGroups.z);
        //commandBarrier(commandBuffer);
        flushCommandBuffers(compute->queue, { commandBuffer }, true);
    }




}; // namespace NSM