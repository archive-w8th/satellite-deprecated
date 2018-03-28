# Satellite OEM (VK-1.1)

[![Gitter](https://badges.gitter.im/world8th/satellite-oem.svg)](https://gitter.im/world8th/satellite-oem?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge) 

> Ray tracing source codes and SDK for making render and game engines (DIY). 

* **10.03.2018**: Support for Vulkan API 1.1.70 runtime 
* **20.03.2018**: Suspend of main support (only non-critical bug fixes)
* **24.03.2018**: Shot of some bug fixes with fp16 BVH and precisions, make new shader names 
* **26.03.2018**: Complete first charging of our project sources 
* **27.03.2018**: Begin of new ray-tracing saga... 

### What we can be do, if we had desires

* Appveyor, etc. support 
* Some validation tests
* Suggestion for AMD GPUOpen partnership

### Features from carton box

* Based on Vulkan API (compute purpose)
* Simple vertex data loader for accelerator builder
* Optimized HLBVH acceleration structure 
* Optimized radix-sort (64/4-bits)
* Optimized ray tracing pipeline (tile-based)
* One testing example application (source code)
* Around 40MRays/s throughput in Sponza scenes (RX Vega 64)

### Demo and preview videos

* https://www.youtube.com/watch?v=ix-Jwl2OchE (from author, preview video, 720p)
* https://www.youtube.com/watch?v=eh9Tp86x8Ps (from author, another scene, 720p)

### Contacts 

* [Our Discord](https://discordapp.com/invite/HFfADHH)
* [Our Reddit](https://www.reddit.com/user/elviras9t/)

### Patreons

* [Make ray-tracing forces with Patreon](https://www.patreon.com/open_ray_traced_forces)

### Minimal requirement for minimal product

* Windows platform with IDE (Visual Studio 2017 and higher)
* GLM (https://github.com/g-truc/glm)
* Vulkan API 1.1 (https://www.lunarg.com/vulkan-sdk/)
* Vulkan memory allocator (https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)

### Eye bleeding picture

<details>
<summary>Prepare yourself...</summary>
<img src="renders-fixed/city0.png" width="720">
<img src="renders-fixed/city1.png" width="720">
<img src="renders-fixed/city2.png" width="720">
</details>
