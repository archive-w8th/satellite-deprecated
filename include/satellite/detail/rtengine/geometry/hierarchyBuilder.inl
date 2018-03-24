#pragma once

#include "../../../rtengine/geometry/hierarchyBuilder.hpp"
#include "../../../rtengine/geometry/hierarchyStorage.hpp"
#include "../../../rtengine/geometry/geometryAccumulator.hpp"

namespace NSM
{
    namespace rt
    {

        void HieararchyBuilder::init(DeviceQueueType &_device)
        {
            this->device = _device;
            radixSort = std::shared_ptr<gr::RadixSort>(new gr::RadixSort(device, shadersPathPrefix));

            // descriptor set bindings
            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // morton codes (64-bit)
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // morton indices (for sort)
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // (been) BVH nodes, should be here metadata
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // leafs
                vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // BVH boxes
                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // refit flags
                vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // in work actives
                vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // leafs indices in BVH
                vk::DescriptorSetLayoutBinding(8, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // counters
                vk::DescriptorSetLayoutBinding(9, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // minmax buffer
                vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // uniform buffer
                vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr),   // bvh metadata
                vk::DescriptorSetLayoutBinding(12, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // BVH boxes (after refit)
            };

            // descriptor set layout of foreign storage (planned to sharing this structure by C++17)
            std::vector<vk::DescriptorSetLayoutBinding> clientDescriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // attributed data (alpha footage)
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // BVH boxes
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // materials
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // orders
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // geometryUniform
                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // BVH metadata
                vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // reserved
                vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // vertex linear buffer
            };

            // recommended alloc 256Mb for all staging
            // but here can be used 4Kb
            generalStagingBuffer = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            generalLoadingBuffer = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_TO_CPU);

            // layouts of descriptor sets
            builderDescriptorLayout = {
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(descriptorSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(clientDescriptorSetLayoutBindings.data()).setBindingCount(clientDescriptorSetLayoutBindings.size())) };

            pipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(builderDescriptorLayout.data()).setSetLayoutCount(builderDescriptorLayout.size()));
            pipelineCache = device->logical.createPipelineCache(vk::PipelineCacheCreateInfo());

            // another part if foreign object (where storing)
            builderDescriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(1).setPSetLayouts(&builderDescriptorLayout[0]));

            // bvh builder pipelines 
            buildBVHPpl = createCompute(device, shadersPathPrefix + "/hlbvh2/bvh-build.comp.spv", pipelineLayout, pipelineCache);
            refitBVH = createCompute(device, shadersPathPrefix + "/hlbvh2/bvh-fit.comp.spv", pipelineLayout, pipelineCache);
            boundPrimitives = createCompute(device, shadersPathPrefix + "/hlbvh2/bound-calc.comp.spv", pipelineLayout, pipelineCache);
            childLink = createCompute(device, shadersPathPrefix + "/hlbvh2/leaf-link.comp.spv", pipelineLayout, pipelineCache);
            aabbCalculate = createCompute(device, shadersPathPrefix + "/hlbvh2/leaf-gen.comp.spv", pipelineLayout, pipelineCache);

            // build bvh command
            buildBVHPpl.dispatch = [&]() {
                dispatchCompute(buildBVHPpl, INTENSIVITY, { builderDescriptorSets[0], hierarchyStorageLink->getClientDescriptorSet() });
            };

            // build global boundary
            boundPrimitives.dispatch = [&]() {
                flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, boundaryBufferReference, boundaryBuffer, { 0, 0, strided<glm::vec4>(CACHED_BBOX*2) }), true);
                dispatchCompute(boundPrimitives, CACHED_BBOX, { builderDescriptorSets[0], hierarchyStorageLink->getClientDescriptorSet() });
            };

            // link childrens
            childLink.dispatch = [&]() {
                dispatchCompute(childLink, INTENSIVITY, { builderDescriptorSets[0], hierarchyStorageLink->getClientDescriptorSet() });
            };

            // refit BVH
            refitBVH.dispatch = [&]() {
                dispatchCompute(refitBVH, INTENSIVITY, { builderDescriptorSets[0], hierarchyStorageLink->getClientDescriptorSet() });
            };

            // dispatch aabb per nodes
            aabbCalculate.dispatch = [&]() {
                dispatchCompute(aabbCalculate, INTENSIVITY, { builderDescriptorSets[0], hierarchyStorageLink->getClientDescriptorSet() });
            };

            { // boundary buffer cache
                countersBuffer = createBuffer(device, strided<uint32_t>(8), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                boundaryBuffer = createBuffer(device, strided<glm::vec4>(CACHED_BBOX * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                boundaryBufferReference = createBuffer(device, strided<glm::vec4>(CACHED_BBOX*2), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            }

            {
                zerosBufferReference = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
                debugOnes32BufferReference = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // minmaxes
                std::vector<bbox> minmaxes(CACHED_BBOX);
                std::for_each(std::execution::par_unseq, minmaxes.begin(), minmaxes.end(), [&](auto&& m) { m.mn = glm::vec4(10000.f), m.mx = glm::vec4(-10000.f); });

                // zeros and ones
                std::vector<uint32_t> zeros(1024), ones(1024);
                std::for_each(std::execution::par_unseq, zeros.begin(), zeros.end(), [&](auto&& m) { m = 0u; });
                std::for_each(std::execution::par_unseq, ones.begin(), ones.end(), [&](auto&& m) { m = 1u; });

                // make reference buffers
                auto command = getCommandBuffer(device, true);
                bufferSubData(command, boundaryBufferReference, minmaxes, 0); // make reference buffer of boundary
                bufferSubData(command, zerosBufferReference, zeros, 0);       // make reference of zeros
                bufferSubData(command, debugOnes32BufferReference, ones, 0);
                flushCommandBuffer(device, command, true);
            }

            {
                // create bvh uniform
                bvhBlockData = std::vector<BVHBlockUniform>(1);
                bvhBlockUniform.buffer = createBuffer(device, strided<BVHBlockUniform>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                bvhBlockUniform.staging = createBuffer(device, strided<BVHBlockUniform>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // descriptor templates
                auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(builderDescriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(10).setPBufferInfo(&bvhBlockUniform.buffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(8).setPBufferInfo(&countersBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(9).setPBufferInfo(&boundaryBuffer->descriptorInfo),
                }, nullptr);
            }
        }

        void HieararchyBuilder::syncUniforms() {
            auto command = getCommandBuffer(device, true);
            bufferSubData(command, bvhBlockUniform.staging, bvhBlockData, 0);
            memoryCopyCmd(command, bvhBlockUniform.staging, bvhBlockUniform.buffer, { 0, 0, strided<BVHBlockUniform>(1) });
            flushCommandBuffer(device, command, true);
        }

        void HieararchyBuilder::build(glm::dmat4 optproj) {
            // confirm geometry accumulation by counter (planned locking ops)
            auto geometrySourceCounterHandler = geometrySourceLink->getGeometrySourceCounterHandler();
            auto geometryBlockUniform = hierarchyStorageLink->getGeometryBlockUniform();
            flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, geometrySourceCounterHandler, geometryBlockUniform.buffer, { strided<uint32_t>(0), offsetof(GeometryBlockUniform, geometryUniform) + offsetof(GeometryUniformStruct, triangleCount), strided<uint32_t>(1) }), true); //
            std::vector<uint32_t> triangleCount(1);

            {
                // get triangle count from staging
                flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, geometrySourceCounterHandler, generalLoadingBuffer, { strided<uint32_t>(0), 0, strided<uint32_t>(1) }), false); // copy to staging
                getBufferSubData(generalLoadingBuffer, triangleCount, 0);
            }

            if (triangleCount[0] <= 0) return; // no need to build BVH

            { // copy geometry accumulation to hierarchy storage
                size_t _ALLOCATED_HEIGHT = (size_t(triangleCount[0]) > 0 ? (size_t(triangleCount[0]) * ATTRIBUTE_EXTENT - 1) / WARPED_WIDTH + 1 : 0) + 1;

                // copy texel storage data
                vk::ImageCopy copyDesc;
                copyDesc.dstOffset = { 0, 0, 0 };
                copyDesc.srcOffset = { 0, 0, 0 };
                copyDesc.extent = { uint32_t(_WIDTH), uint32_t(_ALLOCATED_HEIGHT), 1 };
                copyDesc.srcSubresource = geometrySourceLink->getAttributeTexel()->subresourceLayers;
                copyDesc.dstSubresource = hierarchyStorageLink->getAttributeTexel()->subresourceLayers;

                // copy images command
                auto command = getCommandBuffer(device, true);
                memoryCopyCmd(command, geometrySourceLink->getAttributeTexel(), hierarchyStorageLink->getAttributeTexel(), copyDesc);
                memoryCopyCmd(command, geometrySourceLink->getMaterialIndices(), hierarchyStorageLink->getMaterialIndices(), { 0, 0, strided<uint32_t>(triangleCount[0]) });
                memoryCopyCmd(command, geometrySourceLink->getVertexLinear(), hierarchyStorageLink->getVertexLinear(), { 0, 0, strided<float>(triangleCount[0] * 9) });
                memoryCopyCmd(command, geometrySourceLink->getOrderIndices(), hierarchyStorageLink->getOrderIndices(), { 0, 0, strided<uint32_t>(triangleCount[0]) });
                flushCommandBuffer(device, command, true);
            }

            // use use initial matrix
            {
                glm::dmat4 mat(1.0);
                mat *= glm::inverse(glm::dmat4(optproj));
                bvhBlockData[0].transform = glm::transpose(glm::mat4(mat));
                bvhBlockData[0].transformInv = glm::transpose(glm::inverse(glm::mat4(mat)));
                syncUniforms();
            }
            boundPrimitives.dispatch(); // calculate general box

            // get boundary
            flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, boundaryBuffer, generalLoadingBuffer, { 0, 0, strided<glm::vec4>(CACHED_BBOX*2) }), false);

            // receive boundary
            std::vector<bbox> bounds(CACHED_BBOX);
            getBufferSubData(generalLoadingBuffer, bounds, 0);

            bbox bound = std::reduce(std::execution::par_unseq, bounds.begin(), bounds.end(), bounds[0], [&](auto&& a, auto&& b) {
                register bbox _box;
                _box.mn = glm::min(a.mn, b.mn);
                _box.mx = glm::max(a.mx, b.mx);
                return _box;
            });

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

            // copy bvh optimal transformation to hierarchy storage
            flushCommandBuffers(device, std::vector<vk::CommandBuffer>{ {createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, bvhBlockUniform.buffer, geometryBlockUniform.buffer, { offsetof(GeometryUniformStruct, transform), offsetof(BVHBlockUniform, transform), strided<glm::mat4>(4) }), createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(6), strided<uint32_t>(1) })}}, true);

            // calculate leafs and checksums
            aabbCalculate.dispatch();

            // get leaf count from staging
            flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, countersBuffer, generalLoadingBuffer, { strided<uint32_t>(6), 0, strided<uint32_t>(1) }), false); // copy to staging
            getBufferSubData(generalLoadingBuffer, triangleCount, 0);
            bvhBlockData[0].leafCount = triangleCount[0];
            if (triangleCount[0] <= 0)
                return;

            // need update geometry uniform optimization matrices, and sort morton codes
            flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, countersBuffer, bvhBlockUniform.buffer, { strided<uint32_t>(6), offsetof(BVHBlockUniform, leafCount), strided<uint32_t>(1) }), true);
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
            flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, 0, strided<uint32_t>(6) }), true);

            auto copyCounterCommand = getCommandBuffer(device, true);
            memoryCopyCmd(copyCounterCommand, countersBuffer, countersBuffer, { 5 * sizeof(int32_t), 4 * sizeof(int32_t), sizeof(int32_t) });
            memoryCopyCmd(copyCounterCommand, countersBuffer, countersBuffer, { 2 * sizeof(int32_t), 5 * sizeof(int32_t), sizeof(int32_t) });
            copyCounterCommand.end();

            auto buildCommand = makeDispatchCommand(buildBVHPpl, INTENSIVITY, { builderDescriptorSets[0], hierarchyStorageLink->getClientDescriptorSet() });

            auto stagingCounterCommand = getCommandBuffer(device, true);
            memoryCopyCmd(stagingCounterCommand, countersBuffer, generalLoadingBuffer, { 0, 0, strided<uint32_t>(8) });
            stagingCounterCommand.end();

            std::vector<vk::SubmitInfo> buildSubmitInfos;
            for (int j = 0; j < 8; j++)
            {
                // build level command
                buildSubmitInfos.push_back(vk::SubmitInfo()
                    .setWaitSemaphoreCount(0)
                    .setCommandBufferCount(1)
                    .setPCommandBuffers(&buildCommand));

                // copying commands
                buildSubmitInfos.push_back(vk::SubmitInfo()
                    .setWaitSemaphoreCount(0)
                    .setCommandBufferCount(1)
                    .setPCommandBuffers(&copyCounterCommand));
            }

            // getting to stage buffer command
            buildSubmitInfos.push_back(vk::SubmitInfo()
                .setWaitSemaphoreCount(0)
                .setCommandBufferCount(1)
                .setPCommandBuffers(&stagingCounterCommand));

            // create fence
            vk::Fence fence = device->logical.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));

            // stage building BVH
            std::vector<int32_t> counters(8);
            for (int i = 0; i < 32; i++)
            {
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

            { // resolve BVH buffers for copying
                auto bvhBoxStorage = hierarchyStorageLink->getBvhBox();
                auto bvhMetaStorage = hierarchyStorageLink->getBvhMeta();
                size_t nodeCount = triangleCount[0] * 2, _ALLOCATED_HEIGHT = std::max(nodeCount > 0 ? (nodeCount * ATTRIBUTE_EXTENT - 1) / WARPED_WIDTH + 1 : 0, WARPED_WIDTH) + 1;

                // copy texel storage data
                vk::ImageCopy copyDesc;
                copyDesc.dstOffset = { 0, 0, 0 };
                copyDesc.srcOffset = { 0, 0, 0 };
                copyDesc.extent = { uint32_t(_BVH_WIDTH), uint32_t(_ALLOCATED_HEIGHT), 1 };
                copyDesc.srcSubresource = bvhMetaWorking->subresourceLayers;
                copyDesc.dstSubresource = bvhMetaStorage->subresourceLayers;

                // copy images command
                auto command = getCommandBuffer(device, true);
                memoryCopyCmd(command, bvhMetaWorking, bvhMetaStorage, copyDesc);
                memoryCopyCmd(command, bvhBoxWorkingResulting, bvhBoxStorage, { 0, 0, strided<bbox>(nodeCount) });
                flushCommandBuffer(device, command, true);
            }
        }

        void HieararchyBuilder::allocateNodeReserve(size_t nodeCount)
        {
            // special values
            size_t _MAX_HEIGHT = std::max(nodeCount > 0 ? (nodeCount - 1) / _BVH_WIDTH + 1 : 0, _BVH_WIDTH) + 1;

            // buffer for working with
            bvhNodesFlags = createBuffer(device, strided<uint32_t>(nodeCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            workingBVHNodesBuffer = createBuffer(device, strided<uint32_t>(nodeCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            leafBVHIndicesBuffer = createBuffer(device, strided<uint32_t>(nodeCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            leafsIndicesBuffer = createBuffer(device, strided<uint32_t>(nodeCount * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            mortonCodesBuffer = createBuffer(device, strided<uint64_t>(nodeCount * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            leafsBuffer = createBuffer(device, strided<HlbvhNode>(nodeCount * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // these buffers will sharing 
            bvhMetaWorking = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_BVH_WIDTH), uint32_t(_MAX_HEIGHT * 2), 1 }, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Sint);
            bvhBoxWorking = createBuffer(device, strided<glm::mat4>(nodeCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            bvhBoxWorkingResulting = createBuffer(device, strided<glm::mat4>(nodeCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // write buffer to bvh builder descriptors
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(builderDescriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(0).setPBufferInfo(&mortonCodesBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(1).setPBufferInfo(&leafsIndicesBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(3).setPBufferInfo(&leafsBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(4).setPBufferInfo(&bvhBoxWorking->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(5).setPBufferInfo(&bvhNodesFlags->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(6).setPBufferInfo(&workingBVHNodesBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(7).setPBufferInfo(&leafBVHIndicesBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(10).setPBufferInfo(&bvhBlockUniform.buffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageImage).setDstBinding(11).setPImageInfo(&bvhMetaWorking->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(12).setPBufferInfo(&bvhBoxWorkingResulting->descriptorInfo),
            }, nullptr);
        }
    }
} // namespace NSM
