#ifndef _RAYS_H
#define _RAYS_H

// don't use shared memory for rays (need 32-byte aligment per rays)
//#ifndef EXTENDED_SHARED_CACHE_SUPPORT
//#define DISCARD_SHARED_CACHING
//#endif

// include
#include "../include/mathlib.glsl"
#include "../include/morton.glsl"
#include "../include/ballotlib.glsl"

// paging optimized tiling
const int R_BLOCK_WIDTH = 8, R_BLOCK_HEIGHT = 8;
const uint R_BLOCK_SIZE = R_BLOCK_WIDTH * R_BLOCK_HEIGHT;


// 128bit block heading struct 
struct BlockInfo {
    uint indiceHeader, bitfield, next, blockBinId;
};

// 128bit heading struct of bins
struct BlockBin {
    uint texelHeader, bitfield, blockStart, previousReg;
};


// tile indexing typing
// for ditributed computing highly recommended unified types
#ifdef ENABLE_AMD_INT16
#define IDCTYPE uint16_t
#else
#define IDCTYPE uint
#endif



// blocks data (32 byte node)
struct RayBlockNode { RayRework data; };
layout ( std430, binding = 0, set = 0 ) coherent buffer RaysSSBO { RayBlockNode rayBlockNodes[][R_BLOCK_SIZE]; }; // blocks data

// blocks managment
layout ( std430, binding = 1, set = 0 ) coherent buffer BlocksSSBO { BlockInfo rayBlocks[]; }; // block headers and indices
layout ( std430, binding = 7, set = 0 ) coherent buffer BlockBins { BlockBin blockBins[]; };
layout ( std430, binding = 2, set = 0 ) readonly buffer ActiveSSBO { int activeBlocks[]; }; // current blocks, that will invoked by devices

#ifndef SIMPLIFIED_RAY_MANAGMENT // for traversers don't use
// blocks confirmation
layout ( std430, binding = 3, set = 0 ) restrict buffer ConfirmSSBO { int confirmBlocks[]; }; // preparing blocks for invoking

// reusing blocks
layout ( std430, binding = 4, set = 0 ) restrict buffer AvailableSSBO { int availableBlocks[]; }; // block are free for write
layout ( std430, binding = 5, set = 0 ) restrict buffer PrepareSSBO { int preparingBlocks[]; }; // block where will writing

// texels binding
layout ( std430, binding = 6, set = 0 ) coherent buffer TexelsSSBO { Texel nodes[]; } texelBuf; // 32byte per node
#endif

// intersection vertices
layout ( std430, binding = 9, set = 0 ) coherent buffer HitsSSBO { HitRework hits[]; }; // 96byte per node

// for faster BVH traverse
layout ( std430, binding = 10, set = 0 ) coherent buffer UnorderedSSBO { int unorderedRays[]; };

// globalized index space 
layout ( std430, binding = 11, set = 0 ) coherent buffer BlockIndexedSpace { INDEX16 ispace[]; };




// load and store 16-bit (error secure)
#define m16i(i) (uint(M16(ispace, i))-1u)
#define m16s(a, i) M16S(ispace, INDEX16(a)+INDEX16(1u), i)


// extraction of block length
uint blockLength(inout BlockInfo block){
    return BFE_HW(block.bitfield, 0, 16)&0xFFFFu;
}

void blockLength(inout BlockInfo block, in uint count){
    block.bitfield = BFI_HW(block.bitfield, (count)&0xFFFFu, 0, 16);
}




void resetBlockLength(inout BlockInfo block) { blockLength(block, 0); }



// extraction of block length
uint blockLength(in uint blockPtr){
    return blockLength(rayBlocks[blockPtr]);
}

void blockLength(in uint blockPtr, in uint count){
    blockLength(rayBlocks[blockPtr], count);
}

void resetBlockLength(in uint blockPtr){
    resetBlockLength(rayBlocks[blockPtr]);
}



uint blockIndiceHeader(in uint blockPtr){
    return rayBlocks[blockPtr].indiceHeader-1;
}

uint blockPreparingHeader(in uint blockPtr){
    return blockIndiceHeader(blockPtr) + R_BLOCK_SIZE;
}

uint blockBinIndiceHeader(in uint binPtr){
    return blockBins[binPtr].texelHeader-1;
}






// block states (per lanes)
int currentInBlockPtr = -1;
int currentBlockNode = -1;
uint currentBlockSize = 0;
int currentBlock = -1;

#ifndef DISCARD_SHARED_CACHING
shared RayRework _lRayCache[WORK_SIZE/WARP_SIZE][WARP_SIZE];
#define currentRay _lRayCache[LC_IDX][LANE_IDX]
#else
RayRework currentRay;
#endif

// refering by 
#define currentBlockBin (currentBlock >= 0 ? (int(rayBlocks[currentBlock].blockBinId)-1) : -1)
//int currentBlockBin = 0;



// calculate index in traditional system
uint getGeneralNodeId(){
    return currentBlock * R_BLOCK_SIZE + currentBlockNode;
}

uint getGeneralNodeId(in uint currentBlock){
    return currentBlock * R_BLOCK_SIZE + currentBlockNode;
}

uint getGeneralPlainId(){
    return currentBlock * R_BLOCK_SIZE + currentInBlockPtr;
}

ivec2 decomposeLinearId(in uint linid){
    return ivec2(linid / R_BLOCK_SIZE, linid % R_BLOCK_SIZE);
}


// counters
layout ( std430, binding = 8, set = 0 ) buffer CounterBlock { 
    int bT; // blocks counter
    int aT; // active blocks counter
    int pT; // clearing blocks counters
    int tT; // unordered counters
    
    int mT; // available blocks (reusing)
    uint rT; // allocator of indice blocks (256byte)

    int hT; // hits vertices counters
    int iT; // ???
} arcounter;


// incremental counters (and optimized version)
#ifdef USE_SINGLE_THREAD_RAY_MANAGMENT
#define atomicIncBT() (true?atomicAdd(arcounter.bT,1):0)
#define atomicIncAT() (true?atomicAdd(arcounter.aT,1):0)
#define atomicIncPT() (true?atomicAdd(arcounter.pT,1):0)
#define atomicIncIT() (true?atomicAdd(arcounter.iT,1):0)
#define atomicDecMT() (true?atomicAdd(arcounter.mT,-1):0)
#define atomicIncRT(cct) (cct>0?atomicAdd(arcounter.rT,cct):0)
#else
initAtomicIncFunction(arcounter.bT, atomicIncBT, int)
initAtomicIncFunction(arcounter.aT, atomicIncAT, int)
initAtomicIncFunction(arcounter.pT, atomicIncPT, int)
initAtomicIncFunction(arcounter.iT, atomicIncIT, int)
initAtomicDecFunction(arcounter.mT, atomicDecMT, int)
initAtomicIncByFunction(arcounter.rT, atomicIncRT, uint)
#endif

initAtomicIncFunction(arcounter.tT, atomicIncTT, int)
initAtomicIncFunction(arcounter.hT, atomicIncHT, int)

// should functions have layouts
initNonAtomicIncFunctionTargetFunc(blockLength, atomicIncCM, uint)


// copy node indices
void copyBlockIndices(in int block, in int bidx){
    if (block >= 0) {
        uint idx = (bidx >= min(blockLength(block), R_BLOCK_SIZE)) ? uint(-1) : m16i(blockPreparingHeader(block) + uint(bidx));
        if (int(bidx) >= 0 && bidx < R_BLOCK_SIZE) m16s(uint(idx), blockIndiceHeader(block) + uint(bidx));
    }
}

bool checkIllumination(in int block, in int bidx){
    return (bidx >= 0 && block >= 0 ? mlength(f16_f32(rayBlockNodes[block][bidx].data.dcolor).xyz) >= 0.00001f : false);
}


// accquire rays for processors
void accquireNode(in int block, in int bidx){
    currentInBlockPtr = int(bidx);
    currentBlockNode = int(currentInBlockPtr) >= 0 ? int(m16i(blockIndiceHeader(block) + currentInBlockPtr)) : -1;
    currentRay = rayBlockNodes[block][currentBlockNode].data;

    if (currentBlockNode < 0) {
        currentRay.dcolor = uvec2((0u).xx);
        currentRay.origin.w = 0;
        WriteColor(currentRay.dcolor, 0.0f.xxxx);
        RayActived(currentRay, FALSE_);
        RayBounce(currentRay, 0);
    }
}


void accquireNodeOffload(in int block, in int bidx){
    currentInBlockPtr = int(bidx);
    currentBlockNode = int(currentInBlockPtr) >= 0 ? int(m16i(blockIndiceHeader(block) + currentInBlockPtr)) : -1;

    currentRay = rayBlockNodes[block][currentBlockNode].data;
    if (int(currentBlockNode) >= 0) { // for avoid errors with occupancy, temporarely clean in working memory
        int nid = currentBlockNode;
        rayBlockNodes[block][nid].data.dcolor = uvec2((0u).xx);
        rayBlockNodes[block][nid].data.origin.w = 0;
        WriteColor(rayBlockNodes[block][nid].data.dcolor, 0.0f.xxxx);
        RayActived(rayBlockNodes[block][nid].data, FALSE_);
        RayBounce(rayBlockNodes[block][nid].data, 0);
        m16s(uint(-1), blockIndiceHeader(block) + currentInBlockPtr);
    } else {
        currentRay.dcolor = uvec2((0u).xx);
        currentRay.origin.w = 0;
        WriteColor(currentRay.dcolor, 0.0f.xxxx);
        RayActived(currentRay, FALSE_);
        RayBounce(currentRay, 0);
    }
}


void accquirePlainNode(in uint block, in uint bidx){
    currentInBlockPtr = int(bidx);
    currentBlockNode = int(bidx);
    currentRay = rayBlockNodes[block][currentBlockNode].data;
}


void accquireUnordered(in uint gid){
    uint rid = unorderedRays[gid]-1;
    currentBlock = int(rid / R_BLOCK_SIZE);
    currentBlockNode = int(rid % R_BLOCK_SIZE);
    if (currentBlockNode >= 0) currentRay = rayBlockNodes[currentBlock][currentBlockNode].data;
}


// writing rays to blocks
void storeRay(in uint block, inout RayRework ray) {
    if (int(block) >= 0 && int(currentBlockNode) >= 0) rayBlockNodes[block][currentBlockNode].data = ray;
}


// confirm if current block not occupied somebody else
bool confirmNodeOccupied(in uint block){
    bool occupied = false;
    if (int(block) >= 0) {
        if (SSC(RayActived(rayBlockNodes[block][currentBlockNode].data)) || 
           !SSC(RayActived(rayBlockNodes[block][currentBlockNode].data)) && mlength(f16_f32(rayBlockNodes[block][currentBlockNode].data.dcolor).xyz) > 0.00001) 
        {
            occupied = true;
        }
    }
    if (int(block) < 0) { occupied = true; }
    return occupied;
}



#ifndef SIMPLIFIED_RAY_MANAGMENT
// confirm for processing
void confirmNode(in uint block, in bool actived){
    if (blockLength(block) < currentBlockSize && actived && int(block) >= 0) { // don't overflow
        uint idx = atomicIncCM(block); m16s(currentBlockNode, blockPreparingHeader(block) + idx);
    }
}


// add to actives
void confirmBlock(in uint mt){
    if (int(mt) >= 0) { rayBlocks[mt].next = 0u; confirmBlocks[atomicIncAT()] = int(mt+1); }
}


// utilize blocks
void flushBlock(in uint bid, in uint _mt, in bool illuminated){
    _mt += 1;

    uint prev = uint(-1);
    if (int(bid) >= 0 && int(_mt) > 0 && illuminated) {
        prev = atomicExchange(blockBins[bid].previousReg, _mt)-1u;
        if (int(prev) < 0) atomicCompSwap(blockBins[bid].blockStart, uint(-1), _mt); // avoid wrong condition
    }
    
    if (int(prev) >= 0) {
        resetBlockLength(prev);
        rayBlocks[prev].next = _mt;
        preparingBlocks[atomicIncPT()] = int(_mt);
    }
}

void flushBlock(in uint mt, in bool illuminated){
    flushBlock(rayBlocks[mt].blockBinId-1, mt, illuminated);
}

// create/allocate block 
uint createBlock(inout uint blockId, in uint blockBinId){

    // write block where possible
    uint alu = firstActive(), mt = blockId;
    if (LANE_IDX == alu) {
        if (int(mt) < 0) {
            int st = int(atomicDecMT())-1;
            if (st >= 0) { 
                mt = exchange(availableBlocks[st],-1)-1;
                rayBlocks[mt].bitfield = 0;
                rayBlocks[mt].blockBinId = blockBinId+1u;
                rayBlocks[mt].next = uint(-1);
            }
        }

        if (int(mt) < 0) { 
            mt = atomicIncBT(); 
            rayBlocks[mt].bitfield = 0;
            rayBlocks[mt].next = uint(-1);
            rayBlocks[mt].blockBinId = blockBinId+1u;
            rayBlocks[mt].indiceHeader = 0;
        }

        if (int(mt) >= 0) {
            if (rayBlocks[mt].indiceHeader <= 0) {
                rayBlocks[mt].indiceHeader = atomicIncRT(R_BLOCK_SIZE*2)+1;
            }
        }
    }
    mt = readLane(mt, alu);

    blockId = mt >= 0 ? mt : blockId;
    return blockId;
}

uint createBlock(in uint blockBinId){
    uint nblock = uint(-1); return createBlock(nblock, blockBinId);
}

#endif


// accquire block
void accquireBlock(in uint gid){
    currentBlock = activeBlocks[gid]-1, 
    currentBlockSize = int(currentBlock) >= 0 ? min(blockLength(currentBlock), uint(R_BLOCK_SIZE)) : 0u;
}

void accquirePlainBlock(in uint gid){
    currentBlock = int(gid), 
    currentBlockSize = int(currentBlock) >= 0 ? R_BLOCK_SIZE : 0u;
}


// aliases
void resetBlockIndiceCounter(in uint block) { resetBlockLength(block); }
uint getBlockIndiceCounter(in uint block) { return blockLength(block); }



// aliases
void accquireNode(in int bidx) {
    currentInBlockPtr = int(bidx);
    accquireNode(currentBlock, bidx);
    if (bidx >= currentBlockSize) {
        currentBlockNode = -1;
        RayActived(currentRay, FALSE_);
    }
}

void accquireNodeOffload(in int bidx) {
    currentInBlockPtr = int(bidx);
    accquireNodeOffload(currentBlock, bidx);
    if (bidx >= currentBlockSize) {
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

void storeRay(in uint block) {
    storeRay(block, currentRay);
}

#ifndef SIMPLIFIED_RAY_MANAGMENT
uint createBlock() {
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
