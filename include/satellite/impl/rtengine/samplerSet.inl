//#pragma once

#include "../../rtengine/samplerSet.hpp"

namespace NSM {
    namespace rt {

        void SamplerSet::init(DeviceQueueType& device) {
            this->device = device;

            samplers = std::vector<SamplerType>(0);
            freedomSamplers = std::vector<size_t>(0);
        }

        SamplerSet::SamplerSet(SamplerSet& another) {
            device = another.device;
            freedomSamplers = another.freedomSamplers;
            samplers = another.samplers;
        }

        SamplerSet::SamplerSet(SamplerSet&& another) {
            device = std::move(another.device);
            freedomSamplers = std::move(another.freedomSamplers);
            samplers = std::move(another.samplers);
        }

        void SamplerSet::freeSampler(const uint32_t& idx) {
            freedomSamplers.push_back(idx - 1);
        }

        void SamplerSet::clearSamplers() {
            freedomSamplers.resize(0);
            samplers.resize(0);
        }

        void SamplerSet::setSampler(uint32_t location, const SamplerType& texture) {
            for (int i = 0; i < freedomSamplers.size(); i++) {
                if (freedomSamplers[i] == location - 1) freedomSamplers.erase(freedomSamplers.begin() + i);
            }
            if (samplers.size() < location) samplers.resize(location);
            samplers[location - 1] = texture;
        }

        uint32_t SamplerSet::addSampler(const SamplerType& texture) {
            uint32_t idx = 0;
            if (freedomSamplers.size() > 0) {
                idx = freedomSamplers[freedomSamplers.size() - 1];
                freedomSamplers.pop_back();
                samplers[idx] = texture;
            }
            else {
                samplers.push_back(texture);
                idx = samplers.size();
            }
            return idx;
        };

        bool SamplerSet::haveSamplers() {
            return samplers.size() > 0;
        }

        std::vector<SamplerType>& SamplerSet::getSamplers() {
            return samplers;
        }

    }
}