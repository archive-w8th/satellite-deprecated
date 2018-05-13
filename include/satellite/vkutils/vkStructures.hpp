#pragma once

#include "../utils.hpp"

#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#define DEFAULT_FENCE_TIMEOUT 100000000000

//#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_mem_alloc.h>

namespace NSM
{
    struct QueueType;
    struct DevQueueType;
    struct DeviceType;
    struct QueueType;
    struct BufferType;
    struct ImageType;
    struct SamplerType;
    struct ImageCombinedType;

    using DevQueue = std::shared_ptr<DevQueueType>;
    using Device = std::shared_ptr<DeviceType>;
    using Queue = std::shared_ptr<QueueType>;
    using Buffer = std::shared_ptr<BufferType>;
    using Image = std::shared_ptr<ImageType>;
    using Sampler = std::shared_ptr<SamplerType>;
    using ImageCombined = std::shared_ptr<ImageCombinedType>;


    struct DevQueueType : public std::enable_shared_from_this<DevQueueType> {
        uint32_t familyIndex = 0;
        vk::Queue queue;
    };

    struct DeviceType : public std::enable_shared_from_this<DeviceType> {
        bool initialized = false;
        bool executed = false;
        vk::Device logical;
        std::shared_ptr<vk::PhysicalDevice> physical;

        vk::DescriptorPool descriptorPool;
        vk::PipelineCache pipelineCache;
        VmaAllocator allocator;
        vk::DispatchLoaderDynamic dldid;

        std::vector<DevQueue> queues;
        operator vk::Device() const { return logical; }
    };

    // application device structure
    struct QueueType : public std::enable_shared_from_this<QueueType> {
        Device device;
        vk::CommandPool commandPool;
        vk::Queue queue;
        vk::Fence fence;
        uint32_t familyIndex = 0;

        operator Device() const { return device; }
        operator vk::Device() const { return device->logical; }
        operator vk::Queue() const { return queue; }
    };

    // application surface format information structure
    struct SurfaceFormat : public std::enable_shared_from_this<SurfaceFormat> {
        vk::Format colorFormat;
        vk::Format depthFormat;
        vk::Format stencilFormat;
        vk::ColorSpaceKHR colorSpace;
        vk::FormatProperties colorFormatProperties;
    };

    // framebuffer with command buffer and fence
    struct Framebuffer : public std::enable_shared_from_this<Framebuffer> {
        vk::Framebuffer frameBuffer;
        vk::CommandBuffer commandBuffer; // terminal command (barrier)
        vk::Fence waitFence;
        vk::Semaphore semaphore;
    };

    // buffer with memory
    struct BufferType : public std::enable_shared_from_this<BufferType> {
        bool initialized = false;
        Queue queue;
        // vk::DeviceMemory memory;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
        vk::Buffer buffer;
        vk::DescriptorBufferInfo descriptorInfo;
    };

    // texture
    struct ImageType : public std::enable_shared_from_this<ImageType> {
        bool initialized = false;
        Queue queue;
        // vk::DeviceMemory memory;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
        vk::Image image;
        vk::ImageView view;
        vk::Format format;
        vk::ImageLayout layout;
        vk::ImageLayout initialLayout;
        vk::ImageSubresourceRange subresourceRange;
        vk::ImageSubresourceLayers subresourceLayers;
        vk::DescriptorImageInfo descriptorInfo;
    };

    // sampler
    struct SamplerType : public std::enable_shared_from_this<SamplerType> {
        bool initialized = false;
        Device device;
        vk::Sampler sampler;
        vk::DescriptorImageInfo descriptorInfo;
    };

    struct ImageCombinedType : public std::enable_shared_from_this<ImageCombinedType> {
        Image texture;
        Sampler sampler;
        vk::DescriptorBufferInfo descriptorInfo;
        vk::DescriptorBufferInfo &combine() { return descriptorInfo; }
    };

    // vertex layout
    struct VertexLayout : public std::enable_shared_from_this<VertexLayout>
    {
        std::vector<vk::VertexInputBindingDescription> inputBindings;
        std::vector<vk::VertexInputAttributeDescription> inputAttributes;
    };

    // vertex with layouts
    struct VertexBuffer : public std::enable_shared_from_this<VertexBuffer>
    {
        uint64_t binding = 0;
        vk::DeviceSize voffset = 0;
        Buffer buffer;
    };

    // indice buffer
    struct IndexBuffer : public std::enable_shared_from_this<IndexBuffer>
    {
        Buffer buffer;
        uint32_t count;
        vk::IndexType indexType = vk::IndexType::eUint32;
    };

    // uniform buffer
    struct UniformBuffer : public std::enable_shared_from_this<UniformBuffer>
    {
        Buffer buffer;
        Buffer staging;
        vk::DescriptorBufferInfo descriptor;
    };

    // context for rendering (can be switched)
    struct GraphicsContext : public std::enable_shared_from_this<GraphicsContext>
    {
        Queue queue;     // used device by context
        vk::SwapchainKHR swapchain; // swapchain state
        vk::Pipeline pipeline;      // current pipeline
        vk::PipelineLayout pipelineLayout;
        vk::PipelineCache pipelineCache;
        vk::DescriptorPool descriptorPool; // current descriptor pool
        vk::RenderPass renderpass;
        std::vector<vk::DescriptorSet> descriptorSets; // descriptor sets
        std::vector<Framebuffer> framebuffers;         // swapchain framebuffers
        std::function<void()> draw;
    };

    // compute context
    struct ComputeContext : public std::enable_shared_from_this<ComputeContext>
    {
        Queue queue;          // used device by context
        vk::CommandBuffer commandBuffer; // command buffer of compute context
        vk::Pipeline pipeline;           // current pipeline
        vk::PipelineCache pipelineCache;
        vk::PipelineLayout pipelineLayout;
        vk::Fence waitFence;             // wait fence of computing
        vk::DescriptorPool descriptorPool;             // current descriptor pool
        std::vector<vk::DescriptorSet> descriptorSets; // descriptor sets
        std::function<void()> dispatch;
    };
}; // namespace NSM