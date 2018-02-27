//#pragma once // it is aggregation of implementation and headers

#include "./structs.hpp"
#include "./triangleHierarchy.hpp"
#include "./pipeline.hpp"
#include "./textureSet.hpp"
#include "./materialSet.hpp"
#include "./vertexInstance.hpp"
#include "./vertexSubnodes.hpp"
#include "../utils.hpp"

// for C++ files, or implementations
#ifdef RT_ENGINE_IMPLEMENT
#include "../impl/utils.inl"
#include "../impl/radixSort.inl"
#include "./impl/pipeline.inl"
#include "./impl/triangleHierarchy.inl"
#include "./impl/textureSet.inl"
#include "./impl/materialSet.inl"
#include "./impl/vertexInstance.inl"
#include "./impl/vertexSubnodes.inl"
#endif
