#pragma once





#include <functional>
#include <sstream>
#include <iomanip>

#include "args.hxx"
#include "imgui.h"

#include "satellite/rtengine.hpp"
#include "satellite/appBase.hpp"
#include "service/ambientIO.hpp"






#ifdef OPTIX_DENOISER_HACK
#include "optixu/optixpp.h"
using U_MEM_HANDLE = uint8_t * ;
#else
using U_MEM_HANDLE = uint8_t * ;
#endif




namespace SatelliteExample {

    using namespace ste;


    TextureType loadCubemap(std::string bgTexName, DeviceQueueType& device) {
        cil::CImg<float> image(bgTexName.c_str());
        uint32_t width = image.width(), height = image.height();
        image.channels(0, 3);
        image.permute_axes("cxyz");
        

        // create texture
        auto texture = createTexture(device, vk::ImageViewType::e2D, { width, height, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat, 1);
        auto tstage = createBuffer(device, image.size() * sizeof(float), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

        {
            auto command = getCommandBuffer(device, true);
            bufferSubData(command, tstage, (const uint8_t *)image.data(), image.size() * sizeof(float), 0);
            memoryCopyCmd(command, tstage, texture, vk::BufferImageCopy()
                .setImageExtent({ width, height, 1 })
                .setImageOffset({ 0, 0, 0 })
                .setBufferOffset(0)
                .setBufferRowLength(width)
                .setBufferImageHeight(height)
                .setImageSubresource(texture->subresourceLayers));
            flushCommandBuffer(device, command, [&]() { destroyBuffer(tstage); });
        }

        // create sampler for combined
        texture->descriptorInfo.sampler = device->logical.createSampler(vk::SamplerCreateInfo().setAddressModeU(vk::SamplerAddressMode::eRepeat).setAddressModeV(vk::SamplerAddressMode::eMirroredRepeat).setMinFilter(vk::Filter::eLinear).setMagFilter(vk::Filter::eLinear).setCompareEnable(false));;
        return texture;
    }


    class ControlMap {
    public:
        bool mouseleft = false;
        bool keys[12] = { false , false , false , false , false , false , false, false, false, false, false };

        static const int32_t kW = 0;
        static const int32_t kA = 1;
        static const int32_t kS = 2;
        static const int32_t kD = 3;
        static const int32_t kQ = 4;
        static const int32_t kE = 5;
        static const int32_t kSpc = 6;
        static const int32_t kSft = 7;
        static const int32_t kC = 8;
        static const int32_t kK = 9;
        static const int32_t kM = 10;
        static const int32_t kL = 11;
    };


    class CameraController {
        bool monteCarlo = true;

    public:
        glm::dvec3 dir = glm::normalize(glm::dvec3(0.0, -1.0, 0.01));
        glm::dvec3 eye = glm::dvec3(0.0, 4.0, 0.0);
        glm::dvec3 view = glm::dvec3(0.0, 0.0, 0.0);
        
        glm::dvec2 mposition;
        std::shared_ptr<rt::Pipeline> raysp;

        glm::dmat4 project() {
            view = eye + dir;
            return glm::lookAt(eye, view, glm::dvec3(0.0f, 1.0f, 0.0f));
        }

        void setRays(std::shared_ptr<rt::Pipeline>& r) {
            raysp = r;
        }

        void work(const glm::dvec2 &position, const double &diff, ControlMap& map) {
            glm::dmat4 viewm = project();
            glm::dmat4 unviewm = glm::inverse(viewm);
            glm::dvec3 ca = (viewm * glm::dvec4(eye, 1.0f)).xyz();
            glm::dvec3 vi = glm::normalize((glm::dvec4(dir, 0.0) * unviewm).xyz());
            bool isFocus = true;

            if (map.mouseleft && isFocus)
            {
                glm::dvec2 mpos = glm::dvec2(position) - mposition;
                double diffX = mpos.x, diffY = mpos.y;
                if (glm::abs(diffX) > 0.0) this->rotateX(vi, diffX);
                if (glm::abs(diffY) > 0.0) this->rotateY(vi, diffY);
                if (monteCarlo) raysp->clearSampling();
            }
            mposition = glm::dvec2(position);

            if (map.keys[ControlMap::kW] && isFocus)
            {
                this->forwardBackward(ca, -diff);
                if (monteCarlo) raysp->clearSampling();
            }

            if (map.keys[ControlMap::kS] && isFocus)
            {
                this->forwardBackward(ca, diff);
                if (monteCarlo) raysp->clearSampling();
            }

            if (map.keys[ControlMap::kA] && isFocus)
            {
                this->leftRight(ca, -diff);
                if (monteCarlo) raysp->clearSampling();
            }

            if (map.keys[ControlMap::kD] && isFocus)
            {
                this->leftRight(ca, diff);
                if (monteCarlo) raysp->clearSampling();
            }

            if ((map.keys[ControlMap::kE] || map.keys[ControlMap::kSpc]) && isFocus)
            {
                this->topBottom(ca, diff);
                if (monteCarlo) raysp->clearSampling();
            }

            if ((map.keys[ControlMap::kQ] || map.keys[ControlMap::kSft] || map.keys[ControlMap::kC]) && isFocus)
            {
                this->topBottom(ca, -diff);
                if (monteCarlo) raysp->clearSampling();
            }

            dir = glm::normalize((glm::dvec4(vi, 0.0) * viewm).xyz());
            eye = (unviewm * glm::dvec4(ca, 1.0f)).xyz();
            view = eye + dir;
        }

        void leftRight(glm::dvec3 &ca, const double &diff) {
            ca.x += diff / 100.0f;
        }
        void topBottom(glm::dvec3 &ca, const double &diff) {
            ca.y += diff / 100.0f;
        }
        void forwardBackward(glm::dvec3 &ca, const double &diff) {
            ca.z += diff / 100.0f;
        }
        void rotateY(glm::dvec3 &vi, const double &diff) {
            glm::dmat4 rot = glm::rotate(diff / float(raysp->getCanvasHeight()) / 0.5f, glm::dvec3(-1.0f, 0.0f, 0.0f));
            vi = (rot * glm::dvec4(vi, 1.0f)).xyz();
        }
        void rotateX(glm::dvec3 &vi, const double &diff) {
            glm::dmat4 rot = glm::rotate(diff / float(raysp->getCanvasHeight()) / 0.5f, glm::dvec3(0.0f, -1.0f, 0.0f));
            vi = (rot * glm::dvec4(vi, 1.0f)).xyz();
        }
    };

    class PathTracerApplication : public ApplicationBase {
    public:
        PathTracerApplication(const int32_t& argc, const char ** argv, GLFWwindow * wind) { };
        PathTracerApplication() { };

        void passKeyDown(const int32_t& key);
        void passKeyRelease(const int32_t& key);
        void mousePress(const int32_t& button);
        void mouseRelease(const int32_t& button);
        void mouseMove(const double& x, const double& y);

        void resize(const int32_t& width, const int32_t& height);
        void resizeBuffers(const int32_t& width, const int32_t& height);
        void saveHdr(std::string name = "");

        std::vector<Framebuffer>& getFramebuffers() {
            return currentContext->framebuffers;
        }

        virtual void init(DeviceQueueType& device, const int32_t& argc, const char ** argv) = 0;
        virtual void process() = 0;
        virtual void execute(const int32_t& argc, const char ** argv, GLFWwindow * wind);
        virtual void parseArguments(const int32_t& argc, const char ** argv) = 0;
        virtual void handleGUI() {};
        //virtual void updateSwapchains();

    protected:

        ControlMap kmap;

        // current context of application
        //std::shared_ptr<GuiRenderEngine> grengine;
        std::shared_ptr<GraphicsContext> currentContext;

        // application window title
        const std::string title = "Satellite";

        // declared pipelines for application
        vk::Pipeline trianglePipeline;
        vk::Pipeline computePipeline;

        // default width and height of application
        const double superSampling = 2.0; // super sampling (in high DPI may 1x sample)




        uint32_t gpuID = 0;
        std::string shaderPack = "shaders-spv";
        std::shared_ptr<rt::Pipeline> rays;


        double time = 0, diff = 0;
        glm::dvec2 mousepos = glm::dvec2(0.0);
        int32_t depth = 16;
        int32_t switch360key = false;
        int32_t img_counter = 0;
        uint32_t currentBuffer = 0;
        bool needToUpdate = false;

        // window and canvas sizing
        float windowScale = 1.0f;
        int32_t windowWidth = baseWidth, windowHeight = baseHeight;
        int32_t canvasWidth = baseWidth, canvasHeight = baseHeight;

        // base sizing (for buffers)
        int32_t baseWidth = 1, baseHeight = 1;


        BufferType memoryBufferToHost;
        BufferType memoryBufferFromHost;


#ifdef OPTIX_DENOISER_HACK
        optix::CommandList commandListWithDenoiser;
        optix::Context context = 0;
        optix::PostprocessingStage denoiserStage;
        optix::Buffer denoisedBuffer;
        optix::Buffer inputBuffer; // TODO: albedo, normals support
        optix::Buffer trainingDataBuffer; // training AI buffer

        optix::Buffer albedoBuffer;
        optix::Buffer normalBuffer;
#endif


        virtual void updateSwapchains() {
            if (needToUpdate) {
                needToUpdate = false;

                // update swapchain
                {
                    currentContext->device->logical.waitIdle();
                    currentContext->device->logical.destroySwapchainKHR(currentContext->swapchain);
                    currentContext->swapchain = createSwapchain(currentContext->device, applicationWindow.surface, applicationWindow.surfaceFormat);
                    updateSwapchainFramebuffer(currentContext->device, currentContext->swapchain, currentContext->renderpass, applicationWindow.surfaceFormat, currentContext->framebuffers);
                    currentBuffer = 0;
                }

                // resize renderer canvas
                this->resize(canvasWidth, canvasHeight);
                rays->clearSampling();

                // write descriptors for showing texture
                {
                    // create sampler
                    auto sampler = currentContext->device->logical.createSampler(vk::SamplerCreateInfo().setAddressModeU(vk::SamplerAddressMode::eClampToEdge).setAddressModeV(vk::SamplerAddressMode::eClampToEdge).setMagFilter(vk::Filter::eLinear).setMinFilter(vk::Filter::eLinear).setCompareEnable(false));
                    auto image = rays->getFilteredImage();

                    // update descriptors
                    currentContext->device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                        vk::WriteDescriptorSet()
                            .setDstSet(currentContext->descriptorSets[0])
                            .setDstBinding(0)
                            .setDstArrayElement(0)
                            .setDescriptorCount(1)
                            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                            .setPImageInfo(&vk::DescriptorImageInfo().setImageLayout(image->layout).setImageView(image->view).setSampler(sampler)),
                    }, nullptr);
                }

#ifdef OPTIX_DENOISER_HACK
                if (denoisedBuffer) denoisedBuffer->destroy();
                if (inputBuffer) inputBuffer->destroy();
                if (commandListWithDenoiser) commandListWithDenoiser->destroy();

                // create buffer for processing
                denoisedBuffer = context->createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT4, canvasWidth, canvasHeight);
                inputBuffer = context->createBuffer(RT_BUFFER_INPUT_OUTPUT, RT_FORMAT_FLOAT4, canvasWidth, canvasHeight);
                albedoBuffer = context->createBuffer(RT_BUFFER_INPUT_OUTPUT, RT_FORMAT_FLOAT4, canvasWidth, canvasHeight);
                normalBuffer = context->createBuffer(RT_BUFFER_INPUT_OUTPUT, RT_FORMAT_FLOAT4, canvasWidth, canvasHeight);

                denoiserStage->queryVariable("input_albedo_buffer")->set(albedoBuffer);
                denoiserStage->queryVariable("input_normal_buffer")->set(normalBuffer);
                denoiserStage->queryVariable("input_buffer")->set(inputBuffer);
                denoiserStage->queryVariable("output_buffer")->set(denoisedBuffer);

                // denoise command
                commandListWithDenoiser = context->createCommandList();
                commandListWithDenoiser->appendPostprocessingStage(denoiserStage, canvasWidth, canvasHeight);
                commandListWithDenoiser->finalize();
#endif

            }
        }

        virtual void initVulkan(const int32_t& argc, const char ** argv, GLFWwindow * wind) override {
            applicationWindow.window = wind;
            parseArguments(argc, argv);

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
            if (gpuID >= physicalDevices.size()) {
                gpuID = physicalDevices.size() - 1;
            }
            if (gpuID < 0 || gpuID == -1) gpuID = 0;
            auto& gpu = physicalDevices[gpuID];

            // create surface
            applicationWindow = createWindowSurface(wind, canvasWidth, canvasHeight, title);

            // get surface format from physical device
            applicationWindow.surfaceFormat = getSurfaceFormat(applicationWindow.surface, gpu);

            // create basic Vulkan objects
            auto deviceQueue = createDevice(gpu); // create default graphical device
            auto renderpass = createRenderpass(deviceQueue, applicationWindow.surfaceFormat);

            // create GUI rendering engine 
            //grengine = std::shared_ptr<GuiRenderEngine>(new GuiRenderEngine(deviceQueue, renderpass, shaderPack));





            // create dedicated buffer zones
            memoryBufferToHost = createBuffer(deviceQueue, 4096 * 4096 * sizeof(float), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_GPU_TO_CPU);
            memoryBufferFromHost = createBuffer(deviceQueue, 4096 * 4096 * sizeof(float), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);


            // init ray tracer
            rays = std::shared_ptr<rt::Pipeline>(new rt::Pipeline(deviceQueue, shaderPack));

            // resize buffers and canvas
            glfwGetWindowContentScale(wind, &windowScale, nullptr);
            this->resizeBuffers(baseWidth * superSampling, baseHeight * superSampling);
            this->resize(canvasWidth, canvasHeight);





#ifdef OPTIX_DENOISER_HACK
            context = optix::Context::create();

            // TODO support of albedo/color
            denoiserStage = context->createBuiltinPostProcessingStage("DLDenoiser");
            denoiserStage->declareVariable("blend")->setFloat(0.f);

            // create buffer for processing
            denoisedBuffer = context->createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT4, canvasWidth, canvasHeight);
            inputBuffer = context->createBuffer(RT_BUFFER_INPUT_OUTPUT, RT_FORMAT_FLOAT4, canvasWidth, canvasHeight);
            albedoBuffer = context->createBuffer(RT_BUFFER_INPUT_OUTPUT, RT_FORMAT_FLOAT4, canvasWidth, canvasHeight);
            normalBuffer = context->createBuffer(RT_BUFFER_INPUT_OUTPUT, RT_FORMAT_FLOAT4, canvasWidth, canvasHeight);

            // initial buffers
            denoiserStage->declareVariable("input_albedo_buffer")->set(albedoBuffer);
            denoiserStage->declareVariable("input_normal_buffer")->set(normalBuffer);
            denoiserStage->declareVariable("input_buffer")->set(inputBuffer);
            denoiserStage->declareVariable("output_buffer")->set(denoisedBuffer);
            
            // denoise command
            commandListWithDenoiser = context->createCommandList();
            commandListWithDenoiser->appendPostprocessingStage(denoiserStage, canvasWidth, canvasHeight);
            commandListWithDenoiser->finalize();


            
#endif




            {
                auto app = this;
                //auto geg = this->grengine;
                auto& cb = this->currentBuffer;
                auto& sfz = this->applicationWindow.surfaceSize;

                ambientIO::addMouseActionCallback([app](GLFWwindow* window, int32_t button, int32_t action, int32_t mods) {
                    if (app) {
                        if (action == GLFW_PRESS) app->mousePress(button);
                        if (action == GLFW_RELEASE) app->mouseRelease(button);
                    }
                });

                ambientIO::addKeyboardCallback([app](GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods) {
                    if (app) {
                        if (action == GLFW_PRESS) app->passKeyDown(key);
                        if (action == GLFW_RELEASE) app->passKeyRelease(key);
                    }
                });

                ambientIO::addMouseMoveCallback([app](GLFWwindow* window, double x, double y) {
                    if (app) app->mouseMove(x, y);
                });

                //ambientIO::addGuiDrawListCallback([geg, app, &cb, &sfz](ImDrawData* imData) {
                //    if (geg && app) geg->renderOn(app->getFramebuffers()[cb], sfz, imData);
                //});
            }

            // init or prerender data
            this->init(deviceQueue, argc, argv); // init ray tracers virtually

            // descriptor set bindings
            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = { vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr) };

            // layouts of descriptor sets 
            std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = { deviceQueue->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(1)) };

            // descriptor sets (where will writing binding)
            auto descriptorSets = deviceQueue->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(deviceQueue->descriptorPool).setDescriptorSetCount(descriptorSetLayouts.size()).setPSetLayouts(descriptorSetLayouts.data()));

            // pipeline layout and cache
            auto pipelineLayout = deviceQueue->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(descriptorSetLayouts.data()).setSetLayoutCount(descriptorSetLayouts.size()));
            auto pipelineCache = deviceQueue->logical.createPipelineCache(vk::PipelineCacheCreateInfo());

            // create pipeline
            {
                // pipeline stages
                std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages = {
                    vk::PipelineShaderStageCreateInfo().setModule(loadAndCreateShaderModule(deviceQueue, shaderPack + "/output/render.vert.spv")).setPName("main").setStage(vk::ShaderStageFlagBits::eVertex),
                    vk::PipelineShaderStageCreateInfo().setModule(loadAndCreateShaderModule(deviceQueue, shaderPack + "/output/render.frag.spv")).setPName("main").setStage(vk::ShaderStageFlagBits::eFragment)
                };

                // blend modes per framebuffer targets
                std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {
                    vk::PipelineColorBlendAttachmentState()
                    .setBlendEnable(true)
                    .setSrcColorBlendFactor(vk::BlendFactor::eOne).setDstColorBlendFactor(vk::BlendFactor::eZero).setColorBlendOp(vk::BlendOp::eAdd)
                    .setSrcAlphaBlendFactor(vk::BlendFactor::eOne).setDstAlphaBlendFactor(vk::BlendFactor::eZero).setAlphaBlendOp(vk::BlendOp::eAdd)
                    .setColorWriteMask(vk::ColorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA))
                };

                // dynamic states
                std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

                // create graphics pipeline
                trianglePipeline = deviceQueue->logical.createGraphicsPipeline(pipelineCache,
                    vk::GraphicsPipelineCreateInfo()
                    .setPStages(pipelineShaderStages.data()).setStageCount(pipelineShaderStages.size())
                    .setFlags(vk::PipelineCreateFlagBits::eAllowDerivatives)
                    .setPVertexInputState(&vk::PipelineVertexInputStateCreateInfo())
                    .setPInputAssemblyState(&vk::PipelineInputAssemblyStateCreateInfo().setTopology(vk::PrimitiveTopology::eTriangleStrip))
                    .setPViewportState(&vk::PipelineViewportStateCreateInfo().setViewportCount(1).setScissorCount(1))
                    .setPRasterizationState(&vk::PipelineRasterizationStateCreateInfo()
                        .setDepthClampEnable(false)
                        .setRasterizerDiscardEnable(false)
                        .setPolygonMode(vk::PolygonMode::eFill)
                        .setCullMode(vk::CullModeFlagBits::eBack)
                        .setFrontFace(vk::FrontFace::eCounterClockwise)
                        .setDepthBiasEnable(false)
                        .setDepthBiasConstantFactor(0)
                        .setDepthBiasClamp(0)
                        .setDepthBiasSlopeFactor(0)
                        .setLineWidth(1.f))
                    .setPDepthStencilState(&vk::PipelineDepthStencilStateCreateInfo()
                        .setDepthTestEnable(false)
                        .setDepthWriteEnable(false)
                        .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
                        .setDepthBoundsTestEnable(false)
                        .setStencilTestEnable(false))
                    .setPColorBlendState(&vk::PipelineColorBlendStateCreateInfo()
                        .setLogicOpEnable(false)
                        .setLogicOp(vk::LogicOp::eClear)
                        .setPAttachments(colorBlendAttachments.data())
                        .setAttachmentCount(colorBlendAttachments.size()))
                    .setLayout(pipelineLayout)
                    .setRenderPass(renderpass)
                    .setBasePipelineIndex(0)
                    .setPMultisampleState(&vk::PipelineMultisampleStateCreateInfo().setRasterizationSamples(vk::SampleCountFlagBits::e1))
                    .setPDynamicState(&vk::PipelineDynamicStateCreateInfo().setPDynamicStates(dynamicStates.data()).setDynamicStateCount(dynamicStates.size()))
                    .setPTessellationState(&vk::PipelineTessellationStateCreateInfo())
                    .setPNext(&vk::PipelineRasterizationConservativeStateCreateInfoEXT().setConservativeRasterizationMode(vk::ConservativeRasterizationModeEXT::eDisabled))
                );
            }

            // write descriptors for showing texture
            {
                // create sampler
                vk::SamplerCreateInfo samplerInfo;
                samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
                samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
                samplerInfo.minFilter = vk::Filter::eLinear;
                samplerInfo.magFilter = vk::Filter::eLinear;
                samplerInfo.compareEnable = false;
                auto sampler = deviceQueue->logical.createSampler(samplerInfo);
                auto image = rays->getFilteredImage();

                // desc texture texture
                vk::DescriptorImageInfo imageDesc;
                imageDesc.imageLayout = image->layout;
                imageDesc.imageView = image->view;
                imageDesc.sampler = sampler;

                // update descriptors
                deviceQueue->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet()
                        .setDstSet(descriptorSets[0])
                        .setDstBinding(0)
                        .setDstArrayElement(0)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                        .setPImageInfo(&imageDesc),
                }, nullptr);
            }

            // graphics context
            std::shared_ptr<GraphicsContext> context = std::shared_ptr<GraphicsContext>(new GraphicsContext);

            {
                // create graphics context
                context->device = deviceQueue;
                context->pipeline = trianglePipeline;
                context->descriptorPool = deviceQueue->descriptorPool;
                context->descriptorSets = descriptorSets;
                context->pipelineLayout = pipelineLayout;

                // create framebuffers by size
                context->renderpass = renderpass;
                context->swapchain = createSwapchain(deviceQueue, applicationWindow.surface, applicationWindow.surfaceFormat);
                context->framebuffers = createSwapchainFramebuffer(deviceQueue, context->swapchain, context->renderpass, applicationWindow.surfaceFormat);

                auto app = this;
                auto& window = this->applicationWindow;
                auto& cb = this->currentBuffer;

                std::shared_ptr<int32_t> curr_semaphore = std::make_shared<int32_t>(-1); // use locked ptr () 
                context->draw = [context, curr_semaphore, &cb, &window, app]() {
                    auto& currentContext = context;
                    auto& currentBuffer = cb;

                    int32_t n_semaphore = *curr_semaphore;
                    int32_t c_semaphore = (*curr_semaphore + 1) % context->framebuffers.size();
                    *curr_semaphore = c_semaphore;

                    // acquire next image where will rendered (and get semaphore when will presented finally)
                    n_semaphore = (n_semaphore >= 0 ? n_semaphore : (context->framebuffers.size() - 1));
                    currentContext->device->logical.acquireNextImageKHR(currentContext->swapchain, std::numeric_limits<uint64_t>::max(), currentContext->framebuffers[n_semaphore].semaphore, nullptr, &currentBuffer);

                    // submit rendering (and wait presentation in device)
                    {
                        // prepare viewport and clear info
                        std::vector<vk::ClearValue> clearValues = { vk::ClearColorValue(std::array<float,4>{0.2f, 0.2f, 0.2f, 1.0f}), vk::ClearDepthStencilValue(1.0f, 0) };
                        auto renderArea = vk::Rect2D(vk::Offset2D(0, 0), window.surfaceSize);
                        auto viewport = vk::Viewport(0.0f, 0.0f, window.surfaceSize.width, window.surfaceSize.height, 0, 1.0f);

                        // bind with framebuffer 
                        currentContext->framebuffers[n_semaphore].commandBuffer = getCommandBuffer(currentContext->device, true);

                        // create command buffer (with rewrite)
                        auto& commandBuffer = currentContext->framebuffers[n_semaphore].commandBuffer; // do reference of cmd buffer
                        commandBuffer.beginRenderPass(vk::RenderPassBeginInfo(context->renderpass, currentContext->framebuffers[currentBuffer].frameBuffer, renderArea, clearValues.size(), clearValues.data()), vk::SubpassContents::eInline);
                        commandBuffer.setViewport(0, std::vector<vk::Viewport> { viewport });
                        commandBuffer.setScissor(0, std::vector<vk::Rect2D> { renderArea });
                        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, currentContext->pipeline);
                        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, currentContext->pipelineLayout, 0, currentContext->descriptorSets, nullptr);
                        commandBuffer.draw(4, 1, 0, 0);
                        commandBuffer.endRenderPass();

                        // create render submission 
                        std::vector<vk::Semaphore> waitSemaphores = { currentContext->framebuffers[n_semaphore].semaphore }, signalSemaphores = { currentContext->framebuffers[c_semaphore].semaphore };
                        std::vector<vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
                        flushCommandBuffer(currentContext->device, commandBuffer, vk::SubmitInfo()
                            .setPWaitDstStageMask(waitStages.data()).setPWaitSemaphores(waitSemaphores.data()).setWaitSemaphoreCount(waitSemaphores.size())
                            .setPCommandBuffers(&commandBuffer).setCommandBufferCount(1)
                            .setPSignalSemaphores(signalSemaphores.data()).setSignalSemaphoreCount(signalSemaphores.size()), [&](){});
                    }

                    // present for displaying of this image
                    currentContext->device->queues[1]->queue.presentKHR(vk::PresentInfoKHR(
                        1, &currentContext->framebuffers[c_semaphore].semaphore,
                        1, &currentContext->swapchain,
                        &currentBuffer, nullptr
                    ));
                };
            }

            currentContext = context;
        }

        // preinit
        virtual void initVulkan() {}
        virtual void mainLoop() {}
    };


    void PathTracerApplication::execute(const int32_t& argc, const char ** argv, GLFWwindow * wind) {
        glfwGetWindowSize(wind, &baseWidth, &baseHeight); // initially window too small

        // get DPI scaling
        glfwGetWindowContentScale(wind, &windowScale, nullptr);

        // make DPI scaled
        windowWidth = baseWidth, windowHeight = baseHeight;
        glfwSetWindowSize(wind, windowWidth * windowScale, windowHeight * windowScale);

        // get canvas size
        canvasWidth = baseWidth * windowScale, canvasHeight = baseHeight * windowScale;
        glfwGetFramebufferSize(wind, &canvasWidth, &canvasHeight);

        this->initVulkan(argc, argv, wind);

        double timeAccumulate = 0.0001;
        ImGuiIO& io = ImGui::GetIO();
        bool safe_polling = true;

        auto tIdle = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(applicationWindow.window)) {
            glfwPollEvents();

            int32_t oldWidth = windowWidth, oldHeight = windowHeight;
            float oldScale = windowScale;

            // DPI scaling for Windows
            {
                glfwGetWindowSize(applicationWindow.window, &windowWidth, &windowHeight); // get as base width and height
                windowWidth /= windowScale, windowHeight /= windowScale;
                glfwGetWindowContentScale(applicationWindow.window, &windowScale, nullptr);
            }

            // rescale window by DPI
            if (oldScale != windowScale) glfwSetWindowSize(applicationWindow.window, windowWidth * windowScale, windowHeight * windowScale);

            // on resizing (include DPI scaling)
            if (oldWidth != windowWidth || oldHeight != windowHeight) {
                glfwGetFramebufferSize(applicationWindow.window, &canvasWidth, &canvasHeight);
                applicationWindow.surfaceSize.width = canvasWidth;
                applicationWindow.surfaceSize.height = canvasHeight;
                needToUpdate = true;
            }

            // update swapchain (if need)
            this->updateSwapchains();

            { // compute ray tracing with measurement
                auto tStart = std::chrono::high_resolution_clock::now(); // may have issues 
                this->process(); // do ray tracing
                auto tRenderTime = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tStart).count();
                std::stringstream ststream;
                ststream << title << " - " << int(glm::round(tRenderTime)) << "ms of ray tracing time";
                glfwSetWindowTitle(applicationWindow.window, ststream.str().c_str());
            }

            // save polled frame rendering
            timeAccumulate += std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tIdle).count();
            tIdle = std::chrono::high_resolution_clock::now(); // register polling end time 
            if (timeAccumulate >= 1000.0 / 60.0) { timeAccumulate = 0.0001; currentContext->draw(); }// show ray traced result
        }

        // wait device if anything work in 
        currentContext->device->logical.waitIdle();
        glfwDestroyWindow(applicationWindow.window);
        glfwTerminate();
    }


    void PathTracerApplication::saveHdr(std::string name) {
        auto width = rays->getCanvasWidth();
        auto height = rays->getCanvasHeight();
        std::vector<float> imageData(width * height * 4);

        {
            auto texture = rays->getRawImage();
            flushCommandBuffer(currentContext->device, createCopyCmd<TextureType&, BufferType&, vk::BufferImageCopy>(currentContext->device, texture, memoryBufferToHost, vk::BufferImageCopy()
                .setImageExtent({ width, height, 1 })
                .setImageOffset({ 0, int32_t(height), 0 }) // copy ready (rendered) image
                .setBufferOffset(0)
                .setBufferRowLength(width)
                .setBufferImageHeight(height)
                .setImageSubresource(texture->subresourceLayers)), false);
        }

        {
#ifdef OPTIX_DENOISER_HACK
            auto inp_data = inputBuffer->map(); // to denoiser before
#else
            auto inp_data = imageData.data(); // directly to outputs
#endif

        // merge to 
            getBufferSubData(memoryBufferToHost, (U_MEM_HANDLE)inp_data, sizeof(float) * width * height * 4, 0);

            // process denoising
#ifdef OPTIX_DENOISER_HACK
            inputBuffer->unmap();

            // execute denoising
            commandListWithDenoiser->execute();

            // get denoised data
            memcpy(imageData.data(), denoisedBuffer->map(), sizeof(float) * width * height * 4);
            denoisedBuffer->unmap();
#endif
        }



        {
            cil::CImg<float> image(imageData.data(), 4, width, height, 1, true);
            image.permute_axes("yzcx");
            image.mirror("y");
            image.get_shared_channel(3).fill(1.f);
            image.save_exr(name.c_str());

            /*
            // copy HDR data
            FIBITMAP * btm = FreeImage_AllocateT(FIT_RGBAF, width, height);
            for (int r = 0; r < height; r++) {
                auto row = FreeImage_GetScanLine(btm, r);
                memcpy(row, imageData.data() + r * 4 * width, width * sizeof(float) * 4);
            }

            // save HDR
            FreeImage_Save(FIF_EXR, btm, name.c_str(), EXR_FLOAT | EXR_PIZ);
            FreeImage_Unload(btm);*/
        }
    }

    // key downs
    void PathTracerApplication::passKeyDown(const int32_t& key) {
        if (key == GLFW_KEY_W) kmap.keys[ControlMap::kW] = true;
        if (key == GLFW_KEY_A) kmap.keys[ControlMap::kA] = true;
        if (key == GLFW_KEY_S) kmap.keys[ControlMap::kS] = true;
        if (key == GLFW_KEY_D) kmap.keys[ControlMap::kD] = true;
        if (key == GLFW_KEY_Q) kmap.keys[ControlMap::kQ] = true;
        if (key == GLFW_KEY_E) kmap.keys[ControlMap::kE] = true;
        if (key == GLFW_KEY_C) kmap.keys[ControlMap::kC] = true;
        if (key == GLFW_KEY_SPACE) kmap.keys[ControlMap::kSpc] = true;
        if (key == GLFW_KEY_LEFT_SHIFT) kmap.keys[ControlMap::kSft] = true;
        if (key == GLFW_KEY_K) kmap.keys[ControlMap::kK] = true;
        if (key == GLFW_KEY_L) kmap.keys[ControlMap::kL] = true;
    }

    // key release
    void PathTracerApplication::passKeyRelease(const int32_t& key) {
        if (key == GLFW_KEY_W) kmap.keys[ControlMap::kW] = false;
        if (key == GLFW_KEY_A) kmap.keys[ControlMap::kA] = false;
        if (key == GLFW_KEY_S) kmap.keys[ControlMap::kS] = false;
        if (key == GLFW_KEY_D) kmap.keys[ControlMap::kD] = false;
        if (key == GLFW_KEY_Q) kmap.keys[ControlMap::kQ] = false;
        if (key == GLFW_KEY_E) kmap.keys[ControlMap::kE] = false;
        if (key == GLFW_KEY_C) kmap.keys[ControlMap::kC] = false;
        if (key == GLFW_KEY_SPACE) kmap.keys[ControlMap::kSpc] = false;
        if (key == GLFW_KEY_LEFT_SHIFT) kmap.keys[ControlMap::kSft] = false;
        if (key == GLFW_KEY_K) {
            if (kmap.keys[ControlMap::kK]) switch360key = true;
            kmap.keys[ControlMap::kK] = false;
        }
        if (key == GLFW_KEY_L) {
            saveHdr("snapshots/hdr_snapshot_" + std::to_string(img_counter++) + ".exr");
            kmap.keys[ControlMap::kL] = false;
        }
    }

    // mouse moving and pressing
    void PathTracerApplication::mousePress(const int32_t& button) {
        ImGuiIO& io = ImGui::GetIO();
        if (button == GLFW_MOUSE_BUTTON_LEFT && !io.WantCaptureMouse) kmap.mouseleft = true;
    }


    void PathTracerApplication::mouseRelease(const int32_t& button) { if (button == GLFW_MOUSE_BUTTON_LEFT) kmap.mouseleft = false; }
    void PathTracerApplication::mouseMove(const double& x, const double& y) { mousepos.x = x, mousepos.y = y; }

    // resize buffers and canvas functions
    void PathTracerApplication::resizeBuffers(const int32_t& width, const int32_t& height) { rays->reallocRays(width, height); }
    void PathTracerApplication::resize(const int32_t& width, const int32_t& height) { rays->resizeCanvas(width, height); }
};
