:: It is helper for compilation shaders to SPIR-V

set PATH=C:\Users\elvir\msvc\glslang\bin;%PATH%

cd %~dp0
  set CFLAGSV= --client vulkan100 --target-env vulkan1.1 -d --aml -DAMD_PLATFORM -DUSE_F32_BVH -DENABLE_AMD_INSTRUCTION_SET 

set INDIR=.\
set OUTDIR=..\Build\shaders\amd\
set OUTSHR=..\Build\shaders\
set VRTX=vertex\
set RNDR=rendering\
set HLBV=hlbvh2\
set RDXI=radix\
set OUTP=output\


set CMPPROF=-fshader-stage=compute
set FRGPROF=-fshader-stage=fragment
set VRTPROF=-fshader-stage=vertex
set GMTPROF=-fshader-stage=geometry

call shaders-list.cmd

pause
