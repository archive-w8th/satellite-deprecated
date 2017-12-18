#pragma once

#include "../triangleHierarchy.hpp"

namespace NSM {
    namespace rt {

        void TriangleHierarchy::init(DeviceQueueType& _device) {
            this->device = _device;

            // create radix sort (planned decicated sorter)
            radixSort = std::shared_ptr<RadixSort>(new RadixSort(device, shadersPathPrefix));


            // descriptor set bindings
            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), 
                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // refit flags 
                vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(8, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(9, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // minmax buffer
                vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // BVH creator uniform (TODO)
                vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr)
            };



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



            // descriptor set for loaders
            std::vector<vk::DescriptorSetLayoutBinding> loaderDescriptorSetBindings = {
                // vertex inputs 
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // counters
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // buffer data space
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // buffer regions
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // buffer views
                vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // data formats
                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // data bindings (with buffer views)
                vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // mesh uniforms 

                // write buffers 
                vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // material buffer
                vk::DescriptorSetLayoutBinding(8, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // order buffer

                // write images with vertex data
                vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // vertex
                vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // normal
                vk::DescriptorSetLayoutBinding(12, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // texcoord
                vk::DescriptorSetLayoutBinding(13, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // modifiers
                vk::DescriptorSetLayoutBinding(14, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // reserved (may colors)
            };


            // layouts of descriptor sets 
            descriptorSetLayouts = {
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(descriptorSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(clientDescriptorSetLayoutBindings.data()).setBindingCount(clientDescriptorSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(loaderDescriptorSetBindings.data()).setBindingCount(loaderDescriptorSetBindings.size())),
            };


            // descriptor sets for BVH builders
            pipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(&descriptorSetLayouts[0]).setSetLayoutCount(2));
            
            // pipeline layout for vertex loader
            loaderPipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(&descriptorSetLayouts[2]).setSetLayoutCount(1));

            // descriptor sets
            descriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(1).setPSetLayouts(&descriptorSetLayouts[0]));
            clientDescriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(1).setPSetLayouts(&descriptorSetLayouts[2]));
            loaderDescriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(1).setPSetLayouts(&descriptorSetLayouts[1]));
            

            // pipelines
            pipelineCache = device->logical.createPipelineCache(vk::PipelineCacheCreateInfo());
            buildBVHPpl.pipeline = createCompute(device, shadersPathPrefix + "/hlbvh/build-new.comp.spv", pipelineLayout, pipelineCache);
            refitBVH.pipeline = createCompute(device, shadersPathPrefix + "/hlbvh/refit.comp.spv", pipelineLayout, pipelineCache);
            boundPrimitives.pipeline = createCompute(device, shadersPathPrefix + "/hlbvh/minmax.comp.spv", pipelineLayout, pipelineCache);
            childLink.pipeline = createCompute(device, shadersPathPrefix + "/hlbvh/child-link.comp.spv", pipelineLayout, pipelineCache);
            aabbCalculate.pipeline = createCompute(device, shadersPathPrefix + "/hlbvh/aabbmaker.comp.spv", pipelineLayout, pipelineCache);
            geometryLoader.pipeline = createCompute(device, shadersPathPrefix + "/vertex/loader.comp.spv", loaderPipelineLayout, pipelineCache);
            geometryLoader16bit.pipeline = createCompute(device, shadersPathPrefix + "/vertex/loader-int16.comp.spv", loaderPipelineLayout, pipelineCache);



            // build bvh command
            buildBVHPpl.dispatch = [&]() {
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, { descriptorSets[0], clientDescriptorSets[0] }, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, buildBVHPpl.pipeline);
                commandBuffer.dispatch(INTENSIVITY, 1, 1); // dispatch few counts
                flushCommandBuffer(device, commandBuffer, true);
            };

            // build global boundary
            boundPrimitives.dispatch = [&]() {
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, { descriptorSets[0], clientDescriptorSets[0] }, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, boundPrimitives.pipeline);
                commandBuffer.dispatch(32, 1, 1);
                flushCommandBuffer(device, commandBuffer, true);
            };

            // link childrens
            childLink.dispatch = [&]() {
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, { descriptorSets[0], clientDescriptorSets[0] }, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, childLink.pipeline);
                commandBuffer.dispatch(INTENSIVITY, 1, 1);
                flushCommandBuffer(device, commandBuffer, true);
            };

            // refit BVH
            refitBVH.dispatch = [&]() {
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, { descriptorSets[0], clientDescriptorSets[0] }, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, refitBVH.pipeline);
                commandBuffer.dispatch(INTENSIVITY, 1, 1);
                flushCommandBuffer(device, commandBuffer, true);
            };

            // dispatch aabb per nodes
            aabbCalculate.dispatch = [&]() {
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, { descriptorSets[0], clientDescriptorSets[0] }, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, aabbCalculate.pipeline);
                commandBuffer.dispatch(tiled(triangleCount, WORK_SIZE), 1, 1);
                flushCommandBuffer(device, commandBuffer, true);
            };

            // loading geometry command 
            geometryLoader.dispatch = [&]() {
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, loaderPipelineLayout, 0, loaderDescriptorSets, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, geometryLoader.pipeline);
                commandBuffer.dispatch(tiled(workingTriangleCount, WORK_SIZE), 1, 1);
                flushCommandBuffer(device, commandBuffer, true);
            };

            geometryLoader16bit.dispatch = [&]() {
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, loaderPipelineLayout, 0, loaderDescriptorSets, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, geometryLoader16bit.pipeline);
                commandBuffer.dispatch(tiled(workingTriangleCount, WORK_SIZE), 1, 1);
                flushCommandBuffer(device, commandBuffer, true);
            };


            // recommended alloc 256Mb for all staging
            // but here can be used 4Kb
            generalStagingBuffer = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            generalLoadingBuffer = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_TO_CPU);



            // boundary buffer cache
            boundaryBuffer = createBuffer(device, strided<glm::vec4>(64), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            countersBuffer = createBuffer(device, strided<uint32_t>(8), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            boundaryBufferReference = createBuffer(device, strided<glm::vec4>(64), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            zerosBufferReference = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            debugOnes32BufferReference = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);



            // minmaxes
            std::vector<glm::vec4> minmaxes(64);
            for (int i = 0; i < 32;i++) {
                minmaxes[i * 2 + 0] = glm::vec4( 10000.f);
                minmaxes[i * 2 + 1] = glm::vec4(-10000.f);
            }

            // zeros
            std::vector<uint32_t> zeros(1024);
            std::vector<uint32_t> ones(1024);
            for (int i = 0; i < 1024; i++) {
                zeros[i] = 0;
                ones[i] = 1;
            }

            // make reference buffers
            bufferSubData(boundaryBufferReference, minmaxes, 0); // make reference buffer of boundary
            bufferSubData(zerosBufferReference, zeros, 0); // make reference of zeros
            bufferSubData(debugOnes32BufferReference, ones, 0);

            // create client geometry uniform buffer
            geometryBlockData = std::vector<GeometryBlockUniform>(1);
            geometryBlockUniform.buffer = createBuffer(device, strided<GeometryBlockUniform>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            geometryBlockUniform.staging = createBuffer(device, strided<GeometryBlockUniform>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

            // descriptor templates
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            auto ldesc0Tmpl = vk::WriteDescriptorSet(desc0Tmpl).setDstSet(loaderDescriptorSets[0]).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            auto bvhCounters = vk::DescriptorBufferInfo(countersBuffer->buffer, 0, strided<uint32_t>(8));

            // write buffers to main descriptors
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(8).setPBufferInfo(&bvhCounters),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(9).setPBufferInfo(&boundaryBuffer->descriptorInfo),
            }, nullptr);

            // write buffers to loader descriptors
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(ldesc0Tmpl).setDstBinding(0).setPBufferInfo(&bvhCounters)
            }, nullptr);

            // clear for fix issues
            clearTribuffer();
        }

        void TriangleHierarchy::allocate(size_t maxt) {
            maxTriangles = maxt;

            // allocate these buffers
            //bvhNodesBuffer = createBuffer(device, strided<HlbvhNode>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            bvhNodesBuffer = createBuffer(device, strided<HlbvhNode>(2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            leafsBuffer = createBuffer(device, strided<HlbvhNode>(maxTriangles * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            
            // buffer for working with
            bvhNodesFlags = createBuffer(device, strided<uint32_t>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            workingBVHNodesBuffer = createBuffer(device, strided<uint32_t>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            leafBVHIndicesBuffer = createBuffer(device, strided<uint32_t>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            leafsIndicesBuffer = createBuffer(device, strided<uint32_t>(maxTriangles * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            mortonCodesBuffer = createBuffer(device, strided<uint64_t>(maxTriangles * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // special values
            size_t _MAX_HEIGHT = std::min(maxTriangles > 0 ? (maxTriangles - 1) / WARPED_WIDTH + 1 : 0, WARPED_WIDTH) + 1;

            // storage
            modsTexelStorage = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_WIDTH), uint32_t(_MAX_HEIGHT), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            vertexTexelStorage = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_WIDTH), uint32_t(_MAX_HEIGHT), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            normalsTexelStorage = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_WIDTH), uint32_t(_MAX_HEIGHT), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            texcoordTexelStorage = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_WIDTH), uint32_t(_MAX_HEIGHT), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            materialIndicesStorage = createBuffer(device, strided<uint32_t>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // sideloader
            modsTexelWorking = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_WIDTH), uint32_t(_MAX_HEIGHT), 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat);
            vertexTexelWorking = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_WIDTH), uint32_t(_MAX_HEIGHT), 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat);
            normalsTexelWorking = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_WIDTH), uint32_t(_MAX_HEIGHT), 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat);
            texcoordTexelWorking = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_WIDTH), uint32_t(_MAX_HEIGHT), 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat);
            materialIndicesWorking = createBuffer(device, strided<uint32_t>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);


            // bvh storage (32-bits elements)
            _MAX_HEIGHT = std::min(maxTriangles > 0 ? (maxTriangles - 1) / _BVH_WIDTH + 1 : 0, _BVH_WIDTH) + 1;
            bvhMetaStorage = createTexture(device, vk::ImageType::e2D, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_BVH_WIDTH), uint32_t(_MAX_HEIGHT*2), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sint);
            bvhBoxBuffer = createBuffer(device, strided<glm::mat4>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // create sampler
            vk::SamplerCreateInfo samplerInfo;
            samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
            samplerInfo.addressModeV = vk::SamplerAddressMode::eMirrorClampToEdge;
            samplerInfo.minFilter = vk::Filter::eNearest;
            samplerInfo.magFilter = vk::Filter::eNearest;
            samplerInfo.compareEnable = false;
            auto sampler = device->logical.createSampler(samplerInfo);

            // descriptor templates
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            auto desc1Tmpl = vk::WriteDescriptorSet(desc0Tmpl).setDstSet(descriptorSets[1]).setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
            auto ldesc0Tmpl = vk::WriteDescriptorSet(desc0Tmpl).setDstSet(loaderDescriptorSets[0]).setDescriptorType(vk::DescriptorType::eStorageBuffer);

            // write buffer to bvh builder descriptors
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(0).setPBufferInfo(&mortonCodesBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(1).setPBufferInfo(&leafsIndicesBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(3).setPBufferInfo(&leafsBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(4).setPBufferInfo(&bvhBoxBuffer->descriptorInfo), // planned divide to box working 
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(5).setPBufferInfo(&bvhNodesFlags->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(6).setPBufferInfo(&workingBVHNodesBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(7).setPBufferInfo(&leafBVHIndicesBuffer->descriptorInfo),
                // planned BVH uniform block
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(11).setPImageInfo(&bvhMetaStorage->descriptorInfo.setSampler(sampler))  // planned divide to meta working 
            }, nullptr);

            // write to client descriptors 
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(10).setPImageInfo(&vertexTexelStorage->descriptorInfo.setSampler(sampler)),
                vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(11).setPImageInfo(&normalsTexelStorage->descriptorInfo.setSampler(sampler)),
                vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(12).setPImageInfo(&texcoordTexelStorage->descriptorInfo.setSampler(sampler)),
                vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(13).setPImageInfo(&modsTexelStorage->descriptorInfo.setSampler(sampler)),
                vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(0).setPBufferInfo(&bvhBoxBuffer->descriptorInfo), // planned divide to box storage 
                vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(1).setPBufferInfo(&materialIndicesStorage->descriptorInfo),
                // planned order indexing buffer read
                vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(3).setPBufferInfo(&geometryBlockUniform.buffer->descriptorInfo),
                vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(5).setPImageInfo(&bvhMetaStorage->descriptorInfo.setSampler(sampler)), // planned divide to meta storage 
            }, nullptr);

            // write buffers to loader descriptors
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(ldesc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(10).setPImageInfo(&vertexTexelWorking->descriptorInfo),
                vk::WriteDescriptorSet(ldesc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(11).setPImageInfo(&normalsTexelWorking->descriptorInfo),
                vk::WriteDescriptorSet(ldesc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(12).setPImageInfo(&texcoordTexelWorking->descriptorInfo),
                vk::WriteDescriptorSet(ldesc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(13).setPImageInfo(&modsTexelWorking->descriptorInfo),
                vk::WriteDescriptorSet(ldesc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(7).setPBufferInfo(&materialIndicesWorking->descriptorInfo)
                // planned order indexing buffer write
            }, nullptr);
        }

        void TriangleHierarchy::clearTribuffer() {
            copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(7), 1 }, true);
            isDirty = false;
        }

        void TriangleHierarchy::loadGeometry(std::shared_ptr<VertexInstance>& vertexInstance, bool use16bitIndexing) {
            BufferType bufferSpace = vertexInstance->getBufferSpaceBuffer();
            BufferType bufferRegions = vertexInstance->getBufferSpaceRegions();
            BufferType bufferViewsBuffer = vertexInstance->getBufferViewsBuffer();
            BufferType dataFormatBuffer = vertexInstance->getDataFormatBuffer();
            BufferType dataBindingBuffer = vertexInstance->getBufferBindingBuffer();
            BufferType meshUniformBuffer = vertexInstance->getUniformBuffer();
            
            // write mesh buffers to loaders descriptors
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(loaderDescriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(1).setPBufferInfo(&bufferSpace->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(2).setPBufferInfo(&bufferRegions->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(3).setPBufferInfo(&bufferViewsBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(4).setPBufferInfo(&dataFormatBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(5).setPBufferInfo(&dataBindingBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(6).setPBufferInfo(&meshUniformBuffer->descriptorInfo),
            }, nullptr);

            workingTriangleCount = vertexInstance->getNodeCount(); // not ready
            (use16bitIndexing ? geometryLoader16bit : geometryLoader).dispatch(); // dispatch loading

            isDirty = true;
        }

        void TriangleHierarchy::syncUniforms() {
            bufferSubData(geometryBlockUniform.staging, geometryBlockData, 0);
            copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, geometryBlockUniform.staging, geometryBlockUniform.buffer, { 0, 0, strided<GeometryBlockUniform>(1) }, true);

            // TODO bvh uniform 
        }

        void TriangleHierarchy::markDirty() {
            isDirty = true;
        }

        void TriangleHierarchy::buildBVH() {
            // no need to build BVH
            if (!isDirty) return;

            // copy to staging
            copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, countersBuffer, generalLoadingBuffer, { strided<uint32_t>(7), 0, strided<uint32_t>(1) }, false);

            // get triangle count from staging
            std::vector<uint32_t> triangleCount(1);
            getBufferSubData(generalLoadingBuffer, triangleCount, 0);

            this->triangleCount = triangleCount[0]; // use by 
            if (triangleCount[0] <= 0) return; // no need to build BVH

            // bvh potentially builded
            isDirty = false;

            // special values
            size_t _MAX_HEIGHT = std::min(size_t(maxTriangles) > 0 ? (size_t(maxTriangles) - 1) / WARPED_WIDTH + 1 : 0, WARPED_WIDTH) + 1;
            size_t _ALLOCATED_HEIGHT = std::min(size_t(triangleCount[0]) > 0 ? (size_t(triangleCount[0]) - 1) / WARPED_WIDTH + 1 : 0, _MAX_HEIGHT) + 1;

            // copy texel storage data
            vk::ImageCopy copyDesc;
            copyDesc.dstOffset = { 0, 0, 0 };
            copyDesc.srcOffset = { 0, 0, 0 };
            copyDesc.extent = { uint32_t(_WIDTH), uint32_t(_ALLOCATED_HEIGHT), 1 };
            copyDesc.srcSubresource = vertexTexelWorking->subresourceLayers;
            copyDesc.dstSubresource = vertexTexelStorage->subresourceLayers;

            // copy images command
            auto command = getCommandBuffer(device, true);
            memoryCopyCmd(command, modsTexelWorking, modsTexelStorage, copyDesc);
            memoryCopyCmd(command, vertexTexelWorking, vertexTexelStorage, copyDesc);
            memoryCopyCmd(command, normalsTexelWorking, normalsTexelStorage, copyDesc);
            memoryCopyCmd(command, texcoordTexelWorking, texcoordTexelStorage, copyDesc);
            memoryCopyCmd(command, materialIndicesWorking, materialIndicesStorage, { 0, 0, strided<uint32_t>(triangleCount[0]) });
            flushCommandBuffer(device, command, true);

            // use use initial matrix
            {
                glm::dmat4 mat(1.0);
                //mat *= glm::inverse(glm::dmat4(optimization));
                geometryBlockData[0].geometryUniform.transform = glm::transpose(glm::mat4(mat));
                geometryBlockData[0].geometryUniform.transformInv = glm::transpose(glm::inverse(glm::mat4(mat)));
                geometryBlockData[0].geometryUniform.triangleCount = triangleCount[0];
            }

            // calculate boundary
            copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, boundaryBufferReference, boundaryBuffer, { 0, 0, strided<glm::vec4>(64) }, true);
            syncUniforms();
            boundPrimitives.dispatch();

            // get boundary
            copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, boundaryBuffer, generalLoadingBuffer, { 0, 0, strided<glm::vec4>(64) }, false);

            // receive boundary
            std::vector<bbox> bounds(32);
            getBufferSubData(generalLoadingBuffer, bounds, 0);
            bbox bound = bounds[0];
            for (int i = 1; i < 32; i++) {
                bound.mn = glm::min(bounds[i].mn, bound.mn);
                bound.mx = glm::max(bounds[i].mx, bound.mx);
            }

            // get optimizations
            glm::vec3 scale = (bound.mx - bound.mn).xyz();
            glm::vec3 offset = bound.mn.xyz();
            {
                glm::dmat4 mat(1.0);
                mat *= glm::inverse(glm::translate(glm::dvec3(0.5)) * glm::scale(glm::dvec3(0.5)));
                mat *= glm::inverse(glm::translate(glm::dvec3(offset)) * glm::scale(glm::dvec3(scale)));
                //mat *= glm::inverse(glm::dmat4(optimization));
                geometryBlockData[0].geometryUniform.transform = glm::transpose(glm::mat4(mat));
                geometryBlockData[0].geometryUniform.transformInv = glm::transpose(glm::inverse(glm::mat4(mat)));
            }

            // reset BVH leafs counters
            copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(6), strided<uint32_t>(1) }, true);

            // calculate leafs and boundings of members
            syncUniforms();
            aabbCalculate.dispatch();

            // copy to staging
            copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, countersBuffer, generalLoadingBuffer, { strided<uint32_t>(6), 0, strided<uint32_t>(1) }, false);

            // get triangle count from staging
            getBufferSubData(generalLoadingBuffer, triangleCount, 0);
            geometryBlockData[0].geometryUniform.triangleCount = triangleCount[0];
            if (triangleCount[0] <= 0) return;
            syncUniforms();

            // sort under construction
            radixSort->sort(mortonCodesBuffer, leafsIndicesBuffer, triangleCount[0]); // incomplete function

            // debug code
            {
                //std::vector<uint64_t> mortons(triangleCount[0]);
                //copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, mortonCodesBuffer, generalLoadingBuffer, { 0, 0, strided<uint64_t>(triangleCount[0]) }, false);
                //getBufferSubData(generalLoadingBuffer, mortons, 0);
                //std::vector<uint32_t> mortons(triangleCount[0]);
                //copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, leafsIndicesBuffer, generalLoadingBuffer, { 0, 0, strided<uint32_t>(triangleCount[0]) }, false);
                //getBufferSubData(generalLoadingBuffer, mortons, 0);
            }

            // reset BVH counters
            copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, 0, strided<uint32_t>(6) }, true);

            auto copyCounterCommand = getCommandBuffer(device, true);
            memoryCopyCmd(copyCounterCommand, countersBuffer, countersBuffer, { 5 * sizeof(int32_t), 4 * sizeof(int32_t), sizeof(int32_t) });
            memoryCopyCmd(copyCounterCommand, countersBuffer, countersBuffer, { 2 * sizeof(int32_t), 5 * sizeof(int32_t), sizeof(int32_t) });
            copyCounterCommand.end();

            auto buildCommand = getCommandBuffer(device, true);
            buildCommand.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSets, nullptr);
            buildCommand.bindPipeline(vk::PipelineBindPoint::eCompute, buildBVHPpl.pipeline);
            buildCommand.dispatch(INTENSIVITY, 1, 1); // dispatch few counts
            buildCommand.end();

            auto stagingCounterCommand = getCommandBuffer(device, true);
            memoryCopyCmd(stagingCounterCommand, countersBuffer, generalLoadingBuffer, { 0, 0, strided<uint32_t>(8) });
            stagingCounterCommand.end();

            vk::PipelineStageFlags stageMasks = vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eComputeShader;
            std::vector<vk::SubmitInfo> buildSubmitInfos;

            for (int j = 0; j < 8; j++) {

                // build level command
                buildSubmitInfos.push_back(vk::SubmitInfo()
                    .setWaitSemaphoreCount(0)
                    .setCommandBufferCount(1)
                    .setPCommandBuffers(&buildCommand)
                );

                // copying commands
                buildSubmitInfos.push_back(vk::SubmitInfo()
                    .setWaitSemaphoreCount(0)
                    .setCommandBufferCount(1)
                    .setPCommandBuffers(&copyCounterCommand)
                );
            }

            // getting to stage buffer command
            buildSubmitInfos.push_back(vk::SubmitInfo()
                .setWaitSemaphoreCount(0)
                .setCommandBufferCount(1)
                .setPCommandBuffers(&stagingCounterCommand)
            );

            // create fence
            vk::Fence fence = device->logical.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));

            // stage building BVH
            std::vector<int32_t> counters(8);
            for (int i = 0; i < 32;i++) {
                device->logical.resetFences(1, &fence);
                device->queue.submit(buildSubmitInfos, fence);
                
                // wait and reset fence
                device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);

                // check if complete
                getBufferSubData(generalLoadingBuffer, counters, 0);
                int32_t nodeCount = counters[5] - counters[4];
                if (nodeCount <= 0) break;
            }

            device->logical.destroyFence(fence);
            device->logical.freeCommandBuffers(device->commandPool, 1, &buildCommand);
            device->logical.freeCommandBuffers(device->commandPool, 1, &copyCounterCommand);
            device->logical.freeCommandBuffers(device->commandPool, 1, &stagingCounterCommand);

            // refit BVH with linking leafs
            childLink.dispatch();
            refitBVH.dispatch();

            // restore state
            geometryBlockData[0].geometryUniform.triangleCount = this->triangleCount;
            syncUniforms();
        }

        BufferType& TriangleHierarchy::getMaterialBuffer() {
            return materialIndicesStorage;
        }

        BufferType& TriangleHierarchy::getBVHBuffer() {
            return bvhBoxBuffer;
        }

        UniformBuffer TriangleHierarchy::getUniformBlockBuffer() {
            return geometryBlockUniform;
        }

        vk::DescriptorSet TriangleHierarchy::getClientDescriptorSet() {
            return clientDescriptorSets[0];
        }


    }
}