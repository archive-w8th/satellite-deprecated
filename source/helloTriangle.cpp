#define NSM VKAPP

#include "marble/application.hpp"

namespace NSM {

    class HelloTriangleApplication : public ApplicationBase {
    protected:

        // application window title
        const std::string title = "PUKAN_API";

        // uniform data
        struct UniformAppData {
            glm::mat4 projectionMatrix;
            glm::mat4 modelMatrix;
            glm::mat4 viewMatrix;
        } uboVS;

        // vertex buffer structure
        struct Vertex {
            float position[3];
            float color[3];
        };


        // vertex and uniforms 
        VertexLayout vertexLayout;
        VertexBuffer vertices;
        IndexBuffer indices;
        UniformBuffer uniformDataVS;

        // declared pipelines for application
        vk::Pipeline trianglePipeline;
        vk::Pipeline computePipeline;

        // default width and height of application
        const int WIDTH = 800;
        const int HEIGHT = 600;

        // instance extensions
        std::vector<const char*> wantedExtensions = {
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_KHR_SURFACE_EXTENSION_NAME
        };

        // default device extensions
        std::vector<const char*> wantedDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_DEBUG_MARKER_EXTENSION_NAME
        };

        // instance layers
        std::vector<const char*> wantedLayers = {
            "VK_LAYER_LUNARG_standard_validation",
            "VK_LAYER_RENDERDOC_Capture"
        };

        // default device layers
        std::vector<const char*> wantedDeviceValidationLayers = {
            "VK_LAYER_LUNARG_standard_validation",
            "VK_LAYER_RENDERDOC_Capture"
        };

        virtual void initVulkan() override {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);



            // create vulkan instance
            auto instance = createInstance();

            // get physical devices
            auto physicalDevices = instance.enumeratePhysicalDevices();
            if (physicalDevices.size() < 0) {
                glfwTerminate();
                std::cerr << "Vulkan does not supported, or driver broken." << std::endl;
                exit(0);
            }

            // choice device
            auto& gpu = physicalDevices[0];

            // create surface
            applicationWindow = createWindowSurface(WIDTH, HEIGHT, title);

            // get surface format from physical device
            applicationWindow.surfaceFormat = getSurfaceFormat(applicationWindow.surface, gpu);

            // create basic Vulkan objects
            auto deviceQueue = createDevice(gpu); // create default graphical device
            auto renderpass = createRenderpass(deviceQueue, applicationWindow.surfaceFormat);
            auto swapchain = createSwapchain(deviceQueue, applicationWindow.surface, applicationWindow.surfaceFormat);
            auto framebuffers = createSwapchainFramebuffer(deviceQueue, swapchain, renderpass, applicationWindow.surfaceFormat);

            // create command buffers per framebuffers
            for (int i = 0; i < framebuffers.size(); i++) {
                framebuffers[i].commandBuffer = createCommandBuffer(deviceQueue);
                framebuffers[i].waitFence = createFence(deviceQueue);
            }




            // define descriptor pool sizes
            std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
                vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
                vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 2),
                vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1)
            };

            // descriptor set bindings
            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr), // binding 0 of uniform
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eSampledImage, 2, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr),
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr)
            };

            // layouts of descriptor sets 
            std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {
                deviceQueue.logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(1)),
                deviceQueue.logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data() + 1).setBindingCount(2))
            };

            // create descriptor sets and pool
            auto descriptorPool = deviceQueue.logical.createDescriptorPool(
                vk::DescriptorPoolCreateInfo()
                .setPPoolSizes(descriptorPoolSizes.data())
                .setPoolSizeCount(descriptorPoolSizes.size())
                .setMaxSets(2)
            );

            // descriptor sets (where will writing binding)
            auto descriptorSets = deviceQueue.logical.allocateDescriptorSets(
                vk::DescriptorSetAllocateInfo()
                .setDescriptorPool(descriptorPool)
                .setDescriptorSetCount(descriptorSetLayouts.size())
                .setPSetLayouts(descriptorSetLayouts.data())
            );



            // vertex data
            std::vector<Vertex> vertexBuffer = {
                { { 1.0f,  1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },
                { { -1.0f,  1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
                { { 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
            };
            uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

            // index data
            std::vector<uint32_t> indexBuffer = { 0, 1, 2 };
            indices.count = static_cast<uint32_t>(indexBuffer.size());
            uint32_t indexBufferSize = indices.count * sizeof(uint32_t);



            // staging buffers
            struct {
                Buffer vertices;
                Buffer indices;
            } stagingBuffers;

            // staging vertex and indice buffer
            stagingBuffers.vertices = createBuffer(deviceQueue, vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            stagingBuffers.indices = createBuffer(deviceQueue, indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

            // fill staging buffers
            bufferSubData(stagingBuffers.vertices, vertexBuffer, 0);
            bufferSubData(stagingBuffers.indices, indexBuffer, 0);

            // create vertex and indice buffer
            vertices.binding = 0;
            vertices.buffer = createBuffer(deviceQueue, vertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
            indices.buffer = createBuffer(deviceQueue, indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);

            {
                // copy staging buffers
                vk::CommandBuffer copyCmd = getCommandBuffer(deviceQueue, true);
                memoryCopyCmd(copyCmd, stagingBuffers.vertices, vertices.buffer, { 0, 0, vertexBufferSize });
                memoryCopyCmd(copyCmd, stagingBuffers.indices, indices.buffer, { 0, 0, indexBufferSize });

                // execute command and asynchronous destroy staging buffers
                flushCommandBuffer(deviceQueue, copyCmd, [&]() {
                    destroyBuffer(stagingBuffers.vertices);
                    destroyBuffer(stagingBuffers.indices);
                });
            }

            // resize attributes and bindings
            vertexLayout.inputAttributes.resize(2);
            vertexLayout.inputBindings.resize(1);

            {
                // vertex buffer (binding 0)
                vertexLayout.inputBindings[0].binding = 0;
                vertexLayout.inputBindings[0].stride = sizeof(Vertex);
                vertexLayout.inputBindings[0].inputRate = vk::VertexInputRate::eVertex;
            }

            {
                // vertex attribute (location 0)
                vertexLayout.inputAttributes[0].binding = 0;
                vertexLayout.inputAttributes[0].location = 0;
                vertexLayout.inputAttributes[0].format = vk::Format::eR32G32B32Sfloat;
                vertexLayout.inputAttributes[0].offset = offsetof(Vertex, position);
            }

            {
                // color attribute (location 1)
                vertexLayout.inputAttributes[1].binding = 0;
                vertexLayout.inputAttributes[1].location = 1;
                vertexLayout.inputAttributes[1].format = vk::Format::eR32G32B32Sfloat;
                vertexLayout.inputAttributes[1].offset = offsetof(Vertex, color);
            }



            // uniform
            {
                // create uniform buffer
                uniformDataVS.staging = createBuffer(deviceQueue, sizeof(uboVS), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                uniformDataVS.buffer = createBuffer(deviceQueue, sizeof(uboVS), vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst);

                // create uniform buffer range description
                uniformDataVS.descriptor.buffer = uniformDataVS.buffer.buffer;
                uniformDataVS.descriptor.offset = 0;
                uniformDataVS.descriptor.range = sizeof(uboVS);

                // update uniform buffer
                {
                    float zoom = -2.5f;
                    auto rotation = glm::vec3();

                    // update matrices itself
                    uboVS.projectionMatrix = glm::perspective(glm::radians(60.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 256.0f);
                    uboVS.viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));
                    uboVS.modelMatrix = glm::mat4();
                    uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                    uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                    uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

                    // fill buffer data
                    bufferSubData(uniformDataVS.staging, std::vector<UniformAppData>{ uboVS }, 0);
                    copyMemoryProxy<Buffer&, Buffer&, vk::BufferCopy>(deviceQueue, uniformDataVS.staging, uniformDataVS.buffer, { 0, 0, sizeof(uboVS) }, true);
                }

                // bind uniform buffer to descriptor set ( binding 0 of descriptor set )
                deviceQueue.logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet()
                        .setDstSet(descriptorSets[0])
                        .setDstBinding(0)
                        .setDstArrayElement(0)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                        .setPBufferInfo(&uniformDataVS.descriptor)
                }, nullptr);
            }




            // test texture
            {
                // create texture
                auto texture = createTexture(deviceQueue, vk::ImageType::e2D, vk::ImageViewType::e2D, { 2, 2, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat, 1, vk::MemoryPropertyFlagBits::eDeviceLocal);
                auto tstage = createBuffer(deviceQueue, 4 * sizeof(glm::vec4), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                bufferSubData(tstage, std::vector<glm::vec4>({ glm::vec4(1.f, 0.0f, 0.0f, 1.f), glm::vec4(0.f, 0.0f, 1.0f, 1.f), glm::vec4(0.f, 1.f, 0.0f, 1.f), glm::vec4(0.f, 0.0f, 0.0f, 1.f) }));
                {
                    auto bufferImageCopy = vk::BufferImageCopy()
                        .setImageExtent({ 2, 2, 1 })
                        .setImageOffset({ 0, 0, 0 })
                        .setBufferOffset(0)
                        .setBufferRowLength(2)
                        .setBufferImageHeight(2)
                        .setImageSubresource(texture.subresourceLayers);

                    copyMemoryProxy<Buffer&, Texture&, vk::BufferImageCopy>(deviceQueue, tstage, texture, bufferImageCopy, [&]() {
                        destroyBuffer(tstage);
                    });
                }

                // desc texture texture
                vk::DescriptorImageInfo imageDesc;
                imageDesc.imageLayout = texture.layout;
                imageDesc.imageView = texture.view;


                // create texture 2
                auto texture2 = createTexture(deviceQueue, vk::ImageType::e2D, vk::ImageViewType::e2D, { 2, 2, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat, 1, vk::MemoryPropertyFlagBits::eDeviceLocal);
                tstage = createBuffer(deviceQueue, 4 * sizeof(glm::vec4), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                bufferSubData(tstage, std::vector<glm::vec4>({ glm::vec4(1.0f, 1.0f, 0.0f, 1.f), glm::vec4(0.0f, 1.0f, 1.0f, 1.f), glm::vec4(1.0f, 0.0f, 1.0f, 1.f), glm::vec4(1.0f, 1.0f, 1.0f, 1.f) }));
                {
                    auto bufferImageCopy = vk::BufferImageCopy()
                        .setImageExtent({ 2, 2, 1 })
                        .setImageOffset({ 0, 0, 0 })
                        .setBufferOffset(0)
                        .setBufferRowLength(2)
                        .setBufferImageHeight(2)
                        .setImageSubresource(texture2.subresourceLayers);

                    copyMemoryProxy<Buffer&, Texture&, vk::BufferImageCopy>(deviceQueue, tstage, texture2, bufferImageCopy, [&]() {
                        destroyBuffer(tstage);
                    });
                }

                // desc texture texture
                vk::DescriptorImageInfo imageDesc2;
                imageDesc2.imageLayout = texture2.layout;
                imageDesc2.imageView = texture2.view;


                // update descriptors
                deviceQueue.logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet()
                        .setDstSet(descriptorSets[1])
                        .setDstBinding(0)
                        .setDstArrayElement(0)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eSampledImage)
                        .setPImageInfo(&imageDesc),
                        vk::WriteDescriptorSet()
                        .setDstSet(descriptorSets[1])
                        .setDstBinding(0)
                        .setDstArrayElement(1)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eSampledImage)
                        .setPImageInfo(&imageDesc2)
                }, nullptr);
            }

            // test sampler
            {
                // create sampler
                vk::SamplerCreateInfo samplerInfo;
                samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
                samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
                samplerInfo.minFilter = vk::Filter::eLinear;
                samplerInfo.magFilter = vk::Filter::eLinear;
                samplerInfo.compareEnable = false;
                auto sampler = deviceQueue.logical.createSampler(samplerInfo);

                // use sampler in description set
                vk::DescriptorImageInfo samplerDesc;
                samplerDesc.sampler = sampler;
                deviceQueue.logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet()
                        .setDstSet(descriptorSets[1])
                        .setDstBinding(1)
                        .setDstArrayElement(0)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eSampler)
                        .setPImageInfo(&samplerDesc)
                }, nullptr);
            }







            // surface sizing
            auto renderArea = vk::Rect2D(vk::Offset2D(), applicationWindow.surfaceSize);
            auto viewport = vk::Viewport(0.0f, 0.0f, applicationWindow.surfaceSize.width, applicationWindow.surfaceSize.height, 0, 1.0f);
            std::vector<vk::Viewport> viewports = { viewport };
            std::vector<vk::Rect2D> scissors = { renderArea };

            // pipeline layout and cache
            auto pipelineLayout = deviceQueue.logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(descriptorSetLayouts.data()).setSetLayoutCount(descriptorSetLayouts.size()));
            auto pipelineCache = deviceQueue.logical.createPipelineCache(vk::PipelineCacheCreateInfo());

            // create pipeline
            {
                // pipeline stages
                std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages = {
                    vk::PipelineShaderStageCreateInfo().setModule(loadAndCreateShaderModule(deviceQueue, "triangle.vert.spv")).setPName("main").setStage(vk::ShaderStageFlagBits::eVertex),
                    vk::PipelineShaderStageCreateInfo().setModule(loadAndCreateShaderModule(deviceQueue, "triangle.frag.spv")).setPName("main").setStage(vk::ShaderStageFlagBits::eFragment)
                };

                // vertex state
                auto pvi = vk::PipelineVertexInputStateCreateInfo()
                    .setPVertexBindingDescriptions(vertexLayout.inputBindings.data()).setVertexBindingDescriptionCount(vertexLayout.inputBindings.size())
                    .setPVertexAttributeDescriptions(vertexLayout.inputAttributes.data()).setVertexAttributeDescriptionCount(vertexLayout.inputAttributes.size());

                // tesselation state
                auto pt = vk::PipelineTessellationStateCreateInfo();

                // viewport and scissors state
                auto pv = vk::PipelineViewportStateCreateInfo()
                    .setPViewports(viewports.data()).setViewportCount(viewports.size())
                    .setPScissors(scissors.data()).setScissorCount(scissors.size());

                // input assembly
                auto pia = vk::PipelineInputAssemblyStateCreateInfo()
                    .setTopology(vk::PrimitiveTopology::eTriangleList);

                // rasterization state
                auto pr = vk::PipelineRasterizationStateCreateInfo()
                    .setDepthClampEnable(false)
                    .setRasterizerDiscardEnable(false)
                    .setPolygonMode(vk::PolygonMode::eFill)
                    .setCullMode(vk::CullModeFlagBits::eNone)
                    .setFrontFace(vk::FrontFace::eCounterClockwise)
                    .setDepthBiasEnable(false)
                    .setDepthBiasConstantFactor(0)
                    .setDepthBiasClamp(0)
                    .setDepthBiasSlopeFactor(0)
                    .setLineWidth(1.f);

                // multisampling
                auto pm = vk::PipelineMultisampleStateCreateInfo()
                    .setRasterizationSamples(vk::SampleCountFlagBits::e1);

                // depth stencil comparsion modes
                auto pds = vk::PipelineDepthStencilStateCreateInfo()
                    .setDepthTestEnable(true)
                    .setDepthWriteEnable(true)
                    .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
                    .setDepthBoundsTestEnable(false)
                    .setStencilTestEnable(false);

                // blend modes per framebuffer targets
                std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {
                    vk::PipelineColorBlendAttachmentState()
                    .setBlendEnable(false)
                    .setSrcColorBlendFactor(vk::BlendFactor::eZero).setDstColorBlendFactor(vk::BlendFactor::eOne).setColorBlendOp(vk::BlendOp::eAdd)
                    .setSrcAlphaBlendFactor(vk::BlendFactor::eZero).setDstAlphaBlendFactor(vk::BlendFactor::eOne).setAlphaBlendOp(vk::BlendOp::eAdd)
                    .setColorWriteMask(vk::ColorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA))
                };

                // blend state
                auto pbs = vk::PipelineColorBlendStateCreateInfo()
                    .setLogicOpEnable(false)
                    .setLogicOp(vk::LogicOp::eClear)
                    .setPAttachments(colorBlendAttachments.data())
                    .setAttachmentCount(colorBlendAttachments.size());

                // dynamic states
                std::vector<vk::DynamicState> dynamicStates = {
                    vk::DynamicState::eViewport,
                    vk::DynamicState::eScissor
                };

                // dynamic states
                auto pdy = vk::PipelineDynamicStateCreateInfo()
                    .setPDynamicStates(dynamicStates.data()).setDynamicStateCount(dynamicStates.size());

                // create graphics pipeline
                trianglePipeline = deviceQueue.logical.createGraphicsPipeline(pipelineCache,
                    vk::GraphicsPipelineCreateInfo()
                    .setPStages(pipelineShaderStages.data()).setStageCount(pipelineShaderStages.size())
                    //.setFlags(vk::PipelineCreateFlagBits::eDerivative)
                    .setPVertexInputState(&pvi)
                    .setPInputAssemblyState(&pia)
                    .setPViewportState(&pv)
                    .setPRasterizationState(&pr)
                    .setPDepthStencilState(&pds)
                    .setPColorBlendState(&pbs)
                    .setLayout(pipelineLayout)
                    .setRenderPass(renderpass)
                    .setBasePipelineIndex(0)
                    .setPMultisampleState(&pm)
                    .setPDynamicState(&pdy)
                    .setPTessellationState(&pt)
                );
            }

            // creete compute
            {
                computePipeline = createCompute(deviceQueue, "compute.comp.spv", pipelineLayout, pipelineCache);
            }






            // clear values
            std::vector<vk::ClearValue> clearValues = {
                vk::ClearColorValue(std::array<float,4>{0.2f, 0.2f, 0.2f, 1.0f}),
                vk::ClearDepthStencilValue(1.0f, 0)
            };

            // create command buffer per FBO
            for (int32_t i = 0; i < framebuffers.size(); ++i) {
                framebuffers[i].commandBuffer.begin(vk::CommandBufferBeginInfo());
                framebuffers[i].commandBuffer.beginRenderPass(
                    vk::RenderPassBeginInfo(renderpass, framebuffers[i].frameBuffer, renderArea, clearValues.size(), clearValues.data()),
                    vk::SubpassContents::eInline
                );
                framebuffers[i].commandBuffer.setViewport(0, viewports);
                framebuffers[i].commandBuffer.setScissor(0, scissors);
                framebuffers[i].commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, trianglePipeline);
                framebuffers[i].commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets, nullptr);
                framebuffers[i].commandBuffer.bindVertexBuffers(vertices.binding, 1, &vertices.buffer.buffer, &vertices.voffset);
                framebuffers[i].commandBuffer.bindIndexBuffer(indices.buffer.buffer, 0, indices.indexType);
                framebuffers[i].commandBuffer.drawIndexed(indices.count, 1, 0, 0, 1);
                framebuffers[i].commandBuffer.endRenderPass();
                framebuffers[i].commandBuffer.end();
            }

            // create graphics context
            GraphicsContext context;
            context.device = deviceQueue;
            context.swapchain = swapchain;
            context.framebuffers = framebuffers;
            context.pipeline = trianglePipeline;
            context.descriptorPool = descriptorPool;
            context.descriptorSets = descriptorSets;

            // context draw function
            context.draw = [context]() {
                auto& currentContext = context;

                // acquire next image where will rendered
                uint32_t currentBuffer = 0;
                currentContext.device.logical.acquireNextImageKHR(currentContext.swapchain, std::numeric_limits<uint64_t>::max(), currentContext.device.presentCompleteSemaphore, nullptr, &currentBuffer);

                // wait when this image will previously rendered
                currentContext.device.logical.waitForFences(1, &currentContext.framebuffers[currentBuffer].waitFence, true, std::numeric_limits<uint64_t>::max()); // wait when will ready rendering
                currentContext.device.logical.resetFences(1, &currentContext.framebuffers[currentBuffer].waitFence); // unsignal before next work

                                                                                                                     // pipeline stage flags
                vk::PipelineStageFlags kernelPipelineStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

                // submit rendering (and wait presentation in device)
                auto kernel = vk::SubmitInfo()
                    .setPWaitDstStageMask(&kernelPipelineStageFlags)
                    .setPCommandBuffers(&currentContext.framebuffers[currentBuffer].commandBuffer).setCommandBufferCount(1)
                    .setPWaitSemaphores(&currentContext.device.presentCompleteSemaphore).setWaitSemaphoreCount(1)
                    .setPSignalSemaphores(&currentContext.device.renderCompleteSemaphore).setSignalSemaphoreCount(1);
                currentContext.device.queue.submit(1, &kernel, currentContext.framebuffers[currentBuffer].waitFence);

                // present for displaying of this image
                currentContext.device.queue.presentKHR(vk::PresentInfoKHR(
                    1, &currentContext.device.renderCompleteSemaphore,
                    1, &currentContext.swapchain,
                    &currentBuffer, nullptr
                ));
            };

            // use this context
            currentContext = context;
        }

        virtual void mainLoop() override {
            uint32_t currentBuffer = 0;
            uint32_t imageIndex = 0;
            uint64_t frameCounter = 0;
            double frameTimer = 0.0;
            double fpsTimer = 0.0;
            double lastFPS = 0.0;

            // main loop
            while (!glfwWindowShouldClose(applicationWindow.window)) {
                glfwPollEvents();

                // get current time
                auto tStart = std::chrono::high_resolution_clock::now();

                // draw by graphic context draw function
                currentContext.draw();

                // calculate time of rendered
                frameCounter++;
                auto tEnd = std::chrono::high_resolution_clock::now();
                auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
                frameTimer = tDiff / 1000.0;

                // calculate FPS
                fpsTimer += tDiff;
                if (fpsTimer > 1000.0) {
                    std::string windowTitle = title + " - " + std::to_string(frameCounter) + " fps";
                    glfwSetWindowTitle(applicationWindow.window, windowTitle.c_str());

                    lastFPS = roundf(1.0 / frameTimer);
                    fpsTimer = 0.0;
                    frameCounter = 0;
                }
            }

            // correctly terminate application
            currentContext.device.logical.waitIdle(); // wait presentations or other actions
            glfwTerminate();
        }

    };
};

int main() {
    NSM::HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}