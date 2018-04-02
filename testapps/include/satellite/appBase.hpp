#pragma once

#include <satellite/utils.hpp>
#include <satellite/gapi.hpp>

#ifdef OS_WIN
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#endif

#ifdef OS_LNX
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace NSM
{
    class ApplicationBase
    {
    protected:
        struct SurfaceWindow
        {
            GLFWwindow *window;
            std::shared_ptr<vk::SurfaceKHR> surface;
            SurfaceFormat surfaceFormat;
            vk::Extent2D surfaceSize;
        } applicationWindow;

        // application binding
        vk::Instance instance;

        // cached Vulkan API data
        std::vector<DeviceQueueType> devices;

        // instance extensions
        std::vector<const char *> wantedExtensions = {
            "VK_KHR_get_physical_device_properties2",
            "VK_EXT_debug_report",
            "VK_KHR_surface",
            "VK_KHR_win32_surface"
        };

        // default device extensions
        std::vector<const char *> wantedDeviceExtensions = {
            "VK_EXT_swapchain_colorspace",
            "VK_EXT_direct_mode_display",
            "VK_EXT_debug_marker",
            "VK_EXT_debug_report",
            "VK_EXT_sample_locations",
            "VK_EXT_shader_subgroup_vote",
            "VK_EXT_shader_subgroup_ballot",
            "VK_EXT_external_memory_host",
            "VK_EXT_conservative_rasterization",
            "VK_EXT_hdr_metadata",
            "VK_EXT_queue_family_foreign",
            "VK_EXT_sampler_filter_minmax",


            "VK_KHR_16bit_storage",
            "VK_AMD_gpu_shader_int16",
            "VK_AMD_gpu_shader_half_float",

            "VK_AMD_buffer_marker",
            "VK_AMD_shader_info",
            "VK_AMD_shader_ballot",
            "VK_AMD_texture_gather_bias_lod",
            "VK_AMD_shader_image_load_store_lod",
            "VK_AMD_gcn_shader",
            "VK_AMD_shader_trinary_minmax",
            "VK_AMD_draw_indirect_count",

            "VK_KHR_descriptor_update_template",
            "VK_KHR_dedicated_allocation",
            "VK_KHR_incremental_present",
            "VK_KHR_push_descriptor",
            "VK_KHR_swapchain",
            "VK_KHR_sampler_ycbcr_conversion",
            "VK_KHR_image_format_list",
            "VK_KHR_sampler_mirror_clamp_to_edge",
            "VK_KHR_shader_draw_parameters",
            "VK_KHR_storage_buffer_storage_class",
            "VK_KHR_variable_pointers",
            "VK_KHR_relaxed_block_layout",
            "VK_KHR_display"
            "VK_KHR_display_swapchain"


            "VK_KHR_get_memory_requirements2",
            "VK_KHR_get_physical_device_properties2",
            "VK_KHR_get_surface_capabilities2",
            "VK_KHR_bind_memory2",
            "VK_KHR_maintenance1",
            "VK_KHR_maintenance2",
            "VK_KHR_maintenance3"
        };

        // instance layers
        std::vector<const char *> wantedLayers = {
            //"VK_LAYER_RENDERDOC_Capture"
            //"VK_LAYER_LUNARG_standard_validation",
            //"VK_LAYER_LUNARG_parameter_validation",
            //"VK_LAYER_LUNARG_core_validation",
            //"VK_LAYER_LUNARG_assistant_layer",
            //"VK_LAYER_LUNARG_vktrace",
            //"VK_LAYER_GOOGLE_threading"
        };

        // default device layers
        std::vector<const char *> wantedDeviceValidationLayers = {
            "VK_LAYER_AMD_switchable_graphics"
        };

        vk::Instance createInstance()
        {

            // get required extensions
            unsigned int glfwExtensionCount = 0;
            const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            // add glfw extensions to list
            for (int i = 0; i < glfwExtensionCount; i++) {
                wantedExtensions.push_back(glfwExtensions[i]);
            }

            // get our needed extensions
            auto installedExtensions = vk::enumerateInstanceExtensionProperties();
            auto extensions = std::vector<const char *>();
            for (auto &w : wantedExtensions) {
                for (auto &i : installedExtensions)
                {
                    if (std::string(i.extensionName).compare(w) == 0)
                    {
                        extensions.emplace_back(w);
                        break;
                    }
                }
            }

            // get validation layers
            auto installedLayers = vk::enumerateInstanceLayerProperties();
            auto layers = std::vector<const char *>();
            for (auto &w : wantedLayers) {
                for (auto &i : installedLayers)
                {
                    if (std::string(i.layerName).compare(w) == 0)
                    {
                        layers.emplace_back(w);
                        break;
                    }
                }
            }

            // app info
            vk::ApplicationInfo appinfo;
            appinfo.pApplicationName = "VKTest";
            appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appinfo.apiVersion = VK_API_VERSION_1_1;

            // create instance info
            vk::InstanceCreateInfo cinstanceinfo;
            cinstanceinfo.pApplicationInfo = &appinfo;
            cinstanceinfo.enabledExtensionCount = extensions.size();
            cinstanceinfo.ppEnabledExtensionNames = extensions.data();
            cinstanceinfo.enabledLayerCount = layers.size();
            cinstanceinfo.ppEnabledLayerNames = layers.data();

            // create instance
            instance = vk::createInstance(cinstanceinfo);

            // get physical device for application
            return instance;
        };

        virtual DeviceQueueType createDevice(std::shared_ptr<vk::PhysicalDevice> gpu, bool isComputePrior = false)
        {
            // use extensions
            auto deviceExtensions = std::vector<const char *>();
            auto gpuExtensions = gpu->enumerateDeviceExtensionProperties();
            for (auto &w : wantedDeviceExtensions)
            {
                for (auto &i : gpuExtensions)
                {
                    if (std::string(i.extensionName).compare(w) == 0)
                    {
                        deviceExtensions.emplace_back(w);
                        break;
                    }
                }
            }

            // use layers
            auto layers = std::vector<const char *>();
            auto deviceValidationLayers = std::vector<const char *>();
            auto gpuLayers = gpu->enumerateDeviceLayerProperties();
            for (auto &w : wantedLayers)
            {
                for (auto &i : gpuLayers)
                {
                    if (std::string(i.layerName).compare(w) == 0)
                    {
                        layers.emplace_back(w);
                        break;
                    }
                }
            }

            // get features and queue family properties
            auto gpuFeatures = gpu->getFeatures();
            auto gpuQueueProps = gpu->getQueueFamilyProperties();

            // search graphics supported queue family
            std::vector<DevQueueType> queues;

            float priority = 1.0f;
            uint32_t computeFamilyIndex = -1, graphicsFamilyIndex = -1;
            auto queueCreateInfos = std::vector<vk::DeviceQueueCreateInfo>();




            // compute/graphics queue family
            for (auto &queuefamily : gpuQueueProps) {
                computeFamilyIndex++;
                if (queuefamily.queueFlags & (vk::QueueFlagBits::eCompute)) {
                    queueCreateInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags()).setQueueFamilyIndex(computeFamilyIndex).setQueueCount(1).setPQueuePriorities(&priority));
                    queues.push_back(std::make_shared<DevQueue>(DevQueue{ computeFamilyIndex, vk::Queue{} }));
                    break;
                }
            }


            // graphics/presentation queue family
            for (auto &queuefamily : gpuQueueProps) {
                graphicsFamilyIndex++;
                if (queuefamily.queueFlags & (vk::QueueFlagBits::eGraphics) && gpu->getSurfaceSupportKHR(graphicsFamilyIndex, *applicationWindow.surface) && graphicsFamilyIndex != computeFamilyIndex) {
                    queueCreateInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags()).setQueueFamilyIndex(computeFamilyIndex).setQueueCount(1).setPQueuePriorities(&priority));
                    queues.push_back(std::make_shared<DevQueue>(DevQueue{ graphicsFamilyIndex, vk::Queue{} }));
                    break;
                }
            }


            // assign presentation (may not working)
            if (int(graphicsFamilyIndex) < 0 || graphicsFamilyIndex >= gpuQueueProps.size()) {
                std::wcerr << "Device may not support presentation" << std::endl;
                graphicsFamilyIndex = computeFamilyIndex;
            }


            // make graphics queue family same as compute
            if ((graphicsFamilyIndex == computeFamilyIndex || queues.size() <= 1) && queues.size() > 0) {
                queues.push_back(queues[0]);
            }


            // pre-declare logical device
            auto deviceQueuePtr = std::shared_ptr<DeviceQueue>(new DeviceQueue);

            // if have supported queue family, then use this device
            if (queueCreateInfos.size() > 0)
            {
                deviceQueuePtr->physical = gpu;
                deviceQueuePtr->logical = gpu->createDevice(vk::DeviceCreateInfo().setFlags(vk::DeviceCreateFlags())
                    .setPEnabledFeatures(&gpuFeatures)
                    .setPQueueCreateInfos(queueCreateInfos.data()).setQueueCreateInfoCount(queueCreateInfos.size())
                    .setPpEnabledExtensionNames(deviceExtensions.data()).setEnabledExtensionCount(deviceExtensions.size())
                    .setPpEnabledLayerNames(deviceValidationLayers.data()).setEnabledLayerCount(deviceValidationLayers.size()));

                // init dispatch loader
                deviceQueuePtr->dldid = vk::DispatchLoaderDynamic(instance, deviceQueuePtr->logical);

                // getting queues by family
                for (int i = 0; i < queues.size(); i++) { queues[i]->queue = deviceQueuePtr->logical.getQueue(queues[i]->familyIndex, 0); }
                deviceQueuePtr->queues = queues;
                
                // add device and use this device
                devices.push_back(deviceQueuePtr);

                // create semaphores
                deviceQueuePtr->wsemaphore = deviceQueuePtr->logical.createSemaphore(vk::SemaphoreCreateInfo());

                // create queue and command pool
                deviceQueuePtr->mainQueue = deviceQueuePtr->queues[isComputePrior ? 0 : 1]; // make role prior
                deviceQueuePtr->commandPool = deviceQueuePtr->logical.createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer), deviceQueuePtr->mainQueue->familyIndex));

                // create allocator
                VmaAllocatorCreateInfo allocatorInfo = {};
                allocatorInfo.physicalDevice = *deviceQueuePtr->physical;
                allocatorInfo.device = deviceQueuePtr->logical;
                allocatorInfo.preferredLargeHeapBlockSize = 16384; // 16kb
                allocatorInfo.flags =
                    VmaAllocationCreateFlagBits::
                    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT ||
                    VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
                vmaCreateAllocator(&allocatorInfo, &deviceQueuePtr->allocator);

                // pool sizes, and create descriptor pool
                std::vector<vk::DescriptorPoolSize> psizes = {
                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 64),
                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 16),
                    vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 16),
                    vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 128),
                    vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 16) };
                deviceQueuePtr->descriptorPool =
                    deviceQueuePtr->logical.createDescriptorPool(
                        vk::DescriptorPoolCreateInfo()
                        .setPPoolSizes(psizes.data())
                        .setPoolSizeCount(psizes.size())
                        .setMaxSets(16)
                    );
                deviceQueuePtr->fence = createFence(deviceQueuePtr, false);
                deviceQueuePtr->initialized = true;
            }

            // return device with queue pointer
            return std::move(deviceQueuePtr);
        }

        // create window and surface for this application (multi-window not supported)
        virtual SurfaceWindow &createWindowSurface(GLFWwindow *window, uint32_t WIDTH, uint32_t HEIGHT, std::string title = "TestApp") {
            applicationWindow.window = window;
            applicationWindow.surfaceSize = { WIDTH, HEIGHT };
            auto vksurface = VkSurfaceKHR();
            glfwCreateWindowSurface(instance, applicationWindow.window, nullptr, &vksurface);
            applicationWindow.surface = std::make_shared<vk::SurfaceKHR>(vksurface);
            return applicationWindow;
        }

        // create window and surface for this application (multi-window not supported)
        virtual SurfaceWindow &createWindowSurface(uint32_t WIDTH, uint32_t HEIGHT, std::string title = "TestApp") {
            applicationWindow.window = glfwCreateWindow(WIDTH, HEIGHT, title.c_str(), nullptr, nullptr);
            applicationWindow.surfaceSize = { WIDTH, HEIGHT };
            auto vksurface = VkSurfaceKHR();
            glfwCreateWindowSurface(instance, applicationWindow.window, nullptr, &vksurface);
            applicationWindow.surface = std::make_shared<vk::SurfaceKHR>(vksurface);
            return applicationWindow;
        }

        virtual SurfaceFormat getSurfaceFormat(std::shared_ptr<vk::SurfaceKHR> surface, std::shared_ptr<vk::PhysicalDevice> gpu)
        {
            auto surfaceFormats = gpu->getSurfaceFormatsKHR(*surface);

            const std::vector<vk::Format> preferredFormats = {
                vk::Format::eA2R10G10B10UnormPack32,
                vk::Format::eA2B10G10R10UnormPack32, vk::Format::eR8G8B8A8Unorm,
                vk::Format::eB8G8R8A8Unorm };
            vk::Format surfaceColorFormat =
                surfaceFormats.size() == 1 &&
                surfaceFormats[0].format == vk::Format::eUndefined
                ? vk::Format::eR8G8B8A8Unorm
                : surfaceFormats[0].format;

            // search preferred surface format support
            bool surfaceFormatFound = false;
            uint32_t surfaceFormatID = 0;
            for (int i = 0; i < preferredFormats.size(); i++)
            {
                if (surfaceFormatFound)
                    break;
                for (int f = 0; f < surfaceFormats.size(); f++)
                {
                    if (surfaceFormats[f].format == preferredFormats[i])
                    {
                        surfaceFormatFound = true;
                        surfaceFormatID = f;
                        break;
                    }
                }
            }

            // get supported color format
            surfaceColorFormat = surfaceFormats[surfaceFormatID].format;
            vk::ColorSpaceKHR surfaceColorSpace =
                surfaceFormats[surfaceFormatID].colorSpace;

            // get format properties?
            auto formatProperties = gpu->getFormatProperties(surfaceColorFormat);

            // only if these depth formats
            std::vector<vk::Format> depthFormats = {
                vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat,
                vk::Format::eD24UnormS8Uint, vk::Format::eD16UnormS8Uint,
                vk::Format::eD16Unorm };

            // choice supported depth format
            vk::Format surfaceDepthFormat = depthFormats[0];
            for (auto &format : depthFormats)
            {
                auto depthFormatProperties = gpu->getFormatProperties(format);
                if (depthFormatProperties.optimalTilingFeatures &
                    vk::FormatFeatureFlagBits::eDepthStencilAttachment)
                {
                    surfaceDepthFormat = format;
                    break;
                }
            }

            // return format result
            SurfaceFormat sfd;
            sfd.colorSpace = surfaceColorSpace;
            sfd.colorFormat = surfaceColorFormat;
            sfd.depthFormat = surfaceDepthFormat;
            sfd.colorFormatProperties = formatProperties; // get properties about format
            return sfd;
        }

        virtual vk::RenderPass createRenderpass(DeviceQueueType &device,
            SurfaceFormat &formats)
        {
            // attachments
            std::vector<vk::AttachmentDescription> attachmentDescriptions = {

                vk::AttachmentDescription()
                    .setFormat(formats.colorFormat)
                    .setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(vk::AttachmentLoadOp::eLoad)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setFinalLayout(vk::ImageLayout::ePresentSrcKHR),

                vk::AttachmentDescription()
                    .setFormat(formats.depthFormat)
                    .setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)

            };

            // attachments references
            std::vector<vk::AttachmentReference> colorReferences = {
                vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal) };
            std::vector<vk::AttachmentReference> depthReferences = {
                vk::AttachmentReference(
                    1, vk::ImageLayout::eDepthStencilAttachmentOptimal) };

            // subpasses desc
            std::vector<vk::SubpassDescription> subpasses = {
                vk::SubpassDescription()
                    .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                    .setPColorAttachments(colorReferences.data())
                    .setColorAttachmentCount(colorReferences.size())
                    .setPDepthStencilAttachment(depthReferences.data()) };

            // dependency
            std::vector<vk::SubpassDependency> dependencies = {
                vk::SubpassDependency()
                    .setDependencyFlags(vk::DependencyFlagBits::eByRegion)
                    .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                    .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                     vk::PipelineStageFlagBits::eBottomOfPipe |
                                     vk::PipelineStageFlagBits::eTransfer)
                    .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)

                    .setDstSubpass(0)
                    .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                    .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead |
                                      vk::AccessFlagBits::eColorAttachmentWrite),

                vk::SubpassDependency()
                    .setDependencyFlags(vk::DependencyFlagBits::eByRegion)
                    .setSrcSubpass(0)
                    .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                    .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead |
                                      vk::AccessFlagBits::eColorAttachmentWrite)

                    .setDstSubpass(VK_SUBPASS_EXTERNAL)
                    .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                     vk::PipelineStageFlagBits::eTopOfPipe |
                                     vk::PipelineStageFlagBits::eTransfer)
                    .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead |
                                      vk::AccessFlagBits::eColorAttachmentWrite)

            };

            // create renderpass
            return device->logical.createRenderPass(vk::RenderPassCreateInfo(
                vk::RenderPassCreateFlags(), attachmentDescriptions.size(),
                attachmentDescriptions.data(), subpasses.size(), subpasses.data(),
                dependencies.size(), dependencies.data()));
        }

        // update swapchain framebuffer
        virtual void
            updateSwapchainFramebuffer(DeviceQueueType &device,
                vk::SwapchainKHR &swapchain,
                vk::RenderPass &renderpass, SurfaceFormat &formats,
                std::vector<Framebuffer> &swapchainBuffers)
        {
            // The swapchain handles allocating frame images.
            auto swapchainImages = device->logical.getSwapchainImagesKHR(swapchain);
            auto gpuMemoryProps = device->physical->getMemoryProperties();

            // create depth image
            auto imageInfo = vk::ImageCreateInfo(
                vk::ImageCreateFlags(), vk::ImageType::e2D, formats.depthFormat,
                vk::Extent3D(applicationWindow.surfaceSize.width,
                    applicationWindow.surfaceSize.height, 1),
                1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment |
                vk::ImageUsageFlagBits::eTransferSrc,
                vk::SharingMode::eExclusive, 1, &device->queues[1]->familyIndex,
                vk::ImageLayout::eUndefined);

            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VmaAllocation _allocation;
            VkImage _image;
            vmaCreateImage(device->allocator, &(VkImageCreateInfo)imageInfo,
                &allocCreateInfo, &_image, &_allocation,
                nullptr); // allocators planned structs
            auto depthImage = vk::Image(_image);

            // create image viewer
            auto depthImageView =
                device->logical.createImageView(vk::ImageViewCreateInfo(
                    vk::ImageViewCreateFlags(), depthImage, vk::ImageViewType::e2D,
                    formats.depthFormat, vk::ComponentMapping(),
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth |
                        vk::ImageAspectFlagBits::eStencil,
                        0, 1, 0, 1)));

            // create framebuffers
            for (int i = 0; i < swapchainImages.size(); i++)
            {
                vk::Image &image = swapchainImages[i]; // prelink images
                std::array<vk::ImageView, 2> views;    // predeclare views
                views[1] = depthImageView;

                // color view
                views[0] = device->logical.createImageView(vk::ImageViewCreateInfo(
                    vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D,
                    formats.colorFormat, vk::ComponentMapping(),
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0,
                        1)));

                // create framebuffers
                swapchainBuffers[i].frameBuffer =
                    device->logical.createFramebuffer(vk::FramebufferCreateInfo(
                        vk::FramebufferCreateFlags(), renderpass, views.size(),
                        views.data(), applicationWindow.surfaceSize.width,
                        applicationWindow.surfaceSize.height, 1));
            }
        }

        virtual std::vector<Framebuffer> createSwapchainFramebuffer(
            DeviceQueueType &device, vk::SwapchainKHR &swapchain,
            vk::RenderPass &renderpass, SurfaceFormat &formats)
        {
            // The swapchain handles allocating frame images.
            auto swapchainImages = device->logical.getSwapchainImagesKHR(swapchain);
            auto gpuMemoryProps = device->physical->getMemoryProperties();

            // create depth image
            auto imageInfo = vk::ImageCreateInfo(
                vk::ImageCreateFlags(), vk::ImageType::e2D, formats.depthFormat,
                vk::Extent3D(applicationWindow.surfaceSize.width,
                    applicationWindow.surfaceSize.height, 1),
                1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment |
                vk::ImageUsageFlagBits::eTransferSrc,
                vk::SharingMode::eExclusive, 1, &device->queues[1]->familyIndex,
                vk::ImageLayout::eUndefined);

            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VmaAllocation _allocation;
            VkImage _image;
            vmaCreateImage(device->allocator, &(VkImageCreateInfo)imageInfo,
                &allocCreateInfo, &_image, &_allocation,
                nullptr); // allocators planned structs
            auto depthImage = vk::Image(_image);

            // create image viewer
            auto depthImageView =
                device->logical.createImageView(vk::ImageViewCreateInfo(
                    vk::ImageViewCreateFlags(), depthImage, vk::ImageViewType::e2D,
                    formats.depthFormat, vk::ComponentMapping(),
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth |
                        vk::ImageAspectFlagBits::eStencil,
                        0, 1, 0, 1)));

            // framebuffers vector
            std::vector<Framebuffer> swapchainBuffers;
            swapchainBuffers.resize(swapchainImages.size());

            // create framebuffers
            for (int i = 0; i < swapchainImages.size(); i++)
            {
                vk::Image &image = swapchainImages[i]; // prelink images
                std::array<vk::ImageView, 2> views;    // predeclare views

                // depth view
                views[1] = depthImageView;

                // color view
                views[0] = device->logical.createImageView(vk::ImageViewCreateInfo(
                    vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D,
                    formats.colorFormat, vk::ComponentMapping(),
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0,
                        1)));

                // create framebuffers
                swapchainBuffers[i].frameBuffer =
                    device->logical.createFramebuffer(vk::FramebufferCreateInfo(
                        vk::FramebufferCreateFlags(), renderpass, views.size(),
                        views.data(), applicationWindow.surfaceSize.width,
                        applicationWindow.surfaceSize.height, 1));

                // create semaphore
                swapchainBuffers[i].semaphore =
                    device->logical.createSemaphore(vk::SemaphoreCreateInfo());
                swapchainBuffers[i].waitFence = device->logical.createFence(
                    vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
            }

            return swapchainBuffers;
        }

        // create swapchain template
        virtual vk::SwapchainKHR createSwapchain(DeviceQueueType &device,
            vk::SurfaceKHR &surface,
            SurfaceFormat &formats)
        {
            auto surfaceCapabilities = device->physical->getSurfaceCapabilitiesKHR(surface);
            auto surfacePresentModes = device->physical->getSurfacePresentModesKHR(surface);

            // check the surface width/height.
            if (!(surfaceCapabilities.currentExtent.width == -1 ||
                surfaceCapabilities.currentExtent.height == -1))
            {
                applicationWindow.surfaceSize = surfaceCapabilities.currentExtent;
            }

            // get supported present mode, but prefer mailBox
            auto presentMode = vk::PresentModeKHR::eImmediate;
            std::vector<vk::PresentModeKHR> priorityModes = {
                vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifoRelaxed,
                vk::PresentModeKHR::eFifo };
            for (auto &pm : priorityModes)
            {
                if (presentMode != vk::PresentModeKHR::eImmediate)
                    break;
                for (auto &sfm : surfacePresentModes)
                {
                    if (pm == sfm)
                    {
                        presentMode = pm;
                        break;
                    }
                }
            }

            // swapchain info
            auto swapchainCreateInfo = vk::SwapchainCreateInfoKHR();
            swapchainCreateInfo.surface = surface;
            swapchainCreateInfo.minImageCount = glm::min(surfaceCapabilities.maxImageCount, 3u);
            swapchainCreateInfo.imageFormat = formats.colorFormat;
            swapchainCreateInfo.imageColorSpace = formats.colorSpace;
            swapchainCreateInfo.imageExtent = applicationWindow.surfaceSize;
            swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
            swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
            swapchainCreateInfo.queueFamilyIndexCount = 1;
            swapchainCreateInfo.pQueueFamilyIndices = &device->queues[1]->familyIndex;
            swapchainCreateInfo.preTransform =
                vk::SurfaceTransformFlagBitsKHR::eIdentity;
            swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
            swapchainCreateInfo.presentMode = presentMode;
            swapchainCreateInfo.clipped = true;

            // create swapchain
            return device->logical.createSwapchainKHR(swapchainCreateInfo);
        }

        virtual void initVulkan(const int32_t &argc, const char **argv,
            GLFWwindow *wind) = 0;
        virtual void mainLoop() = 0;

    public:
        virtual void run(const int32_t &argc, const char **argv, GLFWwindow *wind)
        {
            initVulkan(argc, argv, wind);
            mainLoop();
        }
    };

}; // namespace NSM