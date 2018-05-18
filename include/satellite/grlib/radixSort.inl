#pragma once

#include "radixSort.hpp"

namespace NSM {
    namespace gr {

        void RadixSort::init(Queue queue) {
            this->device = queue->device;
            this->queue = queue;

            // define descriptor pool sizes
            std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
                vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 16),
            };

            std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
                vk::DescriptorSetLayoutBinding(20, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(21, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(24, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(25, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(26, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(27, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
                vk::DescriptorSetLayoutBinding(28, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
            };

            descriptorSetLayouts = {
                device->logical.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo().setPBindings(descriptorSetLayoutBindings.data()).setBindingCount(descriptorSetLayoutBindings.size()))
            };

            descriptorSets = device->logical.allocateDescriptorSets(
                vk::DescriptorSetAllocateInfo().setDescriptorPool(device->descriptorPool).setDescriptorSetCount(1).setPSetLayouts(descriptorSetLayouts.data())
            );

            // layout and cache
            pipelineLayout = device->logical.createPipelineLayout(vk::PipelineLayoutCreateInfo().setPSetLayouts(descriptorSetLayouts.data()).setSetLayoutCount(1));

            // pipelines
            histogram = createCompute(queue, shadersPathPrefix + "/radix/histogram.comp.spv", pipelineLayout);
            permute = createCompute(queue, shadersPathPrefix + "/radix/permute.comp.spv", pipelineLayout);
            workPrefixSum = createCompute(queue, shadersPathPrefix + "/radix/pfx-work.comp.spv", pipelineLayout);

            // buffers
            VarStaging = createBuffer(queue, strided<Consts>(8), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            VarBuffer = createBuffer(queue, strided<Consts>(8), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            TmpKeys = createBuffer(queue, strided<uint64_t>(1024 * 1024 * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            TmpValues = createBuffer(queue, strided<uint32_t>(1024 * 1024 * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            Histograms = createBuffer(queue, strided<uint32_t>(WG_COUNT * 16), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            PrefixSums = createBuffer(queue, strided<uint32_t>(WG_COUNT * 16), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);

            // write desks
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(24).setPBufferInfo(&VarBuffer->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(25).setPBufferInfo(&TmpKeys->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(26).setPBufferInfo(&TmpValues->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(27).setPBufferInfo(&Histograms->descriptorInfo),
                    vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(28).setPBufferInfo(&PrefixSums->descriptorInfo)
            }, nullptr);
        }

        // copying/reference constructor
        RadixSort::RadixSort(RadixSort& another) {
            histogram = another.histogram;
            permute = another.permute;
            workPrefixSum = another.workPrefixSum;
            singleRadix = another.singleRadix;
            queue = another.queue;
            device = another.device;
            TmpKeys = another.TmpKeys;
            TmpValues = another.TmpValues;
            VarBuffer = another.VarBuffer;
            Histograms = another.Histograms;
            PrefixSums = another.PrefixSums;
            VarStaging = another.VarStaging;
            descriptorSetLayouts = another.descriptorSetLayouts;
            descriptorSets = another.descriptorSets;
            pipelineLayout = another.pipelineLayout;
        }


        // copying/reference constructor
        RadixSort::RadixSort(RadixSort&& another) {
            histogram = std::move(another.histogram);
            permute = std::move(another.permute);
            workPrefixSum = std::move(another.workPrefixSum);
            singleRadix = std::move(another.singleRadix);
            queue = std::move(another.queue);
            device = std::move(another.device);
            TmpKeys = std::move(another.TmpKeys);
            TmpValues = std::move(another.TmpValues);
            VarBuffer = std::move(another.VarBuffer);
            Histograms = std::move(another.Histograms);
            PrefixSums = std::move(another.PrefixSums);
            VarStaging = std::move(another.VarStaging);
            descriptorSetLayouts = std::move(another.descriptorSetLayouts);
            descriptorSets = std::move(another.descriptorSets);
            pipelineLayout = std::move(another.pipelineLayout);
        }



        void RadixSort::sort(Buffer& InKeys, Buffer& InVals, uint32_t size, uint32_t descending) {

            // write buffers to descriptors
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(20).setPBufferInfo(&InKeys->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(21).setPBufferInfo(&InVals->descriptorInfo),
            }, nullptr);
            
            // create steps data
            const uint32_t stepCount = 16;
            std::vector<Consts> steps(stepCount);
            for (uint32_t i = 0; i < stepCount; i++) steps[i] = { size, i, descending, 0 };

            // upload to buffer
            auto commandBuffer = createCommandBuffer(queue, true);
            bufferSubData(commandBuffer, VarStaging, steps, 0);
            flushCommandBuffers(queue, { commandBuffer }, true);

            // copy headers buffer command
            std::vector<vk::CommandBuffer> copyBuffers;
            for (int i = 0; i < stepCount; i++) {
                auto copyCmd = createCopyCmd<Buffer &, Buffer &, vk::BufferCopy>(queue, VarStaging, VarBuffer, { strided<Consts>(i), 0, strided<Consts>(1) });
                //memoryCopyCmd(copyCmd, InKeys, TmpKeys , { 0, 0, strided<uint64_t>(size) });
                //memoryCopyCmd(copyCmd, InVals, TmpValues, { 0, 0, strided<uint32_t>(size) });
                copyCmd.end(); copyBuffers.push_back(copyCmd);
            }

            // copy back
            auto copyToBuffers = createCommandBuffer(queue, true);
            memoryCopyCmd(copyToBuffers, TmpKeys  , InKeys, { 0, 0, strided<uint64_t>(size) });
            memoryCopyCmd(copyToBuffers, TmpValues, InVals, { 0, 0, strided<uint32_t>(size) });
            copyToBuffers.end();

            // make command buffers
            auto histogramCommand = makeDispatchCommand(histogram, { WG_COUNT, RADICE_AFFINE, 1u }, descriptorSets);
            auto workPrefixCommand = makeDispatchCommand(workPrefixSum, { 1u, 1u, 1u }, descriptorSets);
            auto permuteCommand = makeDispatchCommand(permute, { WG_COUNT, RADICE_AFFINE, 1u }, descriptorSets);
            
            // stepped radix sort
            std::vector<vk::SubmitInfo> buildSubmitInfos;
            for (int j = 0; j < stepCount; j++) {
                buildSubmitInfos.push_back(vk::SubmitInfo().setWaitSemaphoreCount(0).setCommandBufferCount(1).setPCommandBuffers(&copyBuffers[j])); // copy header
                buildSubmitInfos.push_back(vk::SubmitInfo().setWaitSemaphoreCount(0).setCommandBufferCount(1).setPCommandBuffers(&histogramCommand)); // histogram
                buildSubmitInfos.push_back(vk::SubmitInfo().setWaitSemaphoreCount(0).setCommandBufferCount(1).setPCommandBuffers(&workPrefixCommand)); // prefix sum
                buildSubmitInfos.push_back(vk::SubmitInfo().setWaitSemaphoreCount(0).setCommandBufferCount(1).setPCommandBuffers(&permuteCommand)); // permute 
                buildSubmitInfos.push_back(vk::SubmitInfo().setWaitSemaphoreCount(0).setCommandBufferCount(1).setPCommandBuffers(&copyToBuffers)); // copy header
            }

            // submit radix sort
            vk::Fence fence = device->logical.createFence(vk::FenceCreateInfo());
            queue->queue.submit(buildSubmitInfos, fence);

            // async clean up
            std::async(std::launch::async | std::launch::deferred, [=](){
                device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
                device->logical.destroyFence(fence);
                device->logical.freeCommandBuffers(queue->commandPool, copyBuffers);
                device->logical.freeCommandBuffers(queue->commandPool, 1, &histogramCommand);
                device->logical.freeCommandBuffers(queue->commandPool, 1, &workPrefixCommand);
                device->logical.freeCommandBuffers(queue->commandPool, 1, &permuteCommand);
                device->logical.freeCommandBuffers(queue->commandPool, 1, &copyToBuffers);
            });

        }

    }
}
