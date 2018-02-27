//#pragma once

#include "../../rtengine/triangleHierarchy.hpp"

namespace NSM {
    namespace rt {

        void TriangleHierarchy::init(DeviceQueueType& _device) {
            this->device = _device;

            // create radix sort (planned dedicated sorter)
            radixSort = std::shared_ptr<RadixSort>(new RadixSort(device, shadersPathPrefix));


            // descriptor set bindings
            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // morton codes (64-bit)
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // morton indices (for sort)
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // (been) BVH nodes, should be here metadata
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // leafs
                vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // BVH boxes
                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // refit flags
                vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // in work actives
                vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // leafs indices in BVH
                vk::DescriptorSetLayoutBinding(8, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // counters
                vk::DescriptorSetLayoutBinding(9, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // minmax buffer
                vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // uniform buffer
                vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr) // bvh metadata
            };



            // descriptor set connectors for externals
            std::vector<vk::DescriptorSetLayoutBinding> clientDescriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // attributed data (alpha footage) 
                //vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // normal
                //vk::DescriptorSetLayoutBinding(12, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // texcoord
                //vk::DescriptorSetLayoutBinding(13, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // modifiers
                //vk::DescriptorSetLayoutBinding(14, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // reserved (may colors)

                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // BVH boxes
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // materials
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // orders
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // geometryUniform

                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // BVH metadata
                vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // reserved 

                vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // vertex linear buffer 
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
                vk::DescriptorSetLayoutBinding(9, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // vertex linear buffer 

                // write images with vertex data 
                vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // attributed data (alpha footage) 
                //vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // normal
                //vk::DescriptorSetLayoutBinding(12, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // texcoord
                //vk::DescriptorSetLayoutBinding(13, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // modifiers
                //vk::DescriptorSetLayoutBinding(14, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // reserved (may colors)

                
            };

            // layouts of descriptor sets 
            descriptorSetLayouts = {
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(descriptorSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(clientDescriptorSetLayoutBindings.data()).setBindingCount(clientDescriptorSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(loaderDescriptorSetBindings.data()).setBindingCount(loaderDescriptorSetBindings.size())),
            };

            // descriptor sets for BVH builders
            pipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(&descriptorSetLayouts[0]).setSetLayoutCount(2));
            descriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(2).setPSetLayouts(&descriptorSetLayouts[0]));
            clientDescriptorSets.push_back(descriptorSets[1]);

            // pipeline layout for vertex loader
            loaderPipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(&descriptorSetLayouts[2]).setSetLayoutCount(1));
            loaderDescriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(1).setPSetLayouts(&descriptorSetLayouts[2]));

            // pipelines
            pipelineCache = device->logical.createPipelineCache(vk::PipelineCacheCreateInfo());
            buildBVHPpl.pipeline = createCompute(device, shadersPathPrefix + "/hlbvh/build-new.comp.spv", pipelineLayout, pipelineCache);
            refitBVH.pipeline = createCompute(device, shadersPathPrefix + "/hlbvh/refit.comp.spv", pipelineLayout, pipelineCache);
            boundPrimitives.pipeline = createCompute(device, shadersPathPrefix + "/hlbvh/minmax.comp.spv", pipelineLayout, pipelineCache);
            childLink.pipeline = createCompute(device, shadersPathPrefix + "/hlbvh/child-link.comp.spv", pipelineLayout, pipelineCache);
            aabbCalculate.pipeline = createCompute(device, shadersPathPrefix + "/hlbvh/aabbmaker.comp.spv", pipelineLayout, pipelineCache);

            // vertex loader
            geometryLoader.pipeline = createCompute(device, shadersPathPrefix + "/vertex/loader.comp.spv", loaderPipelineLayout, pipelineCache);
            //geometryLoader16bit.pipeline = createCompute(device, shadersPathPrefix + "/vertex/loader-int16.comp.spv", loaderPipelineLayout, pipelineCache);



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
                auto cCmd = createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, boundaryBufferReference, boundaryBuffer, { 0, 0, strided<glm::vec4>(64) });

                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, { descriptorSets[0], clientDescriptorSets[0] }, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, boundPrimitives.pipeline);
                commandBuffer.dispatch(32, 1, 1);

                flushCommandBuffers(device, std::vector<vk::CommandBuffer>{ {cCmd, commandBuffer}}, true);
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
                commandBuffer.dispatch(INTENSIVITY, 1, 1);
                flushCommandBuffer(device, commandBuffer, true);
            };

            // loading geometry command 
            geometryLoader.dispatch = [&]() {
                auto commandBuffer = getCommandBuffer(device, true);
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, loaderPipelineLayout, 0, loaderDescriptorSets, nullptr);
                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, geometryLoader.pipeline);
                commandBuffer.dispatch(INTENSIVITY, 1, 1);
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
            for (int i = 0; i < 32; i++) {
                minmaxes[i * 2 + 0] = glm::vec4(10000.f);
                minmaxes[i * 2 + 1] = glm::vec4(-10000.f);
            }

            // zeros
            std::vector<uint32_t> zeros(1024);
            std::vector<uint32_t> ones(1024);
            for (int i = 0; i < 1024; i++) { zeros[i] = 0, ones[i] = 1; }


            // make reference buffers
            auto command = getCommandBuffer(device, true);
            bufferSubData(command, boundaryBufferReference, minmaxes, 0); // make reference buffer of boundary
            bufferSubData(command, zerosBufferReference, zeros, 0); // make reference of zeros
            bufferSubData(command, debugOnes32BufferReference, ones, 0);
            flushCommandBuffer(device, command, true);


            // create client geometry uniform buffer
            geometryBlockData = std::vector<GeometryBlockUniform>(1);
            geometryBlockUniform.buffer = createBuffer(device, strided<GeometryBlockUniform>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            geometryBlockUniform.staging = createBuffer(device, strided<GeometryBlockUniform>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

            // create bvh uniform 
            bvhBlockData = std::vector<BVHBlockUniform>(1);
            bvhBlockUniform.buffer = createBuffer(device, strided<BVHBlockUniform>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            bvhBlockUniform.staging = createBuffer(device, strided<BVHBlockUniform>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

            // descriptor templates
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            auto desc1Tmpl = vk::WriteDescriptorSet(desc0Tmpl).setDstSet(descriptorSets[1]).setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
            auto ldesc0Tmpl = vk::WriteDescriptorSet(desc0Tmpl).setDstSet(loaderDescriptorSets[0]).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            auto bvhCounters = vk::DescriptorBufferInfo(countersBuffer->buffer, 0, strided<uint32_t>(8));

            // write buffers to main descriptors
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(10).setPBufferInfo(&bvhBlockUniform.buffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(8).setPBufferInfo(&bvhCounters),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(9).setPBufferInfo(&boundaryBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(3).setPBufferInfo(&geometryBlockUniform.buffer->descriptorInfo),
                    vk::WriteDescriptorSet(ldesc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(0).setPBufferInfo(&bvhCounters)
            }, nullptr);

            // clear for fix issues
            clearTribuffer();
        }

        void TriangleHierarchy::allocate(size_t maxt) {
            maxTriangles = maxt;

            // allocate these buffers 
            leafsBuffer = createBuffer(device, strided<HlbvhNode>(maxTriangles * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // buffer for working with
            bvhNodesFlags = createBuffer(device, strided<uint32_t>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            workingBVHNodesBuffer = createBuffer(device, strided<uint32_t>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            leafBVHIndicesBuffer = createBuffer(device, strided<uint32_t>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            leafsIndicesBuffer = createBuffer(device, strided<uint32_t>(maxTriangles * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            mortonCodesBuffer = createBuffer(device, strided<uint64_t>(maxTriangles * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // special values 
            size_t _MAX_HEIGHT = std::max(maxTriangles > 0 ? (maxTriangles*ATTRIBUTE_EXTENT - 1) / WARPED_WIDTH + 1 : 0, WARPED_WIDTH) + 1;

            // storage 
            attributeTexelStorage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_WIDTH), uint32_t(_MAX_HEIGHT), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sfloat);
            materialIndicesStorage = createBuffer(device, strided<uint32_t>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            orderIndicesStorage = createBuffer(device, strided<uint32_t>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            vertexLinearStorage = createBuffer(device, strided<float>(maxTriangles * 2 * 9), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // sideloader
            attributeTexelWorking = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_WIDTH), uint32_t(_MAX_HEIGHT), 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::Format::eR32G32B32A32Sfloat);
            materialIndicesWorking = createBuffer(device, strided<uint32_t>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            orderIndicesWorking = createBuffer(device, strided<uint32_t>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            vertexLinearWorking = createBuffer(device, strided<float>(maxTriangles * 2 * 9), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // bvh storage (32-bits elements)
            _MAX_HEIGHT = std::max(maxTriangles > 0 ? (maxTriangles - 1) / _BVH_WIDTH + 1 : 0, _BVH_WIDTH) + 1;
            bvhMetaStorage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_BVH_WIDTH), uint32_t(_MAX_HEIGHT * 2), 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sint);
            bvhBoxStorage = createBuffer(device, strided<glm::mat4>(maxTriangles * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // planned working dedicated buffers
            bvhMetaWorking = bvhMetaStorage;
            bvhBoxWorking = bvhBoxStorage;

            // create sampler
            auto sampler = device->logical.createSampler(vk::SamplerCreateInfo().setMagFilter(vk::Filter::eNearest).setMinFilter(vk::Filter::eNearest).setAddressModeU(vk::SamplerAddressMode::eRepeat).setAddressModeV(vk::SamplerAddressMode::eMirrorClampToEdge).setCompareEnable(false));

            // descriptor templates
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            auto desc1Tmpl = vk::WriteDescriptorSet(desc0Tmpl).setDstSet(descriptorSets[1]).setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
            auto ldesc0Tmpl = vk::WriteDescriptorSet(desc0Tmpl).setDstSet(loaderDescriptorSets[0]).setDescriptorType(vk::DescriptorType::eStorageBuffer);

            // write buffer to bvh builder descriptors
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(0).setPBufferInfo(&mortonCodesBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(1).setPBufferInfo(&leafsIndicesBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(3).setPBufferInfo(&leafsBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(4).setPBufferInfo(&bvhBoxWorking->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(5).setPBufferInfo(&bvhNodesFlags->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(6).setPBufferInfo(&workingBVHNodesBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(7).setPBufferInfo(&leafBVHIndicesBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(10).setPBufferInfo(&bvhBlockUniform.buffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(11).setPImageInfo(&bvhMetaWorking->descriptorInfo.setSampler(sampler)),

                    // write to client descriptors 
                    vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setDstBinding(10).setPImageInfo(&attributeTexelStorage->descriptorInfo.setSampler(sampler)),
                    vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(0).setPBufferInfo(&bvhBoxStorage->descriptorInfo),
                    vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(1).setPBufferInfo(&materialIndicesStorage->descriptorInfo),
                    vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(2).setPBufferInfo(&orderIndicesStorage->descriptorInfo),
                    vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(3).setPBufferInfo(&geometryBlockUniform.buffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setDstBinding(5).setPImageInfo(&bvhMetaStorage->descriptorInfo.setSampler(sampler)),
                    vk::WriteDescriptorSet(desc1Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(7).setPBufferInfo(&vertexLinearStorage->descriptorInfo),

                    // write buffers to loader descriptors
                    vk::WriteDescriptorSet(ldesc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(10).setPImageInfo(&attributeTexelWorking->descriptorInfo),
                    vk::WriteDescriptorSet(ldesc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(7).setPBufferInfo(&materialIndicesWorking->descriptorInfo),
                    vk::WriteDescriptorSet(ldesc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(8).setPBufferInfo(&orderIndicesWorking->descriptorInfo),
                    vk::WriteDescriptorSet(ldesc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(9).setPBufferInfo(&vertexLinearWorking->descriptorInfo),
            }, nullptr);
        }

        void TriangleHierarchy::clearTribuffer() {
            flushCommandBuffer(device, createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(7), 1 }), true);
            isDirty = false;
        }

        void TriangleHierarchy::loadGeometry(std::shared_ptr<VertexInstance>& vertexInstance) {
            // write mesh buffers to loaders descriptors
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(loaderDescriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(1).setPBufferInfo(&vertexInstance->getBufferSpaceBuffer()->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(2).setPBufferInfo(&vertexInstance->getBufferSpaceRegions()->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(3).setPBufferInfo(&vertexInstance->getBufferViewsBuffer()->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(4).setPBufferInfo(&vertexInstance->getDataFormatBuffer()->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(5).setPBufferInfo(&vertexInstance->getBufferBindingBuffer()->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(6).setPBufferInfo(&vertexInstance->getUniformBuffer()->descriptorInfo),
            }, nullptr);

            // dispatch loading
            geometryLoader.dispatch();
            markDirty();
        }

        void TriangleHierarchy::syncUniforms() {
            auto command = getCommandBuffer(device, true);
            bufferSubData(command, bvhBlockUniform.staging, bvhBlockData, 0);
            memoryCopyCmd(command, bvhBlockUniform.staging, bvhBlockUniform.buffer, { 0, 0, strided<BVHBlockUniform>(1) });
            flushCommandBuffer(device, command, true);
        }

        // very hacky function, preferly don't use
        void TriangleHierarchy::markDirty() {
            isDirty = true;
        }

        void TriangleHierarchy::buildBVH(glm::dmat4 optproj) {
            // no need to build BVH
            if (!isDirty) return;

            // because we not setting triangle count in uniform when loading that geometry, we should update this value manually on device
            // planned deferred/async copy support
            flushCommandBuffer(device, createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, countersBuffer, geometryBlockUniform.buffer, { strided<uint32_t>(7), offsetof(GeometryBlockUniform, geometryUniform) + offsetof(GeometryUniformStruct, triangleCount), strided<uint32_t>(1) }), true); // 


            // get triangle count from staging 
            std::vector<uint32_t> triangleCount(1);
            flushCommandBuffer(device, createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, countersBuffer, generalLoadingBuffer, { strided<uint32_t>(7), 0, strided<uint32_t>(1) }), false); // copy to staging
            getBufferSubData(generalLoadingBuffer, triangleCount, 0);


            if (triangleCount[0] <= 0) return; // no need to build BVH

            // bvh potentially builded
            isDirty = false;

            // special values
            size_t _MAX_HEIGHT = std::max(size_t(maxTriangles) > 0 ? (size_t(maxTriangles)*ATTRIBUTE_EXTENT - 1) / WARPED_WIDTH + 1 : 0, WARPED_WIDTH) + 1;
            size_t _ALLOCATED_HEIGHT = std::min(size_t(triangleCount[0]) > 0 ? (size_t(triangleCount[0])*ATTRIBUTE_EXTENT - 1) / WARPED_WIDTH + 1 : 0, _MAX_HEIGHT) + 1;

            // copy texel storage data
            vk::ImageCopy copyDesc;
            copyDesc.dstOffset = { 0, 0, 0 };
            copyDesc.srcOffset = { 0, 0, 0 };
            copyDesc.extent = { uint32_t(_WIDTH), uint32_t(_ALLOCATED_HEIGHT), 1 };
            copyDesc.srcSubresource = attributeTexelWorking->subresourceLayers;
            copyDesc.dstSubresource = attributeTexelStorage->subresourceLayers;

            // copy images command
            auto command = getCommandBuffer(device, true);
            memoryCopyCmd(command, attributeTexelWorking, attributeTexelStorage, copyDesc);
            memoryCopyCmd(command, materialIndicesWorking, materialIndicesStorage, { 0, 0, strided<uint32_t>(triangleCount[0]) });
            memoryCopyCmd(command, vertexLinearWorking, vertexLinearStorage, { 0, 0, strided<float>(triangleCount[0]*9) });
            memoryCopyCmd(command, orderIndicesWorking, orderIndicesStorage, { 0, 0, strided<uint32_t>(triangleCount[0]) });
            flushCommandBuffer(device, command, true);

            // use use initial matrix
            {
                glm::dmat4 mat(1.0);
                mat *= glm::inverse(glm::dmat4(optproj));
                bvhBlockData[0].transform = glm::transpose(glm::mat4(mat));
                bvhBlockData[0].transformInv = glm::transpose(glm::inverse(glm::mat4(mat)));
                syncUniforms();
            }
            boundPrimitives.dispatch();

            // get boundary
            flushCommandBuffer(device, createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, boundaryBuffer, generalLoadingBuffer, { 0, 0, strided<glm::vec4>(64) }), false);

            // receive boundary
            std::vector<glm::vec4> bounds(64);
            getBufferSubData(generalLoadingBuffer, bounds, 0);
            bbox bound = { bounds[0], bounds[1] };
            for (int i = 1; i < 32; i++) {
                bound.mn = glm::min(bounds[i * 2 + 0], bound.mn);
                bound.mx = glm::max(bounds[i * 2 + 1], bound.mx);
            }

            // get optimizations
            glm::vec3 scale = (bound.mx - bound.mn).xyz();
            glm::vec3 offset = bound.mn.xyz();
            {
                glm::dmat4 mat(1.0);
                mat *= glm::inverse(glm::translate(glm::dvec3(0.5, 0.5, 0.5)) * glm::scale(glm::dvec3(0.5, 0.5, 0.5)));
                mat *= glm::inverse(glm::translate(glm::dvec3(offset)) * glm::scale(glm::dvec3(scale)));
                mat *= glm::inverse(glm::dmat4(optproj));
                bvhBlockData[0].transform = glm::transpose(glm::mat4(mat));
                bvhBlockData[0].transformInv = glm::transpose(glm::inverse(glm::mat4(mat)));
                syncUniforms();
            }


            flushCommandBuffers(device, std::vector<vk::CommandBuffer>{{
                createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, bvhBlockUniform.buffer, geometryBlockUniform.buffer, {
                    offsetof(GeometryUniformStruct, transform),
                    offsetof(BVHBlockUniform, transform),
                    strided<glm::mat4>(4)
                }),
                createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(6), strided<uint32_t>(1) })
            }}, true);


            aabbCalculate.dispatch();

            // get leaf count from staging 
            flushCommandBuffer(device, createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, countersBuffer, generalLoadingBuffer, { strided<uint32_t>(6), 0, strided<uint32_t>(1) }), false); // copy to staging
            getBufferSubData(generalLoadingBuffer, triangleCount, 0);
            bvhBlockData[0].leafCount = triangleCount[0];
            if (triangleCount[0] <= 0) return;

            // need update geometry uniform optimization matrices, and sort morton codes
            flushCommandBuffer(device, createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, countersBuffer, bvhBlockUniform.buffer, { strided<uint32_t>(6), offsetof(BVHBlockUniform, leafCount), strided<uint32_t>(1) }), true);
            radixSort->sort(mortonCodesBuffer, leafsIndicesBuffer, triangleCount[0]); // do radix sort

            // debug code
            {
                //std::vector<uint64_t> mortons(triangleCount[0]);
                //flushCommandBuffer(device, createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, mortonCodesBuffer, generalLoadingBuffer, { 0, 0, strided<uint64_t>(triangleCount[0]) }), false);
                //getBufferSubData(generalLoadingBuffer, mortons, 0);

                //std::vector<uint32_t> mortons(triangleCount[0]);
                //copyMemoryProxy<BufferType&, BufferType&, vk::BufferCopy>(device, leafsIndicesBuffer, generalLoadingBuffer, { 0, 0, strided<uint32_t>(triangleCount[0]) }, false);
                //getBufferSubData(generalLoadingBuffer, mortons, 0);
            }

            // reset BVH counters (and copy to uniform)
            flushCommandBuffer(device, createCopyCmd<BufferType&, BufferType&, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, 0, strided<uint32_t>(6) }), true);

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

            std::vector<vk::SubmitInfo> buildSubmitInfos;
            for (int j = 0; j < 8; j++) {
                buildSubmitInfos.push_back(vk::SubmitInfo()
                    .setWaitSemaphoreCount(0)
                    .setCommandBufferCount(1)
                    .setPCommandBuffers(&buildCommand)
                );  // build level command
                buildSubmitInfos.push_back(vk::SubmitInfo()
                    .setWaitSemaphoreCount(0)
                    .setCommandBufferCount(1)
                    .setPCommandBuffers(&copyCounterCommand)
                ); // copying commands
            }
            buildSubmitInfos.push_back(vk::SubmitInfo()
                .setWaitSemaphoreCount(0)
                .setCommandBufferCount(1)
                .setPCommandBuffers(&stagingCounterCommand)
            ); // getting to stage buffer command

            // create fence
            vk::Fence fence = device->logical.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));

            // stage building BVH
            std::vector<int32_t> counters(8);
            for (int i = 0; i < 32; i++) {
                device->logical.resetFences(1, &fence);
                device->mainQueue->queue.submit(buildSubmitInfos, fence);

                // wait and reset fence
                device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);

                // check if complete
                getBufferSubData(generalLoadingBuffer, counters, 0);
                int32_t nodeCount = counters[5] - counters[4];
                if (nodeCount <= 0) break;
            }

            // because already awaited, but clear command buffers and fence (planned saving/cache commands)
            device->logical.destroyFence(fence);
            device->logical.freeCommandBuffers(device->commandPool, 1, &buildCommand);
            device->logical.freeCommandBuffers(device->commandPool, 1, &copyCounterCommand);
            device->logical.freeCommandBuffers(device->commandPool, 1, &stagingCounterCommand);

            // refit BVH with linking leafs
            childLink.dispatch();
            refitBVH.dispatch();
            syncUniforms();
        }

        vk::DescriptorSet TriangleHierarchy::getClientDescriptorSet() {
            return clientDescriptorSets[0];
        }

    }
}