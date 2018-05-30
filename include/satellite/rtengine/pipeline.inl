#pragma once

#include "pipeline.hpp"

namespace NSM
{
    namespace rt
    {

        void Pipeline::syncUniforms()
        {
            auto command = createCommandBuffer(queue, true);
            bufferSubData(command, rayBlockUniform.staging, rayBlockData, 0);
            bufferSubData(command, lightUniform.staging, lightUniformData, 0);
            bufferSubData(command, rayStreamsUniform.staging, rayStreamsData, 0);
            bufferSubData(command, shuffledSeqUniform.staging, shuffledSeqData, 0);
            memoryCopyCmd(command, rayBlockUniform.staging, rayBlockUniform.buffer, { 0, 0, strided<RayBlockUniform>(1) });
            memoryCopyCmd(command, lightUniform.staging, lightUniform.buffer, { 0, 0, strided<LightUniformStruct>(lightUniformData.size()) });
            memoryCopyCmd(command, rayStreamsUniform.staging, rayStreamsUniform.buffer, { 0, 0, strided<RayStream>(rayStreamsData.size()) });
            memoryCopyCmd(command, shuffledSeqUniform.staging, shuffledSeqUniform.buffer, { 0, 0, strided<uint32_t>(shuffledSeqData.size()) });
            flushCommandBuffers(queue, { command }, true);
        }

        void Pipeline::initDescriptorSets()
        {

            // ray tracing unified descriptors
            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                // raytracing binding set
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // ray block nodes
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // ray blocks meta
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // active blocks
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // preparing blocks
                vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // available for write blocks
                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // preparing for rewrite blocks
                vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // texels
                vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // block bins
                vk::DescriptorSetLayoutBinding(8, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // counters
                vk::DescriptorSetLayoutBinding(9, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // hits buffer
                vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // unordered indexing
                vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // generalized 16-bit index space

                // uniform set
                vk::DescriptorSetLayoutBinding(12, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // LightUniform
                vk::DescriptorSetLayoutBinding(13, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // RayBlockUniform
                vk::DescriptorSetLayoutBinding(14, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // StreamsBlockUniform

                // hit payloads
                vk::DescriptorSetLayoutBinding(15, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(16, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),

                // environment map for ray tracing
                vk::DescriptorSetLayoutBinding(20, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // environment map
            };

            // surfaces descriptor layouts
            std::vector<vk::DescriptorSetLayoutBinding> surfaceImgSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eSampledImage, MAX_SURFACE_IMAGES, vk::ShaderStageFlagBits::eCompute, nullptr), // textures
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eSampler, 16, vk::ShaderStageFlagBits::eCompute, nullptr),                      // samplers for textures
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),                 // material descs
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),                 // virtual textures
            }; 

            // images for sampling and processing
            std::vector<vk::DescriptorSetLayoutBinding> sampleImgSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // sampling image buffer
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // filtered sampled
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // clear samples flags
            };

            // ray tracing unified descriptors
            std::vector<vk::DescriptorSetLayoutBinding> traverseDescriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // rays,
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // output hits
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // output counters
                vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageTexelBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                //vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
            };

            // prepare pools and sets
            std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(descriptorSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(surfaceImgSetLayoutBindings.data()).setBindingCount(surfaceImgSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(sampleImgSetLayoutBindings.data()).setBindingCount(sampleImgSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(traverseDescriptorSetLayoutBindings.data()).setBindingCount(traverseDescriptorSetLayoutBindings.size())),
            };

            // allocate descriptor sets (except 3rd, planned to use templates)
            auto descriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(descriptorSetLayouts.size()).setPSetLayouts(descriptorSetLayouts.data()));

            // layouts
            rayTracingDescriptorsLayout = { descriptorSetLayouts[0] };
            samplingDescriptorsLayout = { descriptorSetLayouts[0], descriptorSetLayouts[2] };
            surfaceDescriptorsLayout = { descriptorSetLayouts[0], descriptorSetLayouts[1] };

            // descriptors
            rayTracingDescriptors = { descriptorSets[0] };
            samplingDescriptors = { descriptorSets[0], descriptorSets[2] };
            surfaceDescriptors = { descriptorSets[0], descriptorSets[1] };
            traverseDescriptors = { descriptorSets[3], nullptr };

            // create staging buffer
            generalStagingBuffer = createBuffer(queue, strided<uint8_t>(1024 * 1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            generalLoadingBuffer = createBuffer(queue, strided<uint8_t>(1024 * 1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_TO_CPU);
        }

        std::vector<vk::DescriptorSet>& Pipeline::getTraverseDescriptorSet() {
            return traverseDescriptors;
        }

        void Pipeline::initPipelines()
        {
            // create pipeline layouts
            rayTracingPipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(rayTracingDescriptorsLayout.data()).setSetLayoutCount(rayTracingDescriptorsLayout.size()));
            samplingPipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(samplingDescriptorsLayout.data()).setSetLayoutCount(samplingDescriptorsLayout.size()));
            surfacePipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(surfaceDescriptorsLayout.data()).setSetLayoutCount(surfaceDescriptorsLayout.size()));

            // create pipelines (TODO: make true names in C++ or host code)
            unorderedFormer = createCompute(queue, shadersPathPrefix + "/rendering/traverse-pre.comp.spv", rayTracingPipelineLayout);
            rayGeneration = createCompute(queue, shadersPathPrefix + "/rendering/gen-primary.comp.spv", rayTracingPipelineLayout);
            
            surfaceShadingPpl = createCompute(queue, shadersPathPrefix + "/rendering/hit-shader.comp.spv", surfacePipelineLayout);
            rayShadePipeline = createCompute(queue, shadersPathPrefix + "/rendering/gen-secondary.comp.spv", rayTracingPipelineLayout);
            sampleCollection = createCompute(queue, shadersPathPrefix + "/rendering/pgather.comp.spv", samplingPipelineLayout);
            clearSamples = createCompute(queue, shadersPathPrefix + "/rendering/pclear.comp.spv", samplingPipelineLayout);
            binCollect = createCompute(queue, shadersPathPrefix + "/rendering/accumulation.comp.spv", rayTracingPipelineLayout);
        }

        void Pipeline::initLights()
        {
            lightUniformData.resize(6);
            for (int i = 0; i < 6; i++)
            {
                lightUniformData[i].lightColor = glm::vec4((glm::vec3(255.f, 250.f, 244.f) / 255.f) * 400.f, 40.0f);
                lightUniformData[i].lightVector = glm::vec4(0.3f, 1.0f, 0.1f, 400.0f).xyzw() * glm::vec4(1.f, 1.f, 1.f, 1.f);
                lightUniformData[i].lightOffset = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f).xyzw() * glm::vec4(1.f, 1.f, 1.f, 1.f);
                lightUniformData[i].lightAmbient = glm::vec4(0.0f);
            }
        }

        void Pipeline::reloadQueuedRays()
        {
            // collect colors to texels
            dispatchCompute(binCollect, { INTENSIVITY, 1u, 1u }, rayTracingDescriptors);

            // getting counters values
            std::vector<int32_t> counters(8);
            auto copyCmd = makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, countersBuffer, generalLoadingBuffer, { 0, 0, strided<uint32_t>(8) });
            flushCommandBuffers(queue, { copyCmd }, false);
            getBufferSubData(generalLoadingBuffer, counters, 0);

            counters[AVAILABLE_COUNTER] = counters[AVAILABLE_COUNTER] >= 0 ? counters[AVAILABLE_COUNTER] : 0;
            int32_t additionalOffset = std::max(counters[AVAILABLE_COUNTER], 0);
            int32_t includeCount = counters[CLEANING_COUNTER];
            counters[AVAILABLE_COUNTER] += includeCount; // add rest counts

            // set bounded
            {
                auto commandBuffer = createCommandBuffer(queue, true);
                bufferSubData(commandBuffer, generalStagingBuffer, counters, 0);
                memoryCopyCmd(commandBuffer, generalStagingBuffer, countersBuffer, { 0, 0, strided<uint32_t>(8) });
                flushCommandBuffers(queue, { commandBuffer }, true);
            }

            // setup active blocks count in host
            rayBlockData[0].samplerUniform.blockCount = counters[PREPARING_BLOCK_COUNTER];

            // copying batch
            auto command = createCommandBuffer(queue, true);
            memoryCopyCmd(command, countersBuffer, rayBlockUniform.buffer, { strided<uint32_t>(PREPARING_BLOCK_COUNTER), offsetof(RayBlockUniform, samplerUniform) + offsetof(SamplerUniformStruct, blockCount), sizeof(uint32_t) });
            memoryCopyCmd(command, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(CLEANING_COUNTER), sizeof(uint32_t) });
            memoryCopyCmd(command, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(PREPARING_BLOCK_COUNTER), sizeof(uint32_t) });
            memoryCopyCmd(command, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(HIT_PAYLOAD_COUNTER), sizeof(uint32_t) });

            // batching commands
            std::vector<vk::CommandBuffer> cmds;
            cmds.push_back(command);
            if (rayBlockData[0].samplerUniform.blockCount > 0)
            {
                cmds.push_back(makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, indicesSwap[1], indicesSwap[0], { 0, 0, strided<uint32_t>(rayBlockData[0].samplerUniform.blockCount) }));
            }
            if (includeCount > 0)
            {
                cmds.push_back(makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, availableSwap[1], availableSwap[0], { 0, strided<uint32_t>(additionalOffset), strided<uint32_t>(includeCount) })); // move unused to preparing
            }
            flushCommandBuffers(queue, cmds, true);
        }

        void Pipeline::initBuffers()
        {
            // filler buffers
            zerosBufferReference = createBuffer(queue, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            debugOnes32BufferReference = createBuffer(queue, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

            // create uniform buffer
            rayBlockUniform.buffer = createBuffer(queue, strided<RayBlockUniform>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            rayBlockUniform.staging = createBuffer(queue, strided<RayBlockUniform>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            lightUniform.buffer = createBuffer(queue, strided<LightUniformStruct>(16), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            lightUniform.staging = createBuffer(queue, strided<LightUniformStruct>(16), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            rayStreamsUniform.buffer = createBuffer(queue, strided<RayStream>(16), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            rayStreamsUniform.staging = createBuffer(queue, strided<RayStream>(16), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);


            shuffledSeqUniform.buffer = createBuffer(queue, strided<uint32_t>(64), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            shuffledSeqUniform.staging = createBuffer(queue, strided<uint32_t>(64), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            shuffledSeqData = std::vector<uint32_t>(64);
            for (uint32_t i = 0; i < shuffledSeqData.size(); i++) { shuffledSeqData[i] = i; }

            // counters buffer
            countersBuffer = createBuffer(queue, strided<uint32_t>(16), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // zeros and ones
            std::vector<uint32_t> zeros(1024), ones(1024);
            std::for_each(std::execution::par_unseq, zeros.begin(), zeros.end(), [&](auto&& m) { m = 0u; });
            std::for_each(std::execution::par_unseq, ones.begin(), ones.end(), [&](auto&& m) { m = 1u; });

            {
                // make reference buffers
                auto command = createCommandBuffer(queue, true);
                bufferSubData(command, zerosBufferReference, zeros, 0); // make reference of zeros
                bufferSubData(command, debugOnes32BufferReference, ones, 0);
                flushCommandBuffers(queue, { command }, true);
            }

            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(rayTracingDescriptors[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(8).setPBufferInfo(&countersBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(12).setPBufferInfo(&lightUniform.buffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(13).setPBufferInfo(&rayBlockUniform.buffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(14).setPBufferInfo(&rayStreamsUniform.buffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(16).setPBufferInfo(&shuffledSeqUniform.buffer->descriptorInfo),
            }, nullptr);

            // null envmap
            {
                auto nullEnvSampler = device->logical.createSampler(vk::SamplerCreateInfo().setAddressModeU(vk::SamplerAddressMode::eRepeat).setAddressModeV(vk::SamplerAddressMode::eClampToEdge).setMinFilter(vk::Filter::eLinear).setMagFilter(vk::Filter::eLinear).setCompareEnable(false));
                nullEnvImage = createImage(queue, vk::ImageViewType::e2D, { 2, 2, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat, 1);
                auto tstage = createBuffer(queue, 4 * sizeof(glm::vec4), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // purple-black square
                {
                    auto command = createCommandBuffer(queue, true);
                    bufferSubData(command, tstage, std::vector<glm::vec4>({ glm::vec4(0.8f, 0.9f, 1.0f, 1.0f), glm::vec4(0.8f, 0.9f, 1.0f, 1.0f), glm::vec4(0.8f, 0.9f, 1.0f, 1.0f), glm::vec4(0.8f, 0.9f, 1.0f, 1.0f) }));
                    memoryCopyCmd(command, tstage, nullEnvImage, vk::BufferImageCopy().setImageExtent({ 2, 2, 1 }).setImageOffset({ 0, 0, 0 }).setBufferOffset(0).setBufferRowLength(2).setBufferImageHeight(2).setImageSubresource(nullEnvImage->subresourceLayers));
                    flushCommandBuffers(queue, { command }, [=]() {});
                }

                // update descriptors
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet()
                        .setDstSet(rayTracingDescriptors[0])
                        .setDstBinding(20)
                        .setDstArrayElement(0)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                        .setPImageInfo(&vk::DescriptorImageInfo().setImageLayout(nullEnvImage->layout).setImageView(nullEnvImage->view).setSampler(nullEnvSampler))},
                    nullptr);
            }

            // null texture define
            {
                // create texture
                nullImage = createImage(queue, vk::ImageViewType::e2D, { 2, 2, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat, 1, VMA_MEMORY_USAGE_GPU_ONLY);
                auto tstage = createBuffer(queue, 4 * sizeof(glm::vec4), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // purple-black square
                {
                    auto command = createCommandBuffer(queue, true);
                    bufferSubData(command, tstage, std::vector<glm::vec4>({ glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), glm::vec4(1.0f, 0.5f, 0.5f, 1.0f), glm::vec4(1.0f, 0.5f, 0.5f, 1.0f), glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) }));
                    memoryCopyCmd(command, tstage, nullImage, vk::BufferImageCopy().setImageExtent({ 2, 2, 1 }).setImageOffset({ 0, 0, 0 }).setBufferOffset(0).setBufferRowLength(2).setBufferImageHeight(2).setImageSubresource(nullEnvImage->subresourceLayers));
                    flushCommandBuffers(queue, { command }, [=]() {});
                }

                // write with same images
                std::vector<vk::DescriptorImageInfo> imageDescs;
                for (int i = 0; i < MAX_SURFACE_IMAGES; i++)
                {
                    imageDescs.push_back(vk::DescriptorImageInfo().setImageLayout(nullImage->layout).setImageView(nullImage->view));
                }

                // update descriptors
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet()
                        .setDstSet(surfaceDescriptors[1])
                        .setDstBinding(0)
                        .setDstArrayElement(0)
                        .setDescriptorCount(imageDescs.size())
                        .setDescriptorType(vk::DescriptorType::eSampledImage)
                        .setPImageInfo(imageDescs.data())},
                    nullptr);
            }

            // null sampler define
            {
                // create sampler
                auto sampler = device->logical.createSampler(vk::SamplerCreateInfo().setAddressModeU(vk::SamplerAddressMode::eRepeat).setAddressModeV(vk::SamplerAddressMode::eRepeat).setMinFilter(vk::Filter::eLinear).setMagFilter(vk::Filter::eLinear).setCompareEnable(false));

                // write with same images
                std::vector<vk::DescriptorImageInfo> samplerDescs;
                for (int i = 0; i < 16; i++)
                {
                    samplerDescs.push_back(vk::DescriptorImageInfo().setSampler(sampler));
                }

                // update descriptors
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet()
                        .setDstSet(surfaceDescriptors[1])
                        .setDstBinding(1)
                        .setDstArrayElement(0)
                        .setDescriptorCount(samplerDescs.size())
                        .setDescriptorType(vk::DescriptorType::eSampler)
                        .setPImageInfo(samplerDescs.data())},
                    nullptr);
            }

            // preinit uniforms
            rayBlockData[0].materialUniform.lightcount = 1;
            rayBlockData[0].cameraUniform.enable360 = 0;
            clearRays();




            // cache size
            const size_t TRAVERSE_CACHE_SIZE = 16;
            const size_t LOCAL_WORK_SIZE = 64;

            // caches
            //traverseBlockData = createBuffer(device, TRAVERSE_BLOCK_SIZE * INTENSIVITY, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            traverseCacheData = createBuffer(queue, TRAVERSE_CACHE_SIZE * LOCAL_WORK_SIZE * INTENSIVITY * sizeof(glm::ivec4) * 2, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer | vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // make TBO
            auto bufferView = createBufferView(traverseCacheData, vk::Format::eR32G32B32A32Sint);

            // set BVH traverse caches
            {
                auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(traverseDescriptors[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageTexelBuffer).setDstBinding(4).setPTexelBufferView(&bufferView),
                        //vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(4).setPBufferInfo(&traverseCacheData->descriptorInfo),
                        //vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(5).setPBufferInfo(&traverseBlockData->descriptorInfo),
                }, nullptr);
            }
        }

        void Pipeline::init(Queue queue)
        {
            pcg_extras::seed_seq_from<std::random_device> seed_source;
            rndEngine = std::make_shared<pcg64>(seed_source);

            this->queue = queue;
            this->device = queue->device;
            rayBlockData.resize(1);
            starttime = milliseconds();

            initDescriptorSets();
            initPipelines();
            initBuffers();
            initLights();
            syncUniforms();
        }

        void Pipeline::clearRays()
        {
            rayBlockData[0].samplerUniform.iterationCount = 0;
            flushCommandBuffers(queue, { makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, zerosBufferReference, countersBuffer, { 0, 0, strided<uint32_t>(16) }) }, true);
        }

        void Pipeline::resizeCanvas(uint32_t width, uint32_t height)
        {
            const double superCanvas = 1;
            canvasWidth = width * superCanvas, canvasHeight = height * superCanvas;

            //device->logical.waitIdle();
            //destroyTexture(accumulationImage);
            //destroyTexture(filteredImage);
            //destroyTexture(flagsImage);

            accumulationImage = createImage(queue, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(canvasWidth), uint32_t(canvasHeight * 2), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            filteredImage = createImage(queue, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(canvasWidth), uint32_t(canvasHeight * 2), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            flagsImage = createImage(queue, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(canvasWidth), uint32_t(canvasHeight * 2), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32Sint);

            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(samplingDescriptors[1]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageImage);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(0).setPImageInfo(&accumulationImage->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(1).setPImageInfo(&filteredImage->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(2).setPImageInfo(&flagsImage->descriptorInfo),
            }, nullptr);

            clearSampling();
            syncUniforms();
        }

        void Pipeline::setSkybox(Image skybox)
        {
            boundSkybox = skybox; nullEnvImage = nullptr;
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(rayTracingDescriptors[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{ vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(20).setPImageInfo(&boundSkybox->descriptorInfo)}, nullptr);
        }

        void Pipeline::reallocRays(uint32_t width, uint32_t height)
        {
            const bool IS_INTERLACED = false; // support dropped at now, and is this rudiment
            const size_t BLOCK_WIDTH = 8, BLOCK_HEIGHT = 8;
            const size_t BLOCK_SIZE = BLOCK_WIDTH * BLOCK_HEIGHT;
            const size_t BLOCK_NODES_SIZE = 32; // 32byte
            const size_t TRAVERSE_CACHE_SIZE = 1024; // 32kb
            const size_t TRAVERSE_BLOCK_SIZE = 4096; // 4kb
            const size_t ALLOC_MULT = 8;

            size_t wrsize = width * height;
            size_t rayLimit = std::min((wrsize * ALLOC_MULT) / (IS_INTERLACED ? 2l : 1l), 4096ull * 8192ull);
            size_t blockLimit = tiled(rayLimit, BLOCK_SIZE);
            rayBlockData[0].samplerUniform.sceneRes = { float(width), float(height) };
            rayBlockData[0].samplerUniform.blockBinCount = tiled(width, BLOCK_WIDTH) * tiled(height, BLOCK_HEIGHT);
            rayBlockData[0].cameraUniform.interlace = IS_INTERLACED ? 1 : 0;
            syncUniforms();

            // block headers 
            rayNodeBuffer = createBuffer(queue, BLOCK_NODES_SIZE * BLOCK_SIZE * blockLimit, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            hitBuffer = createBuffer(queue, strided<HitData>(blockLimit * BLOCK_SIZE / 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            hitPayloadBuffer = createBuffer(queue, strided<glm::mat4>(blockLimit * BLOCK_SIZE / 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // blocked isolated index spaces
            rayIndexSpaceBuffer = createBuffer(queue, strided<uint32_t>(blockLimit * BLOCK_SIZE * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // structured blocks 
            texelBuffer = createBuffer(queue, strided<Texel>(wrsize*4), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            blockBinBuffer = createBuffer(queue, strided<glm::uvec4>(rayBlockData[0].samplerUniform.blockBinCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            rayBlockBuffer = createBuffer(queue, strided<glm::uvec4>(blockLimit * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // wide index spaces
            currentBlocks = createBuffer(queue, strided<uint32_t>(blockLimit), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            preparingBlocks = createBuffer(queue, strided<uint32_t>(blockLimit), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            availableBlocks = createBuffer(queue, strided<uint32_t>(blockLimit), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            clearingBlocks = createBuffer(queue, strided<uint32_t>(blockLimit), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            unorderedTempBuffer = createBuffer(queue, strided<glm::u64vec4>(blockLimit * BLOCK_SIZE), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

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
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(10).setPBufferInfo(&unorderedTempBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(11).setPBufferInfo(&rayIndexSpaceBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(15).setPBufferInfo(&hitPayloadBuffer->descriptorInfo),
                //vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(16).setPBufferInfo(&traverseCacheData->descriptorInfo),
                //vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(17).setPBufferInfo(&traverseBlockData->descriptorInfo),
            }, nullptr);

            {
                auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(traverseDescriptors[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(0).setPBufferInfo(&unorderedTempBuffer->descriptorInfo),
                        vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(1).setPBufferInfo(&hitBuffer->descriptorInfo),
                        vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(2).setPBufferInfo(&countersBuffer->descriptorInfo)
                }, nullptr);
            }

            // init swaps
            indicesSwap[0] = currentBlocks, indicesSwap[1] = preparingBlocks, availableSwap[0] = availableBlocks, availableSwap[1] = clearingBlocks;
        }

        void Pipeline::clearSampling()
        {
            sequenceId = std::uniform_int_distribution<uint32_t>(0, 2147483647)(*rndEngine);
            rayBlockData[0].cameraUniform.prevCamInv = rayBlockData[0].cameraUniform.camInv;
            doCleanSamples = true;
        }

        void Pipeline::collectSamples()
        {

            // copy texel storage data
            vk::ImageCopy copyDesc;
            copyDesc.srcOffset = { 0, 0, 0 };
            copyDesc.dstOffset = { 0, int32_t(canvasHeight), 0 };
            copyDesc.extent = { uint32_t(canvasWidth), uint32_t(canvasHeight), 1 };

            // copy images command
            auto copyCommand = createCommandBuffer(queue, true);
            copyDesc.srcSubresource = accumulationImage->subresourceLayers;
            copyDesc.dstSubresource = accumulationImage->subresourceLayers;
            memoryCopyCmd(copyCommand, accumulationImage, accumulationImage, copyDesc);

            copyDesc.srcSubresource = flagsImage->subresourceLayers;
            copyDesc.dstSubresource = flagsImage->subresourceLayers;
            memoryCopyCmd(copyCommand, flagsImage, flagsImage, copyDesc);

            copyDesc.srcOffset = { 0, 0, 0 };
            copyDesc.dstOffset = { 0, 0, 0 };
            copyDesc.extent = { uint32_t(canvasWidth), uint32_t(canvasHeight) * 2, 1 };
            memoryCopyCmd(copyCommand, accumulationImage, filteredImage, copyDesc);

            // execute commands
            if (doCleanSamples) dispatchCompute(clearSamples, { INTENSIVITY, 1u, 1u }, samplingDescriptors);
            dispatchCompute(sampleCollection, { INTENSIVITY, 1u, 1u }, samplingDescriptors);
            flushCommandBuffers(queue, { copyCommand }, true);

            // unflag
            doCleanSamples = false;
        }

        void Pipeline::rayShading()
        {
            // update iteration counter locally
            //rayBlockData[0].materialUniform.time = randm();
            rayBlockData[0].samplerUniform.iterationCount++;
            auto fft = offsetof(RayBlockUniform, samplerUniform) + offsetof(SamplerUniformStruct, iterationCount); // choice update target offset
            //auto rft = offsetof(RayBlockUniform, materialUniform) + offsetof(MaterialUniformStruct, time); // choice update target offset

            // copy commands
            auto copyCommandBuffer = createCommandBuffer(queue, true);
            bufferSubData(copyCommandBuffer, rayBlockUniform.staging, rayBlockData, 0);
            memoryCopyCmd(copyCommandBuffer, rayBlockUniform.staging, rayBlockUniform.buffer, { fft, fft, sizeof(uint32_t) });                     // don't touch criticals
            memoryCopyCmd(copyCommandBuffer, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(UNORDERED_COUNTER), sizeof(uint32_t) }); // don't touch criticals

            // flush commands
            flushCommandBuffers(queue, { copyCommandBuffer }, true);
            dispatchCompute(rayShadePipeline, { INTENSIVITY, 1u, 1u }, rayTracingDescriptors);

            // refine rays
            reloadQueuedRays();
        }

        void Pipeline::setPerspective(const glm::dmat4 &persp) { rayBlockData[0].cameraUniform.projInv = glm::transpose(glm::inverse(persp)); }
        void Pipeline::setModelView(const glm::dmat4 &mv) { rayBlockData[0].cameraUniform.camInv = glm::transpose(glm::inverse(mv)); }

        void Pipeline::generate()
        {
            clearRays();

            const size_t num_seeds = 16;
            rayStreamsData.resize(num_seeds);


            auto u64distr = std::uniform_int_distribution<uint64_t>(0, _UI64_MAX);
            auto f01distr = std::uniform_real_distribution<float>(0.f, 1.f);

            for (int i = 0; i < rayStreamsData.size(); i++)
            {
                rayStreamsData[i].diffuseStream = glm::vec4(glm::sphericalRand(1.f), 0.f);
                rayStreamsData[i].superseed = glm::u64vec4(u64distr(*rndEngine), u64distr(*rndEngine), u64distr(*rndEngine), u64distr(*rndEngine));
                rayStreamsData[i].frand4 = glm::vec4(f01distr(*rndEngine), f01distr(*rndEngine), f01distr(*rndEngine), f01distr(*rndEngine));
            }
            sequenceId = (sequenceId + 1) % 2147483647;

            // precalculate light center
            for (int i = 0; i < lightUniformData.size(); i++)
            {
                glm::vec3 playerCenter = glm::vec3(0.0f);
                glm::vec3 lvec = glm::normalize(lightUniformData[i].lightVector.xyz()) * (lightUniformData[i].lightVector.y < 0.0f ? -1.0f : 1.0f);
                glm::vec3 lightCenter = glm::fma(lvec, glm::vec3(lightUniformData[i].lightVector.w), (lightUniformData[i].lightOffset.xyz + playerCenter));
                lightUniformData[i].lightRandomizedOrigin = glm::vec4(glm::sphericalRand(lightUniformData[i].lightColor.w - 0.0001f) + lightCenter, 1.f);
            }


            // shuffle uniform blocks
            {
                std::shuffle(shuffledSeqData.begin(), shuffledSeqData.end(), *rndEngine);
            }


            //rayBlockData[0].materialUniform.time = randm();
            rayBlockData[0].cameraUniform.ftime = float((milliseconds() - starttime) / (1000.0));
            rayBlockData[0].cameraUniform.prevCamInv = rayBlockData[0].cameraUniform.camInv;
            syncUniforms();

            // shorter dispatcher of generation
            dispatchCompute(rayGeneration, { INTENSIVITY, 1u, 1u }, rayTracingDescriptors);

            // refine rays
            reloadQueuedRays();
        }


        void Pipeline::setVirtualTextureSet(std::shared_ptr<VirtualTextureSet> &vTextureSet)
        {
            if (!vTextureSet->haveElements()) return;
            boundVirtualTextureSet = vTextureSet;

            auto vtextureBuffer = vTextureSet->getBuffer();
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet> {
                vk::WriteDescriptorSet()
                    .setDstSet(surfaceDescriptors[1])
                    .setDstBinding(3)
                    .setDstArrayElement(0)
                    .setDescriptorCount(1)
                    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                    .setPBufferInfo(&vtextureBuffer->descriptorInfo)
            }, nullptr);
        }


        void Pipeline::setMaterialSet(std::shared_ptr<MaterialSet> &materialSet)
        {
            if (!materialSet->haveElements()) return;
            boundMaterialSet = materialSet;

            // if material set needs to update
            if (boundMaterialSet && boundMaterialSet->needUpdateStatus()) {

                // make material update
                std::vector<glm::uvec2> mcount{ glm::uvec2(0, boundMaterialSet->getCount()) };
                memcpy(&rayBlockData[0].materialUniform.materialOffset, mcount.data(), mcount.size() * sizeof(glm::uvec2)); // copy to original uniform

                // copy to surfaces and update material set
                auto copyCommand = createCommandBuffer(queue, true);
                bufferSubData(copyCommand, generalStagingBuffer, mcount, 0); // upload to staging
                memoryCopyCmd(copyCommand, generalStagingBuffer, rayBlockUniform.buffer, { 0, offsetof(RayBlockUniform, materialUniform) + offsetof(MaterialUniformStruct, materialOffset), sizeof(int32_t) * 2 });
                flushCommandBuffers(queue, { copyCommand }, true);
            }

            auto materialBuffer = boundMaterialSet->getBuffer();
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet> {
                vk::WriteDescriptorSet()
                    .setDstSet(surfaceDescriptors[1])
                    .setDstBinding(2)
                    .setDstArrayElement(0)
                    .setDescriptorCount(1)
                    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                    .setPBufferInfo(&materialBuffer->descriptorInfo)
            }, nullptr);
        }


        void Pipeline::setSamplerSet(std::shared_ptr<SamplerSet> &samplerSet)
        {
            if (!samplerSet || !samplerSet->haveSamplers()) return;

            // fill by textures
            std::vector<vk::DescriptorImageInfo> images;
            auto samplers = samplerSet->getSamplers();

            // push descriptors
            for (int i = 0; i < MAX_SURFACE_IMAGES; i++)
            {
                if (i >= samplers.size()) break;
                images.push_back(samplers[i]->descriptorInfo);
            }

            // update descriptors
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet()
                    .setDstSet(surfaceDescriptors[1])
                    .setDstBinding(1)
                    .setDstArrayElement(0)
                    .setDescriptorCount(images.size())
                    .setDescriptorType(vk::DescriptorType::eSampler)
                    .setPImageInfo(images.data())
            }, nullptr);
        }


        void Pipeline::setTextureSet(std::shared_ptr<TextureSet> &textureSet)
        {
            if (!textureSet || !textureSet->haveTextures()) return;

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
                    .setDstBinding(0)
                    .setDstArrayElement(0)
                    .setDescriptorCount(images.size())
                    .setDescriptorType(vk::DescriptorType::eSampledImage)
                    .setPImageInfo(images.data())
            }, nullptr);
        }


        void Pipeline::setHierarchyStorages(const std::vector<std::shared_ptr<HieararchyStorage>> &hierarchies) {
            hstorages.resize(0);
            for (auto& em : hierarchies) { hstorages.push_back(em); }
        }


        void Pipeline::setHierarchyStorage(std::shared_ptr<HieararchyStorage> &hierarchy) {
            hstorages.resize(0);
            hstorages.push_back(hierarchy);
        }

        // 27.05.2018, we experimented with single command buffer submission of traversing cycle
        void Pipeline::traverse() {
            if (hstorages.size() <= 0) return; // no valid geometry or hierarchy
 
            std::vector<vk::CommandBuffer> cmds;

            // if material set needs to update
            if (boundMaterialSet && boundMaterialSet->needUpdateStatus()) {

                // make material update
                std::vector<glm::uvec2> mcount{ glm::uvec2(0, boundMaterialSet->getCount()) };
                memcpy(&rayBlockData[0].materialUniform.materialOffset, mcount.data(), mcount.size() * sizeof(glm::uvec2)); // copy to original uniform

                // copy to surfaces and update material set
                auto copyCommand = createCommandBuffer(queue, true);
                bufferSubData(copyCommand, generalStagingBuffer, mcount); // upload to staging
                memoryCopyCmd(copyCommand, generalStagingBuffer, rayBlockUniform.buffer, { 0, offsetof(RayBlockUniform, materialUniform) + offsetof(MaterialUniformStruct, materialOffset), sizeof(int32_t) * 2 });
                flushCommandBuffers(queue, { copyCommand }, true);

                // trigger update 
                boundMaterialSet->getBuffer();
            }

            // update virtual texture set
            if (boundVirtualTextureSet && boundVirtualTextureSet->needUpdateStatus()) {
                boundVirtualTextureSet->getBuffer(); // trigger update
            }

            // reset hit counter
            {
                auto copyCommand = createCommandBuffer(queue, true, true);
                memoryCopyCmd(copyCommand, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(HIT_COUNTER), sizeof(uint32_t) });
                //flushCommandBuffers(queue, { copyCommand }, true);
                cmds.push_back(copyCommand);
            }

            cmds.push_back(makeDispatchCmd(unorderedFormer, { INTENSIVITY, 1u, 1u }, rayTracingDescriptors, false, true)); // generate rays for traverser
            //flushCommandBuffers(queue, { makeDispatchCmd(unorderedFormer, { INTENSIVITY, 1u, 1u }, rayTracingDescriptors, false) }, true);

            // push bvh traverse commands
            for (auto& him : hstorages) { him->queryTraverse(traverseDescriptors, cmds); } // traverse and intersection with triangles (closest mode)
            //flushCommandBuffers(queue, cmds, true);

            // push surface shaders commands
            cmds.push_back(makeDispatchCmd(surfaceShadingPpl, { INTENSIVITY, 1u, 1u }, surfaceDescriptors, false, true)); // closest hit shader
            //flushCommandBuffers(queue, { makeDispatchCmd(surfaceShadingPpl,{ INTENSIVITY, 1u, 1u }, surfaceDescriptors, false) }, true);

            flushSecondaries(queue, cmds);
            //flushCommandBuffers(queue, cmds, true);
            
            return;
        }

        void Pipeline::enable360mode(bool mode) { rayBlockData[0].cameraUniform.enable360 = mode; clearSampling(); }
        size_t Pipeline::getRayCount() { return rayBlockData[0].samplerUniform.blockCount; }
        uint32_t Pipeline::getCanvasWidth() { return canvasWidth; }
        uint32_t Pipeline::getCanvasHeight() { return canvasHeight; }

        Image &Pipeline::getRawImage() { return accumulationImage; }
        Image &Pipeline::getFilteredImage() { return filteredImage; }
        Image &Pipeline::getNormalImage() { return normalImage; }
        Image &Pipeline::getAlbedoImage() { return albedoImage; }

        void Pipeline::dispatchRayTracing() {
            const int32_t MDEPTH = 16;
            this->generate();
            for (int32_t j = 0; j < MDEPTH; j++) {
                if (this->getRayCount() <= 1) break;
                this->traverse();
                this->rayShading();
            }
            this->collectSamples();
        }


    }
}