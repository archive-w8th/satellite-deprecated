#pragma once

#include "./vkutils/vkStructures.hpp"
#include "./vkutils/vkUtils.hpp"

namespace NSM {
    class ApplicationBase {
    protected:

        struct SurfaceWindow {
            GLFWwindow* window;
            vk::SurfaceKHR surface;
            SurfaceFormat surfaceFormat;
            vk::Extent2D surfaceSize;
        } applicationWindow;

        // application binding
        vk::Instance instance;

        // cached Vulkan API data
        std::vector<DeviceQueueType> devices;

        // instance extensions
        std::vector<const char*> wantedExtensions = {
            VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_KHR_SURFACE_EXTENSION_NAME
        };

        // default device extensions
        std::vector<const char*> wantedDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
            VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME,
            VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME,
            VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
            VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME,
            VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
            VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,

            // AMD extensions
            VK_AMD_GPU_SHADER_HALF_FLOAT_EXTENSION_NAME,
            VK_AMD_GPU_SHADER_INT16_EXTENSION_NAME,
            VK_AMD_GCN_SHADER_EXTENSION_NAME,
            VK_AMD_SHADER_TRINARY_MINMAX_EXTENSION_NAME,
            VK_AMD_TEXTURE_GATHER_BIAS_LOD_EXTENSION_NAME,
            VK_AMD_SHADER_BALLOT_EXTENSION_NAME,
            VK_AMD_SHADER_INFO_EXTENSION_NAME
        };

        // instance layers
        std::vector<const char*> wantedLayers = {
            "VK_LAYER_LUNARG_standard_validation",
            //"VK_LAYER_LUNARG_parameter_validation",
            //"VK_LAYER_LUNARG_core_validation",
            "VK_LAYER_AMD_switchable_graphics",
            "VK_LAYER_GOOGLE_threading",
            "VK_LAYER_NV_optimus"
        };

        // default device layers
        std::vector<const char*> wantedDeviceValidationLayers = {
            "VK_LAYER_AMD_switchable_graphics",
            "VK_LAYER_NV_optimus"
        };

        vk::Instance createInstance() {

            // get required extensions
            unsigned int glfwExtensionCount = 0;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            // add glfw extensions to list
            for (int i = 0; i < glfwExtensionCount; i++) {
                wantedExtensions.push_back(glfwExtensions[i]);
            }

            // get our needed extensions
            auto installedExtensions = vk::enumerateInstanceExtensionProperties();
            auto extensions = std::vector<const char*>();
            for (auto &w : wantedExtensions) {
                for (auto &i : installedExtensions) {
                    if (std::string(i.extensionName).compare(w) == 0) {
                        extensions.emplace_back(w);
                        break;
                    }
                }
            }

            // get validation layers
            auto installedLayers = vk::enumerateInstanceLayerProperties();
            auto layers = std::vector<const char*>();
            for (auto &w : wantedLayers) {
                for (auto &i : installedLayers) {
                    if (std::string(i.layerName).compare(w) == 0) {
                        layers.emplace_back(w);
                        break;
                    }
                }
            }

            // app info
            vk::ApplicationInfo appinfo;
            appinfo.pApplicationName = "VKTest";
            appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appinfo.apiVersion = VK_MAKE_VERSION(1, 0, 68);

            // create instance info
            vk::InstanceCreateInfo cinstanceinfo;
            cinstanceinfo.pApplicationInfo = &appinfo;
            cinstanceinfo.enabledExtensionCount = extensions.size();
            cinstanceinfo.ppEnabledExtensionNames = extensions.data();
            cinstanceinfo.enabledLayerCount = layers.size();
            cinstanceinfo.ppEnabledLayerNames = layers.data();

            // create instance
            instance = vk::createInstance(cinstanceinfo);
            //physicalDevices = instance.enumeratePhysicalDevices();

            // debug log 
            //instance.createDebugReportCallbackEXT(vk::DebugReportCallbackCreateInfoEXT().setPfnCallback(debugCallback));

            // get physical device for application
            return instance;
        };

        virtual DeviceQueueType createDevice(vk::PhysicalDevice &gpu) {
            auto gpuProps = gpu.getProperties();
            auto gpuMemoryProps = gpu.getMemoryProperties();

            // use extensions
            auto deviceExtensions = std::vector<const char*>();
            auto gpuExtensions = gpu.enumerateDeviceExtensionProperties();
            for (auto &w : wantedDeviceExtensions) {
                for (auto &i : gpuExtensions) {
                    if (std::string(i.extensionName).compare(w) == 0) {
                        deviceExtensions.emplace_back(w);
                        break;
                    }
                }
            }

            // use layers
            auto layers = std::vector<const char *>();
            auto deviceValidationLayers = std::vector<const char*>();
            auto gpuLayers = gpu.enumerateDeviceLayerProperties();
            for (auto &w : wantedLayers) {
                for (auto &i : gpuLayers) {
                    if (std::string(i.layerName).compare(w) == 0) {
                        layers.emplace_back(w);
                        break;
                    }
                }
            }

            // get features and queue family properties
            auto gpuFeatures = gpu.getFeatures();
            auto gpuQueueProps = gpu.getQueueFamilyProperties();

            // search graphics supported queue family
            std::vector<DevQueueType> queues;

            float priority = 0.0f;
            uint32_t computeFamilyIndex = 0, graphicsFamilyIndex = 0;
            auto queueCreateInfos = std::vector<vk::DeviceQueueCreateInfo>();

            // 
            for (auto& queuefamily : gpuQueueProps) {
                if (queuefamily.queueFlags & (vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eGraphics)) { // compute/graphics queue
                    queueCreateInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), computeFamilyIndex, 1, &priority));
                    queues.push_back(std::make_shared<DevQueue>(DevQueue{ computeFamilyIndex, vk::Queue{} }));
                    break;
                }
                computeFamilyIndex++;
            }

            // 
            for (auto& queuefamily : gpuQueueProps) {
                if (queuefamily.queueFlags & (vk::QueueFlagBits::eGraphics)) { // graphics/presentation queue
                    if (graphicsFamilyIndex != computeFamilyIndex) queueCreateInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), graphicsFamilyIndex, 1, &priority));
                    queues.push_back(std::make_shared<DevQueue>(DevQueue{ graphicsFamilyIndex, vk::Queue{} }));
                    break;
                }
                graphicsFamilyIndex++;
            }

            // assign presentation 
            if (graphicsFamilyIndex == computeFamilyIndex) queues[1] = queues[0];



            // pre-declare logical device 
            auto deviceQueuePtr = std::shared_ptr<DeviceQueue>(new DeviceQueue);

            // if have supported queue family, then use this device
            if (queueCreateInfos.size() > 0) {
                deviceQueuePtr->physical = gpu;
                deviceQueuePtr->logical = gpu.createDevice(
                    vk::DeviceCreateInfo(
                        vk::DeviceCreateFlags(),
                        queueCreateInfos.size(),
                        queueCreateInfos.data(),
                        deviceValidationLayers.size(),
                        deviceValidationLayers.data(),
                        deviceExtensions.size(),
                        deviceExtensions.data(),
                        &gpuFeatures)
                );

                for (int i = 0; i < queues.size();i++) { queues[i]->queue = deviceQueuePtr->logical.getQueue(queues[i]->familyIndex, 0); }
                deviceQueuePtr->queues = queues;
                deviceQueuePtr->mainQueue = deviceQueuePtr->queues[0];

                // add device and use this device
                devices.push_back(deviceQueuePtr);

                // create semaphores
                deviceQueuePtr->wsemaphore = deviceQueuePtr->logical.createSemaphore(vk::SemaphoreCreateInfo());
                deviceQueuePtr->commandPool = createCommandPool(deviceQueuePtr);


                // create allocator
                VmaAllocatorCreateInfo allocatorInfo = {};
                allocatorInfo.physicalDevice = deviceQueuePtr->physical;
                allocatorInfo.device = deviceQueuePtr->logical;
                vmaCreateAllocator(&allocatorInfo, &deviceQueuePtr->allocator);

                // pool sizes, and create descriptor pool 
                std::vector<vk::DescriptorPoolSize> psizes = {
                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 64),
                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 16),
                    vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 16),
                    vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 128),
                    vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 16)
                };
                deviceQueuePtr->descriptorPool = deviceQueuePtr->logical.createDescriptorPool(vk::DescriptorPoolCreateInfo().setPPoolSizes(psizes.data()).setPoolSizeCount(psizes.size()).setMaxSets(16));
                deviceQueuePtr->fence = createFence(deviceQueuePtr, false);
                deviceQueuePtr->initialized = true;
            }

            return std::move(deviceQueuePtr);
        }

        // create window and surface for this application (multi-window not supported)
        virtual SurfaceWindow& createWindowSurface(GLFWwindow * window, uint32_t WIDTH, uint32_t HEIGHT, std::string title = "TestApp") {
            applicationWindow.window = window;
            applicationWindow.surfaceSize = { WIDTH, HEIGHT };
            auto vksurface = VkSurfaceKHR();
            glfwCreateWindowSurface(instance, applicationWindow.window, nullptr, &vksurface);
            applicationWindow.surface = vk::SurfaceKHR(vksurface);
            return applicationWindow;
        }

        // create window and surface for this application (multi-window not supported)
        virtual SurfaceWindow& createWindowSurface(uint32_t WIDTH, uint32_t HEIGHT, std::string title = "TestApp") {
            applicationWindow.window = glfwCreateWindow(WIDTH, HEIGHT, title.c_str(), nullptr, nullptr);
            applicationWindow.surfaceSize = { WIDTH, HEIGHT };
            auto vksurface = VkSurfaceKHR();
            glfwCreateWindowSurface(instance, applicationWindow.window, nullptr, &vksurface);
            applicationWindow.surface = vk::SurfaceKHR(vksurface);
            return applicationWindow;
        }

        virtual SurfaceFormat getSurfaceFormat(vk::SurfaceKHR& surface, vk::PhysicalDevice &gpu) {
            auto surfaceFormats = gpu.getSurfaceFormatsKHR(surface);


            const std::vector<vk::Format> preferredFormats = { vk::Format::eA2R10G10B10UnormPack32, vk::Format::eA2B10G10R10UnormPack32, vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8A8Unorm };
            vk::Format surfaceColorFormat = surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined ? vk::Format::eR8G8B8A8Unorm : surfaceFormats[0].format;

            // search preferred surface format support
            bool surfaceFormatFound = false;
            uint32_t surfaceFormatID = 0;
            for (int i = 0; i < preferredFormats.size(); i++) {
                if (surfaceFormatFound) break;
                for (int f = 0; f < surfaceFormats.size(); f++) {
                    if (surfaceFormats[f].format == preferredFormats[i]) {
                        surfaceFormatFound = true; surfaceFormatID = f; break;
                    }
                }
            }

            // get supported color format
            surfaceColorFormat = surfaceFormats[surfaceFormatID].format;
            vk::ColorSpaceKHR surfaceColorSpace = surfaceFormats[surfaceFormatID].colorSpace;

            // get format properties?
            auto formatProperties = gpu.getFormatProperties(surfaceColorFormat);

            // only if these depth formats
            std::vector<vk::Format> depthFormats = {
                vk::Format::eD32SfloatS8Uint,
                vk::Format::eD32Sfloat,
                vk::Format::eD24UnormS8Uint,
                vk::Format::eD16UnormS8Uint,
                vk::Format::eD16Unorm
            };

            // choice supported depth format
            vk::Format surfaceDepthFormat = depthFormats[0];
            for (auto& format : depthFormats) {
                auto depthFormatProperties = gpu.getFormatProperties(format);
                if (depthFormatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
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

        virtual vk::RenderPass createRenderpass(DeviceQueueType& device, SurfaceFormat &formats) {
            // attachments 
            std::vector<vk::AttachmentDescription> attachmentDescriptions = {

                vk::AttachmentDescription()
                    .setFormat(formats.colorFormat).setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(vk::AttachmentLoadOp::eLoad).setStoreOp(vk::AttachmentStoreOp::eStore)
                    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare).setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setInitialLayout(vk::ImageLayout::eUndefined).setFinalLayout(vk::ImageLayout::ePresentSrcKHR),

                vk::AttachmentDescription()
                    .setFormat(formats.depthFormat).setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(vk::AttachmentLoadOp::eClear).setStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare).setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setInitialLayout(vk::ImageLayout::eUndefined).setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)

            };

            // attachments references
            std::vector<vk::AttachmentReference> colorReferences = { vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal) };
            std::vector<vk::AttachmentReference> depthReferences = { vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal) };

            // subpasses desc
            std::vector<vk::SubpassDescription> subpasses = {
                vk::SubpassDescription()
                .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                .setPColorAttachments(colorReferences.data()).setColorAttachmentCount(colorReferences.size())
                .setPDepthStencilAttachment(depthReferences.data())
            };

            // dependency
            std::vector<vk::SubpassDependency> dependencies = {
                vk::SubpassDependency().setDependencyFlags(vk::DependencyFlagBits::eByRegion)
                    .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                    .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eBottomOfPipe | vk::PipelineStageFlagBits::eTransfer)
                    .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)

                    .setDstSubpass(0)
                    .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                    .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite),

                vk::SubpassDependency().setDependencyFlags(vk::DependencyFlagBits::eByRegion)
                    .setSrcSubpass(0)
                    .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                    .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)

                    .setDstSubpass(VK_SUBPASS_EXTERNAL)
                    .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eTopOfPipe | vk::PipelineStageFlagBits::eTransfer)
                    .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
                    
            };

            // create renderpass
            return device->logical.createRenderPass(vk::RenderPassCreateInfo(
                vk::RenderPassCreateFlags(),
                attachmentDescriptions.size(),
                attachmentDescriptions.data(),
                subpasses.size(),
                subpasses.data(),
                dependencies.size(),
                dependencies.data()
            ));
        }

        // update swapchain framebuffer
        virtual void updateSwapchainFramebuffer(DeviceQueueType& device, vk::SwapchainKHR& swapchain, vk::RenderPass &renderpass, SurfaceFormat& formats, std::vector<Framebuffer>& swapchainBuffers) {
            // The swapchain handles allocating frame images.
            auto swapchainImages = device->logical.getSwapchainImagesKHR(swapchain);
            auto gpuMemoryProps = device->physical.getMemoryProperties();

            // create depth image
            auto imageInfo = vk::ImageCreateInfo(
                vk::ImageCreateFlags(), vk::ImageType::e2D,
                formats.depthFormat, vk::Extent3D(applicationWindow.surfaceSize.width, applicationWindow.surfaceSize.height, 1), 1, 1, vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive,
                1, &device->queues[1]->familyIndex,
                vk::ImageLayout::eUndefined
            );

            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VmaAllocation _allocation;
            VkImage _image;
            vmaCreateImage(device->allocator, &(VkImageCreateInfo)imageInfo, &allocCreateInfo, &_image, &_allocation, nullptr); // allocators planned structs
            auto depthImage = vk::Image(_image);

            // create image viewer
            auto depthImageView = device->logical.createImageView(vk::ImageViewCreateInfo(
                vk::ImageViewCreateFlags(), depthImage,
                vk::ImageViewType::e2D, formats.depthFormat, vk::ComponentMapping(),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1)
            ));

            // create framebuffers
            for (int i = 0; i < swapchainImages.size(); i++) {
                vk::Image& image = swapchainImages[i]; // prelink images
                std::array<vk::ImageView, 2> views; // predeclare views
                views[1] = depthImageView;

                // color view
                views[0] = device->logical.createImageView(vk::ImageViewCreateInfo(
                    vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, formats.colorFormat, vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                ));

                // create framebuffers
                swapchainBuffers[i].frameBuffer = device->logical.createFramebuffer(vk::FramebufferCreateInfo(
                    vk::FramebufferCreateFlags(), renderpass, views.size(), views.data(), applicationWindow.surfaceSize.width, applicationWindow.surfaceSize.height, 1
                ));
            }
        }

        virtual std::vector<Framebuffer> createSwapchainFramebuffer(DeviceQueueType& device, vk::SwapchainKHR& swapchain, vk::RenderPass &renderpass, SurfaceFormat& formats) {
            // The swapchain handles allocating frame images.
            auto swapchainImages = device->logical.getSwapchainImagesKHR(swapchain);
            auto gpuMemoryProps = device->physical.getMemoryProperties();

            // create depth image
            auto imageInfo = vk::ImageCreateInfo(
                vk::ImageCreateFlags(), vk::ImageType::e2D,
                formats.depthFormat, vk::Extent3D(applicationWindow.surfaceSize.width, applicationWindow.surfaceSize.height, 1), 1, 1, vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive,
                1, &device->queues[1]->familyIndex,
                vk::ImageLayout::eUndefined
            );

            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VmaAllocation _allocation;
            VkImage _image;
            vmaCreateImage(device->allocator, &(VkImageCreateInfo)imageInfo, &allocCreateInfo, &_image, &_allocation, nullptr); // allocators planned structs
            auto depthImage = vk::Image(_image);

            // create image viewer
            auto depthImageView = device->logical.createImageView(vk::ImageViewCreateInfo(
                vk::ImageViewCreateFlags(), depthImage,
                vk::ImageViewType::e2D, formats.depthFormat, vk::ComponentMapping(),
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1)
            ));

            // framebuffers vector
            std::vector<Framebuffer> swapchainBuffers;
            swapchainBuffers.resize(swapchainImages.size());

            // create framebuffers
            for (int i = 0; i < swapchainImages.size(); i++) {
                vk::Image& image = swapchainImages[i]; // prelink images
                std::array<vk::ImageView, 2> views; // predeclare views

                // depth view
                views[1] = depthImageView;

                // color view
                views[0] = device->logical.createImageView(vk::ImageViewCreateInfo(
                    vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, formats.colorFormat, vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                ));

                // create framebuffers
                swapchainBuffers[i].frameBuffer = device->logical.createFramebuffer(vk::FramebufferCreateInfo(
                    vk::FramebufferCreateFlags(), renderpass, views.size(), views.data(), applicationWindow.surfaceSize.width, applicationWindow.surfaceSize.height, 1
                ));

                // create semaphore
                swapchainBuffers[i].semaphore = device->logical.createSemaphore(vk::SemaphoreCreateInfo());
                swapchainBuffers[i].waitFence = device->logical.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
            }

            return swapchainBuffers;
        }

        // create swapchain template
        virtual vk::SwapchainKHR createSwapchain(DeviceQueueType& device, vk::SurfaceKHR& surface, SurfaceFormat &formats) {
            auto surfaceCapabilities = device->physical.getSurfaceCapabilitiesKHR(surface);
            auto surfacePresentModes = device->physical.getSurfacePresentModesKHR(surface);

            // check the surface width/height.
            if (!(surfaceCapabilities.currentExtent.width == -1 || surfaceCapabilities.currentExtent.height == -1)) {
                applicationWindow.surfaceSize = surfaceCapabilities.currentExtent;
            }

            // get supported present mode, but prefer mailBox
            auto presentMode = vk::PresentModeKHR::eImmediate;
            for (auto& pm : surfacePresentModes) {
                if ((pm == vk::PresentModeKHR::eMailbox || pm == vk::PresentModeKHR::eFifo || pm == vk::PresentModeKHR::eFifoRelaxed || pm == vk::PresentModeKHR::eImmediate) && device->physical.getSurfaceSupportKHR(device->queues[1]->familyIndex, surface)) {
                    presentMode = pm; break;
                }
            }

            // swapchain info
            auto swapchainCreateInfo = vk::SwapchainCreateInfoKHR();
            swapchainCreateInfo.surface = surface;
            swapchainCreateInfo.minImageCount = std::min(surfaceCapabilities.maxImageCount, 3u);
            swapchainCreateInfo.imageFormat = formats.colorFormat;
            swapchainCreateInfo.imageColorSpace = formats.colorSpace;
            swapchainCreateInfo.imageExtent = applicationWindow.surfaceSize;
            swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
            swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
            swapchainCreateInfo.queueFamilyIndexCount = 1;
            swapchainCreateInfo.pQueueFamilyIndices = &device->queues[1]->familyIndex;
            swapchainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
            swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
            swapchainCreateInfo.presentMode = presentMode;
            swapchainCreateInfo.clipped = true;

            // create swapchain
            return device->logical.createSwapchainKHR(swapchainCreateInfo);
        }

        virtual void initVulkan(const int32_t& argc, const char ** argv, GLFWwindow * wind) = 0;
        virtual void mainLoop() = 0;

    public:
        virtual void run(const int32_t& argc, const char ** argv, GLFWwindow * wind) {
            initVulkan(argc, argv, wind);
            mainLoop();
        }

    };

};