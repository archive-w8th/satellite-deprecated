//#pragma once

#include "../../utils.hpp"
#include "../pipeline.hpp"

namespace NSM {
    namespace rt {

        void Pipeline::syncUniforms() {


            auto command = getCommandBuffer(device, true);
            bufferSubData(command, rayBlockUniform.staging, rayBlockData, 0);
            bufferSubData(command, lightUniform.staging, lightUniformData, 0);
            bufferSubData(command, rayStreamsUniform.staging, rayStreamsData, 0);
            memoryCopyCmd(command, rayBlockUniform.staging, rayBlockUniform.buffer, { 0, 0, strided<RayBlockUniform>(1) });
            memoryCopyCmd(command, lightUniform.staging, lightUniform.buffer, { 0, 0, strided<LightUniformStruct>(lightUniformData.size()) });
            memoryCopyCmd(command, rayStreamsUniform.staging, rayStreamsUniform.buffer, { 0, 0, strided<RayStream>(rayStreamsData.size()) });
            flushCommandBuffer(device, command, true);
        }

        void Pipeline::initDescriptorSets() {

            // descriptor set connectors for externals
            std::vector<vk::DescriptorSetLayoutBinding> clientDescriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // vertex
                vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // normal
                vk::DescriptorSetLayoutBinding(12, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // texcoord
                vk::DescriptorSetLayoutBinding(13, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // modifiers
                vk::DescriptorSetLayoutBinding(14, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // reserved (may colors)

                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // BVH boxes
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // materials
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // orders
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // geometryUniform

                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // BVH metadata
                vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // reserved 
            };

            // ray tracing unified descriptors
            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                // raytracing binding set
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // ray block nodes
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // ray blocks meta
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // active blocks
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // preparing blocks
                vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // available for write blocks
                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // preparing for rewrite blocks 
                vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // texels
                vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // block bins
                vk::DescriptorSetLayoutBinding(8, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // counters 
                vk::DescriptorSetLayoutBinding(9, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // hits buffer
                vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // unordered indexing

                // surface binding set 
                //vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // surface material set

                // uniform set
                vk::DescriptorSetLayoutBinding(12, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // LightUniform
                vk::DescriptorSetLayoutBinding(13, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // RayBlockUniform
                vk::DescriptorSetLayoutBinding(14, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // StreamsBlockUniform

                // BVH cache binding set 
                vk::DescriptorSetLayoutBinding(16, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(17, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr)
            };

            // images for sampling
            std::vector<vk::DescriptorSetLayoutBinding> sampleImgSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // sampling image buffer
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // filtered sampled
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // previous sampled image
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // clear samples flags
                vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // normal buffer
                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // albedo buffer
            };

            // textures for surfaces 
            std::vector<vk::DescriptorSetLayoutBinding> surfaceImgSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eSampledImage, MAX_SURFACE_IMAGES, vk::ShaderStageFlagBits::eCompute, nullptr), // textures 
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eSampler, 16, vk::ShaderStageFlagBits::eCompute, nullptr), // samplers for textures
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr)
            };

            // ray shading environment 
            std::vector<vk::DescriptorSetLayoutBinding> shadingDescSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // environment map
            };


            // prepare pools and sets
            std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = {
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(descriptorSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(clientDescriptorSetLayoutBindings.data()).setBindingCount(clientDescriptorSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(sampleImgSetLayoutBindings.data()).setBindingCount(sampleImgSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(surfaceImgSetLayoutBindings.data()).setBindingCount(surfaceImgSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(shadingDescSetLayoutBindings.data()).setBindingCount(shadingDescSetLayoutBindings.size()))
            };
            auto descriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(descriptorSetLayouts.size()).setPSetLayouts(descriptorSetLayouts.data()));

            // layouts
            rayTracingDescriptorsLayout = { descriptorSetLayouts[0] };
            rayTraverseDescriptorsLayout = { descriptorSetLayouts[0], descriptorSetLayouts[1] };
            samplingDescriptorsLayout = { descriptorSetLayouts[0], descriptorSetLayouts[2] };
            surfaceDescriptorsLayout = { descriptorSetLayouts[0], descriptorSetLayouts[1], descriptorSetLayouts[3] };
            rayShadingDescriptorsLayout = { descriptorSetLayouts[0], descriptorSetLayouts[4] };

            // descriptors
            rayTracingDescriptors = { descriptorSets[0] };
            //rayTraverseDescriptors = { descriptorSets[0], descriptorSets[1] };
            samplingDescriptors = { descriptorSets[0], descriptorSets[2] };
            surfaceDescriptors = { descriptorSets[0], nullptr, descriptorSets[3] };
            rayShadingDescriptors = { descriptorSets[0], descriptorSets[4] };

            // create staging buffer
            generalStagingBuffer = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            generalLoadingBuffer = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_TO_CPU);
        }

        void Pipeline::initPipelines() {
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
            //vertexInterp.pipeline = createCompute(device, shadersPathPrefix + "/rendering/vertex-interp.comp.spv", rayTraversePipelineLayout, pipelineCache);

            /*
            { // AMD ONLY - save assembly file of traverser
                PFN_vkGetShaderInfoAMD pfnGetShaderInfoAMD = (PFN_vkGetShaderInfoAMD)vkGetDeviceProcAddr(device->logical, "vkGetShaderInfoAMD");

                // query disassembly size (if available)
                size_t dataSize = 0;
                //device->logical.getShaderInfoAMD(bvhTraverse.pipeline, vk::ShaderStageFlagBits::eCompute, vk::ShaderInfoTypeAMD::eDisassembly, &dataSize, nullptr);
                VkResult res = pfnGetShaderInfoAMD(VkDevice(device->logical), VkPipeline(bvhTraverse.pipeline), VkShaderStageFlagBits(vk::ShaderStageFlagBits::eCompute), VkShaderInfoTypeAMD(vk::ShaderInfoTypeAMD::eDisassembly), &dataSize, nullptr);

                // query disassembly and print
                if (dataSize > 0) {
                    std::cout << "BVH traverse dissasembly" << std::endl;
                    char * disassembly = new char[dataSize];
                    //device->logical.getShaderInfoAMD(bvhTraverse.pipeline, vk::ShaderStageFlagBits::eCompute, vk::ShaderInfoTypeAMD::eDisassembly, &dataSize, disassembly);
                    res = pfnGetShaderInfoAMD(VkDevice(device->logical), VkPipeline(bvhTraverse.pipeline), VkShaderStageFlagBits(vk::ShaderStageFlagBits::eCompute), VkShaderInfoTypeAMD(vk::ShaderInfoTypeAMD::eDisassembly), &dataSize, disassembly);
                    //std::cout << std::string(disassembly, disassembly + dataSize) << std::endl;

                    std::ofstream assemblyAMD;
                    assemblyAMD.open("bvhTraverseDisAMD.gasm");
                    assemblyAMD << std::string(disassembly, disassembly + dataSize);
                    assemblyAMD.close();

                    delete disassembly;
                }
            }
            */
        }

        void Pipeline::initLights() {
            lightUniformData.resize(6);
            for (int i = 0; i < 6; i++) {
                lightUniformData[i].lightColor = glm::vec4((glm::vec3(255.f, 250.f, 244.f) / 255.f) * 400.f, 40.0f);
                lightUniformData[i].lightVector = glm::vec4(0.3f, 1.0f, 0.1f, 400.0f).xzyw()*glm::vec4(1.f, -1.f, 1.f, 1.f);
                lightUniformData[i].lightOffset = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f).xzyw()*glm::vec4(1.f, -1.f, 1.f, 1.f);
                lightUniformData[i].lightAmbient = glm::vec4(0.0f);
            }
        }

        void Pipeline::reloadQueuedRays() {

            // collect colors to texels
            auto commandBuffer = getCommandBuffer(device, true);
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, rayShadingPipelineLayout, 0, rayShadingDescriptors, nullptr);
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, binCollect.pipeline);
            commandBuffer.dispatch(INTENSIVITY, 1, 1);
            flushCommandBuffer(device, commandBuffer, true);

            // getting counters values
            std::vector<int32_t> counters(8);
            auto copyCmd = createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, countersBuffer, generalLoadingBuffer, { 0, 0, strided<uint32_t>(8) }); flushCommandBuffer(device, copyCmd, false);
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
            if (rayBlockData[0].samplerUniform.blockCount > 0) {
                cmds.push_back(createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, indicesSwap[1], indicesSwap[0], { 0, 0, strided<uint32_t>(rayBlockData[0].samplerUniform.blockCount) }));
            }
            if (includeCount > 0) {
                cmds.push_back(createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, availableSwap[1], availableSwap[0], { 0, strided<uint32_t>(additionalOffset), strided<uint32_t>(includeCount) })); // move unused to preparing
            }
            flushCommandBuffers(device, cmds, true);
        }

        void Pipeline::initBuffers() {
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
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(14).setPBufferInfo(&rayStreamsUniform.buffer->descriptorInfo)
            }, nullptr);

            // null envmap
            {
                auto sampler = device->logical.createSampler(vk::SamplerCreateInfo().setAddressModeU(vk::SamplerAddressMode::eRepeat).setAddressModeV(vk::SamplerAddressMode::eClampToEdge).setMinFilter(vk::Filter::eLinear).setMagFilter(vk::Filter::eLinear).setCompareEnable(false));
                auto texture = createTexture(device, vk::ImageViewType::e2D, { 2, 2, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat, 1);
                auto tstage = createBuffer(device, 4 * sizeof(glm::vec4), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // purple-black square
                {
                    auto command = getCommandBuffer(device, true);
                    bufferSubData(command, tstage, std::vector<glm::vec4>({
                        glm::vec4(0.8f, 0.9f, 1.0f, 1.0f), glm::vec4(0.8f, 0.9f, 1.0f, 1.0f),
                        glm::vec4(0.8f, 0.9f, 1.0f, 1.0f), glm::vec4(0.8f, 0.9f, 1.0f, 1.0f)
                        }));
                    memoryCopyCmd(command, tstage, texture, vk::BufferImageCopy()
                        .setImageExtent({ 2, 2, 1 })
                        .setImageOffset({ 0, 0, 0 })
                        .setBufferOffset(0)
                        .setBufferRowLength(2)
                        .setBufferImageHeight(2)
                        .setImageSubresource(texture->subresourceLayers));
                    flushCommandBuffer(device, command, [&]() { destroyBuffer(tstage); });
                }

                // update descriptors
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet> {
                    vk::WriteDescriptorSet()
                        .setDstSet(rayShadingDescriptors[1])
                        .setDstBinding(0)
                        .setDstArrayElement(0)
                        .setDescriptorCount(1)
                        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                        .setPImageInfo(&vk::DescriptorImageInfo().setImageLayout(texture->layout).setImageView(texture->view).setSampler(sampler))
                }, nullptr);
            }

            // null texture define
            {
                // create texture 
                auto texture = createTexture(device, vk::ImageViewType::e2D, { 2, 2, 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat, 1, VMA_MEMORY_USAGE_GPU_ONLY);
                auto tstage = createBuffer(device, 4 * sizeof(glm::vec4), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageTexelBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // purple-black square
                {
                    auto command = getCommandBuffer(device, true);
                    bufferSubData(command, tstage, std::vector<glm::vec4>({
                        glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), glm::vec4(1.0f, 0.5f, 0.5f, 1.0f),
                        glm::vec4(1.0f, 0.5f, 0.5f, 1.0f), glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)
                        }));
                    memoryCopyCmd(command, tstage, texture, vk::BufferImageCopy()
                        .setImageExtent({ 2, 2, 1 })
                        .setImageOffset({ 0, 0, 0 })
                        .setBufferOffset(0)
                        .setBufferRowLength(2)
                        .setBufferImageHeight(2)
                        .setImageSubresource(texture->subresourceLayers));
                    flushCommandBuffer(device, command, [&]() { destroyBuffer(tstage); });
                }

                // write with same images
                std::vector<vk::DescriptorImageInfo> imageDescs;
                for (int i = 0; i < MAX_SURFACE_IMAGES; i++) {
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
                        .setPImageInfo(imageDescs.data())
                }, nullptr);
            }

            // null sampler define
            {
                // create sampler
                auto sampler = device->logical.createSampler(vk::SamplerCreateInfo().setAddressModeU(vk::SamplerAddressMode::eRepeat).setAddressModeV(vk::SamplerAddressMode::eRepeat).setMinFilter(vk::Filter::eLinear).setMagFilter(vk::Filter::eLinear).setCompareEnable(false));

                // write with same images
                std::vector<vk::DescriptorImageInfo> samplerDescs;
                for (int i = 0; i < 16; i++) {
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
                        .setPImageInfo(samplerDescs.data())
                }, nullptr);
            }

            // preinit uniforms
            rayBlockData[0].materialUniform.lightcount = 1;
            rayBlockData[0].cameraUniform.enable360 = 0;
            clearRays();
        }

        void Pipeline::init(DeviceQueueType& device) {
            this->device = device;
            rayBlockData.resize(1);
            starttime = milliseconds();

            initDescriptorSets();
            initPipelines();
            initBuffers();
            initLights();
            syncUniforms();
        }


        void Pipeline::clearRays() {
            rayBlockData[0].samplerUniform.iterationCount = 0;
            flushCommandBuffer(device, createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, 0, strided<uint32_t>(16) }), true);
        }

        void Pipeline::resizeCanvas(uint32_t width, uint32_t height) {
            const double superCanvas = 1;
            canvasWidth = width * superCanvas, canvasHeight = height * superCanvas;

            device->logical.waitIdle();
            destroyTexture(accumulationImage);
            destroyTexture(filteredImage);
            destroyTexture(flagsImage);
            destroyTexture(depthImage);
            destroyTexture(normalImage);
            destroyTexture(albedoImage);

            accumulationImage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(canvasWidth), uint32_t(canvasHeight * 2), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            filteredImage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(canvasWidth), uint32_t(canvasHeight * 2), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            flagsImage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(canvasWidth), uint32_t(canvasHeight * 2), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32Sint);
            depthImage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(canvasWidth), uint32_t(canvasHeight * 2), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            normalImage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(canvasWidth), uint32_t(canvasHeight), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            albedoImage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(canvasWidth), uint32_t(canvasHeight), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);

            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(samplingDescriptors[1]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageImage);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(0).setPImageInfo(&accumulationImage->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(1).setPImageInfo(&filteredImage->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(2).setPImageInfo(&flagsImage->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(3).setPImageInfo(&depthImage->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(4).setPImageInfo(&normalImage->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(5).setPImageInfo(&albedoImage->descriptorInfo),
            }, nullptr);

            clearSampling();
            syncUniforms();
        }

        void Pipeline::setSkybox(TextureType& skybox) {
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(rayShadingDescriptors[1]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(0).setPImageInfo(&skybox->descriptorInfo)
            }, nullptr);
        }

        void Pipeline::reallocRays(uint32_t width, uint32_t height) {
            const bool IS_INTERLACED = false;

            //const size_t BLOCK_WIDTH = 16, BLOCK_HEIGHT = 16;
            const size_t BLOCK_WIDTH = 32, BLOCK_HEIGHT = 32;
            const size_t BLOCK_SIZE = BLOCK_WIDTH * BLOCK_HEIGHT;
            const size_t BLOCK_NODES_SIZE = 64 * BLOCK_SIZE;
            const size_t BLOCK_INDICES_SIZE = 8 * BLOCK_SIZE + 64;
            const size_t BLOCK_BIN_SIZE = 4 * BLOCK_SIZE + 64;
            const size_t TRAVERSE_CACHE_SIZE = 1024 * 256;
            const size_t TRAVERSE_BLOCK_SIZE = 2048 + 64;
            const size_t ALLOC_MULT = 8;


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
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(10).setPBufferInfo(&unorderedTempBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(16).setPBufferInfo(&traverseCacheData->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(17).setPBufferInfo(&traverseBlockData->descriptorInfo)
            }, nullptr);

            // init swaps
            indicesSwap[0] = currentBlocks, indicesSwap[1] = preparingBlocks, availableSwap[0] = availableBlocks, availableSwap[1] = clearingBlocks;
        }

        void Pipeline::clearSampling() {
            sequenceId = randm(2147483647); // regenerate starting sequence
            rayBlockData[0].cameraUniform.prevCamInv = rayBlockData[0].cameraUniform.camInv;
            doCleanSamples = true;
        }

        void Pipeline::collectSamples() {

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

            copyDesc.srcSubresource = depthImage->subresourceLayers;
            copyDesc.dstSubresource = depthImage->subresourceLayers;
            memoryCopyCmd(copyCommand, depthImage, depthImage, copyDesc);

            copyDesc.srcSubresource = flagsImage->subresourceLayers;
            copyDesc.dstSubresource = flagsImage->subresourceLayers;
            memoryCopyCmd(copyCommand, flagsImage, flagsImage, copyDesc);


            copyDesc.srcOffset = { 0, 0, 0 };
            copyDesc.dstOffset = { 0, 0, 0 };
            copyDesc.extent = { uint32_t(canvasWidth), uint32_t(canvasHeight)*2, 1 };
            memoryCopyCmd(copyCommand, accumulationImage, filteredImage, copyDesc);
            



            // clear command
            vk::CommandBuffer clearCommand;
            if (doCleanSamples) {
                clearCommand = getCommandBuffer(device, true);
                clearCommand.bindDescriptorSets(vk::PipelineBindPoint::eCompute, samplingPipelineLayout, 0, samplingDescriptors, nullptr);
                clearCommand.bindPipeline(vk::PipelineBindPoint::eCompute, clearSamples.pipeline);
                clearCommand.dispatch(INTENSIVITY, 1, 1);
            }

            // collect command
            auto commandBuffer = getCommandBuffer(device, true);
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, samplingPipelineLayout, 0, samplingDescriptors, nullptr);
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, sampleCollection.pipeline);
            commandBuffer.dispatch(INTENSIVITY, 1, 1);

            // execute commands
            if (doCleanSamples) flushCommandBuffer(device, clearCommand, true);
            flushCommandBuffer(device, commandBuffer, true);
            flushCommandBuffer(device, copyCommand, true);

            // unflag
            doCleanSamples = false;
        }

        void Pipeline::rayShading() {
            // update iteration counter locally
            //rayBlockData[0].materialUniform.time = randm();
            rayBlockData[0].samplerUniform.iterationCount++;
            auto fft = offsetof(RayBlockUniform, samplerUniform) + offsetof(SamplerUniformStruct, iterationCount); // choice update target offset
            //auto rft = offsetof(RayBlockUniform, materialUniform) + offsetof(MaterialUniformStruct, time); // choice update target offset

            // copy commands 
            auto copyCommandBuffer = getCommandBuffer(device, true);
            bufferSubData(copyCommandBuffer, rayBlockUniform.staging, rayBlockData, 0);
            memoryCopyCmd(copyCommandBuffer, rayBlockUniform.staging, rayBlockUniform.buffer, { fft, fft, sizeof(uint32_t) }); // don't touch criticals
            memoryCopyCmd(copyCommandBuffer, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(UNORDERED_COUNTER), sizeof(uint32_t) }); // don't touch criticals

            // shade commands
            auto shadeCommandBuffer = getCommandBuffer(device, true);
            shadeCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, rayShadingPipelineLayout, 0, rayShadingDescriptors, nullptr);
            shadeCommandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, rayShadePipeline.pipeline);
            shadeCommandBuffer.dispatch(INTENSIVITY, 1, 1);

            // flush commands
            flushCommandBuffer(device, copyCommandBuffer, true);
            flushCommandBuffer(device, shadeCommandBuffer, true);

            // refine rays
            reloadQueuedRays();
        }

        void Pipeline::generate(const glm::mat4 &persp, const glm::mat4 &frontSide) {
            clearRays();

            const size_t num_seeds = 16;
            rayStreamsData.resize(num_seeds);
            for (int i = 0; i < rayStreamsData.size(); i++) {
                rayStreamsData[i].diffuseStream = glm::vec4(glm::sphericalRand(1.f), 0.f);
                rayStreamsData[i].superseed = glm::ivec4(sequenceId + i, sequenceId + i + num_seeds * 1, sequenceId + i + num_seeds * 2, sequenceId + i + num_seeds * 3);
                rayStreamsData[i].frand4 = glm::vec4(randf(), randf(), randf(), randf());
            }
            sequenceId = (sequenceId + 1) % 2147483647;

            // precalculate light center
            for (int i = 0; i < lightUniformData.size(); i++) {
                glm::vec3 playerCenter = glm::vec3(0.0f);
                glm::vec3 lvec = glm::normalize(lightUniformData[i].lightVector.xyz()) * (lightUniformData[i].lightVector.y < 0.0f ? -1.0f : 1.0f);
                glm::vec3 lightCenter = glm::fma(lvec, glm::vec3(lightUniformData[i].lightVector.w), (lightUniformData[i].lightOffset.xyz() + playerCenter));
                lightUniformData[i].lightRandomizedOrigin = glm::vec4(glm::sphericalRand(lightUniformData[i].lightColor.w - 0.0001f) + lightCenter, 1.f);
            }


            //rayBlockData[0].materialUniform.time = randm();
            rayBlockData[0].cameraUniform.ftime = float((milliseconds() - starttime) / (1000.0));
            rayBlockData[0].cameraUniform.prevCamInv = rayBlockData[0].cameraUniform.camInv;
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

        void Pipeline::setMaterialSet(std::shared_ptr<MaterialSet>& materialSet) {
            if (!materialSet->haveMaterials()) return;
            boundMaterialSet = materialSet;

            auto materialBuffer = materialSet->getMaterialBuffer();
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet()
                    .setDstSet(surfaceDescriptors[2])
                    .setDstBinding(2)
                    .setDstArrayElement(0)
                    .setDescriptorCount(1)
                    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                    .setPBufferInfo(&materialBuffer->descriptorInfo)
            }, nullptr);
        }

        void Pipeline::setTextureSet(std::shared_ptr<TextureSet>& textureSet) {
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
                    .setDstSet(surfaceDescriptors[2])
                    .setDstBinding(0)
                    .setDstArrayElement(0)
                    .setDescriptorCount(images.size())
                    .setDescriptorType(vk::DescriptorType::eSampledImage)
                    .setPImageInfo(images.data())
            }, nullptr);
        }

        void Pipeline::traverse(std::shared_ptr<TriangleHierarchy>& hierarchy) {
            auto vdescs = hierarchy->getClientDescriptorSet();

            // traverse BVH
            auto commandBuffer = getCommandBuffer(device, true);
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, rayTraversePipelineLayout, 0, { rayTracingDescriptors[0], vdescs }, nullptr);
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, bvhTraverse.pipeline);
            commandBuffer.dispatch(INTENSIVITY, 1, 1);

            // copy to surfaces
            auto copyCommand = getCommandBuffer(device, true);
            memoryCopyCmd(copyCommand, this->boundMaterialSet->getCountBuffer(), rayBlockUniform.buffer, { 0, offsetof(RayBlockUniform, materialUniform) + offsetof(MaterialUniformStruct, materialOffset), sizeof(int32_t) * 2 });
            if (!hitCountGot) {
                memoryCopyCmd(copyCommand, countersBuffer, rayBlockUniform.buffer, { strided<uint32_t>(HIT_COUNTER), offsetof(RayBlockUniform, samplerUniform) + offsetof(SamplerUniformStruct, hitCount), sizeof(uint32_t) });
                memoryCopyCmd(copyCommand, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(HIT_COUNTER), sizeof(uint32_t) });
            }

            // surface shading command 
            auto srfCommandBuffer = getCommandBuffer(device, true);
            srfCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, surfacePipelineLayout, 0, { surfaceDescriptors[0], vdescs, surfaceDescriptors[2] }, nullptr);
            srfCommandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, surfaceShadingPpl.pipeline);
            srfCommandBuffer.dispatch(INTENSIVITY, 1, 1);

            // push commands
            flushCommandBuffer(device, commandBuffer, true);
            flushCommandBuffer(device, copyCommand, true);
            flushCommandBuffer(device, srfCommandBuffer, true);
        }

        uint32_t Pipeline::getCanvasWidth() { return canvasWidth; }
        uint32_t Pipeline::getCanvasHeight() { return canvasHeight; }

        TextureType& Pipeline::getRawImage() { return accumulationImage; }
        TextureType& Pipeline::getFilteredImage() { return filteredImage; }
        TextureType& Pipeline::getNormalImage() { return normalImage; }
        TextureType& Pipeline::getAlbedoImage() { return albedoImage; }

        // panorama mode
        void Pipeline::enable360mode(bool mode) { rayBlockData[0].cameraUniform.enable360 = mode; clearSampling(); }
        size_t Pipeline::getRayCount() { return rayBlockData[0].samplerUniform.blockCount; }

    }
}