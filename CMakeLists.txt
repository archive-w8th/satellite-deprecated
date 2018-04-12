cmake_minimum_required (VERSION 3.0)
set (PROJECT_NAME satellite-core)
set (APPLICATION_NAME satellite-gltf)
project (${PROJECT_NAME})

if (COMPILER_ID MATCHES "MSVC")
  set(MSVC_${lang}_ARCHITECTURE_ID "${ARCHITECTURE_ID}")
endif()

set(SOURCES_LIST 
    "./testapps/include/*/*.hpp"
    "./testapps/include/*/*.hh"
    "./testapps/include/*/*.h"
    "./testapps/include/*/*/*.hpp"
    "./testapps/include/*/*/*.hh"
    "./testapps/include/*/*/*.h"
    "./testapps/include/*.hpp"
    "./testapps/include/*.hh"
    "./testapps/include/*.h"
    "./testapps/imgui/*.hpp"
    "./testapps/imgui/*.hh"
    "./testapps/imgui/*.h"
    "./testapps/imgui/imgui_draw.cpp"
    "./testapps/imgui/imgui.cpp"
    "./testapps/source/*.hpp"
    
    "./include/*/*.h"
    "./include/*/*.hpp"
    "./include/*/*.inl"
    "./include/*/*/*.h"
    "./include/*/*/*.hpp"
    "./include/*/*/*.inl"
    "./include/*/*/*/*.h"
    "./include/*/*/*/*.hpp"
    "./include/*/*/*/*.inl"
    "./include/*/*/*/*/*.h"
    "./include/*/*/*/*/*.hpp"
    "./include/*/*/*/*/*.inl"
    "./include/*/*/*/*/*/*.h"
    "./include/*/*/*/*/*/*.hpp"
    "./include/*/*/*/*/*/*.inl"
)

set (DEFINES
    -DNOMINMAX
    -DGLM_ENABLE_EXPERIMENTAL
    -DGLFW_INCLUDE_NONE
    -DUSE_CIMG
    -DEXPERIMENTAL_GLTF
    -DNSM=ste
)

if(MSVC OR MSVS OR MSYS OR MINGW)
    set(DEFINES ${DEFINES} -DOS_WIN) 
endif()

if(UNIX AND NOT APPLE)
    set(DEFINES ${DEFINES} -DOS_LNX) 
endif()


set (DEP_DIR "${PROJECT_SOURCE_DIR}")
set (LIB_DIR "${CMAKE_MODULE_PATH}/lib"     "${DEP_DIR}/lib"     "${DEP_DIR}/testapps/lib")
set (INC_DIR "${CMAKE_MODULE_PATH}/include" "${DEP_DIR}/include" "${DEP_DIR}/testapps/" "${DEP_DIR}/testapps/include" "${DEP_DIR}/testapps/imgui" "${PROJECT_SOURCE_DIR}")

set (CMAKE_MODULE_PATH 
    "${DEP_DIR}/modules" 
    "${CMAKE_MODULE_PATH}/modules" 
    "${PROJECT_SOURCE_DIR}/modules/" )

#set(ENABLE_OPTIX_DENOISER OFF CACHE BOOL "Enable support of OptiX denoiser (be aware with NVidia EULA)")

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
    turbojpeg
    turbojpeg-static
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

set (CMAKE_C_EXTENSIONS ON)
set (CMAKE_CXX_EXTENSIONS ON)

if(MSVC)
set (CMAKE_CXX_FLAGS "/EHa /Qspectre /std:c++latest /permissive- /arch:AVX2 /MD /DAVX2 /Ox /Ob2 /Oi /Ot /Oy /GT /GL /Gy /DMSVC /Debug:fastlink")
set (CMAKE_C_FLAGS "/std:c11 /debug:fastlink")
else()
set (CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++2a -march=skylake -mtune=skylake")
set (CMAKE_C_FLAGS "")
endif()

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS}")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS}")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS}")

set (INCLUDE_LIST 
    ${INC_DIR}
    ${Vulkan_INCLUDE_DIRS}
)

add_definitions(${DEFINES})
link_directories(${LIB_DIR})
include_directories(${INCLUDE_LIST})
file (GLOB RSOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${SOURCES_LIST})

add_executable(${APPLICATION_NAME} testapps/source/gltfViewer.cpp ${RSOURCES})
target_link_libraries(${APPLICATION_NAME} ${LIBS})

foreach(source IN LISTS RSOURCES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()




add_executable(${APPLICATION_NAME}-instance testapps/source/gltfInstanced.cpp ${RSOURCES})
target_link_libraries(${APPLICATION_NAME}-instance ${LIBS})

foreach(source IN LISTS RSOURCES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()



