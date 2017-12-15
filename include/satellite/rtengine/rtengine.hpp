//#pragma once // it is aggregation of implementation and headers

#include "./structs.hpp"
#include "./triangleHierarchy.hpp"
#include "./pipeline.hpp"
#include "./textureSet.hpp"
#include "./materialSet.hpp"
#include "./vertexInstance.hpp"
#include "./vertexSubnodes.hpp"

// for C++ files, or implementations
#ifdef RT_ENGINE_IMPLEMENT
// dependency
#include "../utils.hpp"

// implementations
#include "./impl/pipeline.inl"
#include "./impl/triangleHierarchy.inl"
#include "./impl/textureSet.inl"
#include "./impl/materialSet.inl"
#include "./impl/vertexInstance.inl"
#include "./impl/vertexSubnodes.inl"
#endif
