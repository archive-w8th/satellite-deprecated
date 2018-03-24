#pragma once

#include "../../rtengine/pipeline.hpp"

namespace NSM
{
    namespace rt
    {

        void Pipeline::syncUniforms()
        {
            auto command = getCommandBuffer(device, true);
            bufferSubData(command, rayBlockUniform.staging, rayBlockData, 0);
            bufferSubData(command, lightUniform.staging, lightUniformData, 0);
            bufferSubData(command, rayStreamsUniform.staging, rayStreamsData, 0);
            memoryCopyCmd(command, rayBlockUniform.staging, rayBlockUniform.buffer, { 0, 0, strided<RayBlockUniform>(1) });
            memoryCopyCmd(command, lightUniform.staging, lightUniform.buffer, { 0, 0, strided<LightUniformStruct>(lightUniformData.size()) });
            memoryCopyCmd(command, rayStreamsUniform.staging, rayStreamsUniform.buffer, { 0, 0, strided<RayStream>(rayStreamsData.size()) });
            flushCommandBuffer(device, command, true);
        }

        void Pipeline::initDescriptorSets()
        {
            // descriptor set layout of foreign storage (planned to sharing this structure by C++17)
            std::vector<vk::DescriptorSetLayoutBinding> clientDescriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // attribute (alpha footage)
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // BVH boxes
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // materials
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // orders
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // geometryUniform
                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // BVH metadata
                vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // reserved
                vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // vertex linear buffer
            };

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

                // BVH cache binding set
                vk::DescriptorSetLayoutBinding(16, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(17, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),

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

            // prepare pools and sets
            std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(descriptorSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(surfaceImgSetLayoutBindings.data()).setBindingCount(surfaceImgSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(sampleImgSetLayoutBindings.data()).setBindingCount(sampleImgSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(clientDescriptorSetLayoutBindings.data()).setBindingCount(clientDescriptorSetLayoutBindings.size())),
            };

            // allocate descriptor sets (except 3rd, planned to use templates)
            auto descriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(3).setPSetLayouts(descriptorSetLayouts.data()));

            // layouts
            rayTracingDescriptorsLayout = { descriptorSetLayouts[0] };
            rayTraverseDescriptorsLayout = { descriptorSetLayouts[0], descriptorSetLayouts[3] };
            samplingDescriptorsLayout = { descriptorSetLayouts[0], descriptorSetLayouts[2] };
            surfaceDescriptorsLayout = { descriptorSetLayouts[0], descriptorSetLayouts[3], descriptorSetLayouts[1] };

            // descriptors
            rayTracingDescriptors = { descriptorSets[0] };
            rayTraverseDescriptors = { descriptorSets[0], nullptr };
            samplingDescriptors = { descriptorSets[0], descriptorSets[2] };
            surfaceDescriptors = { descriptorSets[0], nullptr, descriptorSets[1] };

            // create staging buffer
            generalStagingBuffer = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            generalLoadingBuffer = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_TO_CPU);
        }

        void Pipeline::initPipelines()
        {
            auto pipelineCache = device->logical.createPipelineCache(vk::PipelineCacheCreateInfo());

            // create pipeline layouts
            rayTracingPipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(rayTracingDescriptorsLayout.data()).setSetLayoutCount(rayTracingDescriptorsLayout.size()));
            rayTraversePipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(rayTraverseDescriptorsLayout.data()).setSetLayoutCount(rayTraverseDescriptorsLayout.size()));
            samplingPipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(samplingDescriptorsLayout.data()).setSetLayoutCount(samplingDescriptorsLayout.size()));
            surfacePipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(surfaceDescriptorsLayout.data()).setSetLayoutCount(surfaceDescriptorsLayout.size()));

            // create pipelines (TODO: make true names in C++ or host code)
            unorderedFormer = createCompute(device, shadersPathPrefix + "/rendering/traverse-pre.comp.spv", rayTracingPipelineLayout, pipelineCache);
            rayGeneration = createCompute(device, shadersPathPrefix + "/rendering/gen-primary.comp.spv", rayTracingPipelineLayout, pipelineCache);
            bvhTraverse = createCompute(device, shadersPathPrefix + "/rendering/traverse-bvh.comp.spv", rayTraversePipelineLayout, pipelineCache);
            surfaceShadingPpl = createCompute(device, shadersPathPrefix + "/rendering/hit-shader.comp.spv", surfacePipelineLayout, pipelineCache);
            rayShadePipeline = createCompute(device, shadersPathPrefix + "/rendering/gen-secondary.comp.spv", rayTracingPipelineLayout, pipelineCache);
            sampleCollection = createCompute(device, shadersPathPrefix + "/rendering/pgather.comp.spv", samplingPipelineLayout, pipelineCache);
            clearSamples = createCompute(device, shadersPathPrefix + "/rendering/pclear.comp.spv", samplingPipelineLayout, pipelineCache);
            binCollect = createCompute(device, shadersPathPrefix + "/rendering/accumulation.comp.spv", rayTracingPipelineLayout, pipelineCache);
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
            dispatchCompute(binCollect, INTENSIVITY, rayTracingDescriptors);

            // getting counters values
            std::vector<int32_t> counters(8);
            auto copyCmd = createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, countersBuffer, generalLoadingBuffer, { 0, 0, strided<uint32_t>(8) });
            flushCommandBuffer(device, copyCmd, false);
            getBufferSubData(generalLoadingBuffer, counters, 0);

            counters[AVAILABLE_COUNTER] = counters[AVAILABLE_COUNTER] >= 0 ? counters[AVAILABLE_COUNTER] : 0;
            int32_t additionalOffset = std::max(counters[AVAILABLE_COUNTER], 0);
            int32_t includeCount = counters[CLEANING_COUNTER];
            counters[AVAILABLE_COUNTER] += includeCount; // add rest counts

            // set bounded
            {
                auto commandBuffer = getCommandBuffer(device, true);
                bufferSubData(commandBuffer, generalStagingBuffer, counters, 0);
                memoryCopyCmd(commandBuffer, generalStagingBuffer, countersBuffer, { 0, 0, strided<uint32_t>(8) });
                flushCommandBuffer(device, commandBuffer, true);
            }

            // setup active blocks count in host
            rayBlockData[0].samplerUniform.blockCount = counters[PREPARING_BLOCK_COUNTER];

            // copying batch
            auto command = getCommandBuffer(device, true);
            memoryCopyCmd(command, countersBuffer, rayBlockUniform.buffer, { strided<uint32_t>(PREPARING_BLOCK_COUNTER), offsetof(RayBlockUniform, samplerUniform) + offsetof(SamplerUniformStruct, blockCount), sizeof(uint32_t) });
            memoryCopyCmd(command, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(CLEANING_COUNTER), sizeof(uint32_t) });
            memoryCopyCmd(command, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(PREPARING_BLOCK_COUNTER), sizeof(uint32_t) });
            memoryCopyCmd(command, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(INTERSECTIONS_COUNTER), sizeof(uint32_t) });

            // batching commands
            std::vector<vk::CommandBuffer> cmds;
            cmds.push_back(command);
            if (rayBlockData[0].samplerUniform.blockCount > 0)
            {
                cmds.push_back(createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, indicesSwap[1], indicesSwap[0], { 0, 0, strided<uint32_t>(rayBlockData[0].samplerUniform.blockCount) }));
            }
            if (includeCount > 0)
            {
                cmds.push_back(createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, availableSwap[1], availableSwap[0], { 0, strided<uint32_t>(additionalOffset), strided<uint32_t>(includeCount) })); // move unused to preparing
            }
            flushCommandBuffers(device, cmds, true);
        }

        void Pipeline::initBuffers()
        {
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

            // zeros and ones
            std::vector<uint32_t> zeros(1024), ones(1024);
            std::for_each(std::execution::par_unseq, zeros.begin(), zeros.end(), [&](auto&& m) { m = 0u; });
            std::for_each(std::execution::par_unseq, ones.begin(), ones.end(), [&](auto&& m) { m = 1u; });

            {
                // make reference buffers
                auto command = getCommandBuffer(device, true);
                bufferSubData(command, zerosBufferReference, zeros, 0); // make reference of zeros
                bufferSubData(command, debugOnes32BufferReference, ones, 0);
                flushCommandBuffer(device, command, true);
            }

            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(rayTracingDescriptors[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(8).setPBufferInfo(&countersBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(12).setPBufferInfo(&lightUniform.buffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(13).setPBufferInfo(&rayBlockUniform.buffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(14).setPBufferInfo(&rayStreamsUniform.buffer->descriptorInfo)},
                nullptr);

            // null envmap
            {
                auto sampler = device->logical.createSampler(vk::SamplerCreateInfo().setAddressModeU(vk::SamplerAddressMode::eRepeat).setAddressModeV(vk::SamplerAddressMode::eClampToEdge).setMinFilter(vk::Filter::eLinear).setMagFilter(vk::Filter::eLinear).setCompareEnable(false));
                auto texture = createTexture(device, vk::ImageViewType::e2D, { 2, 2, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat, 1);
                auto tstage = createBuffer(device, 4 * sizeof(glm::vec4), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // purple-black square
                {
                    auto command = getCommandBuffer(device, true);
                    bufferSubData(command, tstage, std::vector<glm::vec4>({ glm::vec4(0.8f, 0.9f, 1.0f, 1.0f), glm::vec4(0.8f, 0.9f, 1.0f, 1.0f), glm::vec4(0.8f, 0.9f, 1.0f, 1.0f), glm::vec4(0.8f, 0.9f, 1.0f, 1.0f) }));
                    memoryCopyCmd(command, tstage, texture, vk::BufferImageCopy().setImageExtent({ 2, 2, 1 }).setImageOffset({ 0, 0, 0 }).setBufferOffset(0).setBufferRowLength(2).setBufferImageHeight(2).setImageSubresource(texture->subresourceLayers));
                    flushCommandBuffer(device, command, [&]() { destroyBuffer(tstage); });
                }

                // update descriptors
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet()
                        .setDstSet(rayTracingDescriptors[0])
                        .setDstBinding(20)
                        .setDstArrayElement(0)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                        .setPImageInfo(&vk::DescriptorImageInfo().setImageLayout(texture->layout).setImageView(texture->view).setSampler(sampler))},
                    nullptr);
            }

            // null texture define
            {
                // create texture
                auto texture = createTexture(device, vk::ImageViewType::e2D, { 2, 2, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat, 1, VMA_MEMORY_USAGE_GPU_ONLY);
                auto tstage = createBuffer(device, 4 * sizeof(glm::vec4), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // purple-black square
                {
                    auto command = getCommandBuffer(device, true);
                    bufferSubData(command, tstage, std::vector<glm::vec4>({ glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), glm::vec4(1.0f, 0.5f, 0.5f, 1.0f), glm::vec4(1.0f, 0.5f, 0.5f, 1.0f), glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) }));
                    memoryCopyCmd(command, tstage, texture, vk::BufferImageCopy().setImageExtent({ 2, 2, 1 }).setImageOffset({ 0, 0, 0 }).setBufferOffset(0).setBufferRowLength(2).setBufferImageHeight(2).setImageSubresource(texture->subresourceLayers));
                    flushCommandBuffer(device, command, [&]() { destroyBuffer(tstage); });
                }

                // write with same images
                std::vector<vk::DescriptorImageInfo> imageDescs;
                for (int i = 0; i < MAX_SURFACE_IMAGES; i++)
                {
                    imageDescs.push_back(vk::DescriptorImageInfo().setImageLayout(texture->layout).setImageView(texture->view));
                }

                // update descriptors
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet()
                        .setDstSet(surfaceDescriptors[2])
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
                        .setDstSet(surfaceDescriptors[2])
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
        }

        void Pipeline::init(DeviceQueueType &device)
        {
            this->device = device;
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
            flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, 0, strided<uint32_t>(16) }), true);
        }

        void Pipeline::resizeCanvas(uint32_t width, uint32_t height)
        {
            const double superCanvas = 1;
            canvasWidth = width * superCanvas, canvasHeight = height * superCanvas;

            device->logical.waitIdle();
            destroyTexture(accumulationImage);
            destroyTexture(filteredImage);
            destroyTexture(flagsImage);

            accumulationImage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(canvasWidth), uint32_t(canvasHeight * 2), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            filteredImage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(canvasWidth), uint32_t(canvasHeight * 2), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            flagsImage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(canvasWidth), uint32_t(canvasHeight * 2), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32Sint);

            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(samplingDescriptors[1]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageImage);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(0).setPImageInfo(&accumulationImage->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(1).setPImageInfo(&filteredImage->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(2).setPImageInfo(&flagsImage->descriptorInfo),
            }, nullptr);

            clearSampling();
            syncUniforms();
        }

        void Pipeline::setSkybox(TextureType &skybox)
        {
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(rayTracingDescriptors[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{ vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(20).setPImageInfo(&skybox->descriptorInfo)}, nullptr);
        }

        void Pipeline::reallocRays(uint32_t width, uint32_t height)
        {
            const bool IS_INTERLACED = false; // support dropped at now, and is this rudiment
            const size_t BLOCK_WIDTH = 8, BLOCK_HEIGHT = 8;
            const size_t BLOCK_SIZE = BLOCK_WIDTH * BLOCK_HEIGHT;
            const size_t BLOCK_NODES_SIZE = 32; // 32byte
            const size_t TRAVERSE_CACHE_SIZE = 1024; // 32kb
            const size_t TRAVERSE_BLOCK_SIZE = 4096; // 4kb
            const size_t ALLOC_MULT = 12;

            size_t wrsize = width * height;
            size_t rayLimit = std::min((wrsize * ALLOC_MULT) / (IS_INTERLACED ? 2l : 1l), 4096ull * 8192ull);
            size_t blockLimit = tiled(rayLimit, BLOCK_SIZE);
            rayBlockData[0].samplerUniform.sceneRes = { float(width), float(height) };
            rayBlockData[0].samplerUniform.blockBinCount = tiled(width, BLOCK_WIDTH) * tiled(height, BLOCK_HEIGHT);
            rayBlockData[0].cameraUniform.interlace = IS_INTERLACED ? 1 : 0;
            syncUniforms();

            // block headers 
            rayNodeBuffer = createBuffer(device, BLOCK_NODES_SIZE * BLOCK_SIZE * blockLimit, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            hitBuffer = createBuffer(device, strided<HitRework>(blockLimit * BLOCK_SIZE / 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // blocked isolated index spaces
            rayIndexSpaceBuffer = createBuffer(device, strided<uint32_t>(blockLimit * BLOCK_SIZE * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // structured blocks 
            texelBuffer = createBuffer(device, strided<Texel>(wrsize), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            blockBinBuffer = createBuffer(device, strided<glm::uvec4>(rayBlockData[0].samplerUniform.blockBinCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            rayBlockBuffer = createBuffer(device, strided<glm::uvec4>(blockLimit * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // wide index spaces
            currentBlocks = createBuffer(device, strided<uint32_t>(blockLimit), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            preparingBlocks = createBuffer(device, strided<uint32_t>(blockLimit), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            availableBlocks = createBuffer(device, strided<uint32_t>(blockLimit), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            clearingBlocks = createBuffer(device, strided<uint32_t>(blockLimit), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            unorderedTempBuffer = createBuffer(device, strided<glm::u64vec4>(blockLimit * BLOCK_SIZE), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // caches
            traverseBlockData = createBuffer(device, TRAVERSE_BLOCK_SIZE * INTENSIVITY, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            traverseCacheData = createBuffer(device, TRAVERSE_CACHE_SIZE * 64 * INTENSIVITY, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

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
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(16).setPBufferInfo(&traverseCacheData->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(17).setPBufferInfo(&traverseBlockData->descriptorInfo),
            }, nullptr);

            // init swaps
            indicesSwap[0] = currentBlocks, indicesSwap[1] = preparingBlocks, availableSwap[0] = availableBlocks, availableSwap[1] = clearingBlocks;
        }

        void Pipeline::clearSampling()
        {
            sequenceId = randm(2147483647); // regenerate starting sequence
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
            auto copyCommand = getCommandBuffer(device, true);
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
            if (doCleanSamples) dispatchCompute(clearSamples, INTENSIVITY, samplingDescriptors);
            dispatchCompute(sampleCollection, INTENSIVITY, samplingDescriptors);
            flushCommandBuffer(device, copyCommand, true);

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
            auto copyCommandBuffer = getCommandBuffer(device, true);
            bufferSubData(copyCommandBuffer, rayBlockUniform.staging, rayBlockData, 0);
            memoryCopyCmd(copyCommandBuffer, rayBlockUniform.staging, rayBlockUniform.buffer, { fft, fft, sizeof(uint32_t) });                     // don't touch criticals
            memoryCopyCmd(copyCommandBuffer, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(UNORDERED_COUNTER), sizeof(uint32_t) }); // don't touch criticals

            // flush commands
            flushCommandBuffer(device, copyCommandBuffer, true);
            dispatchCompute(rayShadePipeline, INTENSIVITY, rayTracingDescriptors);

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
            for (int i = 0; i < rayStreamsData.size(); i++)
            {
                rayStreamsData[i].diffuseStream = glm::vec4(glm::sphericalRand(1.f), 0.f);
                rayStreamsData[i].superseed = glm::u64vec4(rand64u(), rand64u(), rand64u(), rand64u());
                rayStreamsData[i].frand4 = glm::vec4(randf(), randf(), randf(), randf());
            }
            sequenceId = (sequenceId + 1) % 2147483647;

            // precalculate light center
            for (int i = 0; i < lightUniformData.size(); i++)
            {
                glm::vec3 playerCenter = glm::vec3(0.0f);
                glm::vec3 lvec = glm::normalize(lightUniformData[i].lightVector.xyz()) * (lightUniformData[i].lightVector.y < 0.0f ? -1.0f : 1.0f);
                glm::vec3 lightCenter = glm::fma(lvec, glm::vec3(lightUniformData[i].lightVector.w), (lightUniformData[i].lightOffset.xyz() + playerCenter));
                lightUniformData[i].lightRandomizedOrigin = glm::vec4(glm::sphericalRand(lightUniformData[i].lightColor.w - 0.0001f) + lightCenter, 1.f);
            }

            //rayBlockData[0].materialUniform.time = randm();
            rayBlockData[0].cameraUniform.ftime = float((milliseconds() - starttime) / (1000.0));
            rayBlockData[0].cameraUniform.prevCamInv = rayBlockData[0].cameraUniform.camInv;
            syncUniforms();

            // shorter dispatcher of generation
            dispatchCompute(rayGeneration, INTENSIVITY, rayTracingDescriptors);

            // refine rays
            reloadQueuedRays();
        }

        void Pipeline::setMaterialSet(std::shared_ptr<MaterialSet> &materialSet)
        {
            if (!materialSet->haveMaterials()) return;
            boundMaterialSet = materialSet;
            materialSet->loadToVGA();

            auto materialBuffer = materialSet->getMaterialBuffer();
            auto vtextureBuffer = materialSet->getVTextureBuffer();

            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>
            {
                vk::WriteDescriptorSet()
                    .setDstSet(surfaceDescriptors[2])
                    .setDstBinding(2)
                    .setDstArrayElement(0)
                    .setDescriptorCount(1)
                    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                    .setPBufferInfo(&materialBuffer->descriptorInfo),
                    
               vk::WriteDescriptorSet()
                    .setDstSet(surfaceDescriptors[2])
                    .setDstBinding(3)
                    .setDstArrayElement(0)
                    .setDescriptorCount(1)
                    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                    .setPBufferInfo(&vtextureBuffer->descriptorInfo)
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
                    .setDstSet(surfaceDescriptors[2])
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
                    .setDstSet(surfaceDescriptors[2])
                    .setDstBinding(0)
                    .setDstArrayElement(0)
                    .setDescriptorCount(images.size())
                    .setDescriptorType(vk::DescriptorType::eSampledImage)
                    .setPImageInfo(images.data())
            }, nullptr);
        }


        void Pipeline::setHierarchyStorage(std::shared_ptr<HieararchyStorage> &hierarchy) {
            rayTraverseDescriptors[1] = hierarchy->getClientDescriptorSet();
            surfaceDescriptors[1] = rayTraverseDescriptors[1];
        }

        void Pipeline::traverse() {
            if (!rayTraverseDescriptors[1]) return; // no valid geometry or hierarchy

            // copy to surfaces
            auto copyCommand = getCommandBuffer(device, true);
            memoryCopyCmd(copyCommand, this->boundMaterialSet->getCountBuffer(), rayBlockUniform.buffer, { 0, offsetof(RayBlockUniform, materialUniform) + offsetof(MaterialUniformStruct, materialOffset), sizeof(int32_t) * 2 });
            if (!hitCountGot)
            {
                memoryCopyCmd(copyCommand, countersBuffer, rayBlockUniform.buffer, { strided<uint32_t>(HIT_COUNTER), offsetof(RayBlockUniform, samplerUniform) + offsetof(SamplerUniformStruct, hitCount), sizeof(uint32_t) });
                memoryCopyCmd(copyCommand, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(HIT_COUNTER), sizeof(uint32_t) });
            }

            // push commands
            dispatchCompute(unorderedFormer, INTENSIVITY, rayTracingDescriptors);
            dispatchCompute(bvhTraverse, INTENSIVITY, rayTraverseDescriptors);
            flushCommandBuffer(device, copyCommand, true);
            dispatchCompute(surfaceShadingPpl, INTENSIVITY, surfaceDescriptors);
        }

        void Pipeline::enable360mode(bool mode) { rayBlockData[0].cameraUniform.enable360 = mode; clearSampling(); }
        size_t Pipeline::getRayCount() { return rayBlockData[0].samplerUniform.blockCount; }
        uint32_t Pipeline::getCanvasWidth() { return canvasWidth; }
        uint32_t Pipeline::getCanvasHeight() { return canvasHeight; }

        TextureType &Pipeline::getRawImage() { return accumulationImage; }
        TextureType &Pipeline::getFilteredImage() { return filteredImage; }
        TextureType &Pipeline::getNormalImage() { return normalImage; }
        TextureType &Pipeline::getAlbedoImage() { return albedoImage; }

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