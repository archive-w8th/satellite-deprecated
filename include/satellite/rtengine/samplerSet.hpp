#pragma once

#include "./structs.hpp"

namespace NSM
{
    namespace rt
    {
        class SamplerSet
        {
        protected:
            friend class SamplerSet;

            DeviceQueueType device;
            void init(DeviceQueueType &device);

            std::vector<size_t> freedomSamplers;
            std::vector<SamplerType> samplers;

        public:
            SamplerSet() {}
            SamplerSet(DeviceQueueType &device) { init(device); }
            SamplerSet(SamplerSet &another);
            SamplerSet(SamplerSet &&another);

            void freeSampler(const int32_t& idx);
            void clearSamplers();
            void setSampler(const int32_t& location, const SamplerType &texture);
            bool haveSamplers();
            std::vector<SamplerType> &getSamplers();
            int32_t addSampler(const SamplerType &texture);
        };
    } // namespace rt
} // namespace NSM