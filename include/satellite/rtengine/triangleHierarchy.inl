#pragma once

#include "triangleHierarchy.hpp"

#ifndef ACCELERATION_IMPLEMENTATION
#define ACCELERATION_IMPLEMENTATION "hlBVH2-native" // identify accelerator implementation by string name
#include "./accelerator/hierarchyBuilder.inl"
#include "./accelerator/hierarchyStorage.inl"
#endif
