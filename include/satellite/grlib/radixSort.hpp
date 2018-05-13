#pragma once

#include "../gapi.hpp"

namespace NSM {
    namespace gr {

        class RadixSort : public std::enable_shared_from_this<RadixSort> {
        protected:
            friend class RadixSort;

            ComputeContext histogram, permute, workPrefixSum, singleRadix;

            Queue queue;
            Device device;

            Buffer TmpKeys, TmpValues, VarBuffer, Histograms, PrefixSums, VarStaging;

            std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
            std::vector<vk::DescriptorSet> descriptorSets;
            vk::PipelineLayout pipelineLayout;

            struct Consts { uint32_t NumKeys, Shift, Descending, IsSigned; };
            const uint32_t WG_COUNT = 64;
            const uint32_t RADICE_AFFINE = 1;
            std::string shadersPathPrefix = "shaders-spv";
            void init(Queue& queue);

        public:
            RadixSort() {}
            RadixSort(Queue& queue, std::string shadersPack) { shadersPathPrefix = shadersPack; init(queue); }
            RadixSort(RadixSort& another); // copying/reference constructor
            RadixSort(RadixSort&& another); // copying/reference constructor

            // sorting
            void sort(Buffer& InKeys, Buffer& InVals, uint32_t size = 1, uint32_t descending = 0);
        };

    }
}