:: It is helper for compilation shaders to SPIR-V

mkdir %OUTDIR%
mkdir %OUTDIR%%VRTX%
mkdir %OUTDIR%%RNDR%
mkdir %OUTDIR%%HLBV%
mkdir %OUTDIR%%RDXI%
mkdir %OUTDIR%%OUTP%
mkdir %OUTDIR%%GENG%
mkdir %OUTDIR%%HLBV%next-gen-sort

call glslc %CFLAGS% %CMPPROF% %INDIR%%VRTX%loader.comp        -o %OUTDIR%%VRTX%loader.comp.spv -DINVERT_TX_Y
call glslc %CFLAGS% %CMPPROF% %INDIR%%VRTX%loader.comp        -o %OUTDIR%%VRTX%loader-int16.comp.spv -DINVERT_TX_Y -DENABLE_INT16_LOADING

call glslc %CFLAGS% %FRGPROF% %INDIR%%OUTP%render.frag        -o %OUTDIR%%OUTP%render.frag.spv
call glslc %CFLAGS% %VRTPROF% %INDIR%%OUTP%render.vert        -o %OUTDIR%%OUTP%render.vert.spv

call glslc %CFLAGS% %CMPPROF% %INDIR%%RNDR%clear.comp         -o %OUTDIR%%RNDR%clear.comp.spv
call glslc %CFLAGS% %CMPPROF% %INDIR%%RNDR%scatter.comp       -o %OUTDIR%%RNDR%scatter.comp.spv
call glslc %CFLAGS% %CMPPROF% %INDIR%%RNDR%bin-collect.comp   -o %OUTDIR%%RNDR%bin-collect.comp.spv

call glslc %CFLAGS% %CMPPROF% %INDIR%%RNDR%deinterlace.comp   -o %OUTDIR%%RNDR%deinterlace.comp.spv
call glslc %CFLAGS% %CMPPROF% %INDIR%%RNDR%filter.comp        -o %OUTDIR%%RNDR%filter.comp.spv

call glslc %CFLAGS% %CMPPROF% %INDIR%%RNDR%generation.comp    -o %OUTDIR%%RNDR%generation.comp.spv
call glslc %CFLAGS% %CMPPROF% %INDIR%%RNDR%bvh-traverse.comp  -o %OUTDIR%%RNDR%bvh-traverse.comp.spv
call glslc %CFLAGS% %CMPPROF% %INDIR%%RNDR%surface.comp       -o %OUTDIR%%RNDR%surface.comp.spv
call glslc %CFLAGS% %CMPPROF% %INDIR%%RNDR%rayshading.comp    -o %OUTDIR%%RNDR%rayshading.comp.spv

call glslc %CFLAGS% %CMPPROF% %INDIR%%HLBV%build-new.comp     -o %OUTDIR%%HLBV%build-new.comp.spv
call glslc %CFLAGS% %CMPPROF% %INDIR%%HLBV%aabbmaker.comp     -o %OUTDIR%%HLBV%aabbmaker.comp.spv
call glslc %CFLAGS% %CMPPROF% %INDIR%%HLBV%minmax.comp        -o %OUTDIR%%HLBV%minmax.comp.spv
call glslc %CFLAGS% %CMPPROF% %INDIR%%HLBV%refit.comp         -o %OUTDIR%%HLBV%refit.comp.spv
call glslc %CFLAGS% %CMPPROF% %INDIR%%HLBV%child-link.comp    -o %OUTDIR%%HLBV%child-link.comp.spv

call glslc %CFLAGS% %CMPPROF% %INDIR%%RDXI%permute.comp       -o %OUTDIR%%RDXI%permute.comp.spv
call glslc %CFLAGS% %CMPPROF% %INDIR%%RDXI%histogram.comp     -o %OUTDIR%%RDXI%histogram.comp.spv
call glslc %CFLAGS% %CMPPROF% %INDIR%%RDXI%pfx-work.comp      -o %OUTDIR%%RDXI%pfx-work.comp.spv

call glslc %CFLAGS% %FRGPROF% %INDIR%%GENG%pixel.frag         -o %OUTDIR%%GENG%pixel.frag.spv
call glslc %CFLAGS% %VRTPROF% %INDIR%%GENG%vertex.vert        -o %OUTDIR%%GENG%vertex.vert.spv
call glslc %CFLAGS% %VRTPROF% %INDIR%%GENG%vertex.vert        -o %OUTDIR%%GENG%vertex16i.vert.spv -DENABLE_INT16_LOADING


set OPTFLAGS= ^
--unify-const ^
--flatten-decorations ^
--fold-spec-const-op-composite ^
--strip-debug ^
--strength-reduction ^
--freeze-spec-const ^
--cfg-cleanup ^
--merge-blocks ^
--inline-entry-points-exhaustive ^
--convert-local-access-chains ^
--eliminate-insert-extract ^
--eliminate-dead-code-aggressive ^
--eliminate-dead-branches ^
--eliminate-dead-const ^
--eliminate-local-single-block ^
--eliminate-local-single-store ^
--eliminate-local-multi-store ^
--eliminate-common-uniform 



call spirv-opt %OPTFLAGS% %OUTDIR%%VRTX%loader.comp.spv         -o %OUTDIR%%VRTX%loader.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%VRTX%loader-int16.comp.spv   -o %OUTDIR%%VRTX%loader-int16.comp.spv

call spirv-opt %OPTFLAGS% %OUTDIR%%RNDR%bin-collect.comp.spv    -o %OUTDIR%%RNDR%bin-collect.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RNDR%bvh-traverse.comp.spv   -o %OUTDIR%%RNDR%bvh-traverse.comp.spv

call spirv-opt %OPTFLAGS% %OUTDIR%%RNDR%rayshading.comp.spv     -o %OUTDIR%%RNDR%rayshading.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RNDR%generation.comp.spv     -o %OUTDIR%%RNDR%generation.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RNDR%scatter.comp.spv        -o %OUTDIR%%RNDR%scatter.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RNDR%clear.comp.spv          -o %OUTDIR%%RNDR%clear.comp.spv

call spirv-opt %OPTFLAGS% %OUTDIR%%HLBV%aabbmaker.comp.spv      -o %OUTDIR%%HLBV%aabbmaker.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%HLBV%build-new.comp.spv      -o %OUTDIR%%HLBV%build-new.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%HLBV%minmax.comp.spv         -o %OUTDIR%%HLBV%minmax.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%HLBV%refit.comp.spv          -o %OUTDIR%%HLBV%refit.comp.spv

call spirv-opt %OPTFLAGS% %OUTDIR%%RDXI%histogram.comp.spv      -o %OUTDIR%%RDXI%histogram.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RDXI%permute.comp.spv        -o %OUTDIR%%RDXI%permute.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RDXI%pfx-work.comp.spv       -o %OUTDIR%%RDXI%pfx-work.comp.spv

:: optimized surface shader brokes application work
::call spirv-opt %OPTFLAGS% %OUTDIR%%RNDR%surface.comp.spv        -o %OUTDIR%%RNDR%surface.comp.spv 
