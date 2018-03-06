#pragma once

#include "./vkStructures.hpp"

namespace NSM
{

// create command pool function
vk::CommandPool createCommandPool(const DeviceQueueType &deviceQueue)
{
  return deviceQueue->logical.createCommandPool(vk::CommandPoolCreateInfo(
      vk::CommandPoolCreateFlags(
          vk::CommandPoolCreateFlagBits::eResetCommandBuffer),
      deviceQueue->mainQueue->familyIndex));
}

// get or create command buffer
auto getCommandBuffer(const DeviceQueueType &deviceQueue, bool begin = true)
{
  vk::CommandBuffer cmdBuffer =
      deviceQueue->logical.allocateCommandBuffers(vk::CommandBufferAllocateInfo(
          deviceQueue->commandPool, vk::CommandBufferLevel::ePrimary, 1))[0];
  if (begin)
    cmdBuffer.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eSimultaneousUse));
  cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe |
                                vk::PipelineStageFlagBits::eTransfer,
                            vk::PipelineStageFlagBits::eTopOfPipe |
                                vk::PipelineStageFlagBits::eTransfer,
                            {}, nullptr, nullptr, nullptr);
  return cmdBuffer;
};

// finish temporary command buffer function
auto flushCommandBuffers(const DeviceQueueType &deviceQueue,
                         const std::vector<vk::CommandBuffer> &commandBuffers,
                         bool async = false)
{
  std::vector<vk::SubmitInfo> submitInfos = {
      vk::SubmitInfo()
          .setWaitSemaphoreCount(0)
          .setCommandBufferCount(commandBuffers.size())
          .setPCommandBuffers(commandBuffers.data())};

  if (async)
  {
    std::async([=]() { // async submit and await for destruction command buffers
      for (auto &cmdf : commandBuffers)
        cmdf.end(); // end cmd buffers
      vk::Fence fence = deviceQueue->logical.createFence(vk::FenceCreateInfo());
      deviceQueue->mainQueue->queue.submit(submitInfos, fence);
      deviceQueue->logical.waitForFences(1, &fence, true,
                                         DEFAULT_FENCE_TIMEOUT);
      deviceQueue->logical.destroyFence(fence);
      deviceQueue->logical.freeCommandBuffers(deviceQueue->commandPool,
                                              commandBuffers);
    });
  }
  else
  {
    for (auto &cmdf : commandBuffers)
      cmdf.end(); // end cmd buffers
    auto fence = deviceQueue->fence;
    deviceQueue->mainQueue->queue.submit(submitInfos, fence);
    deviceQueue->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
    deviceQueue->logical.resetFences(1, &fence);
    deviceQueue->logical.freeCommandBuffers(deviceQueue->commandPool,
                                            commandBuffers);
  }
};

// finish temporary command buffer function
auto flushCommandBuffers(const DeviceQueueType &deviceQueue,
                         const std::vector<vk::CommandBuffer> &commandBuffers,
                         const std::function<void()> &asyncCallback)
{
  for (auto &cmdf : commandBuffers)
    cmdf.end(); // end cmd buffers

  std::vector<vk::SubmitInfo> submitInfos = {
      vk::SubmitInfo()
          .setWaitSemaphoreCount(0)
          .setCommandBufferCount(commandBuffers.size())
          .setPCommandBuffers(commandBuffers.data())};

  std::async([=]() { // async submit and await for destruction command buffers
    for (auto &cmdf : commandBuffers)
      cmdf.end(); // end cmd buffers
    vk::Fence fence = deviceQueue->logical.createFence(vk::FenceCreateInfo());
    deviceQueue->mainQueue->queue.submit(submitInfos, fence);
    deviceQueue->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
    asyncCallback();
    deviceQueue->logical.destroyFence(fence);
    deviceQueue->logical.freeCommandBuffers(deviceQueue->commandPool,
                                            commandBuffers);
  });
};

// finish temporary command buffer function
auto flushCommandBuffer(const DeviceQueueType &deviceQueue,
                        const vk::CommandBuffer &commandBuffer,
                        bool async = false)
{
  commandBuffer.end();

  std::vector<vk::SubmitInfo> submitInfos = {
      vk::SubmitInfo()
          .setWaitSemaphoreCount(0)
          .setCommandBufferCount(1)
          .setPCommandBuffers(&commandBuffer)};

  // vk::PipelineStageFlags stageMasks =
  // vk::PipelineStageFlagBits::eAllCommands; if (!deviceQueue->executed ||
  // !deviceQueue->currentSemaphore) {
  //    deviceQueue->currentSemaphore =
  //    deviceQueue->logical.createSemaphore(vk::SemaphoreCreateInfo());
  //    deviceQueue->executed = true;
  //}

  if (async)
  {
    std::async([=]() { // async submit and await for destruction command buffers
      vk::Fence fence = deviceQueue->logical.createFence(vk::FenceCreateInfo());
      deviceQueue->mainQueue->queue.submit(submitInfos, fence);
      deviceQueue->logical.waitForFences(1, &fence, true,
                                         DEFAULT_FENCE_TIMEOUT);
      deviceQueue->logical.destroyFence(fence);
      deviceQueue->logical.freeCommandBuffers(deviceQueue->commandPool, 1,
                                              &commandBuffer);
    });
  }
  else
  {
    auto fence = deviceQueue->fence;
    deviceQueue->mainQueue->queue.submit(submitInfos, fence);
    deviceQueue->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
    deviceQueue->logical.resetFences(1, &fence);
    deviceQueue->logical.freeCommandBuffers(deviceQueue->commandPool, 1,
                                            &commandBuffer);
  }
};

// finish temporary command buffer function
auto flushCommandBuffer(const DeviceQueueType &deviceQueue,
                        const vk::CommandBuffer &commandBuffer,
                        const std::function<void()> &asyncCallback)
{
  commandBuffer.end();

  std::vector<vk::SubmitInfo> submitInfos = {
      vk::SubmitInfo()
          .setWaitSemaphoreCount(0)
          .setCommandBufferCount(1)
          .setPCommandBuffers(&commandBuffer)};

  // vk::PipelineStageFlags stageMasks =
  // vk::PipelineStageFlagBits::eAllCommands; if (!deviceQueue->executed ||
  // !deviceQueue->currentSemaphore) {
  //    deviceQueue->currentSemaphore =
  //    deviceQueue->logical.createSemaphore(vk::SemaphoreCreateInfo());
  //    deviceQueue->executed = true;
  //}

  std::async([=]() { // async submit and await for destruction command buffers
    vk::Fence fence = deviceQueue->logical.createFence(vk::FenceCreateInfo());
    deviceQueue->mainQueue->queue.submit(submitInfos, fence);
    deviceQueue->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
    asyncCallback();
    deviceQueue->logical.destroyFence(fence);
    deviceQueue->logical.freeCommandBuffers(deviceQueue->commandPool, 1,
                                            &commandBuffer);
  });
};

// flush command for rendering
auto flushCommandBuffer(const DeviceQueueType &deviceQueue,
                        const vk::CommandBuffer &commandBuffer,
                        vk::SubmitInfo kernel,
                        const std::function<void()> &asyncCallback)
{
  commandBuffer.end();

  kernel.setCommandBufferCount(1).setPCommandBuffers(&commandBuffer);
  std::async([=]() { // async submit and await for destruction command buffers
    vk::Fence fence = deviceQueue->logical.createFence(vk::FenceCreateInfo());
    deviceQueue->mainQueue->queue.submit(1, &kernel, fence);
    deviceQueue->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
    asyncCallback();
    deviceQueue->logical.destroyFence(fence);
    deviceQueue->logical.freeCommandBuffers(deviceQueue->commandPool, 1,
                                            &commandBuffer);
  });
};

// transition texture layout
void imageBarrier(const vk::CommandBuffer &cmd, const TextureType &image,
                  vk::ImageLayout oldLayout)
{
  // image memory barrier transfer
  vk::ImageMemoryBarrier srcImb;
  srcImb.oldLayout = oldLayout;
  srcImb.newLayout = image->layout;
  srcImb.subresourceRange = image->subresourceRange;
  srcImb.image = image->image;
  srcImb.srcAccessMask = {};

  // barrier
  cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                      vk::PipelineStageFlagBits::eAllCommands, {}, nullptr,
                      nullptr, std::array<vk::ImageMemoryBarrier, 1>{srcImb});
  image->initialLayout = srcImb.newLayout;
}

// transition texture layout
void imageBarrier(const vk::CommandBuffer &cmd, const TextureType &image)
{
  imageBarrier(cmd, image, image->initialLayout);
}

// create texture object
auto createTexture(const DeviceQueueType &deviceQueue,
                   vk::ImageViewType imageViewType, vk::ImageLayout layout,
                   vk::Extent3D size, vk::ImageUsageFlags usage,
                   vk::Format format = vk::Format::eR8G8B8A8Unorm,
                   uint32_t mipLevels = 1,
                   VmaMemoryUsage usageType = VMA_MEMORY_USAGE_GPU_ONLY)
{
  std::shared_ptr<Texture> texture(new Texture);
  texture->device = deviceQueue;
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
  imageInfo.extent = {size.width, size.height,
                      size.depth * (isCubemap ? 6 : 1)};
  imageInfo.format = format;
  imageInfo.mipLevels = mipLevels;
  imageInfo.pQueueFamilyIndices = &deviceQueue->mainQueue->familyIndex;
  imageInfo.queueFamilyIndexCount = 1;
  imageInfo.samples = vk::SampleCountFlagBits::e1; // at now not supported MSAA
  imageInfo.usage = usage;

  // create image with allocation
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = usageType;
  vmaCreateImage(deviceQueue->allocator, &(VkImageCreateInfo)imageInfo,
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
  texture->subresourceLayers.baseArrayLayer =
      texture->subresourceRange.baseArrayLayer;
  texture->subresourceLayers.aspectMask = texture->subresourceRange.aspectMask;
  texture->subresourceLayers.mipLevel = texture->subresourceRange.baseMipLevel;

  // descriptor for usage
  texture->view = deviceQueue->logical.createImageView(
      vk::ImageViewCreateInfo()
          .setSubresourceRange(texture->subresourceRange)
          .setViewType(imageViewType)
          .setComponents(vk::ComponentMapping(
              vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
              vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA))
          .setImage(texture->image)
          .setFormat(format));
  texture->descriptorInfo =
      vk::DescriptorImageInfo(nullptr, texture->view, texture->layout);
  texture->initialized = true;

  // do layout transition
  auto commandBuffer = getCommandBuffer(deviceQueue, true);
  imageBarrier(commandBuffer, texture); // transit to new layouts
  flushCommandBuffer(deviceQueue, commandBuffer, true);
  return std::move(texture);
}

//
auto createTexture(const DeviceQueueType &deviceQueue,
                   vk::ImageViewType viewType, vk::Extent3D size,
                   vk::ImageUsageFlags usage,
                   vk::Format format = vk::Format::eR8G8B8A8Unorm,
                   uint32_t mipLevels = 1,
                   VmaMemoryUsage usageType = VMA_MEMORY_USAGE_GPU_ONLY)
{
  return createTexture(deviceQueue, viewType, vk::ImageLayout::eGeneral, size,
                       usage, format, mipLevels, usageType);
}

// create buffer function
auto createBuffer(const DeviceQueueType &deviceQueue, size_t bufferSize,
                  vk::BufferUsageFlags usageBits, VmaMemoryUsage usageType)
{
  std::shared_ptr<Buffer> buffer(new Buffer);

  // link with device
  buffer->device = deviceQueue;

  auto binfo = vk::BufferCreateInfo(vk::BufferCreateFlags(), bufferSize,
                                    usageBits, vk::SharingMode::eExclusive, 1,
                                    &deviceQueue->mainQueue->familyIndex);

  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = usageType;
  if (usageType != VMA_MEMORY_USAGE_GPU_ONLY)
  {
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
  }

  vmaCreateBuffer(deviceQueue->allocator, &(VkBufferCreateInfo)binfo,
                  &allocCreateInfo, (VkBuffer *)&buffer->buffer,
                  &buffer->allocation, &buffer->allocationInfo);

  // add state descriptor info
  buffer->descriptorInfo =
      vk::DescriptorBufferInfo(buffer->buffer, 0, bufferSize);
  buffer->initialized = true;
  return std::move(buffer);
}

// fill buffer function
template <class T>
void bufferSubData(vk::CommandBuffer &cmd, const BufferType &buffer,
                   const std::vector<T> &hostdata, intptr_t offset = 0)
{
  const size_t bufferSize = hostdata.size() * sizeof(T);
  if (bufferSize > 0)
    memcpy((uint8_t *)buffer->allocationInfo.pMappedData + offset,
           hostdata.data(), bufferSize);
}

void bufferSubData(vk::CommandBuffer &cmd, const BufferType &buffer,
                   const uint8_t *hostdata, const size_t bufferSize,
                   intptr_t offset = 0)
{
  if (bufferSize > 0)
    memcpy((uint8_t *)buffer->allocationInfo.pMappedData + offset, hostdata,
           bufferSize);
}

// get buffer data function
template <class T>
void getBufferSubData(const BufferType &buffer, std::vector<T> &hostdata,
                      intptr_t offset = 0)
{
  memcpy(hostdata.data(),
         (const uint8_t *)buffer->allocationInfo.pMappedData + offset,
         hostdata.size() * sizeof(T));
}

// get buffer data function
void getBufferSubData(const BufferType &buffer, uint8_t *hostdata,
                      const size_t bufferSize, intptr_t offset = 0)
{
  memcpy(hostdata, (const uint8_t *)buffer->allocationInfo.pMappedData + offset,
         bufferSize);
}

// copy buffer command by region
void memoryCopyCmd(vk::CommandBuffer &cmd, const BufferType &src,
                   const BufferType &dst, vk::BufferCopy region)
{
  cmd.copyBuffer(src->buffer, dst->buffer, 1, &region);
}

// store buffer data to subimage
void memoryCopyCmd(vk::CommandBuffer &cmd, const BufferType &src,
                   const TextureType &dst, vk::BufferImageCopy region,
                   vk::ImageLayout oldLayout)
{
  cmd.copyBufferToImage(src->buffer, dst->image, dst->layout, 1, &region);
}

// load image subdata to buffer
void memoryCopyCmd(vk::CommandBuffer &cmd, const TextureType &src,
                   const BufferType &dst, vk::BufferImageCopy region,
                   vk::ImageLayout oldLayout)
{
  cmd.copyImageToBuffer(src->image, src->layout, dst->buffer, 1, &region);
}

// copy image to image
void memoryCopyCmd(vk::CommandBuffer &cmd, const TextureType &src,
                   const TextureType &dst, vk::ImageCopy region,
                   vk::ImageLayout srcOldLayout, vk::ImageLayout dstOldLayout)
{
  cmd.copyImage(src->image, src->layout, dst->image, dst->layout, 1, &region);
}

// store buffer data to subimage
void memoryCopyCmd(vk::CommandBuffer &cmd, const BufferType &src,
                   const TextureType &dst, vk::BufferImageCopy region)
{
  memoryCopyCmd(cmd, src, dst, region, dst->initialLayout);
}

// store buffer data to subimage
void memoryCopyCmd(vk::CommandBuffer &cmd, const TextureType &src,
                   const TextureType &dst, vk::ImageCopy region)
{
  memoryCopyCmd(cmd, src, dst, region, src->initialLayout, dst->initialLayout);
}

// load image subdata to buffer
void memoryCopyCmd(vk::CommandBuffer &cmd, const TextureType &src,
                   const BufferType &dst, vk::BufferImageCopy region)
{
  memoryCopyCmd(cmd, src, dst, region, src->initialLayout);
}

/*
template<class ...T>
void copyMemoryProxy(const DeviceQueueType& deviceQueue, T... args, bool async =
true) { // copy staging buffers vk::CommandBuffer copyCmd =
getCommandBuffer(deviceQueue, true); memoryCopyCmd(copyCmd, args...);
flushCommandBuffer(deviceQueue, copyCmd, async);
}

template<class ...T>
void copyMemoryProxy(const DeviceQueueType& deviceQueue, T... args, const
std::function<void()>& asyncCallback) { // copy staging buffers
    vk::CommandBuffer copyCmd = getCommandBuffer(deviceQueue, true);
    memoryCopyCmd(copyCmd, args...); flushCommandBuffer(deviceQueue, copyCmd,
asyncCallback);
}
*/

template <class... T>
vk::CommandBuffer createCopyCmd(const DeviceQueueType &deviceQueue,
                                T... args)
{ // copy staging buffers
  vk::CommandBuffer copyCmd = getCommandBuffer(deviceQueue, true);
  memoryCopyCmd(copyCmd,
                args...); // flushCommandBuffer(deviceQueue, copyCmd, async);
  return copyCmd;
}

// create fence function
vk::Fence createFence(DeviceQueueType &device, bool signaled = true)
{
  vk::FenceCreateInfo info;
  if (signaled)
    info.setFlags(vk::FenceCreateFlagBits::eSignaled);
  return device->logical.createFence(info);
}

// create command buffer function
vk::CommandBuffer createCommandBuffer(DeviceQueueType &deviceQueue)
{
  return deviceQueue->logical.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo(deviceQueue->commandPool,
                                    vk::CommandBufferLevel::ePrimary, 1))[0];
}

// create wait fences
std::vector<vk::Fence> createFences(DeviceQueueType &device,
                                    uint32_t fenceCount)
{
  std::vector<vk::Fence> waitFences;
  for (int i = 0; i < fenceCount; i++)
  {
    waitFences.push_back(device->logical.createFence(vk::FenceCreateInfo()));
  }
  return waitFences;
}

// create command buffers
std::vector<vk::CommandBuffer>
createCommandBuffers(DeviceQueueType &deviceQueue, uint32_t bcount = 1)
{
  return deviceQueue->logical.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo(deviceQueue->commandPool,
                                    vk::CommandBufferLevel::ePrimary, bcount));
}

// destroy buffer function (by linked device)
void destroyBuffer(BufferType &buffer)
{
  if (buffer && buffer->initialized)
  {
    vmaDestroyBuffer(buffer->device->allocator, buffer->buffer,
                     buffer->allocation);
    buffer->initialized = false;
  }
}

void destroyTexture(TextureType &image)
{
  if (image && image->initialized)
  {
    vmaDestroyImage(image->device->allocator, image->image, image->allocation);
    image->initialized = false;
  }
}

// void destroyBuffer(Buffer& buffer) {
//    if (buffer.initialized) {
//        vmaDestroyBuffer(buffer.device->allocator, buffer.buffer,
//        buffer.allocation); buffer.initialized = false;
//    }
//}

// read source (unused)
std::string readSource(const std::string &filePath,
                       const bool &lineDirective = false)
{
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
std::vector<char> readBinary(const std::string &filePath)
{
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
vk::ShaderModule loadAndCreateShaderModule(DeviceQueueType &device,
                                           std::string path)
{
  auto code = readBinary(path);
  return device->logical.createShaderModule(vk::ShaderModuleCreateInfo(
      vk::ShaderModuleCreateFlags(), code.size(), (uint32_t *)code.data()));
}

// create compute pipeline
auto createCompute(DeviceQueueType &device, std::string path,
                   vk::PipelineLayout &layout, vk::PipelineCache &cache)
{
  vk::ComputePipelineCreateInfo cmpi;
  cmpi.setStage(vk::PipelineShaderStageCreateInfo()
                    .setModule(loadAndCreateShaderModule(device, path))
                    .setPName("main")
                    .setStage(vk::ShaderStageFlagBits::eCompute));
  ;
  cmpi.setLayout(layout);

  vk::Pipeline pipeline;
  try
  {
    pipeline = device->logical.createComputePipeline(cache, cmpi);
  }
  catch (std::exception const &e)
  {
    std::cerr << e.what() << std::endl;
  }
  return pipeline;
}

}; // namespace NSM