#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <functional>
#include <vk_mem_alloc.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace NSM {

    // application device structure
    struct DeviceQueue {
        bool initialized = false;
        bool executed = false;
        vk::Device logical;
        vk::PhysicalDevice physical;
        vk::Queue queue;
        uint32_t familyIndex = 0;
        vk::Semaphore presentCompleteSemaphore;
        vk::Semaphore renderCompleteSemaphore;
        vk::CommandPool commandPool;
        vk::Semaphore currentSemaphore = nullptr;
        VmaAllocator allocator;
    };

    using DeviceQueueType = std::shared_ptr<DeviceQueue>;


    // application surface format information structure
    struct SurfaceFormat {
        vk::Format colorFormat;
        vk::Format depthFormat;
        vk::Format stencilFormat;
        vk::ColorSpaceKHR colorSpace;
        vk::FormatProperties colorFormatProperties;
    };

    // framebuffer with command buffer and fence
    struct Framebuffer {
        vk::Framebuffer frameBuffer;
        //vk::CommandBuffer commandBuffer;
        vk::Fence waitFence;
    };

    // buffer with memory
    struct Buffer {
        bool initialized = false;
        DeviceQueueType device;
        //vk::DeviceMemory memory;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
        vk::Buffer buffer;
        vk::DescriptorBufferInfo descriptorInfo;
    };

    // texture
    struct Texture {
        bool initialized = false;
        DeviceQueueType device;
        //vk::DeviceMemory memory;
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

    // use alias
    using BufferType = std::shared_ptr<Buffer>;
    using TextureType = std::shared_ptr<Texture>;




    // vertex layout
    struct VertexLayout {
        std::vector<vk::VertexInputBindingDescription> inputBindings;
        std::vector<vk::VertexInputAttributeDescription> inputAttributes;
    };

    // vertex with layouts
    struct VertexBuffer {
        uint64_t binding = 0;
        vk::DeviceSize voffset = 0;
        BufferType buffer;
    };

    // indice buffer
    struct IndexBuffer {
        BufferType buffer;
        uint32_t count;
        vk::IndexType indexType = vk::IndexType::eUint32;
    };

    // uniform buffer
    struct UniformBuffer {
        BufferType buffer;
        BufferType staging;
        vk::DescriptorBufferInfo descriptor;
    };





    // sampler
    //struct Sampler {
    //    vk::Sampler sampler;
    //};


    // context for rendering (can be switched)
    struct GraphicsContext {
        DeviceQueueType device; // used device by context
        vk::SwapchainKHR swapchain; // swapchain state
        vk::Pipeline pipeline; // current pipeline
        vk::PipelineLayout pipelineLayout;
        vk::PipelineCache pipelineCache;
        vk::DescriptorPool descriptorPool; // current descriptor pool
        vk::RenderPass renderpass;
        std::vector<vk::DescriptorSet> descriptorSets; // descriptor sets
        std::vector<Framebuffer> framebuffers; // swapchain framebuffers
        std::function<void()> draw;
    };

    // compute context
    struct ComputeContext {
        DeviceQueueType device; // used device by context
        vk::CommandBuffer commandBuffer; // command buffer of compute context
        vk::Pipeline pipeline; // current pipeline
        vk::Fence waitFence; // wait fence of computing
        vk::PipelineLayout pipelineLayout;
        vk::PipelineCache pipelineCache;
        vk::DescriptorPool descriptorPool; // current descriptor pool
        std::vector<vk::DescriptorSet> descriptorSets; // descriptor sets
        std::function<void()> dispatch;
    };


    


};