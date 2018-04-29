#pragma once
#pragma once

#include "../../../rtengine/accelerator/hierarchyStorage.hpp"

namespace NSM
{
    namespace rt
    {

        void HieararchyStorage::init(DeviceQueueType &_device)
        {
            this->device = _device;

            {
                // descriptor set layout of foreign storage (planned to sharing this structure by C++17)
                std::vector<vk::DescriptorSetLayoutBinding> clientDescriptorSetLayoutDesc = {
                    vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // attributed data (alpha footage)
                    vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // BVH boxes
                    vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // materials
                    vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // orders
                    vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // geometryUniform
                    vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // BVH metadata
                    vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // reserved
                    vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),         // vertex linear buffer
                };

                // ray tracing unified descriptors
                std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                    vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // rays,
                    vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // output hits
                    vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr), // output counters


                    vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageTexelBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                    //vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                };

                // create simpler descriptors
                clientDescriptorLayout = std::vector<vk::DescriptorSetLayout>{ 
                    device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setBindingCount(descriptorSetLayoutBindings.size()).setPBindings(descriptorSetLayoutBindings.data())),
                    device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setBindingCount(clientDescriptorSetLayoutDesc.size()).setPBindings(clientDescriptorSetLayoutDesc.data()))
                };
                clientDescriptorSets = device->logical.allocateDescriptorSets(vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(2).setPSetLayouts(clientDescriptorLayout.data()));

                // create traverse pipeline
                auto pipelineCache = device->logical.createPipelineCache(vk::PipelineCacheCreateInfo());
                rayTraversePipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(clientDescriptorLayout.data()).setSetLayoutCount(clientDescriptorLayout.size()));
                bvhTraverse = createCompute(device, shadersPathPrefix + "/rendering/traverse-bvh.comp.spv", rayTraversePipelineLayout);
                vertexInterpolator = createCompute(device, shadersPathPrefix + "/rendering/interpolator.comp.spv", rayTraversePipelineLayout);

                // cache size
                const size_t TRAVERSE_CACHE_SIZE = 16;
                const size_t LOCAL_WORK_SIZE = 64;

                // caches
                //traverseBlockData = createBuffer(device, TRAVERSE_BLOCK_SIZE * INTENSIVITY, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                traverseCacheData = createBuffer(device, TRAVERSE_CACHE_SIZE * LOCAL_WORK_SIZE * INTENSIVITY * sizeof(glm::ivec4), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer | vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

                // make TBO
                auto bufferView = device->logical.createBufferView(vk::BufferViewCreateInfo().setBuffer(traverseCacheData->buffer).setFormat(vk::Format::eR32G32B32A32Sint).setOffset(0).setRange(TRAVERSE_CACHE_SIZE * INTENSIVITY * LOCAL_WORK_SIZE * sizeof(glm::ivec4)));

                // set BVH traverse caches
                auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(clientDescriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                     vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageTexelBuffer).setDstBinding(4).setPTexelBufferView(&bufferView),
                     //vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(4).setPBufferInfo(&traverseCacheData->descriptorInfo),
                     //vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(5).setPBufferInfo(&traverseBlockData->descriptorInfo),
                }, nullptr);
            }

            {
                zerosBufferReference = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
                debugOnes32BufferReference = createBuffer(device, strided<uint32_t>(1024), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // minmaxes
                std::vector<bbox> minmaxes(32);
                std::for_each(std::execution::par_unseq, minmaxes.begin(), minmaxes.end(), [&](auto&& m) { m.mn = glm::vec4(10000.f), m.mx = glm::vec4(-10000.f); });

                // zeros and ones
                std::vector<uint32_t> zeros(1024), ones(1024);
                std::for_each(std::execution::par_unseq, zeros.begin(), zeros.end(), [&](auto&& m) { m = 0u; });
                std::for_each(std::execution::par_unseq, ones.begin(), ones.end(), [&](auto&& m) { m = 1u; });

                // make reference buffers
                auto command = getCommandBuffer(device, true);
                bufferSubData(command, zerosBufferReference, zeros, 0); // make reference of zeros
                bufferSubData(command, debugOnes32BufferReference, ones, 0);
                flushCommandBuffer(device, command, true);
            }

            {
                // create client geometry uniform buffer
                geometryBlockData = std::vector<GeometryBlockUniform>(1);
                geometryBlockUniform.buffer = createBuffer(device, strided<GeometryBlockUniform>(1), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
                geometryBlockUniform.staging = createBuffer(device, strided<GeometryBlockUniform>(1), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);

                // descriptor templates
                auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(clientDescriptorSets[1]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
                device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(3).setPBufferInfo(&geometryBlockUniform.buffer->descriptorInfo)
                }, nullptr);
            }
        }

        void HieararchyStorage::allocatePrimitiveReserve(size_t primitiveCount)
        {
            // storage
            size_t _MAX_HEIGHT = tiled(primitiveCount * 3 *ATTRIBUTE_EXTENT, _WIDTH)+1;
            attributeTexelStorage = createTexture(device, vk::ImageViewType::e2D, vk::Extent3D{ uint32_t(_WIDTH), uint32_t(_MAX_HEIGHT), 1 }, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR32G32B32A32Uint);
            materialIndicesStorage = createBuffer(device, strided<uint32_t>(primitiveCount), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            orderIndicesStorage = createBuffer(device, strided<uint32_t>(primitiveCount), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            vertexLinearStorage = createBuffer(device, strided<float>(primitiveCount * 9), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // descriptor templates
            auto sampler = device->logical.createSampler(vk::SamplerCreateInfo().setMagFilter(vk::Filter::eNearest).setMinFilter(vk::Filter::eNearest).setAddressModeU(vk::SamplerAddressMode::eRepeat).setAddressModeV(vk::SamplerAddressMode::eMirrorClampToEdge).setCompareEnable(false));
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(clientDescriptorSets[1]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setDstBinding(10).setPImageInfo(&attributeTexelStorage->descriptorInfo.setSampler(sampler)),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(1).setPBufferInfo(&materialIndicesStorage->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(2).setPBufferInfo(&orderIndicesStorage->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(7).setPBufferInfo(&vertexLinearStorage->descriptorInfo),
            }, nullptr);
        }

        void HieararchyStorage::allocateNodeReserve(size_t nodeCount) {
            bvhBoxStorage = createBuffer(device, strided<glm::mat4>(nodeCount), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            bvhMetaStorage = createBuffer(device, strided<glm::ivec4>(nodeCount * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // descriptor templates
            auto sampler = device->logical.createSampler(vk::SamplerCreateInfo().setMagFilter(vk::Filter::eNearest).setMinFilter(vk::Filter::eNearest).setAddressModeU(vk::SamplerAddressMode::eRepeat).setAddressModeV(vk::SamplerAddressMode::eMirrorClampToEdge).setCompareEnable(false));
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(clientDescriptorSets[1]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(0).setPBufferInfo(&bvhBoxStorage->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(5).setPBufferInfo(&bvhMetaStorage->descriptorInfo),
            }, nullptr);
        }

        void HieararchyStorage::queryTraverse(TraversibleData& tbsData) {
            // set ray-tracing buffers for traversing
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(clientDescriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(0).setPBufferInfo(&tbsData.raysUnordered),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(1).setPBufferInfo(&tbsData.hitBuffer),
                vk::WriteDescriptorSet(desc0Tmpl).setDescriptorType(vk::DescriptorType::eStorageBuffer).setDstBinding(2).setPBufferInfo(&tbsData.counterBuffer)
            }, nullptr);

            // dispatch traversing
            dispatchCompute(bvhTraverse, INTENSIVITY, clientDescriptorSets);
            dispatchCompute(vertexInterpolator, INTENSIVITY, clientDescriptorSets);
        }
    }
} // namespace NSM
