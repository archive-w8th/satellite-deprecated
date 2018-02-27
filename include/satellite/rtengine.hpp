//#pragma once // it is aggregation of implementation and headers

#include "./utils.hpp"
#include "./grlib/radixSort.hpp"
#include "./rtengine/structs.hpp"
#include "./rtengine/triangleHierarchy.hpp"
#include "./rtengine/pipeline.hpp"
#include "./rtengine/textureSet.hpp"
#include "./rtengine/materialSet.hpp"
#include "./rtengine/vertexInstance.hpp"
#include "./rtengine/vertexSubnodes.hpp"

// for C++ files, or implementations
#ifdef RT_ENGINE_IMPLEMENT
#include "./impl/utils.inl"
#include "./impl/grlib/radixSort.inl"
#include "./impl/rtengine/pipeline.inl"
#include "./impl/rtengine/triangleHierarchy.inl"
#include "./impl/rtengine/textureSet.inl"
#include "./impl/rtengine/materialSet.inl"
#include "./impl/rtengine/vertexInstance.inl"
#include "./impl/rtengine/vertexSubnodes.inl"
#endif
