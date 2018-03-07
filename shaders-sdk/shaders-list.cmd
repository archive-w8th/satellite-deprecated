:: It is helper for compilation shaders to SPIR-V

mkdir %OUTDIR%
mkdir %OUTDIR%%VRTX%
mkdir %OUTDIR%%RNDR%
mkdir %OUTDIR%%HLBV%
mkdir %OUTDIR%%RDXI%
mkdir %OUTDIR%%OUTP%
mkdir %OUTDIR%%GENG%
mkdir %OUTDIR%%HLBV%next-gen-sort

start /b /wait glslangValidator %CFLAGSV% %INDIR%%VRTX%loader.comp        -o %OUTDIR%%VRTX%loader.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%VRTX%loader.comp        -o %OUTDIR%%VRTX%loader-int16.comp.spv -DENABLE_INT16_LOADING

start /b /wait glslangValidator %CFLAGSV% %INDIR%%OUTP%render.frag        -o %OUTDIR%%OUTP%render.frag.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%OUTP%render.vert        -o %OUTDIR%%OUTP%render.vert.spv

start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%clear.comp         -o %OUTDIR%%RNDR%clear.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%scatter.comp       -o %OUTDIR%%RNDR%scatter.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%bin-collect.comp   -o %OUTDIR%%RNDR%bin-collect.comp.spv

start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%deinterlace.comp   -o %OUTDIR%%RNDR%deinterlace.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%filter.comp        -o %OUTDIR%%RNDR%filter.comp.spv

start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%generation.comp    -o %OUTDIR%%RNDR%generation.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%bvh-traverse.comp  -o %OUTDIR%%RNDR%bvh-traverse.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%surface.comp       -o %OUTDIR%%RNDR%surface.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%rayshading.comp    -o %OUTDIR%%RNDR%rayshading.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%unordered.comp     -o %OUTDIR%%RNDR%unordered.comp.spv

start /b /wait glslangValidator %CFLAGSV% %INDIR%%HLBV%build-new.comp     -o %OUTDIR%%HLBV%build-new.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%HLBV%aabbmaker.comp     -o %OUTDIR%%HLBV%aabbmaker.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%HLBV%minmax.comp        -o %OUTDIR%%HLBV%minmax.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%HLBV%refit-incore.comp  -o %OUTDIR%%HLBV%refit.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%HLBV%child-link.comp    -o %OUTDIR%%HLBV%child-link.comp.spv

start /b /wait glslangValidator %CFLAGSV% %INDIR%%RDXI%permute.comp       -o %OUTDIR%%RDXI%permute.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RDXI%histogram.comp     -o %OUTDIR%%RDXI%histogram.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RDXI%pfx-work.comp      -o %OUTDIR%%RDXI%pfx-work.comp.spv

start /b /wait glslangValidator %CFLAGSV% %INDIR%%GENG%pixel.frag         -o %OUTDIR%%GENG%pixel.frag.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%GENG%vertex.vert        -o %OUTDIR%%GENG%vertex.vert.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%GENG%vertex.vert        -o %OUTDIR%%GENG%vertex16i.vert.spv -DENABLE_INT16_LOADING



:: --ccp not supported by that renderer 

set FIXFLAGS = ^
--skip-validation ^
--strip-debug ^
--workaround-1209 ^
--replace-invalid-opcode

set OPTFLAGS= ^
--skip-validation ^
--private-to-local ^
--ccp ^
--unify-const ^
--flatten-decorations ^
--fold-spec-const-op-composite ^
--strip-debug ^
--freeze-spec-const ^
--cfg-cleanup ^
--merge-blocks ^
--merge-return ^
--strength-reduction ^
--inline-entry-points-exhaustive ^
--convert-local-access-chains ^
--eliminate-dead-code-aggressive ^
--eliminate-dead-branches ^
--eliminate-dead-const ^
--eliminate-dead-variables ^
--eliminate-dead-functions ^
--eliminate-local-single-block ^
--eliminate-local-single-store ^
--eliminate-local-multi-store ^
--eliminate-common-uniform ^
--eliminate-insert-extract ^
--scalar-replacement ^
--relax-struct-store ^
--redundancy-elimination ^
--remove-duplicates ^
--private-to-local ^
--local-redundancy-elimination ^
--cfg-cleanup ^
--workaround-1209 ^
--replace-invalid-opcode ^
--if-conversion ^
--scalar-replacement

:: for optimize
call spirv-opt %OPTFLAGS% %OUTDIR%%RDXI%permute.comp.spv         -o %OUTDIR%%RDXI%permute.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RDXI%histogram.comp.spv       -o %OUTDIR%%RDXI%histogram.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RDXI%pfx-work.comp.spv        -o %OUTDIR%%RDXI%pfx-work.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RNDR%bvh-traverse.comp.spv    -o %OUTDIR%%RNDR%bvh-traverse.comp.spv

:: for some workarounds
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%unordered.comp.spv       -o %OUTDIR%%RNDR%unordered.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%rayshading.comp.spv      -o %OUTDIR%%RNDR%rayshading.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%generation.comp.spv      -o %OUTDIR%%RNDR%generation.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%surface.comp.spv         -o %OUTDIR%%RNDR%surface.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%scatter.comp.spv         -o %OUTDIR%%RNDR%scatter.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%HLBV%build-new.comp.spv       -o %OUTDIR%%HLBV%build-new.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%HLBV%aabbmaker.comp.spv       -o %OUTDIR%%HLBV%aabbmaker.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%HLBV%minmax.comp.spv          -o %OUTDIR%%HLBV%minmax.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%HLBV%refit.comp.spv           -o %OUTDIR%%HLBV%refit.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%HLBV%child-link.comp.spv      -o %OUTDIR%%HLBV%child-link.comp.spv

