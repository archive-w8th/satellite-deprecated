
// under scratching register system 
// do not use it


#ifndef F4CNT
#define F4CNT 1
#endif

#ifndef I4CNT
#define I4CNT 1
#endif

#ifndef F2CNT
#define F2CNT 2
#endif

#ifndef I2CNT
#define I2CNT 2
#endif

#ifndef FCNT
#define FCNT 8
#endif

#ifndef ICNT
#define ICNT 8
#endif



#ifndef HF4CNT
#define HF4CNT 2
#endif

#ifndef IS4CNT
#define IS4CNT 2
#endif

#ifndef HF2CNT
#define HF2CNT 4
#endif

#ifndef IS2CNT
#define IS2CNT 4
#endif

#ifndef HFCNT
#define HFCNT 16
#endif

#ifndef ISCNT
#define ISCNT 16
#endif


vec4 f4regs[F4CNT];
ivec4 i4regs[I4CNT];
vec2 f4regs[F2CNT];
ivec2 i4regs[I2CNT];
float fregs[FCNT];
int iregs[ICNT];

#ifdef ENABLE_AMD_INSTRUCTION_SET
f16vec4 hf4regs[HF4CNT];
i16vec4 is4regs[IS4CNT];
f16vec2 hf4regs[HF2CNT];
i16vec2 is4regs[IS2CNT];
float16_t hfregs[HFCNT];
int16_t isregs[ISCNT];
#endif

// for registers only, not for global memory
#define vptr_t int 

// declare named register (may used any compatible "id")
#define decl_t(name,type,id)\
    #define name type[id]

// types of registers
#define f32 fregs
#define i32 iregs
#define f16 hfregs
#define i16 isregs
#define f32x2 f2regs
#define i32x2 i2regs
#define f16x2 hf2regs
#define i16x2 is2regs
#define f32x4 f4regs
#define i32x4 i4regs
#define f16x4 hf4regs
#define i16x4 is4regs

// TEST ZONE
// declare "tmpf" in "0" cell
#define decl_t(tmpf, f32, 0) 


// planned: allocation, counters for every type
// it will not included to final production
