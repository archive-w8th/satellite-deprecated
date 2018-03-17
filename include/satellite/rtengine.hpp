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
#include "./detail/gapi.inl"
#include "./detail/grlib/radixSort.inl"
#include "./detail/rtengine/materialSet.inl"
#include "./detail/rtengine/pipeline.inl"
#include "./detail/rtengine/samplerSet.inl"
#include "./detail/rtengine/textureSet.inl"
#include "./detail/rtengine/triangleHierarchy.inl"
#include "./detail/rtengine/vertexInstance.inl"
#include "./detail/rtengine/vertexSubnodes.inl"
#endif
