# Satellite OEM (VK-1.1.70)

[![Gitter](https://badges.gitter.im/world8th/satellite-oem.svg)](https://gitter.im/world8th/satellite-oem?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge) 

> Ray tracing source codes and SDK for making render and game engines (DIY). 

* **10.03.2018**: Support for Vulkan API 1.1.70 runtime 
* **20.03.2018**: Suspend of main support (only non-critical bug fixes)

### Features from carton box

* Based on Vulkan API (compute purpose)
* Simple vertex data loader for accelerator builder
* Optimized HLBVH acceleration structure 
* Optimized radix-sort (64/4-bits)
* Optimized ray tracing pipeline (tile-based)
* One testing example application (source code)

### Contacts 

* [Discord channel](https://discordapp.com/invite/HFfADHH)

### Minimal requirement for minimal product

* Windows platform with IDE (Visual Studio 2017 and higher)
* GLM (https://github.com/g-truc/glm)
* Vulkan API (preferly 1.1.70, includes Vulkan-Hpp) (https://www.lunarg.com/vulkan-sdk/)
* Vulkan memory allocator (https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)

### Remark 11.03.2018

State GLSL compiler with SPIR-V tools works wrong (LunarG). Very recommended to recompile glslang with SPIR-V Tools. 
