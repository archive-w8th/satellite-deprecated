:: It is helper for compilation shaders to SPIR-V

cd %~dp0
  set CFLAGSV= -V120 --target-env vulkan1.0 -t -r --nsf -DUSE_F32_BVH -DWARP_SIZE=8

set INDIR=.\
::set OUTDIR=..\Build\shaders-spv\
set OUTDIR=..\Build\intel\
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
