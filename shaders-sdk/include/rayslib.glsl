#ifndef _RAYS_H
#define _RAYS_H

// don't use shared memory for rays (need 32-byte aligment per rays)
#ifndef EXTENDED_SHARED_CACHE_SUPPORT
#define DISCARD_SHARED_CACHING
#endif

// include
#include "../include/mathlib.glsl"
#include "../include/morton.glsl"
#include "../include/ballotlib.glsl"

// paging optimized tiling
const int R_BLOCK_WIDTH = 32;
const int R_BLOCK_HEIGHT = 32;

// performance optimized tiling
//const int R_BLOCK_WIDTH = 16;
//const int R_BLOCK_HEIGHT = 16;


const int R_BLOCK_SIZE = R_BLOCK_WIDTH * R_BLOCK_HEIGHT;


// blocks with indices
struct BlockInfo {
    int indiceCount; // don't forget refresh
    int blockBinId;
    uint bitfield;
    int next;
};


// tile indexing typing
// for ditributed computing highly recommended unified types
#ifdef ENABLE_AMD_INT16
#define IDCTYPE int16_t
#else
#define IDCTYPE int
#endif


// 8kb + 16 byte every block (4kb+ if 16-bits)
// planned fully 16-bit layouting
struct RBlock {
    BlockInfo info;
    IDCTYPE indices[R_BLOCK_SIZE];   // in possible 16-bit
    IDCTYPE preparing[R_BLOCK_SIZE]; // in possible 16-bit
};


// blocks bins (where will picking final colors)
struct BlockBinInfo {
    int header;
    int count;
    int previous;
    int reserved;
};


// 4kb + 16 byte every block
struct BlockBin {
    BlockBinInfo info;
    int texels[R_BLOCK_SIZE]; // scatter to texels
};


// 64byte every node, 64kb per blocks
struct RayBlockNode {
    RayRework data;
};


// blocks data
layout ( std430, binding = 0, set = 0 ) coherent buffer RaysSSBO { RayBlockNode rayBlockNodes[][R_BLOCK_SIZE]; }; // blocks data

// blocks managment
layout ( std430, binding = 1, set = 0 ) coherent buffer BlocksSSBO { RBlock rayBlocks[]; }; // block headers and indices
layout ( std430, binding = 2, set = 0 ) readonly buffer ActiveSSBO { int activeBlocks[]; }; // current blocks, that will invoked by devices

#ifndef SIMPLIFIED_RAY_MANAGMENT // for traversers don't use
// blocks confirmation
layout ( std430, binding = 3, set = 0 ) restrict buffer ConfirmSSBO { int confirmBlocks[]; }; // preparing blocks for invoking

// reusing blocks
layout ( std430, binding = 4, set = 0 ) restrict buffer AvailableSSBO { int availableBlocks[]; }; // block are free for write
layout ( std430, binding = 5, set = 0 ) restrict buffer PrepareSSBO { int preparingBlocks[]; }; // block where will writing

// texels binding
layout ( std430, binding = 6, set = 0 ) coherent buffer TexelsSSBO { Texel nodes[]; } texelBuf; // 32byte per node
layout ( std430, binding = 7, set = 0 ) coherent buffer BlockBins { BlockBin blockBins[]; };
#endif

// intersection vertices
layout ( std430, binding = 9, set = 0 ) coherent buffer HitsSSBO { HitRework hits[]; }; // 96byte per node

// for faster BVH traverse
layout ( std430, binding = 10, set = 0 ) coherent buffer UnorderedSSBO { int unorderedRays[]; };


// block states (per lanes)
int currentInBlockPtr = -1;
int currentBlockNode = -1;
int currentBlockSize = 0;
int currentBlock = -1;


// store ray in shared blocks (~48kb)
#ifndef DISCARD_SHARED_CACHING
shared RayRework _currentRay[WORK_SIZE/WARP_SIZE][WARP_SIZE];
#define currentRay _currentRay[LC_IDX][LANE_IDX]
#else
RayRework currentRay;
#endif

// refering by 
#define currentBlockBin (currentBlock >= 0 ? (rayBlocks[currentBlock].info.blockBinId-1) : -1)
//int currentBlockBin = 0;



// calculate index in traditional system
int getGeneralNodeId(){
    return currentBlock * R_BLOCK_SIZE + currentBlockNode;
}

int getGeneralNodeId(in int currentBlock){
    return currentBlock * R_BLOCK_SIZE + currentBlockNode;
}

int getGeneralPlainId(){
    return currentBlock * R_BLOCK_SIZE + currentInBlockPtr;
}


ivec2 decomposeLinearId(in int linid){
    return ivec2(linid / R_BLOCK_SIZE, linid % R_BLOCK_SIZE);
}


// counters
layout ( std430, binding = 8, set = 0 ) restrict buffer CounterBlock { 
    int bT; // blocks counter
    int aT; // active blocks counter
    int pT; // clearing blocks counters
    int tT; // unordered counters
    
    int mT; // available blocks (reusing)
    int rT; // ???

    int hT; // hits vertices counters
    int iT; // ???
} arcounter;

// incremental counters (and optimized version)
#ifdef USE_SINGLE_THREAD_RAY_MANAGMENT
#define atomicIncBT(cnd) (SSC(cnd)?atomicAdd(arcounter.bT,1):0)
#define atomicIncAT(cnd) (SSC(cnd)?atomicAdd(arcounter.aT,1):0)
#define atomicIncPT(cnd) (SSC(cnd)?atomicAdd(arcounter.pT,1):0)
#define atomicIncIT(cnd) (SSC(cnd)?atomicAdd(arcounter.iT,1):0)
#define atomicDecMT(cnd) (SSC(cnd)?atomicAdd(arcounter.mT,-1):0)
#define atomicIncRT(cnd) (SSC(cnd)?atomicAdd(arcounter.rT,1):0)
#else
initAtomicIncFunction(arcounter.bT, atomicIncBT, int)
initAtomicIncFunction(arcounter.aT, atomicIncAT, int)
initAtomicIncFunction(arcounter.pT, atomicIncPT, int)
initAtomicIncFunction(arcounter.iT, atomicIncIT, int)
initAtomicDecFunction(arcounter.mT, atomicDecMT, int)
initAtomicIncFunction(arcounter.rT, atomicIncRT, int)
#endif

initAtomicIncFunction(arcounter.tT, atomicIncTT, int)
initAtomicIncFunction(arcounter.hT, atomicIncHT, int)
initNonAtomicIncFunctionTarget(rayBlocks[WHERE].info.indiceCount, atomicIncCM, int)

// copy node indices
void copyBlockIndices(in int block, in int bidx){
    if (block >= 0) {
        int idx = bidx >= min(rayBlocks[block].info.indiceCount, R_BLOCK_SIZE) ? -1 : rayBlocks[block].preparing[bidx];
        if (bidx >= 0 && bidx < R_BLOCK_SIZE) rayBlocks[block].indices[bidx] = IDCTYPE(idx);
    }
}


bool checkIllumination(in int block, in int bidx){
    int nd = int(bidx);//rayBlocks[block].indices[bidx]-1;
    return (nd >= 0 && block >= 0 ? mlength(rayBlockNodes[block][nd].data.final.xyz) >= 0.00001f : false);
}


// accquire rays for processors
void accquireNode(in int block, in int bidx){
    currentInBlockPtr = int(bidx);
    currentBlockNode = currentInBlockPtr >= 0 ? rayBlocks[block].indices[currentInBlockPtr]-1 : -1;
    if (currentBlockNode >= 0) currentRay = rayBlockNodes[block][currentBlockNode].data;
}


void accquirePlainNode(in int block, in int bidx){
    currentInBlockPtr = int(bidx);
    currentBlockNode = int(bidx);
    currentRay = rayBlockNodes[block][currentBlockNode].data;
}

void accquireUnordered(in int gid){
    int rid = unorderedRays[gid]-1;
    currentBlock = rid / R_BLOCK_SIZE;
    currentBlockNode = rid % R_BLOCK_SIZE;
    if (currentBlockNode >= 0) currentRay = rayBlockNodes[currentBlock][currentBlockNode].data;
}


// writing rays to blocks
void storeRay(in int block, inout RayRework ray) {
    if (block >= 0 && currentBlockNode >= 0) rayBlockNodes[block][currentBlockNode].data = ray;
}


#ifndef SIMPLIFIED_RAY_MANAGMENT
// confirm for processing
void confirmNode(in int block, in bool actived){
    if (rayBlocks[block].info.indiceCount < currentBlockSize && actived && block >= 0) { // don't overflow
        int idx = atomicIncCM(block, TRUE_);
        rayBlocks[block].preparing[idx] = IDCTYPE(currentBlockNode+1);
        unorderedRays[atomicIncTT(TRUE_)] = int(getGeneralNodeId(block))+1;
    }
}

// add to actives
void confirmBlock(in int mt){
    if (mt >= 0) {
        rayBlocks[mt].info.next = -1;
        confirmBlocks[atomicIncAT(TRUE_)] = mt+1;
    }
}

// utilize blocks
void flushBlock(in int bid, in int _mt, in bool illuminated){
    int prev = -1;
    if (bid >= 0 && _mt >= 0 && illuminated) {
        prev = atomicExchange(blockBins[bid].info.previous, _mt+1)-1;
        if (prev < 0) atomicCompSwap(blockBins[bid].info.header, -1, _mt+1); // avoid wrong condition
        atomicAdd(blockBins[bid].info.count, 1); // counting
    }
    if (prev >= 0) {
        rayBlocks[prev].info.indiceCount = 0; // zerify indice count
        rayBlocks[prev].info.next = _mt+1;
        preparingBlocks[atomicIncPT(TRUE_)] = _mt+1;
    }
}

void flushBlock(in int mt, in bool illuminated){
    flushBlock(rayBlocks[mt].info.blockBinId-1, mt, illuminated);
}


// create/allocate block 
int createBlock(in int blockBinId){
    // write block where possible
    int mt = atomicDecMT(TRUE_)-1;
    if (mt >= 0) mt = exchange(availableBlocks[mt], -1)-1; 
    if (mt <  0) mt = atomicIncBT(TRUE_);
    if (mt >= 0) {
        rayBlocks[mt].info.indiceCount = 0;
        rayBlocks[mt].info.blockBinId = blockBinId+1;
        rayBlocks[mt].info.bitfield = 0;
        rayBlocks[mt].info.next = -1;
    }
    return mt >= 0 ? mt : -1;
}
#endif


// accquire block
void accquireBlock(in int gid){
    currentBlock = activeBlocks[gid]-1;
    currentBlockSize = currentBlock >= 0 ? rayBlocks[currentBlock].info.indiceCount : 0;
    //currentBlockBin = currentBlock >= 0 ? rayBlocks[currentBlock].info.blockBinId-1 : -1;
}

void accquirePlainBlock(in int gid){
    currentBlock = int(gid);
    currentBlockSize = currentBlock >= 0 ? R_BLOCK_SIZE : 0;
    //currentBlockBin = currentBlock >= 0 ? rayBlocks[currentBlock].info.blockBinId-1 : -1;
}



void resetBlockIndiceCounter(in int block){
    rayBlocks[block].info.indiceCount = 0;
}

int getBlockIndiceCounter(in int block){
    return rayBlocks[block].info.indiceCount;
}


// aliases
void accquireNode(in int bidx) {
    accquireNode(currentBlock, bidx);
    if (bidx >= currentBlockSize) {
        currentInBlockPtr = int(bidx);
        currentBlockNode = -1;
        RayActived(currentRay, FALSE_);
    }
}

void accquirePlainNode(in int bidx) {
    accquirePlainNode(currentBlock, bidx);
}

void storeRay(inout RayRework ray) {
    storeRay(int(currentBlock), ray);
}

void storeRay() {
    storeRay(currentRay);
}

void storeRay(in int block) {
    storeRay(block, currentRay);
}

#ifndef SIMPLIFIED_RAY_MANAGMENT
int createBlock() {
    return createBlock(currentBlockBin);
}

void confirmNode(in bool actived) {
    confirmNode(int(currentBlock), actived);
}

void confirmNode() {
    confirmNode(RayActived(currentRay) == TRUE_); // confirm node by ray
}
#endif

#endif
