#pragma once

#include "hierarchyBuilder.hpp"
#include "hierarchyStorage.hpp"
#include "../vertex/geometryAccumulator.hpp"

namespace NSM
{
    namespace rt
    {

        void HieararchyBuilder::init(Queue _queue)
        {
            this->queue = _queue;
            this->device = _queue->device;


            radixSort = std::make_shared<gr::RadixSort>(this->queue, shadersPathPrefix);

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
                vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // bvh metadata
                vk::DescriptorSetLayoutBinding(12, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // BVH boxes (after refit)
            };

            // descriptor set layout of foreign storage (planned to sharing this structure by C++17)
            std::vector<vk::DescriptorSetLayoutBinding> clientDescriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // attributed data (alpha footage)
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // BVH boxes
                vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // materials
                vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // orders
                vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // geometryUniform
                vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // BVH metadata
                vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),  // reserved
                vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // vertex linear buffer
            };

            // recommended alloc 256Mb for all staging
            // but here can be used 4Kb
            generalStagingBuffer = createBuffer(queue, strided<uint8_t>(1024 * 1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            generalLoadingBuffer = createBuffer(queue, strided<uint8_t>(1024 * 1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_TO_CPU);

            // layouts of descriptor sets
            builderDescriptorLayout = {
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(descriptorSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(clientDescriptorSetLayoutBindings.data()).setBindingCount(clientDescriptorSetLayoutBindings.size())) };

            auto ranges = vk::PushConstantRange().setOffset(0).setSize(strided<uint32_t>(2)).setStageFlags(vk::ShaderStageFlagBits::eCompute);
            auto pipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(builderDescriptorLayout.data()).setSetLayoutCount(builderDescriptorLayout.size()));
            auto pipelineLayoutCnst = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(builderDescriptorLayout.data()).setSetLayoutCount(builderDescriptorLayout.size()).setPushConstantRangeCount(1).setPPushConstantRanges(&ranges));

            // another part if foreign object (where storing)
            builderDescriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(1).setPSetLayouts(&builderDescriptorLayout[0]));

            // bvh builder pipelines 
            buildBVHPpl = createCompute(queue, shadersPathPrefix + "/hlbvh2/bvh-build.comp.spv", pipelineLayout);
            refitBVH = createCompute(queue, shadersPathPrefix + "/hlbvh2/bvh-fit.comp.spv", pipelineLayout);
            boundPrimitives = createCompute(queue, shadersPathPrefix + "/hlbvh2/bound-calc.comp.spv", pipelineLayout);
            childLink = createCompute(queue, shadersPathPrefix + "/hlbvh2/leaf-link.comp.spv", pipelineLayout);
            aabbCalculate = createCompute(queue, shadersPathPrefix + "/hlbvh2/leaf-gen.comp.spv", pipelineLayout);
            buildBVHLargePpl = createCompute(queue, shadersPathPrefix + "/hlbvh2/bvh-build-large.comp.spv", pipelineLayoutCnst);

            { // boundary buffer cache
                countersBuffer = createBuffer(queue, strided<uint32_t>(8), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                boundaryBuffer = createBuffer(queue, strided<glm::vec4>(CACHED_BBOX * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                boundaryBufferReference = createBuffer(queue, strided<glm::vec4>(CACHED_BBOX*2), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            }

            {
                zerosBufferReference = createBuffer(queue, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
                debugOnes32BufferReference = createBuffer(queue, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // minmaxes
                std::vector<bbox> minmaxes(CACHED_BBOX);
                std::for_each(std::execution::par_unseq, minmaxes.begin(), minmaxes.end(), [&](auto&& m) { m.mn = glm::vec4(10000.f), m.mx = glm::vec4(-10000.f); });

                // zeros and ones
                std::vector<uint32_t> zeros(1024), ones(1024);
                std::for_each(std::execution::par_unseq, zeros.begin(), zeros.end(), [&](auto&& m) { m = 0u; });
                std::for_each(std::execution::par_unseq, ones.begin(), ones.end(), [&](auto&& m) { m = 1u; });

                // make reference buffers
                auto command = createCommandBuffer(queue, true);
                bufferSubData(command, boundaryBufferReference, minmaxes, 0); // make reference buffer of boundary
                bufferSubData(command, zerosBufferReference, zeros, 0);       // make reference of zeros
                bufferSubData(command, debugOnes32BufferReference, ones, 0);
                flushCommandBuffers(queue, { command }, true);
            }

            {
                // create bvh uniform
                bvhBlockData = std::vector<BVHBlockUniform>(1);
                bvhBlockUniform.buffer = createBuffer(queue, strided<BVHBlockUniform>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                bvhBlockUniform.staging = createBuffer(queue, strided<BVHBlockUniform>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // descriptor templates
                auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(builderDescriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(10).setPBufferInfo(&bvhBlockUniform.buffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(8).setPBufferInfo(&countersBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(9).setPBufferInfo(&boundaryBuffer->descriptorInfo),
                }, nullptr);
            }

            // initial counters
            flushCommandBuffers(queue, { makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, zerosBufferReference, countersBuffer, { 0, 0, strided<uint32_t>(4) }) }, true);
        }

        void HieararchyBuilder::syncUniforms() {
            auto command = createCommandBuffer(queue, true);
            bufferSubData(command, bvhBlockUniform.staging, bvhBlockData, 0);
            memoryCopyCmd(command, bvhBlockUniform.staging, bvhBlockUniform.buffer, { 0, 0, strided<BVHBlockUniform>(1) });
            flushCommandBuffers(queue, { command }, true);
        }

        void HieararchyBuilder::build(glm::dmat4 optproj) {
            // reset BVH counters
            //flushCommandBuffer(device, makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, 0, strided<uint32_t>(4) }), true);

            // confirm geometry accumulation by counter (planned locking ops)
            auto geometrySourceCounterHandler = geometrySourceLink->getGeometrySourceCounterHandler();
            auto geometryBlockUniform = hierarchyStorageLink->getGeometryBlockUniform();
            flushCommandBuffers(queue, {makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, geometrySourceCounterHandler, geometryBlockUniform.buffer, { strided<uint32_t>(0), offsetof(GeometryBlockUniform, geometryUniform) + offsetof(GeometryUniformStruct, triangleCount), strided<uint32_t>(1)})}, true); //

            // get triangle count from staging
            std::vector<uint32_t> triangleCount(1);
            flushCommandBuffers(queue, { makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, geometrySourceCounterHandler, generalLoadingBuffer, { strided<uint32_t>(0), 0, strided<uint32_t>(1) }) }, false); // copy to staging
            getBufferSubData(generalLoadingBuffer, triangleCount, 0);

            // no need to build BVH
            if (triangleCount[0] <= 0) return;

            { // copy geometry accumulation to hierarchy storage
                size_t _ALLOCATED_HEIGHT = tiled(triangleCount[0]*ATTRIBUTE_EXTENT, WARPED_WIDTH) + 1;

                // copy texel storage data
                vk::ImageCopy copyDesc;
                copyDesc.dstOffset = { 0, 0, 0 };
                copyDesc.srcOffset = { 0, 0, 0 };
                copyDesc.extent = { uint32_t(_WIDTH), uint32_t(_ALLOCATED_HEIGHT), 1 };
                copyDesc.srcSubresource = geometrySourceLink->getAttributeTexel()->subresourceLayers;
                copyDesc.dstSubresource = hierarchyStorageLink->getAttributeTexel()->subresourceLayers;

                // copy images command
                auto command = createCommandBuffer(queue, true);
                memoryCopyCmd(command, geometrySourceLink->getAttributeTexel(), hierarchyStorageLink->getAttributeTexel(), copyDesc);
                memoryCopyCmd(command, geometrySourceLink->getMaterialIndices(), hierarchyStorageLink->getMaterialIndices(), { 0, 0, strided<uint32_t>(triangleCount[0]) });
                memoryCopyCmd(command, geometrySourceLink->getVertexLinear(), hierarchyStorageLink->getVertexLinear(), { 0, 0, strided<float>(triangleCount[0] * 9) });
                memoryCopyCmd(command, geometrySourceLink->getOrderIndices(), hierarchyStorageLink->getOrderIndices(), { 0, 0, strided<uint32_t>(triangleCount[0]) });
                flushCommandBuffers(queue, { command }, true);
            }

            { // use use initial matrix
                glm::dmat4 mat(1.0);
                mat *= glm::inverse(glm::dmat4(optproj));
                bvhBlockData[0].leafCount = triangleCount[0];
                bvhBlockData[0].transform = glm::transpose(glm::mat4(mat));
                bvhBlockData[0].transformInv = glm::transpose(glm::inverse(glm::mat4(mat)));
                syncUniforms();
            }

            // calculate general AABB
            flushCommandBuffers(queue, { makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, boundaryBufferReference, boundaryBuffer, { 0, 0, strided<glm::vec4>(CACHED_BBOX * 2) }) }, true);
            dispatchCompute(boundPrimitives, { CACHED_BBOX, 1, 1 }, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() });
            flushCommandBuffers(queue, { makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, boundaryBuffer, generalLoadingBuffer, { 0, 0, strided<glm::vec4>(CACHED_BBOX * 2) }) }, false);

            // receive boundary (planned to save in GPU)
            std::vector<bbox> bounds(CACHED_BBOX);
            getBufferSubData(generalLoadingBuffer, bounds, 0);
            bbox bound = std::reduce(std::execution::par_unseq, bounds.begin(), bounds.end(), bounds[0], [&](auto&& a, auto&& b) {
                bbox _box;
                _box.mn = glm::min(a.mn, b.mn);
                _box.mx = glm::max(a.mx, b.mx);
                return _box;
            });

            // get optimizations
            glm::vec3 scale = (bound.mx - bound.mn).xyz();
            glm::vec3 offset = bound.mn.xyz();
            {
                glm::mat4 mat(1.0);
                mat *= glm::inverse(glm::translate(glm::vec3(0.5, 0.5, 0.5)) * glm::scale(glm::vec3(0.5, 0.5, 0.5)));
                mat *= glm::inverse(glm::translate(glm::vec3(offset)) * glm::scale(glm::vec3(scale)));
                mat *= glm::inverse(glm::mat4(optproj));
                bvhBlockData[0].transform = glm::transpose(glm::mat4(mat));
                bvhBlockData[0].transformInv = glm::transpose(glm::inverse(glm::mat4(mat)));
                syncUniforms();
            }

            // copy bvh optimal transformation to hierarchy storage
            flushCommandBuffers(queue, { makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, bvhBlockUniform.buffer, geometryBlockUniform.buffer, { offsetof(GeometryUniformStruct, transform), offsetof(BVHBlockUniform, transform), strided<glm::mat4>(4) }) }, true);
            flushCommandBuffers(queue, { makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(3), strided<uint32_t>(1) }) }, true);

            // calculate leafs and checksums
            dispatchCompute(aabbCalculate, { uint32_t(INTENSIVITY), 1u, 1u }, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() });
            flushCommandBuffers(queue, { makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, countersBuffer, bvhBlockUniform.buffer, { strided<uint32_t>(3), offsetof(BVHBlockUniform, leafCount), strided<uint32_t>(1) }) }, true); // copy to staging  

            // need update geometry uniform optimization matrices, and sort morton codes
            radixSort->sort(mortonCodesBuffer, mortonIndicesBuffer, triangleCount[0]); // do radix sort
            
            // refit BVH with linking leafs
            flushCommandBuffers(queue, { makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, zerosBufferReference, countersBuffer, { 0, 0, strided<uint32_t>(4) }) }, true);
            dispatchCompute(buildBVHPpl, { 1u, 1u, 1u }, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() });

            const int swap_ = 1, swap_inv_ = 0;
            auto cnst = glm::uvec2(swap_, 0u);
            auto cnst_inv = glm::uvec2(swap_inv_, 0u);
            
            auto copy_cmd_ = makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(swap_ * 4), strided<uint32_t>(1) }, true);
            auto copy_cmd_inv_ = makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(swap_inv_ * 4), strided<uint32_t>(1) }, true);
            auto disp_cmd_ = makeDispatchCmd(buildBVHLargePpl, { INTENSIVITY, 1u, 1u }, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() }, &cnst);
            auto disp_cmd_inv_ = makeDispatchCmd(buildBVHLargePpl, { INTENSIVITY, 1u, 1u }, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() }, &cnst_inv);

            // large stages of BVH building
            for (int i = 0; i < 128;i++) {

                // getting counter data
                std::vector<uint32_t> nodeTaskCount(1);
                flushCommandBuffers(queue, { makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, countersBuffer, generalLoadingBuffer,{ strided<uint32_t>(swap_inv_ * 4), 0, strided<uint32_t>(1) }) }, false); // copy to staging
                getBufferSubData(generalLoadingBuffer, nodeTaskCount, 0);
                if (nodeTaskCount[0] <= 0) break;

                // submit GPU operate long as possible 
                for (int j = 0; j < 4;j++) {
                    //executeCommands(queue, { copy_cmd_, disp_cmd_, copy_cmd_inv_, disp_cmd_inv_ }, true);
                    executeCommands(queue, { copy_cmd_ }, true);
                    executeCommands(queue, { disp_cmd_ }, true);
                    executeCommands(queue, { copy_cmd_inv_ }, true);
                    executeCommands(queue, { disp_cmd_inv_ }, true);
                }
            }

            // anti-pattern, but we does not made waiter for resolve to free resources
            //flushCommandBuffers(queue, { copy_cmd_, disp_cmd_, copy_cmd_inv_, disp_cmd_inv_ }, true, false);
            flushCommandBuffers(queue, { copy_cmd_ }, true, false);
            flushCommandBuffers(queue, { disp_cmd_ }, true, false);
            flushCommandBuffers(queue, { copy_cmd_inv_ }, true, false);
            flushCommandBuffers(queue, { disp_cmd_inv_ }, true, false);

            
            dispatchCompute(childLink, { uint32_t(INTENSIVITY), 1u, 1u }, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() });
            dispatchCompute(refitBVH, { uint32_t(INTENSIVITY), 1u, 1u }, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() });
            //dispatchCompute(refitBVH, { 1u, 1u, 1u }, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() });

            
            if (triangleCount[0] > 0) {
                //std::vector<bbox> bboxes(triangleCount[0] * 2);
                //flushCommandBuffer(device, makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(device, bvhBoxWorking, generalLoadingBuffer, { 0, 0, strided<bbox>(triangleCount[0] * 2) }), false);
                //getBufferSubData(generalLoadingBuffer, bboxes);
                
                // debug BVH Meta
                //std::vector<uint64_t> mortons(triangleCount[0]);
                //flushCommandBuffer(device, makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(device, mortonCodesBuffer, generalLoadingBuffer, { 0, 0, strided<uint64_t>(triangleCount[0]) }), false);
                //getBufferSubData(generalLoadingBuffer, mortons);

                // debug BVH Meta
                //std::vector<bvh_meta> bvhMeta(triangleCount[0] * 2);
                //flushCommandBuffer(device, makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(device, bvhMetaWorking, generalLoadingBuffer, { 0, 0, strided<bvh_meta>(triangleCount[0] * 2) }), false);
                //getBufferSubData(generalLoadingBuffer, bvhMeta);
            }

            { // resolve BVH buffers for copying
                auto command = createCommandBuffer(queue, true);
                memoryCopyCmd(command, bvhMetaWorking, hierarchyStorageLink->getBvhMeta(), { 0, 0, strided<glm::ivec4>(triangleCount[0] * 2) });
                memoryCopyCmd(command, bvhBoxWorkingResulting, hierarchyStorageLink->getBvhBox(), { 0, 0, strided<glm::mat4>(triangleCount[0]) });
                flushCommandBuffers(queue, { command }, true);
            }

            // finish BVH building
            syncUniforms();
            flushCommandBuffers(queue, { makeCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, zerosBufferReference, countersBuffer, { 0, 0, strided<uint32_t>(4) }) }, true);
        }

        void HieararchyBuilder::allocateNodeReserve(size_t nodeCount)
        {
            // buffer for working with
            bvhNodesFlags = createBuffer(queue, strided<uint32_t>(nodeCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            workingBVHNodesBuffer = createBuffer(queue, strided<uint32_t>(nodeCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            leafBVHIndicesBuffer = createBuffer(queue, strided<uint32_t>(nodeCount * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            mortonIndicesBuffer = createBuffer(queue, strided<uint32_t>(nodeCount * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            mortonCodesBuffer = createBuffer(queue, strided<uint64_t>(nodeCount * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            leafsBuffer = createBuffer(queue, strided<HlbvhNode>(nodeCount * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // these buffers will sharing
            bvhMetaWorking = createBuffer(queue, strided<glm::ivec4>(nodeCount*2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            bvhBoxWorking = createBuffer(queue, strided<glm::mat4>(nodeCount), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            bvhBoxWorkingResulting = createBuffer(queue, strided<glm::mat4>(nodeCount), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // write buffer to bvh builder descriptors
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(builderDescriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(0).setPBufferInfo(&mortonCodesBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(1).setPBufferInfo(&mortonIndicesBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(3).setPBufferInfo(&leafsBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(4).setPBufferInfo(&bvhBoxWorking->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(5).setPBufferInfo(&bvhNodesFlags->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(6).setPBufferInfo(&workingBVHNodesBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(7).setPBufferInfo(&leafBVHIndicesBuffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(10).setPBufferInfo(&bvhBlockUniform.buffer->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(11).setPBufferInfo(&bvhMetaWorking->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(12).setPBufferInfo(&bvhBoxWorkingResulting->descriptorInfo),
            }, nullptr);
        }
    }
} // namespace NSM
