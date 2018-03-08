#pragma once

#include "../utils.hpp"

#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#define DEFAULT_FENCE_TIMEOUT 100000000000

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_mem_alloc.h>

namespace NSM
{

    struct DevQueue
    {
        uint32_t familyIndex;
        vk::Queue queue;
    };

    using DevQueueType = std::shared_ptr<DevQueue>;

    // application device structure
    struct DeviceQueue
    {
        bool initialized = false;
        bool executed = false;
        vk::Device logical;
        std::shared_ptr<vk::PhysicalDevice> physical;

        vk::Semaphore wsemaphore;
        vk::CommandPool commandPool;
        vk::Semaphore currentSemaphore = nullptr;
        vk::DescriptorPool descriptorPool;
        vk::Fence fence;
        VmaAllocator allocator;

        // queue managment
        DevQueueType mainQueue;
        std::vector<DevQueueType> queues;
    };

    using DeviceQueueType = std::shared_ptr<DeviceQueue>;

    // application surface format information structure
    struct SurfaceFormat
    {
        vk::Format colorFormat;
        vk::Format depthFormat;
        vk::Format stencilFormat;
        vk::ColorSpaceKHR colorSpace;
        vk::FormatProperties colorFormatProperties;
    };

    // framebuffer with command buffer and fence
    struct Framebuffer
    {
        vk::Framebuffer frameBuffer;
        vk::CommandBuffer commandBuffer; // terminal command (barrier)
        vk::Fence waitFence;
        vk::Semaphore semaphore;
    };

    // buffer with memory
    struct Buffer
    {
        bool initialized = false;
        DeviceQueueType device;
        // vk::DeviceMemory memory;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
        vk::Buffer buffer;
        vk::DescriptorBufferInfo descriptorInfo;
    };

    // texture
    struct Texture
    {
        bool initialized = false;
        DeviceQueueType device;
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
    struct Sampler
    {
        bool initialized = false;
        DeviceQueueType device;
        vk::Sampler sampler;
        vk::DescriptorImageInfo descriptorInfo;
    };

    // shared pointer for objects
    using BufferType = std::shared_ptr<Buffer>;
    using TextureType = std::shared_ptr<Texture>;
    using SamplerType = std::shared_ptr<Sampler>;

    struct TextureCombined
    {
        TextureType texture;
        SamplerType sampler;
        vk::DescriptorBufferInfo descriptorInfo;
        vk::DescriptorBufferInfo &combine()
        { // TODO for implement

            return descriptorInfo;
        }
    };

    // vertex layout
    struct VertexLayout
    {
        std::vector<vk::VertexInputBindingDescription> inputBindings;
        std::vector<vk::VertexInputAttributeDescription> inputAttributes;
    };

    // vertex with layouts
    struct VertexBuffer
    {
        uint64_t binding = 0;
        vk::DeviceSize voffset = 0;
        BufferType buffer;
    };

    // indice buffer
    struct IndexBuffer
    {
        BufferType buffer;
        uint32_t count;
        vk::IndexType indexType = vk::IndexType::eUint32;
    };

    // uniform buffer
    struct UniformBuffer
    {
        BufferType buffer;
        BufferType staging;
        vk::DescriptorBufferInfo descriptor;
    };

    // sampler
    // struct Sampler {
    //    vk::Sampler sampler;
    //};

    // context for rendering (can be switched)
    struct GraphicsContext
    {
        DeviceQueueType device;     // used device by context
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
    struct ComputeContext
    {
        DeviceQueueType device;          // used device by context
        vk::CommandBuffer commandBuffer; // command buffer of compute context
        vk::Pipeline pipeline;           // current pipeline
        vk::Fence waitFence;             // wait fence of computing
        vk::PipelineLayout pipelineLayout;
        vk::PipelineCache pipelineCache;
        vk::DescriptorPool descriptorPool;             // current descriptor pool
        std::vector<vk::DescriptorSet> descriptorSets; // descriptor sets
        std::function<void()> dispatch;
    };
}; // namespace NSM