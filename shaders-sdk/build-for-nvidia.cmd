:: It is helper for compilation shaders to SPIR-V

set PATH=C:\Users\elvir\msvc\glslang\bin;%PATH%

cd %~dp0
  set CFLAGSV= --client vulkan100 --target-env vulkan1.1 -d --aml -DNVIDIA_PLATFORM -DUSE_F32_BVH -DPLAIN_BINDLESS_TEXTURE_FETCH
  
set INDIR=.\
set OUTDIR=..\Build\shaders\nvidia\
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
