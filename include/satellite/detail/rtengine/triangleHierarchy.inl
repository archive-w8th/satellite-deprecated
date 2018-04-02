#pragma once

#include "../../rtengine/triangleHierarchy.hpp"

#ifndef ACCELERATION_IMPLEMENTATION
#define ACCELERATION_IMPLEMENTATION "hlBVH2-native" // identify accelerator implementation by string name
#include "./hlbvh2-accelerator/geometryAccumulator.inl"
#include "./hlbvh2-accelerator/hierarchyBuilder.inl"
#include "./hlbvh2-accelerator/hierarchyStorage.inl"
#endif