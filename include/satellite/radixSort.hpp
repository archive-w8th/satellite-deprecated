#pragma once

#include "./utils.hpp"

namespace NSM {
    namespace rt {

        class RadixSort {
        protected:
            friend class RadixSort;

            ComputeContext histogram, permute, workPrefixSum, singleRadix;
            DeviceQueueType device;
            BufferType TmpKeys, TmpValues, VarBuffer, Histograms, PrefixSums, VarStaging;

            std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
            std::vector<vk::DescriptorSet> descriptorSets;
            vk::DescriptorPool descriptorPool;
            vk::PipelineCache pipelineCache;
            vk::PipelineLayout pipelineLayout;

            struct Consts { uint32_t NumKeys, Shift, Descending, IsSigned; };
            const uint32_t WG_COUNT = 32;
            std::string shadersPathPrefix = "shaders-spv";
            void init(DeviceQueueType& device);

        public:
            RadixSort() {}
            RadixSort(DeviceQueueType& device, std::string shadersPack) { shadersPathPrefix = shadersPack; init(device); }
            RadixSort(RadixSort& another); // copying/reference constructor
            RadixSort(RadixSort&& another); // copying/reference constructor

            // sorting
            void sort(BufferType& InKeys, BufferType& InVals, uint32_t size = 1, uint32_t descending = 0);
        };

    }
}