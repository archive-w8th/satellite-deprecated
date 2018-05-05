:: It is helper for compilation shaders to SPIR-V

mkdir %OUTDIR%
mkdir %OUTDIR%%VRTX%
mkdir %OUTDIR%%RNDR%
mkdir %OUTDIR%%HLBV%
mkdir %OUTDIR%%RDXI%
mkdir %OUTDIR%%OUTP%
mkdir %OUTDIR%%GENG%
mkdir %OUTDIR%%HLBV%next-gen-sort


start /b /wait glslangValidator %CFLAGSV% %INDIR%%OUTP%render.frag        -o %OUTDIR%%OUTP%render.frag.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%OUTP%render.vert        -o %OUTDIR%%OUTP%render.vert.spv

start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%pclear.comp         -o %OUTDIR%%RNDR%pclear.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%pgather.comp        -o %OUTDIR%%RNDR%pgather.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%accumulation.comp   -o %OUTDIR%%RNDR%accumulation.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%gen-primary.comp    -o %OUTDIR%%RNDR%gen-primary.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%gen-secondary.comp  -o %OUTDIR%%RNDR%gen-secondary.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%traverse-pre.comp   -o %OUTDIR%%RNDR%traverse-pre.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%traverse-bvh.comp   -o %OUTDIR%%RNDR%traverse-bvh.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%interpolator.comp   -o %OUTDIR%%RNDR%interpolator.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RNDR%hit-shader.comp     -o %OUTDIR%%RNDR%hit-shader.comp.spv

start /b /wait glslangValidator %CFLAGSV% %INDIR%%VRTX%vloader.comp       -o %OUTDIR%%VRTX%vloader.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%HLBV%bound-calc.comp    -o %OUTDIR%%HLBV%bound-calc.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%HLBV%bvh-build.comp     -o %OUTDIR%%HLBV%bvh-build.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%HLBV%bvh-fit.comp       -o %OUTDIR%%HLBV%bvh-fit.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%HLBV%leaf-gen.comp      -o %OUTDIR%%HLBV%leaf-gen.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%HLBV%leaf-link.comp     -o %OUTDIR%%HLBV%leaf-link.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RDXI%permute.comp       -o %OUTDIR%%RDXI%permute.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RDXI%histogram.comp     -o %OUTDIR%%RDXI%histogram.comp.spv
start /b /wait glslangValidator %CFLAGSV% %INDIR%%RDXI%pfx-work.comp      -o %OUTDIR%%RDXI%pfx-work.comp.spv

:: --ccp not supported by that renderer 

set FIXFLAGS = ^
--skip-validation ^
--strip-debug ^
--workaround-1209 ^
--replace-invalid-opcode ^
--simplify-instructions ^
--cfg-cleanup ^
-Os

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
--scalar-replacement ^
--simplify-instructions


:: for workarounds
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%interpolator.comp.spv    -o %OUTDIR%%RNDR%interpolator.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%accumulation.comp.spv    -o %OUTDIR%%RNDR%accumulation.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%gen-primary.comp.spv     -o %OUTDIR%%RNDR%gen-primary.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%gen-secondary.comp.spv   -o %OUTDIR%%RNDR%gen-secondary.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%hit-shader.comp.spv      -o %OUTDIR%%RNDR%hit-shader.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%pgather.comp.spv         -o %OUTDIR%%RNDR%pgather.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%pclear.comp.spv          -o %OUTDIR%%RNDR%pclear.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RDXI%permute.comp.spv         -o %OUTDIR%%RDXI%permute.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RDXI%histogram.comp.spv       -o %OUTDIR%%RDXI%histogram.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RDXI%pfx-work.comp.spv        -o %OUTDIR%%RDXI%pfx-work.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%traverse-pre.comp.spv    -o %OUTDIR%%RNDR%traverse-pre.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%RNDR%traverse-bvh.comp.spv    -o %OUTDIR%%RNDR%traverse-bvh.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%HLBV%bound-calc.comp.spv      -o %OUTDIR%%HLBV%bound-calc.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%HLBV%bvh-build.comp.spv       -o %OUTDIR%%HLBV%bvh-build.comp
call spirv-opt %FIXFLAGS% %OUTDIR%%HLBV%bvh-fit.comp.spv         -o %OUTDIR%%HLBV%bvh-fit.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%HLBV%leaf-gen.comp.spv        -o %OUTDIR%%HLBV%leaf-gen.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%HLBV%leaf-link.comp.spv       -o %OUTDIR%%HLBV%leaf-link.comp.spv
call spirv-opt %FIXFLAGS% %OUTDIR%%VRTX%vloader.comp.spv         -o %OUTDIR%%VRTX%vloader.comp.spv

:: for optimize
call spirv-opt %OPTFLAGS% %OUTDIR%%HLBV%bvh-build.comp.spv       -o %OUTDIR%%HLBV%bvh-build.comp
call spirv-opt %OPTFLAGS% %OUTDIR%%HLBV%bvh-fit.comp.spv         -o %OUTDIR%%HLBV%bvh-fit.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RDXI%permute.comp.spv         -o %OUTDIR%%RDXI%permute.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RDXI%histogram.comp.spv       -o %OUTDIR%%RDXI%histogram.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RDXI%pfx-work.comp.spv        -o %OUTDIR%%RDXI%pfx-work.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%RNDR%traverse-pre.comp.spv    -o %OUTDIR%%RNDR%traverse-pre.comp.spv
call spirv-opt %OPTFLAGS% %OUTDIR%%VRTX%vloader.comp.spv         -o %OUTDIR%%VRTX%vloader.comp.spv

:: unsupported
::call spirv-opt %OPTFLAGS% %OUTDIR%%RNDR%traverse-bvh.comp.spv    -o %OUTDIR%%RNDR%traverse-bvh.comp.spv 
