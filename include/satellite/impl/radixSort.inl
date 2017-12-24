#pragma once

#include "../utils.hpp"

namespace NSM {
    namespace rt {

        void RadixSort::init(DeviceQueueType& device) {
            this->device = device;

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
            pipelineCache = device->logical.createPipelineCache(vk::PipelineCacheCreateInfo());

            // pipelines
            histogram.pipeline = createCompute(device, shadersPathPrefix + "/radix/histogram.comp.spv", pipelineLayout, pipelineCache);
            permute.pipeline = createCompute(device, shadersPathPrefix + "/radix/permute.comp.spv", pipelineLayout, pipelineCache);
            workPrefixSum.pipeline = createCompute(device, shadersPathPrefix + "/radix/pfx-work.comp.spv", pipelineLayout, pipelineCache);

            // buffers
            VarStaging = createBuffer(device, strided<Consts>(8), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
            VarBuffer = createBuffer(device, strided<Consts>(8), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            TmpKeys = createBuffer(device, strided<uint64_t>(1024 * 1024 * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            TmpValues = createBuffer(device, strided<uint32_t>(1024 * 1024 * 2), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            Histograms = createBuffer(device, strided<uint32_t>(WG_COUNT * 256), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            PrefixSums = createBuffer(device, strided<uint32_t>(WG_COUNT * 256), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_GPU_ONLY);
            
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
            device = another.device;
            TmpKeys = another.TmpKeys;
            TmpValues = another.TmpValues;
            VarBuffer = another.VarBuffer;
            Histograms = another.Histograms;
            PrefixSums = another.PrefixSums;
            VarStaging = another.VarStaging;
            descriptorSetLayouts = another.descriptorSetLayouts;
            descriptorSets = another.descriptorSets;
            pipelineCache = another.pipelineCache;
            pipelineLayout = another.pipelineLayout;
        }


        // copying/reference constructor
        RadixSort::RadixSort(RadixSort&& another) {
            histogram = std::move(another.histogram);
            permute = std::move(another.permute);
            workPrefixSum = std::move(another.workPrefixSum);
            singleRadix = std::move(another.singleRadix);
            device = std::move(another.device);
            TmpKeys = std::move(another.TmpKeys);
            TmpValues = std::move(another.TmpValues);
            VarBuffer = std::move(another.VarBuffer);
            Histograms = std::move(another.Histograms);
            PrefixSums = std::move(another.PrefixSums);
            VarStaging = std::move(another.VarStaging);
            descriptorSetLayouts = std::move(another.descriptorSetLayouts);
            descriptorSets = std::move(another.descriptorSets);
            pipelineCache = std::move(another.pipelineCache);
            pipelineLayout = std::move(another.pipelineLayout);
        }



        void RadixSort::sort(BufferType& InKeys, BufferType& InVals, uint32_t size, uint32_t descending) {

            // write buffers to descriptors
            auto desc0Tmpl = vk::WriteDescriptorSet().setDstSet(descriptorSets[0]).setDstArrayElement(0).setDescriptorCount(1).setDescriptorType(vk::DescriptorType::eStorageBuffer);
            device->logical.updateDescriptorSets(std::vector<vk::WriteDescriptorSet>{
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(20).setPBufferInfo(&InKeys->descriptorInfo),
                vk::WriteDescriptorSet(desc0Tmpl).setDstBinding(21).setPBufferInfo(&InVals->descriptorInfo),
            }, nullptr);

            //const uint32_t stepCount = 32;
            const uint32_t stepCount = 16;
            //const uint32_t stepCount = 8;

            // create steps data
            std::vector<Consts> steps(stepCount);
            for (uint32_t i = 0; i < stepCount; i++) steps[i] = { size, i, descending, 0 };
            bufferSubData(VarStaging, steps, 0);


            // copy headers buffer command
            std::vector<vk::CommandBuffer> copyBuffers;
            for (int i = 0; i < stepCount; i++) {
                auto copyCounterCommand = getCommandBuffer(device, true);
                memoryCopyCmd(copyCounterCommand, VarStaging, VarBuffer, { strided<Consts>(i), 0, strided<Consts>(1) });
                copyCounterCommand.end();
                copyBuffers.push_back(copyCounterCommand);
            }

            // histogram command (include copy command)
            auto histogramCommand = getCommandBuffer(device, true);
            memoryCopyCmd(histogramCommand, InKeys, TmpKeys, { 0, 0, strided<uint64_t>(size) });
            memoryCopyCmd(histogramCommand, InVals, TmpValues, { 0, 0, strided<uint32_t>(size) });
            histogramCommand.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSets, nullptr);
            histogramCommand.bindPipeline(vk::PipelineBindPoint::eCompute, histogram.pipeline);
            histogramCommand.dispatch(WG_COUNT, RADICE_AFFINE, 1); // dispatch few counts
            histogramCommand.end();

            // work prefix command
            auto workPrefixCommand = getCommandBuffer(device, true);
            workPrefixCommand.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSets, nullptr);
            workPrefixCommand.bindPipeline(vk::PipelineBindPoint::eCompute, workPrefixSum.pipeline);
            workPrefixCommand.dispatch(1, 1, 1); // dispatch few counts
            workPrefixCommand.end();

            // permute command
            auto permuteCommand = getCommandBuffer(device, true);
            permuteCommand.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, descriptorSets, nullptr);
            permuteCommand.bindPipeline(vk::PipelineBindPoint::eCompute, permute.pipeline);
            permuteCommand.dispatch(WG_COUNT, RADICE_AFFINE, 1); // dispatch few counts
            permuteCommand.end();


            vk::PipelineStageFlags stageMasks = vk::PipelineStageFlagBits::eAllCommands;
            std::vector<vk::SubmitInfo> buildSubmitInfos;

            for (int j = 0; j < stepCount; j++) {
                // copy header
                buildSubmitInfos.push_back(vk::SubmitInfo()
                    .setWaitSemaphoreCount(0)
                    .setCommandBufferCount(1)
                    .setPCommandBuffers(&copyBuffers[j])
                );

                // histogram
                buildSubmitInfos.push_back(vk::SubmitInfo()
                    .setWaitSemaphoreCount(0)
                    .setCommandBufferCount(1)
                    .setPCommandBuffers(&histogramCommand)
                );

                // prefix sum
                buildSubmitInfos.push_back(vk::SubmitInfo()
                    .setWaitSemaphoreCount(0)
                    .setCommandBufferCount(1)
                    .setPCommandBuffers(&workPrefixCommand)
                );

                // permute
                buildSubmitInfos.push_back(vk::SubmitInfo()
                    .setWaitSemaphoreCount(0)
                    .setCommandBufferCount(1)
                    .setPCommandBuffers(&permuteCommand)
                );
            }

            // submit radix sort
            vk::Fence fence = device->logical.createFence(vk::FenceCreateInfo());
            device->queue.submit(buildSubmitInfos, fence);

            // asynchronous await for cleans
            //std::async([=]() {
                device->logical.waitForFences(1, &fence, true, DEFAULT_FENCE_TIMEOUT);
                device->logical.destroyFence(fence);
                device->logical.freeCommandBuffers(device->commandPool, copyBuffers);
                device->logical.freeCommandBuffers(device->commandPool, 1, &histogramCommand);
                device->logical.freeCommandBuffers(device->commandPool, 1, &workPrefixCommand);
                device->logical.freeCommandBuffers(device->commandPool, 1, &permuteCommand);
            //});

        }

    }
}