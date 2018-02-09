cmake_minimum_required (VERSION 3.0)
set (PROJECT_NAME satellite-core)
set (APPLICATION_NAME satellite-gltf)
project (${PROJECT_NAME})

if (COMPILER_ID MATCHES "MSVC")
  set(MSVC_${lang}_ARCHITECTURE_ID "${ARCHITECTURE_ID}")
endif()

set(SOURCES_LIST 
    "./external/include/*/*.hpp"
    "./external/include/*/*.hh"
    "./external/include/*/*.h"
    "./external/include/*/*/*.hpp"
    "./external/include/*/*/*.hh"
    "./external/include/*/*/*.h"
    "./external/include/*.hpp"
    "./external/include/*.hh"
    "./external/include/*.h"
    "./external/imgui/*.hpp"
    "./external/imgui/*.hh"
    "./external/imgui/*.h"
    "./external/imgui/imgui_draw.cpp"
    "./external/imgui/imgui.cpp"
    "./include/satellite/*.hpp"
    "./include/satellite/*.inl"
    "./include/satellite/*/*.hpp"
    "./include/satellite/*/*.inl"
    "./include/satellite/*/*/*.hpp"
    "./include/satellite/*/*/*.inl"
    "./source/*.hpp"
)

set (DEFINES
    -DNOMINMAX
    -DGLM_FORCE_SWIZZLE
    -DGLM_ENABLE_EXPERIMENTAL
    -DGLFW_INCLUDE_NONE
    -DGLM_FORCE_INLINE
    -DGLM_FORCE_EXPLICIT_CTOR
    -DGLM_FORCE_CXX14
    -DUSE_CIMG
    -DEXPERIMENTAL_GLTF
    -DTINYGLTF_IMPLEMENTATION
    -DSTB_IMAGE_IMPLEMENTATION
    -DNSM=ste
)

set (DEP_DIR "${PROJECT_SOURCE_DIR}")
set (LIB_DIR "${CMAKE_MODULE_PATH}/lib"     "${DEP_DIR}/lib"     "${DEP_DIR}/external/lib"     "${DEP_DIR}/external/vk-sdk/lib"     "${DEP_DIR}/external/vk-sdk/bin")
set (INC_DIR "${CMAKE_MODULE_PATH}/include" "${DEP_DIR}/include" "${DEP_DIR}/external/include" "${DEP_DIR}/external/vk-sdk/include" "${DEP_DIR}/external/imgui" "${PROJECT_SOURCE_DIR}")

set (CMAKE_MODULE_PATH 
    "${DEP_DIR}/modules" 
    "${CMAKE_MODULE_PATH}/modules" 
    "${PROJECT_SOURCE_DIR}/modules/" )

set(ENABLE_OPTIX_DENOISER OFF CACHE BOOL "Enable support of OptiX denoiser (be aware with NVidia EULA)")

find_package(Vulkan QUIET)

# search package as optional
if (ENABLE_OPTIX_DENOISER) 
find_package(CUDA QUIET)
find_package(OptiX QUIET)
endif()


set (LIBS 
	${Vulkan_LIBRARY} 
    glfw3 
    libpng16_static
    jpeg
    jpeg-static
    zlibstatic
)


# if OptiX was has
if (OptiX_INSTALL_DIR AND ENABLE_OPTIX_DENOISER) 
set (INC_DIR "${INC_DIR}" "${OptiX_INCLUDE}" ${CUDA_INCLUDE_DIRS})
set (LIB_DIR "${LIB_DIR}" "${OptiX_INSTALL_DIR}/lib")
set (LIBS ${LIBS} ${optix_LIBRARY} ${optix_prime_LIBRARY} ${optixu_LIBRARY} ${CUDA_LIBRARIES})
if (ENABLE_OPTIX_DENOISER) 
set (DEFINES ${DEFINES} -DOPTIX_DENOISER_HACK)
endif()
endif()






set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_EXTENSIONS ON)

if(MSVC)
set (CMAKE_CXX_FLAGS "/EHsc /MD /std:c++latest /arch:AVX2 /tune:skylake /permissive- /DAVX2 /DGLM_FORCE_AVX2 /Ox /Ob2 /Oi /Ot /Oy /GT /GL /DMSVC")
set (CMAKE_C_FLAGS "/EHsc /std:c11 /debug:fastlink")
else()
set (CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++2a -march=skylake -mtune=skylake -DGLM_FORCE_AVX2")
set (CMAKE_C_FLAGS "")
endif()

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -O9")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O9")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O9")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O9")

set (INCLUDE_LIST 
    ${INC_DIR}
    ${Vulkan_INCLUDE_DIRS}
)

add_definitions(${DEFINES})
link_directories(${LIB_DIR})
include_directories(${INCLUDE_LIST})
file (GLOB RSOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${SOURCES_LIST})

add_executable(${APPLICATION_NAME} source/gltfViewer.cpp ${RSOURCES})
target_link_libraries(${APPLICATION_NAME} ${LIBS})

foreach(source IN LISTS RSOURCES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()


