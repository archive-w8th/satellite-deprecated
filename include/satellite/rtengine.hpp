//#pragma once // it is aggregation of implementation and headers

#ifndef RAY_TRACING_ENGINE
#define RAY_TRACING_ENGINE
#endif

#include "./gapi.hpp"
#include "./grlib/radixSort.hpp"
#include "./rtengine/structs.hpp"
#include "./rtengine/materialSet.hpp"
#include "./rtengine/pipeline.hpp"
#include "./rtengine/samplerSet.hpp"
#include "./rtengine/textureSet.hpp"
#include "./rtengine/triangleHierarchy.hpp"
#include "./rtengine/vertexInstance.hpp"
#include "./rtengine/vertexSubnodes.hpp"

// for C++ files, or implementations
#ifdef RT_ENGINE_IMPLEMENT
#include "./impl/gapi.inl"
#include "./impl/grlib/radixSort.inl"
#include "./impl/rtengine/materialSet.inl"
#include "./impl/rtengine/pipeline.inl"
#include "./impl/rtengine/samplerSet.inl"
#include "./impl/rtengine/textureSet.inl"
#include "./impl/rtengine/triangleHierarchy.inl"
#include "./impl/rtengine/vertexInstance.inl"
#include "./impl/rtengine/vertexSubnodes.inl"
#endif
