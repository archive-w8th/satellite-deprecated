#pragma once

#include "imgui.h"
#include "./vkutils/vkStructures.hpp"
#include "./vkutils/vkUtils.hpp"

namespace NSM {
    class GuiRenderEngine {
    protected:

        DeviceQueueType device;
        std::string shadersPathPrefix = "shaders-spv";


        vk::Pipeline guiPipeline, guiPipeline16i;
        vk::RenderPass renderpass;
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
        std::vector<vk::DescriptorSet> descriptorSets;


        vk::PipelineLayout pipelineLayout;
        vk::PipelineCache pipelineCache;

        // where will upload/loading data
        BufferType generalStagingBuffer, generalLoadingBuffer;

        // vertex and indices buffers
        BufferType vertBuffer, idcsBuffer;
        ImageType fontTexture;


        struct GuiUniform {
            glm::ivec4 mdata = glm::ivec4(0);
            glm::mat4 gproj = glm::mat4(1.f);
        };


        std::vector<GuiUniform> guiBlockData;
        UniformBuffer guiBlockUniform; // buffer of uniforms


        void init(DeviceQueueType& deviceQueue, vk::RenderPass& _renderpass) {
            device = deviceQueue, renderpass = _renderpass;

            // define descriptor pool sizes
            std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
                vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1),
                vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 4),
            };

            // descriptor set bindings
            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr),
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr),
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr),
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr)
            };

            // layouts of descriptor sets 
            std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {
                deviceQueue->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(4))
            };

            // create descriptor sets and pool
            auto descriptorPool = deviceQueue->logical.createDescriptorPool(
                vk::DescriptorPoolCreateInfo()
                .setPPoolSizes(descriptorPoolSizes.data())
                .setPoolSizeCount(descriptorPoolSizes.size())
                .setMaxSets(1)
            );

            // descriptor sets (where will writing binding)
            descriptorSets = deviceQueue->logical.allocateDescriptorSets(
                vk::DescriptorSetAllocateInfo()
                .setDescriptorPool(descriptorPool)
                .setDescriptorSetCount(descriptorSetLayouts.size())
                .setPSetLayouts(descriptorSetLayouts.data())
            );

            // surface sizing
            //vk::Extent2D surfaceSize = {1280, 720};
            //auto renderArea = vk::Rect2D(vk::Offset2D(0, 0), surfaceSize);
            //auto viewport = vk::Viewport(0.0f, 0.0f, surfaceSize.width, surfaceSize.height, 0, 1.0f);
            //std::vector<vk::Viewport> viewports = { viewport };
            //std::vector<vk::Rect2D> scissors = { renderArea };

            // pipeline layout and cache
            pipelineLayout = deviceQueue->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(descriptorSetLayouts.data()).setSetLayoutCount(descriptorSetLayouts.size()));
            pipelineCache = deviceQueue->logical.createPipelineCache(vk::PipelineCacheCreateInfo());

            // create pipeline
            {
                // vertex state
                auto pvi = vk::PipelineVertexInputStateCreateInfo();

                // tesselation state
                auto pt = vk::PipelineTessellationStateCreateInfo();

                // viewport and scissors state
                auto pv = vk::PipelineViewportStateCreateInfo().setViewportCount(1).setScissorCount(1);
                //.setPViewports(viewports.data()).setViewportCount(viewports.size())
                //.setPScissors(scissors.data()).setScissorCount(scissors.size());

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
                    .setDepthTestEnable(false)
                    .setDepthWriteEnable(false)
                    .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
                    .setDepthBoundsTestEnable(false)
                    .setStencilTestEnable(false);

                // blend modes per framebuffer targets
                std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {
                    vk::PipelineColorBlendAttachmentState()
                    .setBlendEnable(true)
                    .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha).setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha).setColorBlendOp(vk::BlendOp::eAdd)
                    .setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha).setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha).setAlphaBlendOp(vk::BlendOp::eAdd)
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



                // pipeline stages
                std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages = {
                    vk::PipelineShaderStageCreateInfo().setModule(loadAndCreateShaderModule(deviceQueue, shadersPathPrefix + "/guiengine/vertex.vert.spv")).setPName("main").setStage(vk::ShaderStageFlagBits::eVertex),
                    vk::PipelineShaderStageCreateInfo().setModule(loadAndCreateShaderModule(deviceQueue, shadersPathPrefix + "/guiengine/pixel.frag.spv")).setPName("main").setStage(vk::ShaderStageFlagBits::eFragment)
                };



                // create graphics pipeline
                guiPipeline = deviceQueue->logical.createGraphicsPipeline(pipelineCache,
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

                pipelineShaderStages[0] = vk::PipelineShaderStageCreateInfo().setModule(loadAndCreateShaderModule(deviceQueue, shadersPathPrefix + "/guiengine/vertex16i.vert.spv")).setPName("main").setStage(vk::ShaderStageFlagBits::eVertex);

                // create graphics pipeline
                guiPipeline16i = deviceQueue->logical.createGraphicsPipeline(pipelineCache,
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



            // create staging buffers
            generalStagingBuffer = createBuffer(device, strided<uint32_t>(1024 * 1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            generalLoadingBuffer = createBuffer(device, strided<uint32_t>(1024 * 1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_TO_CPU);

            // create vertex/indice buffers
            vertBuffer = createBuffer(device, strided<ImDrawVert>(1024 * 1024), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            idcsBuffer = createBuffer(device, strided<ImDrawIdx>(1024 * 1024), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // create uniform buffer
            guiBlockUniform.buffer = createBuffer(device, strided<GuiUniform>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            guiBlockUniform.staging = createBuffer(device, strided<GuiUniform>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

            // descriptor templates
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            auto desc0Imge = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler);

            // write buffers to main descriptors
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(0).setPBufferInfo(&guiBlockUniform.buffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(1).setPBufferInfo(&vertBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(2).setPBufferInfo(&idcsBuffer->descriptorInfo)
            }, nullptr);

            // create uniform 
            guiBlockData.resize(1);

            // may be in defer
            loadImGuiResources();
        }

    public:
        GuiRenderEngine() {}

        GuiRenderEngine(DeviceQueueType& device, vk::RenderPass& renderpass, std::string shadersPack) {
            shadersPathPrefix = shadersPack;
            init(device, renderpass);
        }


        // update uniforms
        void updateUniforms() {


            //bufferSubData(guiBlockUniform.staging, guiBlockData, 0);
            //copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, guiBlockUniform.staging, guiBlockUniform.buffer, { 0, 0, strided<GuiUniform>(1) }, true);
        }


        void loadImGuiResources() {
            /*
            // descriptors templates
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            auto desc0Imge = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler);

            // Build texture atlas
            ImGuiIO& io = ImGui::GetIO();
            unsigned char* pixels;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

            // create texture
            fontTexture = createTexture(device, vk::ImageViewType::e2D, { uint32_t(width), uint32_t(height), 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR8G8B8A8Unorm, 1);
            auto tstage = createBuffer(device, strided<uint8_t>(width * height * 4), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

            // purple-black square
            bufferSubData(tstage, pixels, strided<uint8_t>(width * height * 4));

            {
                auto bufferImageCopy = vk::BufferImageCopy()
                    .setImageExtent({ uint32_t(width), uint32_t(height), 1 })
                    .setImageOffset({ 0, 0, 0 })
                    .setBufferOffset(0)
                    .setBufferRowLength(width)
                    .setBufferImageHeight(height)
                    .setImageSubresource(fontTexture->subresourceLayers);

                copyMemoryProxy<BufferType&, ImageType&, vk::BufferImageCopy>(device, tstage, fontTexture, bufferImageCopy, [&]() {
                    destroyBuffer(tstage);
                });
            }

            // create sampler for combined
            vk::SamplerCreateInfo samplerInfo;
            samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
            samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
            samplerInfo.minFilter = vk::Filter::eLinear;
            samplerInfo.magFilter = vk::Filter::eLinear;
            samplerInfo.compareEnable = false;
            fontTexture->descriptorInfo.sampler = device->logical.createSampler(samplerInfo);

            // descriptors
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Imge).setDstBinding(3).setPImageInfo(&fontTexture->descriptorInfo)
            }, nullptr);

            // imgui IO identifier
            io.Fonts->TexID = (void *)(intptr_t)fontTexture.get(); // don't know, WHY?!
            */
        }


        void renderOn(Framebuffer& framebuffer, vk::Extent2D surfaceSize, ImDrawData* imData) {
            /*
            ImGuiIO& io = ImGui::GetIO();
            imData->ScaleClipRects(io.DisplayFramebufferScale);

            // load projections into buffers


            // get optimizations
            glm::vec3 scale = glm::vec3(2.f / float(io.DisplaySize.x), 2.f / float(io.DisplaySize.y), 1.f);
            glm::vec3 offset = glm::vec3(-1.f, -1.f, 0.f);

            // render area and clear values
            auto renderArea = vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(io.DisplaySize.x * io.DisplayFramebufferScale.x, io.DisplaySize.y * io.DisplayFramebufferScale.y));
            std::vector<vk::Viewport> viewports = { vk::Viewport(0.0f, 0.0f, float(io.DisplaySize.x) * io.DisplayFramebufferScale.x, float(io.DisplaySize.y) * io.DisplayFramebufferScale.y, 0.f, 1.0f) };
            std::vector<vk::ClearValue> clearValues = { vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f}), vk::ClearDepthStencilValue(1.0f, 0) };

            // copy vertex data to unified buffer
            {
                size_t vtx_buffer_offset = 0, idx_buffer_offset = 0;
                for (int n = 0; n < imData->CmdListsCount; n++) {
                    const ImDrawList* cmd_list = imData->CmdLists[n];
                    {
                        bufferSubData(generalStagingBuffer, (const uint8_t*)cmd_list->VtxBuffer.Data, strided<ImDrawVert>(cmd_list->VtxBuffer.Size), 0);
                        copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, generalStagingBuffer, vertBuffer, { 0, vtx_buffer_offset, strided<ImDrawVert>(cmd_list->VtxBuffer.Size) }, true);
                        vtx_buffer_offset += strided<ImDrawVert>(cmd_list->VtxBuffer.Size);
                    }
                    {
                        bufferSubData(generalStagingBuffer, (const uint8_t*)cmd_list->IdxBuffer.Data, strided<ImDrawIdx>(cmd_list->IdxBuffer.Size), 0);
                        copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, generalStagingBuffer, idcsBuffer, { 0, idx_buffer_offset, strided<ImDrawIdx>(cmd_list->IdxBuffer.Size) }, true);
                        idx_buffer_offset += strided<ImDrawIdx>(cmd_list->IdxBuffer.Size);
                    }
                }
            }

            // create command buffer per FBO
            {
                guiBlockData[0].gproj = glm::transpose(glm::translate(glm::dvec3(offset)) * glm::scale(glm::dvec3(scale))); // use transposed matrix
                uint32_t vtx_buffer_offset = 0, idx_buffer_offset = 0;
                for (int n = 0; n < imData->CmdListsCount; n++) {
                    const ImDrawList* cmd_list = imData->CmdLists[n];
                    guiBlockData[0].mdata.x = vtx_buffer_offset;
                    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
                        guiBlockData[0].mdata.y = idx_buffer_offset;
                        updateUniforms();

                        const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                        if (pcmd->UserCallback) {
                            pcmd->UserCallback(cmd_list, pcmd);
                        } else {
                            std::vector<vk::Rect2D> scissors(1);
                            scissors[0].offset.x = int32_t(std::min(pcmd->ClipRect.x, pcmd->ClipRect.z));
                            scissors[0].offset.y = int32_t(std::min(pcmd->ClipRect.y, pcmd->ClipRect.w));
                            scissors[0].extent.width = uint32_t(abs(pcmd->ClipRect.z - pcmd->ClipRect.x));
                            scissors[0].extent.height = uint32_t(abs(pcmd->ClipRect.w - pcmd->ClipRect.y));

                            auto commandBuffer = getCommandBuffer(device, true);
                            commandBuffer.beginRenderPass(vk::RenderPassBeginInfo(renderpass, framebuffer.frameBuffer, renderArea, clearValues.size(), clearValues.data()), vk::SubpassContents::eInline);
                            commandBuffer.setViewport(0, viewports);
                            commandBuffer.setScissor(0, scissors);
                            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, sizeof(ImDrawIdx) == 2 ? guiPipeline16i : guiPipeline);
                            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets, nullptr);
                            commandBuffer.draw(pcmd->ElemCount, 1, 0, 0);
                            commandBuffer.endRenderPass();
                            flushCommandBuffer(device, commandBuffer, true);
                        }
                        idx_buffer_offset += pcmd->ElemCount;
                    }
                    vtx_buffer_offset += cmd_list->VtxBuffer.Size;
                }
            }
        */

        }
    };
};