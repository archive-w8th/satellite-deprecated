:: It is helper for compilation shaders to SPIR-V

set PATH=C:\VulkanSDK\1.0.68.0\Bin;%PATH%

cd %~dp0
  set CFLAGSV= --client vulkan100 --target-env vulkan1.0 -s -r -DAMD_PLATFORM -DENABLE_AMD_INSTRUCTION_SET -DUSE_F32_BVH

set INDIR=.\
::set OUTDIR=..\Build\shaders-spv\
set OUTDIR=..\Build\amd\
set OUTSHR=..\Build\shaders\
set VRTX=vertex\
set RNDR=rendering\
set EXTM=exitum\
set ENGM=enigma\
set HLBV=hlbvh\
set RDXI=radix\
set OUTP=output\
set GENG=guiengine\

set CMPPROF=-fshader-stage=compute
set FRGPROF=-fshader-stage=fragment
set VRTPROF=-fshader-stage=vertex
set GMTPROF=-fshader-stage=geometry

call shaders-list.cmd

pause
