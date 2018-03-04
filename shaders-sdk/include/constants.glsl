#ifndef _CONSTANTS_H
#define _CONSTANTS_H



    // data format extensions
    #extension GL_AMD_gcn_shader : enable
    #extension GL_AMD_gpu_shader_half_float : enable
    //#extension GL_AMD_gpu_shader_half_float_fetch : enable
    #extension GL_AMD_gpu_shader_int16 : enable 
    #extension GL_ARB_gpu_shader_int64 : require

    // intrinsics extensions
    #extension GL_AMD_shader_trinary_minmax : enable
    
    // texture extensions
    #extension GL_AMD_texture_gather_bias_lod : enable
    #extension GL_AMD_shader_image_load_store_lod : enable
    #extension GL_EXT_shader_image_load_formatted : enable

    // wave operations
    #extension GL_AMD_shader_ballot : enable
    #extension GL_ARB_shader_ballot : require
    #extension GL_ARB_shader_group_vote : require
    
    




// hardware or driver options
#define INT64_MORTON

#ifdef INT64_MORTON
#define MORTONTYPE uint64_t
#else
#define MORTONTYPE uint
#endif

// ray tracing options
//#define EXPERIMENTAL_DOF
#define SUNLIGHT_CAUSTICS false
#define REFRACTION_SKIP_SUN
#define DIRECT_LIGHT_ENABLED

// sampling options
//#define MOTION_BLUR
#ifndef SAMPLES_LOCK
#define SAMPLES_LOCK 1
#endif


// enable required GAPI extensions
#ifdef ENABLE_AMD_INSTRUCTION_SET
    #define ENABLE_AMD_INT16
    #define ENABLE_AMD_INT16_CONDITION
#endif

#ifndef ENABLE_AMD_INSTRUCTION_SET
#undef ENABLE_AMD_INT16 // not supported combination
#endif

#ifndef ENABLE_AMD_INT16
#undef ENABLE_AMD_INT16_CONDITION // required i16
#endif

#ifdef ENABLE_NVIDIA_INSTRUCTION_SET
#extension GL_NV_gpu_shader5 : enable // not supported by SPIRV
#endif

// System Constants
#define PZERO 0.0001f

// Platform-oriented compute
#ifndef WORK_SIZE
#ifdef ENABLE_AMD_INSTRUCTION_SET
#define WORK_SIZE 64
#else
#define WORK_SIZE 32
#endif
#endif
#define LOCAL_SIZE_LAYOUT layout ( local_size_x = WORK_SIZE ) in

// Math Constants
#define PHI 1.6180339887498948482
#define LONGEST -1
#define INFINITY 10000.0
#define PI 3.1415926535897932384626422832795028841971
#define TWO_PI 6.2831853071795864769252867665590057683943
#define SQRT_OF_ONE_THIRD 0.5773502691896257645091487805019574556476
#define E 2.7182818284590452353602874713526624977572

#endif
