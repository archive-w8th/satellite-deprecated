#pragma once

#define RAY_TRACING_ENGINE

#include <vector>
#include <iostream>
#include <chrono>
#include <array>
#include <random>
#include <map>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/component_wise.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/transform.hpp"

#ifdef USE_FREEIMAGE
#include "FreeImage.h"
#endif

#ifdef USE_CIMG
#define cimg_use_tinyexr
#define cimg_use_png
#define cimg_use_jpeg
#include "CImg.h"
#endif

#include "./vkutils/vkStructures.hpp"
#include "./vkutils/vkUtils.hpp"

#ifndef NSM
#define NSM ste
#endif

namespace NSM {

    auto randm() {
        auto dvc = std::mt19937(std::random_device()());
        return std::uniform_int_distribution<int>(0, 2147483647)(dvc);
    }

    template<typename T> auto sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }

    static int32_t tiled(int32_t sz, int32_t gmaxtile) {
        //return (int32_t)ceil((double)sz / (double)gmaxtile);
        return sz <= 0 ? 0 : (sz / gmaxtile + sgn(sz%gmaxtile));
    }

    static double milliseconds() {
        auto duration = std::chrono::high_resolution_clock::now();
        double millis = std::chrono::duration_cast<std::chrono::nanoseconds>(duration.time_since_epoch()).count() / 1000000.0;
        return millis;
    }

    template<class T>
    size_t strided(size_t sizeo) {
        return sizeof(T) * sizeo;
    }

    const int32_t zero[1] = { 0 };





}
