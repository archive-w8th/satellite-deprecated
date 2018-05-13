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
#include "./rtengine/vertex/vertexInstance.hpp"
#include "./rtengine/vertex/vertexSubnodes.hpp"
#include "./rtengine/vertex/geometryAccumulator.hpp"

// for C++ files, or implementations
#ifdef RT_ENGINE_IMPLEMENT
#include "./gapi.inl"
#include "./grlib/radixSort.inl"
#include "./rtengine/materialSet.inl"
#include "./rtengine/pipeline.inl"
#include "./rtengine/samplerSet.inl"
#include "./rtengine/textureSet.inl"
#include "./rtengine/triangleHierarchy.inl"
#include "./rtengine/vertex/vertexInstance.inl"
#include "./rtengine/vertex/vertexSubnodes.inl"
#include "./rtengine/vertex/geometryAccumulator.inl"
#endif
