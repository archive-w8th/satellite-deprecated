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

            Queue queue;
            Device device;
            void init(Queue &device);

            std::vector<size_t> freedomSamplers;
            std::vector<Sampler> samplers;

        public:
            SamplerSet() {}
            SamplerSet(Queue &device) { init(device); }
            SamplerSet(SamplerSet &another);
            SamplerSet(SamplerSet &&another);

            void freeSampler(const int32_t& idx);
            void clearSamplers();
            void setSampler(const int32_t& location, const Sampler &texture);
            bool haveSamplers();
            std::vector<Sampler> &getSamplers();
            int32_t addSampler(const Sampler &texture);
        };
    } // namespace rt
} // namespace NSM