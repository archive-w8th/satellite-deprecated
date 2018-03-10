#pragma once

#include <chrono>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <array>
#include <map>
#include <random>
#include <vector>
#include <algorithm>

#include <half.hpp> // force include half's
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>

#ifdef USE_CIMG
#include "tinyexr.h"
#define cimg_plugin "CImg/tinyexr_plugin.hpp"
#define cimg_use_png
#define cimg_use_jpeg
#include "CImg.h"
#endif

#ifndef NSM
#define NSM ste
#endif

namespace NSM
{
    auto rand64u()
    {
        auto dvc = std::mt19937_64(std::random_device()());
        return std::uniform_int_distribution<uint64_t>(0ull, _UI64_MAX)(dvc);
    }

    auto randm()
    {
        auto dvc = std::mt19937(std::random_device()());
        return std::uniform_int_distribution<uint32_t>(0u, UINT_MAX)(dvc);
    }

    auto randm(const unsigned max)
    {
        auto dvc = std::mt19937(std::random_device()());
        return std::uniform_int_distribution<uint32_t>(0u, max)(dvc);
    }

    auto randf()
    {
        auto dvc = std::mt19937(std::random_device()());
        return std::uniform_real_distribution<float>(0.f, 1.f)(dvc);
    }

    template <typename T>
    auto sgn(T val) { return (T(0) < val) - (val < T(0)); }

    static int32_t tiled(int32_t sz, int32_t gmaxtile)
    {
        // return (int32_t)ceil((double)sz / (double)gmaxtile);
        return sz <= 0 ? 0 : (sz / gmaxtile + sgn(sz % gmaxtile));
    }

    static double milliseconds()
    {
        auto duration = std::chrono::high_resolution_clock::now();
        double millis = std::chrono::duration_cast<std::chrono::nanoseconds>(
            duration.time_since_epoch())
            .count() /
            1000000.0;
        return millis;
    }

    template <class T>
    size_t strided(size_t sizeo) { return sizeof(T) * sizeo; }

    const int32_t zero[1] = { 0 };
} // namespace NSM
