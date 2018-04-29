#pragma once

#include "../../../rtengine/accelerator/hierarchyBuilder.hpp"
#include "../../../rtengine/accelerator/hierarchyStorage.hpp"
#include "../../../rtengine/accelerator/geometryAccumulator.hpp"

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
            generalStagingBuffer = createBuffer(device, strided<uint64_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            generalLoadingBuffer = createBuffer(device, strided<uint64_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_TO_CPU);

            // layouts of descriptor sets
            builderDescriptorLayout = {
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(descriptorSetLayoutBindings.size())),
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(clientDescriptorSetLayoutBindings.data()).setBindingCount(clientDescriptorSetLayoutBindings.size())) };

            pipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(builderDescriptorLayout.data()).setSetLayoutCount(builderDescriptorLayout.size()));
            pipelineCache = device->logical.createPipelineCache(vk::PipelineCacheCreateInfo());

            // another part if foreign object (where storing)
            builderDescriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(1).setPSetLayouts(&builderDescriptorLayout[0]));

            // bvh builder pipelines 
            buildBVHPpl = createCompute(device, shadersPathPrefix + "/hlbvh2/bvh-build.comp.spv", pipelineLayout);
            refitBVH = createCompute(device, shadersPathPrefix + "/hlbvh2/bvh-fit.comp.spv", pipelineLayout);
            boundPrimitives = createCompute(device, shadersPathPrefix + "/hlbvh2/bound-calc.comp.spv", pipelineLayout);
            childLink = createCompute(device, shadersPathPrefix + "/hlbvh2/leaf-link.comp.spv", pipelineLayout);
            aabbCalculate = createCompute(device, shadersPathPrefix + "/hlbvh2/leaf-gen.comp.spv", pipelineLayout);

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
                size_t _ALLOCATED_HEIGHT = tiled(triangleCount[0]*ATTRIBUTE_EXTENT, WARPED_WIDTH) + 1;

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

            // calculate general AABB
            flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, boundaryBufferReference, boundaryBuffer, { 0, 0, strided<glm::vec4>(CACHED_BBOX*2) }), true);
            dispatchCompute(boundPrimitives, CACHED_BBOX, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() });

            // get boundary
            flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, boundaryBuffer, generalLoadingBuffer, { 0, 0, strided<glm::vec4>(CACHED_BBOX*2) }), false);

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
            flushCommandBuffers(device, std::vector<vk::CommandBuffer>{ {createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, bvhBlockUniform.buffer, geometryBlockUniform.buffer, { offsetof(GeometryUniformStruct, transform), offsetof(BVHBlockUniform, transform), strided<glm::mat4>(4) }), createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, strided<uint32_t>(6), strided<uint32_t>(1) })}}, true);

            // calculate leafs and checksums
            dispatchCompute(aabbCalculate, INTENSIVITY, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() });

            // get leaf count from staging
            {
                flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, countersBuffer, generalLoadingBuffer, { strided<uint32_t>(6), 0, strided<uint32_t>(1) }), false); // copy to staging
                getBufferSubData(generalLoadingBuffer, triangleCount, 0);
                bvhBlockData[0].leafCount = triangleCount[0];
                if (triangleCount[0] <= 0) return;
            }

            // need update geometry uniform optimization matrices, and sort morton codes
            radixSort->sort(mortonCodesBuffer, mortonIndicesBuffer, triangleCount[0]); // do radix sort

            // reset BVH counters (and copy to uniform)
            flushCommandBuffer(device, createCopyCmd<BufferType &, BufferType &, vk::BufferCopy>(device, zerosBufferReference, countersBuffer, { 0, 0, strided<uint32_t>(6) }), true);

            // refit BVH with linking leafs
            dispatchCompute(buildBVHPpl, 1, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() });
            dispatchCompute(childLink, INTENSIVITY, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() });
            dispatchCompute(refitBVH, INTENSIVITY, { builderDescriptorSets[0], hierarchyStorageLink->getStorageDescSec() });
            syncUniforms();

            { // resolve BVH buffers for copying
                auto bvhBoxStorage = hierarchyStorageLink->getBvhBox();
                auto bvhMetaStorage = hierarchyStorageLink->getBvhMeta();
                size_t _ALLOCATED_HEIGHT = tiled(triangleCount[0] * 2, _BVH_WIDTH)+1;

                // copy images command
                auto command = getCommandBuffer(device, true);
                memoryCopyCmd(command, bvhMetaWorking, bvhMetaStorage, { 0, 0, strided<glm::ivec4>(triangleCount[0] * 2) });
                memoryCopyCmd(command, bvhBoxWorkingResulting, bvhBoxStorage, { 0, 0, strided<bbox>(triangleCount[0] * 2) });
                flushCommandBuffer(device, command, true);
            }
        }

        void HieararchyBuilder::allocateNodeReserve(size_t nodeCount)
        {
            // buffer for working with
            bvhNodesFlags = createBuffer(device, strided<uint32_t>(nodeCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            workingBVHNodesBuffer = createBuffer(device, strided<uint32_t>(nodeCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            leafBVHIndicesBuffer = createBuffer(device, strided<uint32_t>(nodeCount * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            mortonIndicesBuffer = createBuffer(device, strided<uint32_t>(nodeCount * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            mortonCodesBuffer = createBuffer(device, strided<uint64_t>(nodeCount * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            leafsBuffer = createBuffer(device, strided<HlbvhNode>(nodeCount * 1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // these buffers will sharing
            bvhMetaWorking = createBuffer(device, strided<glm::ivec4>(nodeCount*2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            bvhBoxWorking = createBuffer(device, strided<glm::mat4>(nodeCount), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            bvhBoxWorkingResulting = createBuffer(device, strided<glm::mat4>(nodeCount), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

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
