#pragma once

#include "samplerSet.hpp"

namespace NSM
{
    namespace rt
    {

        void SamplerSet::init(Queue &queue)
        {
            this->queue = queue;
            this->device = queue->device;

            samplers = std::vector<Sampler>(0);
            freedomSamplers = std::vector<size_t>(0);
        }

        SamplerSet::SamplerSet(SamplerSet &another)
        {
            device = another.device;
            freedomSamplers = another.freedomSamplers;
            samplers = another.samplers;
        }

        SamplerSet::SamplerSet(SamplerSet &&another)
        {
            device = std::move(another.device);
            freedomSamplers = std::move(another.freedomSamplers);
            samplers = std::move(another.samplers);
        }

        void SamplerSet::freeSampler(const int32_t& idx)
        {
            freedomSamplers.push_back(idx);
        }

        void SamplerSet::clearSamplers()
        {
            freedomSamplers.resize(0);
            samplers.resize(0);
        }

        void SamplerSet::setSampler(const int32_t& location, const Sampler &texture)
        {
            for (int i = 0; i < freedomSamplers.size(); i++) {
                if (freedomSamplers[i] == location) freedomSamplers.erase(freedomSamplers.begin() + i);
            }
            if (samplers.size() <= location) samplers.resize(location+1);
            samplers[location] = texture;
        }

        int32_t SamplerSet::addSampler(const Sampler &sampler)
        {
            int32_t idx = -1;
            if (freedomSamplers.size() > 0) {
                idx = freedomSamplers[freedomSamplers.size() - 1];
                freedomSamplers.pop_back();
                samplers[idx] = sampler;
            } else {
                idx = samplers.size();
                samplers.push_back(sampler);
            }
            
            return idx;
        }

        bool SamplerSet::haveSamplers() { return samplers.size() > 0; }
        std::vector<Sampler>& SamplerSet::getSamplers() { return samplers; }
    }
}