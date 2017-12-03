#pragma once

#include "../utils.hpp"
#include "./structs.hpp"
#include "./triangleHierarchy.hpp"
#include "./materialSet.hpp"
#include "glm/gtc/random.hpp"

namespace NSM {
    namespace rt {

        class Pipeline {
        protected:
            const size_t WORK_SIZE = 128;
            const size_t INTENSIVITY = 256;
            const size_t MAX_SURFACE_IMAGES = 72;

            const size_t BLOCK_COUNTER = 0;
            const size_t PREPARING_BLOCK_COUNTER = 1;
            const size_t CLEANING_COUNTER = 2;
            const size_t UNORDERED_COUNTER = 3;
            const size_t AVAILABLE_COUNTER = 4;
            const size_t HIT_COUNTER = 6;
            const size_t INTERSECTIONS_COUNTER = 7;

            DeviceQueueType device;

            ComputeContext rayGeneration;
            ComputeContext sampleCollection;
            ComputeContext surfaceShadingPpl;
            ComputeContext clearSamples;
            ComputeContext bvhTraverse;
            ComputeContext rayShadePipeline;
            ComputeContext binCollect;

            BufferType indicesSwap[2];
            BufferType availableSwap[2];

            // for staging 
            BufferType generalStagingBuffer;
            BufferType generalLoadingBuffer;


            BufferType zerosBufferReference;
            BufferType debugOnes32BufferReference;

            // blocks
            BufferType rayNodeBuffer; // ray nodes
            BufferType rayBlockBuffer; // indices and headers
            BufferType blockBinBuffer; // block color bins 
            BufferType texelBuffer; // gatherable texels
            BufferType hitBuffer; // hit and chains
            BufferType countersBuffer;
            BufferType unorderedTempBuffer; // 

            // active blocks 
            BufferType currentBlocks;
            BufferType preparingBlocks;

            // block where will rewrite
            BufferType clearingBlocks;
            BufferType availableBlocks;

            // traverse buffers
            BufferType traverseBlockData;
            BufferType traverseCacheData;

            // output images
            TextureType accumulationImage;
            TextureType filteredImage;
            TextureType flagsImage;


            std::vector<LightUniformStruct> lightUniformData;
            UniformBuffer lightUniform;

            std::vector<RayBlockUniform> rayBlockData;
            UniformBuffer rayBlockUniform;

            std::vector<RayStream> rayStreamsData;
            UniformBuffer rayStreamsUniform;



            vk::DescriptorPool descriptorPool;

            std::vector<vk::DescriptorSet> rayShadingDescriptors;
            std::vector<vk::DescriptorSet> rayTracingDescriptors;
            std::vector<vk::DescriptorSet> rayTraverseDescriptors;
            std::vector<vk::DescriptorSet> samplingDescriptors;
            std::vector<vk::DescriptorSet> surfaceDescriptors;

            std::vector<vk::DescriptorSetLayout> rayShadingDescriptorsLayout;
            std::vector<vk::DescriptorSetLayout> rayTracingDescriptorsLayout;
            std::vector<vk::DescriptorSetLayout> rayTraverseDescriptorsLayout;
            std::vector<vk::DescriptorSetLayout> samplingDescriptorsLayout;
            std::vector<vk::DescriptorSetLayout> surfaceDescriptorsLayout;

            vk::PipelineLayout rayShadingPipelineLayout;
            vk::PipelineLayout rayTracingPipelineLayout;
            vk::PipelineLayout rayTraversePipelineLayout;
            vk::PipelineLayout samplingPipelineLayout;
            vk::PipelineLayout surfacePipelineLayout;

            uint32_t canvasWidth = 1, canvasHeight = 1;

            BufferType leafTraverseBuffer;
            BufferType prepareTraverseBuffer;

            std::string shadersPathPrefix = "shaders-spv";

            double starttime = 0.f;
            bool hitCountGot = false;


        protected:

            void syncUniforms() {
                bufferSubData(rayBlockUniform.staging, rayBlockData, 0);
                bufferSubData(lightUniform.staging, lightUniformData, 0);
                bufferSubData(rayStreamsUniform.staging, rayStreamsData, 0);

                auto command = getCommandBuffer(device, true);
                memoryCopyCmd(command, rayBlockUniform.staging, rayBlockUniform.buffer, { 0, 0, strided<RayBlockUniform>(1) });
                memoryCopyCmd(command, lightUniform.staging, lightUniform.buffer, { 0, 0, strided<LightUniformStruct>(lightUniformData.size()) });
                memoryCopyCmd(command, rayStreamsUniform.staging, rayStreamsUniform.buffer, { 0, 0, strided<RayStream>(rayStreamsData.size()) });
                flushCommandBuffer(device, command, true);
            }


            void initDescriptorSets() {
                std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 24),
                    vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 8),
                    vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 6),

                    // for surface shading
                    vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 32),
                    vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, MAX_SURFACE_IMAGES)
                };

                // ray tracing unified descriptors
                std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                    vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(8, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(9, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(12, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(13, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(14, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(15, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(17, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(18, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(19, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(20, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(21, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(22, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(23, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(24, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr)
                };

                // vertex storage image buffers
                std::vector<vk::DescriptorSetLayoutBinding> vertexImgSetLayoutBindings = {
                    vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                };

                // images for sampling
                std::vector<vk::DescriptorSetLayoutBinding> sampleImgSetLayoutBindings = {
                    vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // sampling image buffer
                    vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // filtered sampled
                    vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // previous sampled image
                    vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // clear samples flags
                };

                // textures for surfaces
                std::vector<vk::DescriptorSetLayoutBinding> surfaceImgSetLayoutBindings = {
                    vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eSampledImage, MAX_SURFACE_IMAGES, vk::ShaderStageFlagBits::eCompute, nullptr), // textures 
                    vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eSampler, 32, vk::ShaderStageFlagBits::eCompute, nullptr),      // samplers for textures
                };

                // ray shading environment
                std::vector<vk::DescriptorSetLayoutBinding> shadingDescSetLayoutBindings = {
                    vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // environment map
                };


                // prepare pools and sets
                descriptorPool = device->logical.createDescriptorPool(vk::DescriptorPoolCreateInfo().setPPoolSizes(&descriptorPoolSizes[0]).setPoolSizeCount(descriptorPoolSizes.size()).setMaxSets(6));
                std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {
                    device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(descriptorSetLayoutBindings.size())),
                    device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(vertexImgSetLayoutBindings.data()).setBindingCount(vertexImgSetLayoutBindings.size())),
                    device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(sampleImgSetLayoutBindings.data()).setBindingCount(sampleImgSetLayoutBindings.size())),
                    device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(surfaceImgSetLayoutBindings.data()).setBindingCount(surfaceImgSetLayoutBindings.size())),
                    device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(shadingDescSetLayoutBindings.data()).setBindingCount(shadingDescSetLayoutBindings.size()))
                };
                auto descriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(descriptorPool).setDescriptorSetCount(descriptorSetLayouts.size()).setPSetLayouts(descriptorSetLayouts.data()));

                // layouts
                rayTracingDescriptorsLayout = { descriptorSetLayouts[0] };
                rayTraverseDescriptorsLayout = { descriptorSetLayouts[0] , descriptorSetLayouts[1] };
                samplingDescriptorsLayout = { descriptorSetLayouts[0] , descriptorSetLayouts[2] };
                surfaceDescriptorsLayout = { descriptorSetLayouts[0], descriptorSetLayouts[3] };
                rayShadingDescriptorsLayout = { descriptorSetLayouts[0], descriptorSetLayouts[4] };

                // descriptors
                rayTracingDescriptors = { descriptorSets[0] };
                rayTraverseDescriptors = { descriptorSets[0], descriptorSets[1] };
                samplingDescriptors = { descriptorSets[0] , descriptorSets[2] };
                surfaceDescriptors = { descriptorSets[0] , descriptorSets[3] };
                rayShadingDescriptors = { descriptorSets[0] , descriptorSets[4] };

                // create staging buffer
                generalStagingBuffer = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
                generalLoadingBuffer = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_TO_CPU);
            }


            void initPipelines() {
                auto pipelineCache = device->logical.createPipelineCache(vk::PipelineCacheCreateInfo());;

                // create pipeline layouts
                rayTracingPipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(rayTracingDescriptorsLayout.data()).setSetLayoutCount(rayTracingDescriptorsLayout.size()));
                rayTraversePipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(rayTraverseDescriptorsLayout.data()).setSetLayoutCount(rayTraverseDescriptorsLayout.size()));
                samplingPipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(samplingDescriptorsLayout.data()).setSetLayoutCount(samplingDescriptorsLayout.size()));
                surfacePipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(surfaceDescriptorsLayout.data()).setSetLayoutCount(surfaceDescriptorsLayout.size()));
                rayShadingPipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(rayShadingDescriptorsLayout.data()).setSetLayoutCount(rayShadingDescriptorsLayout.size()));

                // create pipelines
                rayGeneration.pipeline = createCompute(device, shadersPathPrefix + "/rendering/generation.comp.spv", rayTracingPipelineLayout, pipelineCache);
                bvhTraverse.pipeline = createCompute(device, shadersPathPrefix + "/rendering/bvh-traverse.comp.spv", rayTraversePipelineLayout, pipelineCache);
                surfaceShadingPpl.pipeline = createCompute(device, shadersPathPrefix + "/rendering/surface.comp.spv", surfacePipelineLayout, pipelineCache);
                rayShadePipeline.pipeline = createCompute(device, shadersPathPrefix + "/rendering/rayshading.comp.spv", rayShadingPipelineLayout, pipelineCache);
                sampleCollection.pipeline = createCompute(device, shadersPathPrefix + "/rendering/scatter.comp.spv", samplingPipelineLayout, pipelineCache);
                clearSamples.pipeline = createCompute(device, shadersPathPrefix + "/rendering/clear.comp.spv", samplingPipelineLayout, pipelineCache);
                binCollect.pipeline = createCompute(device, shadersPathPrefix + "/rendering/bin-collect.comp.spv", rayShadingPipelineLayout, pipelineCache);
            }


            void initLights() {
                lightUniformData.resize(6);
                for (int i = 0; i < 6; i++) {
                    lightUniformData[i].lightColor = glm::vec4((glm::vec3(255.f, 250.f, 244.f) / 255.f) * 400.f, 40.0f);
                    //lightUniformData[i].lightColor = glm::vec4((glm::vec3(255.f, 250.f, 244.f) / 255.f) * 3600.f, 10.0f);
                    lightUniformData[i].lightVector = glm::vec4(0.3f, 1.0f, 0.1f, 400.0f);
                    lightUniformData[i].lightOffset = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
                    lightUniformData[i].lightAmbient = glm::vec4(0.0f);
                }
            }


            void reloadQueuedRays() {

                // collect colors to texels
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, rayShadingPipelineLayout, 0, rayShadingDescriptors, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, binCollect.pipeline);
                commandBuffer.dispatch(INTENSIVITY, 1, 1);
                flushCommandBuffer(device, commandBuffer, true);

                // getting counters values
                std::vector<int32_t> counters(8);
                copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, countersBuffer, generalLoadingBuffer, { 0, 0, strided<uint32_t>(8) }, false);
                getBufferSubData(generalLoadingBuffer, counters, 0);

                counters[AVAILABLE_COUNTER] = counters[AVAILABLE_COUNTER] >= 0 ? counters[AVAILABLE_COUNTER] : 0;
                int32_t additionalOffset = std::max(counters[AVAILABLE_COUNTER], 0);
                int32_t includeCount = counters[CLEANING_COUNTER];
                counters[AVAILABLE_COUNTER] += includeCount; // add rest counts

                // set bounded
                bufferSubData(generalStagingBuffer, counters, 0);
                copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, generalStagingBuffer, countersBuffer, { 0, 0, strided<uint32_t>(8) }, true);

                // setup active blocks count in host
                rayBlockData[0].samplerUniform.blockCount = counters[PREPARING_BLOCK_COUNTER];

                // copying batch
                auto command = getCommandBuffer(device, true);
                memoryCopyCmd(command, countersBuffer, rayBlockUniform.buffer, { strided<uint32_t>(PREPARING_BLOCK_COUNTER), offsetof(RayBlockUniform, samplerUniform) + offsetof(SamplerUniformStruct, blockCount), sizeof(uint32_t) });
                memoryCopyCmd(command, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(CLEANING_COUNTER), sizeof(uint32_t) });
                memoryCopyCmd(command, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(PREPARING_BLOCK_COUNTER), sizeof(uint32_t) });
                memoryCopyCmd(command, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(INTERSECTIONS_COUNTER), sizeof(uint32_t) });
                flushCommandBuffer(device, command, true);

                // swap indices
                if (rayBlockData[0].samplerUniform.blockCount > 0) {
                    copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, indicesSwap[1], indicesSwap[0], { 0, 0, strided<uint32_t>(rayBlockData[0].samplerUniform.blockCount) }, true);
                }

                // swap availables
                if (includeCount > 0) {
                    copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, availableSwap[1], availableSwap[0], { 0, strided<uint32_t>(additionalOffset), strided<uint32_t>(includeCount) }, true); // move unused to preparing
                }
            }


            void initBuffers() {
                // filler buffers
                zerosBufferReference = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
                debugOnes32BufferReference = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // create uniform buffer
                rayBlockUniform.buffer = createBuffer(device, strided<RayBlockUniform>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                rayBlockUniform.staging = createBuffer(device, strided<RayBlockUniform>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
                lightUniform.buffer = createBuffer(device, strided<LightUniformStruct>(16), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                lightUniform.staging = createBuffer(device, strided<LightUniformStruct>(16), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
                rayStreamsUniform.buffer = createBuffer(device, strided<RayStream>(16), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                rayStreamsUniform.staging = createBuffer(device, strided<RayStream>(16), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // counters buffer
                countersBuffer = createBuffer(device, strided<uint32_t>(16), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

                // zeros
                std::vector<uint32_t> zeros(1024);
                std::vector<uint32_t> ones(1024);
                for (int i = 0; i < 1024; i++) { zeros[i] = 0, ones[i] = 1; }

                // make reference buffers
                bufferSubData(zerosBufferReference, zeros, 0); // make reference of zeros
                bufferSubData(debugOnes32BufferReference, ones, 0);

                auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(rayTracingDescriptors[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(8).setPBufferInfo(&countersBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(12).setPBufferInfo(&lightUniform.buffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(13).setPBufferInfo(&rayBlockUniform.buffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(15).setPBufferInfo(&rayStreamsUniform.buffer->descriptorInfo)
                }, nullptr);

                // null envmap
                {
                    // create sampler
                    vk::SamplerCreateInfo samplerInfo;
                    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
                    samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
                    samplerInfo.minFilter = vk::Filter::eLinear;
                    samplerInfo.magFilter = vk::Filter::eLinear;
                    samplerInfo.compareEnable = false;
                    auto sampler = device->logical.createSampler(samplerInfo);
                    auto texture = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, { 2, 2, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat, 1);
                    auto tstage = createBuffer(device, 4 * sizeof(glm::vec4), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

                    // purple-black square
                    bufferSubData(tstage, std::vector<glm::vec4>({
                        glm::vec4(0.8f, 0.9f, 1.0f, 1.0f), glm::vec4(0.8f, 0.9f, 1.0f, 1.0f),
                        glm::vec4(0.8f, 0.9f, 1.0f, 1.0f), glm::vec4(0.8f, 0.9f, 1.0f, 1.0f)
                    }));

                    {
                        auto bufferImageCopy = vk::BufferImageCopy()
                            .setImageExtent({ 2, 2, 1 })
                            .setImageOffset({ 0, 0, 0 })
                            .setBufferOffset(0)
                            .setBufferRowLength(2)
                            .setBufferImageHeight(2)
                            .setImageSubresource(texture->subresourceLayers);

                        copyMemoryProxy<BufferType&, TextureType&, vk::BufferImageCopy>(device, tstage, texture, bufferImageCopy, [&]() {
                            destroyBuffer(tstage);
                        });
                    }

                    // desc texture texture
                    vk::DescriptorImageInfo imageDesc;
                    imageDesc.imageLayout = texture->layout;
                    imageDesc.imageView = texture->view;
                    imageDesc.sampler = sampler;

                    // update descriptors
                    device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet> {
                        vk::WriteDescriptorSet()
                            .setDstSet(rayShadingDescriptors[1])
                            .setDstBinding(5)
                            .setDstArrayElement(0)
                            .setDescriptorCount(1)
                            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                            .setPImageInfo(&imageDesc)
                    }, nullptr);
                }

                // null texture define
                {
                    // create texture
                    auto texture = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, { 2, 2, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat, 1, VMA_MEMORY_USAGE_GPU_ONLY);
                    auto tstage = createBuffer(device, 4 * sizeof(glm::vec4), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

                    // purple-black square
                    bufferSubData(tstage, std::vector<glm::vec4>({
                        glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), glm::vec4(1.0f, 0.5f, 0.5f, 1.0f),
                        glm::vec4(1.0f, 0.5f, 0.5f, 1.0f), glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)
                    }));

                    {
                        auto bufferImageCopy = vk::BufferImageCopy()
                            .setImageExtent({ 2, 2, 1 })
                            .setImageOffset({ 0, 0, 0 })
                            .setBufferOffset(0)
                            .setBufferRowLength(2)
                            .setBufferImageHeight(2)
                            .setImageSubresource(texture->subresourceLayers);

                        copyMemoryProxy<BufferType&, TextureType&, vk::BufferImageCopy>(device, tstage, texture, bufferImageCopy, [&]() {
                            destroyBuffer(tstage);
                        });
                    }

                    // desc texture texture
                    vk::DescriptorImageInfo imageDesc;
                    imageDesc.imageLayout = texture->layout;
                    imageDesc.imageView = texture->view;

                    // write with same images
                    std::vector<vk::DescriptorImageInfo> imageDescs;
                    for (int i = 0; i < MAX_SURFACE_IMAGES; i++) {
                        imageDescs.push_back(imageDesc);
                    }

                    // update descriptors
                    device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                        vk::WriteDescriptorSet()
                            .setDstSet(surfaceDescriptors[1])
                            .setDstBinding(6)
                            .setDstArrayElement(0)
                            .setDescriptorCount(imageDescs.size())
                            .setDescriptorType(vk::DescriptorType::eSampledImage)
                            .setPImageInfo(imageDescs.data())
                    }, nullptr);
                }

                // null sampler define
                {
                    // create sampler
                    vk::SamplerCreateInfo samplerInfo;
                    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
                    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
                    samplerInfo.minFilter = vk::Filter::eLinear;
                    samplerInfo.magFilter = vk::Filter::eLinear;
                    samplerInfo.compareEnable = false;
                    auto sampler = device->logical.createSampler(samplerInfo);

                    // use sampler in description set
                    vk::DescriptorImageInfo samplerDesc;
                    samplerDesc.sampler = sampler;

                    // write with same images
                    std::vector<vk::DescriptorImageInfo> samplerDescs;
                    for (int i = 0; i < 32; i++) {
                        samplerDescs.push_back(samplerDesc);
                    }

                    // update descriptors
                    device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                        vk::WriteDescriptorSet()
                            .setDstSet(surfaceDescriptors[1])
                            .setDstBinding(7)
                            .setDstArrayElement(0)
                            .setDescriptorCount(samplerDescs.size())
                            .setDescriptorType(vk::DescriptorType::eSampler)
                            .setPImageInfo(samplerDescs.data())
                    }, nullptr);
                }

                // preinit uniforms
                rayBlockData[0].materialUniform.lightcount = 1;
                rayBlockData[0].cameraUniform.enable360 = 0;
                clearRays();
            }

            
            void init(DeviceQueueType& device) {
                this->device = device;
                rayBlockData.resize(1);

                initDescriptorSets();
                initPipelines();
                initBuffers();
                initLights();
                syncUniforms();

                starttime = milliseconds();
            }


        public:

            void clearRays() {
                rayBlockData[0].samplerUniform.iterationCount = 0;
                copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, 0, strided<uint32_t>(16) }, true);
            }


            void resizeCanvas(uint32_t width, uint32_t height) {
                canvasWidth = width, canvasHeight = height;

                device->logical.waitIdle();
                destroyTexture(accumulationImage);
                destroyTexture(filteredImage);
                destroyTexture(flagsImage);

                accumulationImage = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(width), uint32_t(height), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
                filteredImage = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(width), uint32_t(height), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
                flagsImage = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(width), uint32_t(height), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32Uint);

                auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(samplingDescriptors[1]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageImage);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(0).setPImageInfo(&accumulationImage->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(1).setPImageInfo(&filteredImage->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(2).setPImageInfo(&accumulationImage->descriptorInfo), // TODO previous sample not equalize to current
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(3).setPImageInfo(&flagsImage->descriptorInfo),
                }, nullptr);

                syncUniforms();
            }


            void setSkybox(TextureType& skybox) {
                auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(rayShadingDescriptors[1]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(5).setPImageInfo(&skybox->descriptorInfo)
                }, nullptr);
            }


            void reallocRays(uint32_t width, uint32_t height) {
                const bool IS_INTERLACED = false;

                //const size_t BLOCK_WIDTH = 16, BLOCK_HEIGHT = 16;
                const size_t BLOCK_WIDTH = 32, BLOCK_HEIGHT = 32;
                
                const size_t BLOCK_SIZE = BLOCK_WIDTH * BLOCK_HEIGHT;
                const size_t BLOCK_NODES_SIZE = 64 * BLOCK_SIZE;
                const size_t BLOCK_INDICES_SIZE = 8 * BLOCK_SIZE + 64;
                const size_t BLOCK_BIN_SIZE = 4 * BLOCK_SIZE + 64;
                const size_t TRAVERSE_CACHE_SIZE = 1024 * 256;
                const size_t TRAVERSE_BLOCK_SIZE = 2048 + 64;
                const size_t ALLOC_MULT = 4;


                size_t wrsize = width * height;
                size_t rayLimit = std::min((wrsize * ALLOC_MULT) / (IS_INTERLACED ? 2l : 1l), 4096ull * 4096ull);
                size_t blockLimit = tiled(rayLimit, BLOCK_SIZE);
                rayBlockData[0].samplerUniform.sceneRes = { float(width), float(height) };
                rayBlockData[0].samplerUniform.blockBinCount = tiled(width, BLOCK_WIDTH) * tiled(height, BLOCK_HEIGHT);
                rayBlockData[0].cameraUniform.interlace = IS_INTERLACED ? 1 : 0;
                syncUniforms();

                // 
                rayNodeBuffer = createBuffer(device, BLOCK_NODES_SIZE * blockLimit, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                rayBlockBuffer = createBuffer(device, BLOCK_INDICES_SIZE * blockLimit, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                currentBlocks = createBuffer(device, strided<uint32_t>(blockLimit), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                preparingBlocks = createBuffer(device, strided<uint32_t>(blockLimit), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                availableBlocks = createBuffer(device, strided<uint32_t>(blockLimit), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                clearingBlocks = createBuffer(device, strided<uint32_t>(blockLimit), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                texelBuffer = createBuffer(device, strided<Texel>(wrsize), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                blockBinBuffer = createBuffer(device, rayBlockData[0].samplerUniform.blockBinCount * BLOCK_BIN_SIZE, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                hitBuffer = createBuffer(device, strided<HitRework>(blockLimit * BLOCK_SIZE / 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                traverseBlockData = createBuffer(device, TRAVERSE_BLOCK_SIZE * INTENSIVITY, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                traverseCacheData = createBuffer(device, TRAVERSE_CACHE_SIZE * INTENSIVITY, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                unorderedTempBuffer = createBuffer(device, strided<uint32_t>(blockLimit * BLOCK_SIZE), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

                // minmaxes
                std::vector<glm::vec4> minmaxes(64);
                for (int i = 0; i < 32; i++) { minmaxes[i * 2 + 0] = glm::vec4(10000.f), minmaxes[i * 2 + 1] = glm::vec4(-10000.f); }

                // zeros
                std::vector<uint32_t> zeros(1024), ones(1024);
                for (int i = 0; i < 1024; i++) { zeros[i] = 0, ones[i] = 1; }

                // write descriptos
                auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(rayTracingDescriptors[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(0).setPBufferInfo(&rayNodeBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(1).setPBufferInfo(&rayBlockBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(2).setPBufferInfo(&currentBlocks->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(3).setPBufferInfo(&preparingBlocks->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(4).setPBufferInfo(&availableBlocks->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(5).setPBufferInfo(&clearingBlocks->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(6).setPBufferInfo(&texelBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(7).setPBufferInfo(&blockBinBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(8).setPBufferInfo(&countersBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(9).setPBufferInfo(&hitBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(11).setPBufferInfo(&unorderedTempBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(19).setPBufferInfo(&traverseCacheData->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(20).setPBufferInfo(&traverseBlockData->descriptorInfo)
                }, nullptr);

                // init swaps
                indicesSwap[0] = currentBlocks, indicesSwap[1] = preparingBlocks, availableSwap[0] = availableBlocks, availableSwap[1] = clearingBlocks;
            }


            void clearSampling() {
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, samplingPipelineLayout, 0, samplingDescriptors, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, clearSamples.pipeline);
                commandBuffer.dispatch(INTENSIVITY, 1, 1);
                flushCommandBuffer(device, commandBuffer, true);
            }


            void collectSamples() {
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, samplingPipelineLayout, 0, samplingDescriptors, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, sampleCollection.pipeline);
                commandBuffer.dispatch(INTENSIVITY, 1, 1);
                flushCommandBuffer(device, commandBuffer, true);
            }


            void rayShading() {
                // update iteration counter locally
                //rayBlockData[0].materialUniform.time = randm();
                rayBlockData[0].samplerUniform.iterationCount++;
                bufferSubData(rayBlockUniform.staging, rayBlockData, 0);
                auto fft = offsetof(RayBlockUniform, samplerUniform) + offsetof(SamplerUniformStruct, iterationCount); // choice update target offset
                //auto rft = offsetof(RayBlockUniform, materialUniform) + offsetof(MaterialUniformStruct, time); // choice update target offset

                
                auto commandBuffer = getCommandBuffer(device, true);
                memoryCopyCmd(commandBuffer, rayBlockUniform.staging, rayBlockUniform.buffer, { fft, fft, sizeof(uint32_t) }); // don't touch criticals
                //memoryCopyCmd(commandBuffer, rayBlockUniform.staging, rayBlockUniform.buffer, { rft, rft, sizeof(uint32_t) }); // don't touch criticals
                memoryCopyCmd(commandBuffer, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(UNORDERED_COUNTER), sizeof(uint32_t) }); // don't touch criticals
                flushCommandBuffer(device, commandBuffer, true);

                // shade rays
                commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, rayShadingPipelineLayout, 0, rayShadingDescriptors, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, rayShadePipeline.pipeline);
                commandBuffer.dispatch(INTENSIVITY, 1, 1);
                flushCommandBuffer(device, commandBuffer, true);

                // refine rays
                reloadQueuedRays();
            }


            void generate(const glm::mat4 &persp, const glm::mat4 &frontSide) {
                clearRays();

                rayStreamsData.resize(16);
                for (int i = 0; i < rayStreamsData.size();i++) {
                    rayStreamsData[i].diffuseStream = glm::vec4(glm::sphericalRand(1.f), 0.f);
                    rayStreamsData[i].superseed.x = randm();
                }
                
                //rayBlockData[0].materialUniform.time = randm();
                rayBlockData[0].cameraUniform.ftime = float((milliseconds() - starttime) / (1000.0));
                rayBlockData[0].cameraUniform.camInv = glm::transpose(glm::inverse(frontSide));
                rayBlockData[0].cameraUniform.projInv = glm::transpose(glm::inverse(persp));
                syncUniforms();

                //
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, rayTracingPipelineLayout, 0, rayTracingDescriptors, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, rayGeneration.pipeline);
                commandBuffer.dispatch(INTENSIVITY, 1, 1);
                flushCommandBuffer(device, commandBuffer, true);

                // refine rays
                reloadQueuedRays();
            }


            void surfaceShading(std::shared_ptr<MaterialSet>& materialSet) {
                if (!materialSet->haveMaterials()) return;

                // copy to surfaces
                auto command = getCommandBuffer(device, true);
                memoryCopyCmd(command, materialSet->getCountBuffer(), rayBlockUniform.buffer, { 0, offsetof(RayBlockUniform, materialUniform) + offsetof(MaterialUniformStruct, materialOffset), sizeof(int32_t) * 2 });
                if (!hitCountGot) {
                    memoryCopyCmd(command, countersBuffer, rayBlockUniform.buffer, { strided<uint32_t>(HIT_COUNTER), offsetof(RayBlockUniform, samplerUniform) + offsetof(SamplerUniformStruct, hitCount), sizeof(uint32_t) });
                    memoryCopyCmd(command, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(HIT_COUNTER), sizeof(uint32_t) });
                }
                flushCommandBuffer(device, command, true);
                hitCountGot = true;

                // material array loading
                auto materialBuffer = materialSet->getMaterialBuffer();
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet()
                        .setDstSet(surfaceDescriptors[0])
                        .setDstBinding(17)
                        .setDstArrayElement(0)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                        .setPBufferInfo(&materialBuffer->descriptorInfo)
                }, nullptr);

                //
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, surfacePipelineLayout, 0, surfaceDescriptors, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, surfaceShadingPpl.pipeline);
                commandBuffer.dispatch(INTENSIVITY, 1, 1);
                flushCommandBuffer(device, commandBuffer, true);
            }


            void setTextureSet(std::shared_ptr<TextureSet>& textureSet) {
                if (!textureSet->haveTextures()) return;

                // fill by textures
                std::vector<vk::DescriptorImageInfo> images;
                auto textures = textureSet->getTextures();

                // push descriptors
                for (int i = 0; i < MAX_SURFACE_IMAGES; i++) {
                    if (i >= textures.size()) break;
                    images.push_back(textures[i]->descriptorInfo);
                }

                // update descriptors
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet()
                        .setDstSet(surfaceDescriptors[1])
                        .setDstBinding(6)
                        .setDstArrayElement(0)
                        .setDescriptorCount(images.size())
                        .setDescriptorType(vk::DescriptorType::eSampledImage)
                        .setPImageInfo(images.data())
                }, nullptr);
            }


            void surfaceShading(std::shared_ptr<MaterialSet>& materialSet, std::shared_ptr<TextureSet>& textureSet) {
                setTextureSet(textureSet);
                surfaceShading(materialSet);
            }


            void traverse(std::shared_ptr<TriangleHierarchy>& hierarchy) {
                // write descriptors for BVH traverse
                auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(rayTracingDescriptors[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
                device->logical.updateDescriptorSets(hierarchy->getVertexImageDescriptors(rayTraverseDescriptors[1]), nullptr);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(10).setPBufferInfo(&hierarchy->getMaterialBuffer()->descriptorInfo)}, nullptr);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(14).setPBufferInfo(&hierarchy->getUniformBlockBuffer().buffer->descriptorInfo)}, nullptr);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(18).setPBufferInfo(&hierarchy->getBVHBuffer()->descriptorInfo)}, nullptr);

                // traverse BVH
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, rayTraversePipelineLayout, 0, rayTraverseDescriptors, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, bvhTraverse.pipeline);
                commandBuffer.dispatch(INTENSIVITY, 1, 1);
                flushCommandBuffer(device, commandBuffer, true);

                hitCountGot = false;
            }


            uint32_t getCanvasWidth() { return canvasWidth; }
            uint32_t getCanvasHeight() { return canvasHeight; }

            TextureType& getRawImage() { return accumulationImage; }
            TextureType& getFilteredImage() { return filteredImage; }

            // panorama mode
            void enable360mode(bool mode) { rayBlockData[0].cameraUniform.enable360 = mode; }
            size_t getRayCount() { return rayBlockData[0].samplerUniform.blockCount; }

        public:
            Pipeline() {}
            Pipeline(DeviceQueueType& device, std::string shadersPack) {
                shadersPathPrefix = shadersPack;
                init(device);
            }
        };

    }
}