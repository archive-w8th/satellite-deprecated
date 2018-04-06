#ifndef _RAYS_H
#define _RAYS_H

// include
#include "../include/mathlib.glsl"
#include "../include/morton.glsl"
#include "../include/ballotlib.glsl"

// paging optimized tiling
const int R_BLOCK_WIDTH = 8, R_BLOCK_HEIGHT = 8;
const int R_BLOCK_SIZE = R_BLOCK_WIDTH * R_BLOCK_HEIGHT;

// 128bit block heading struct 
struct BlockInfo {
    int indiceHeader; int indiceCount; int next, blockBinId;
};

// 128bit heading struct of bins
struct BlockBin {
    int texelHeader; int texelFrom; int blockStart, previousReg;
};



// blocks data (32 byte node)
struct RayBlockNode { RayRework data; };
layout ( std430, binding = 0, set = 0 ) restrict buffer RaysSSBO { RayBlockNode rayBlockNodes[][R_BLOCK_SIZE]; }; // blocks data

// blocks managment
layout ( std430, binding = 1, set = 0 ) restrict buffer BlocksSSBO { BlockInfo rayBlocks[]; }; // block headers and indices
layout ( std430, binding = 7, set = 0 ) restrict buffer BlockBins { BlockBin blockBins[]; };
layout ( std430, binding = 2, set = 0 ) restrict readonly buffer ActiveSSBO { int activeBlocks[]; }; // current blocks, that will invoked by devices

#ifndef SIMPLIFIED_RAY_MANAGMENT // for traversers don't use
// blocks confirmation
layout ( std430, binding = 3, set = 0 ) restrict buffer ConfirmSSBO { int confirmBlocks[]; }; // preparing blocks for invoking

// reusing blocks
layout ( std430, binding = 4, set = 0 ) restrict buffer AvailableSSBO { int availableBlocks[]; }; // block are free for write
layout ( std430, binding = 5, set = 0 ) restrict buffer PrepareSSBO { int preparingBlocks[]; }; // block where will writing

// texels binding
layout ( std430, binding = 6, set = 0 ) restrict buffer TexelsSSBO { Texel nodes[]; } texelBuf; // 32byte per node
#endif

// intersection vertices
layout ( std430, binding = 9, set = 0 ) restrict buffer HitsSSBO { HitData hits[]; }; 

// for faster BVH traverse
layout ( std430, binding = 10, set = 0 ) restrict buffer UnorderedSSBO { ElectedRay unorderedRays[]; };






#ifdef USE_16BIT_ADDRESS_SPACE
layout ( std430, binding = 11, set = 0 ) restrict buffer BlockIndexedSpace { uint16_t ispace[][R_BLOCK_SIZE]; };
#define m16i(b,i) (int(ispace[b][i])-1)
#define m16s(a,b,i) (ispace[b][i] = uint16_t(a+1))
#else
layout ( std430, binding = 11, set = 0 ) restrict buffer BlockIndexedSpace { mediump uint ispace[][R_BLOCK_SIZE]; };
#define m16i(b,i) (int(ispace[b][i])-1)
#define m16s(a,b,i) (ispace[b][i] = uint(a+1))
#endif

layout ( std430, binding = 15, set = 0 ) restrict buffer HitPayloadSSBO { HitPayload hitPayload[]; }; 



// extraction of block length
int blockLength(inout BlockInfo block){
    return block.indiceCount;
}

void blockLength(inout BlockInfo block, in int count){
    block.indiceCount = min(count, R_BLOCK_SIZE);
}

void resetBlockLength(inout BlockInfo block) { blockLength(block, 0); }


// extraction of block length
int blockLength(in int blockPtr){
    return blockLength(rayBlocks[blockPtr]);
}

void blockLength(in int blockPtr, in int count){
    blockLength(rayBlocks[blockPtr], count);
}

void resetBlockLength(in int blockPtr){
    resetBlockLength(rayBlocks[blockPtr]);
}



int blockIndiceHeader(in int blockPtr){
    return rayBlocks[blockPtr].indiceHeader-1;
}

int blockPreparingHeader(in int blockPtr){
    return blockIndiceHeader(blockPtr)+1;
}

int blockBinIndiceHeader(in int binPtr){
    return blockBins[binPtr].texelHeader-1;
}






// block states (per lanes)
int currentInBlockPtr = -1;
int currentBlockNode = -1;
int currentBlockSize = 0;
int currentBlock = -1;

// static cached ray
#ifndef DISCARD_SHARED_CACHING
shared RayRework rayCache[WORK_SIZE];
#define currentRay rayCache[Local_Idx]
#else
RayRework currentRay;
#endif

// refering by 
#define currentBlockBin (currentBlock >= 0 ? (int(rayBlocks[currentBlock].blockBinId)-1) : -1)
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
    int rT; // allocator of indice blocks 

    int hT; // hits vertices counters
    int iT; // hits payload counters
} arcounter;


// incremental counters (and optimized version)
#ifdef USE_SINGLE_THREAD_RAY_MANAGMENT
#define atomicIncBT() (true?atomicAdd(arcounter.bT,1):0)
#define atomicIncAT() (true?atomicAdd(arcounter.aT,1):0)
#define atomicIncPT() (true?atomicAdd(arcounter.pT,1):0)
#define atomicDecMT() (true?atomicAdd(arcounter.mT,-1):0)
#define atomicIncRT(cct) (cct>0?atomicAdd(arcounter.rT,cct):0)
#else
initAtomicSubgroupIncFunction(arcounter.bT, atomicIncBT,  1, int)
initAtomicSubgroupIncFunction(arcounter.aT, atomicIncAT,  1, int)
initAtomicSubgroupIncFunction(arcounter.pT, atomicIncPT,  1, int)
initAtomicSubgroupIncFunction(arcounter.mT, atomicDecMT, -1, int)
initAtomicSubgroupIncFunctionDyn(arcounter.rT, atomicIncRT,  int)
#endif
initAtomicSubgroupIncFunction(arcounter.tT, atomicIncTT, 1, int)
initAtomicSubgroupIncFunction(arcounter.hT, atomicIncHT, 1, int)
initAtomicSubgroupIncFunction(arcounter.iT, atomicIncIT, 1, int)

// should functions have layouts
initSubgroupIncFunctionTarget(rayBlocks[WHERE].indiceCount, atomicIncCM, 1, int)


// copy node indices
void copyBlockIndices(in int block, in int bidx){
    if (block >= 0) {
        int idx = int((bidx >= min(blockLength(block), R_BLOCK_SIZE)) ? uint(-1) : m16i(blockPreparingHeader(block), bidx));
        if (int(bidx) >= 0 && bidx < R_BLOCK_SIZE) m16s(idx, blockIndiceHeader(block), bidx);
    }
}

bool checkIllumination(in int block, in int bidx){
    return (bidx >= 0 && block >= 0 ? mlength(f16_f32(rayBlockNodes[block][bidx].data.dcolor).xyz) >= 0.00001f : false);
}


// accquire rays for processors
void accquireNode(in int block, in int bidx){
    currentInBlockPtr = int(bidx);
    currentBlockNode = (currentInBlockPtr >= 0 && currentInBlockPtr < R_BLOCK_SIZE) ? int(m16i(blockIndiceHeader(block), currentInBlockPtr)) : -1;
    currentRay = rayBlockNodes[block][currentBlockNode].data;

    if (currentBlockNode < 0) {
        currentRay.dcolor = uvec2((0u).xx);
        currentRay.origin.w = FINT_ZERO;
        WriteColor(currentRay.dcolor, 0.0f.xxxx);
        RayActived(currentRay, false_);
        RayBounce(currentRay, 0);
    }
}


void accquireNodeOffload(in int block, in int bidx){
    currentInBlockPtr = int(bidx);
    currentBlockNode = (currentInBlockPtr >= 0 && currentInBlockPtr < R_BLOCK_SIZE) ? int(m16i(blockIndiceHeader(block),currentInBlockPtr)) : -1;

    currentRay = rayBlockNodes[block][currentBlockNode].data;
    if (int(currentBlockNode) >= 0) { // for avoid errors with occupancy, temporarely clean in working memory
        int nid = currentBlockNode;
        rayBlockNodes[block][nid].data.dcolor = uvec2((0u).xx);
        rayBlockNodes[block][nid].data.origin.w = FINT_ZERO;
        WriteColor(rayBlockNodes[block][nid].data.dcolor, 0.0f.xxxx);
        RayActived(rayBlockNodes[block][nid].data, false_);
        RayBounce(rayBlockNodes[block][nid].data, 0);
        m16s(-1, blockIndiceHeader(block), currentInBlockPtr);
    } else {
        currentRay.dcolor = uvec2((0u).xx);
        currentRay.origin.w = FINT_ZERO;
        WriteColor(currentRay.dcolor, 0.0f.xxxx);
        RayActived(currentRay, false_);
        RayBounce(currentRay, 0);
    }
}


void accquirePlainNode(in int block, in int bidx){
    currentInBlockPtr = int(bidx);
    currentBlockNode = int(bidx);
    currentRay = rayBlockNodes[block][currentBlockNode].data;
}


void accquireUnordered(in int rid){
    currentBlock = int(rid / R_BLOCK_SIZE);
    currentBlockNode = int(rid % R_BLOCK_SIZE);
    if (currentBlockNode >= 0) currentRay = rayBlockNodes[currentBlock][currentBlockNode].data;
}


// writing rays to blocks
void storeRay(in int block, inout RayRework ray) {
    if (int(block) >= 0 && int(currentBlockNode) >= 0) rayBlockNodes[block][currentBlockNode].data = ray;
}


// confirm if current block not occupied somebody else
bool confirmNodeOccupied(in int block){
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
void confirmNode(in int block, in bool actived){
    if (blockLength(block) < currentBlockSize && actived && int(block) >= 0) { // don't overflow
        int idx = atomicIncCM(block); 
        if (idx >= 0 && idx < R_BLOCK_SIZE) {
            m16s(currentBlockNode, blockPreparingHeader(block), idx);
        }
    }
}


// add to actives
void confirmBlock(in int mt){
    if (mt >= 0) { rayBlocks[mt].next = 0; confirmBlocks[atomicIncAT()] = mt+1; }
}


// utilize blocks
void flushBlock(in int bid, in int _mt, in bool illuminated){
    if (_mt >= 0 ) {
        rayBlocks[_mt].indiceCount = 0;
        rayBlocks[_mt].blockBinId = bid+1;
    }
    _mt += 1;

    int prev = int(-1);
    if (bid >= 0 && _mt > 0 && illuminated) {
        rayBlocks[_mt-1].next = atomicExchange(blockBins[bid].blockStart, _mt);
        //prev = atomicExchange(blockBins[bid].previousReg, _mt)-1;
        //if (int(prev) < 0) atomicCompSwap(blockBins[bid].blockStart, 0, _mt); // avoid wrong condition
    }
    
    if (_mt > 0 ) {
        preparingBlocks[atomicIncPT()] = _mt;
    }
    
}

void flushBlock(in int mt, in bool illuminated){
    flushBlock(rayBlocks[mt].blockBinId-1, mt, illuminated);
}

// create/allocate block 
int createBlock(inout int blockId, in int blockBinId){
    int mt = -1;
    {
        if (mt < 0) {
            int st = int(atomicDecMT())-1;
            if (st >= 0) { 
                mt = exchange(availableBlocks[st],-1)-1;
                rayBlocks[mt].indiceCount = 0;
                rayBlocks[mt].blockBinId = blockBinId+1;
                rayBlocks[mt].next = 0;
            }
        }

        if (mt < 0) { 
            mt = atomicIncBT(); 
            rayBlocks[mt].indiceCount = 0;
            rayBlocks[mt].next = 0;
            rayBlocks[mt].blockBinId = blockBinId+1;
            rayBlocks[mt].indiceHeader = 0;
        }

        if (mt >= 0 && rayBlocks[mt].indiceHeader <= 0) {
            rayBlocks[mt].indiceHeader = atomicIncRT(2)+1;
        }
    }
    blockId = (mt >= 0 ? mt : int(blockId));
    return (mt);
}

#endif


// accquire block
void accquireBlock(in int gid){
    currentBlock = activeBlocks[gid]-1, 
    currentBlockSize = int(int(currentBlock) >= 0 ? min(blockLength(currentBlock), uint(R_BLOCK_SIZE)) : 0u);
}

void accquirePlainBlock(in int gid){
    currentBlock = int(gid), 
    currentBlockSize = int(currentBlock) >= 0 ? R_BLOCK_SIZE : 0;
}


// aliases
void resetBlockIndiceCounter(in int block) { resetBlockLength(block); }
int getBlockIndiceCounter(in int block) { return blockLength(block); }



// aliases
void accquireNode(in int bidx) {
    currentInBlockPtr = int(bidx);
    accquireNode(currentBlock, bidx);
    if (bidx >= currentBlockSize) {
        currentBlockNode = -1;
        RayActived(currentRay, false_);
    }
}

void accquireNodeOffload(in int bidx) {
    currentInBlockPtr = int(bidx);
    accquireNodeOffload(currentBlock, bidx);
    if (bidx >= currentBlockSize) {
        currentBlockNode = -1;
        RayActived(currentRay, false_);
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
void confirmNode(in bool actived) {
    confirmNode(int(currentBlock), actived);
}

void confirmNode() {
    confirmNode(RayActived(currentRay) == true_); // confirm node by ray
}
#endif








#ifdef USE_EXTENDED_RAYLIB
void invalidateRay(inout RayRework rayTemplate, in bool overflow){
    if (overflow || SSC(RayActived(rayTemplate)) && (RayBounce(rayTemplate) <= 0 || RayDiffBounce(rayTemplate) <= 0 || mlength(f16_f32(rayTemplate.dcolor).xyz) <= 0.00001f)) {
        RayActived(rayTemplate, false_);
        WriteColor(rayTemplate.dcolor, 0.f.xxxx); // no mechanism for detect emission
    }
}


int createBlockOnce(inout int block, in bool minimalCondition, in int binID){
    SB_BARRIER
    if (anyInvoc(int(block) < 0 && minimalCondition)) {
        if (electedInvoc()) { block = createBlock(block, binID); }; block = readFLane(block);

        [[unroll]]
        for (int tb = 0; tb < int(R_BLOCK_SIZE); tb += int(Wave_Size_RT)) {
            int nid = tb + int(Lane_Idx);
            rayBlockNodes[block][nid].data.dcolor = uvec2((0u).xx);
            rayBlockNodes[block][nid].data.origin.w = FINT_ZERO;
            WriteColor(rayBlockNodes[block][nid].data.dcolor, 0.0f.xxxx);
            RayActived(rayBlockNodes[block][nid].data, false_);
            RayBounce(rayBlockNodes[block][nid].data, 0);
            if (nid < R_BLOCK_SIZE) {
                m16s(-1, blockIndiceHeader(block), nid);
                m16s(-1, blockPreparingHeader(block), nid);
            }
        }
    }

    SB_BARRIER
    return block;
}


int createBlockOnce(inout int block, in bool minimalCondition){
    return createBlockOnce(block, minimalCondition, currentBlockBin);
}


void invokeBlockForNodes(inout RayRework rayTemplate, inout int outNewBlock, inout int prevNonOccupiedBlock) {
    invalidateRay(rayTemplate, false);
    bool occupyCriteria = SSC(RayActived(rayTemplate)) || !SSC(RayActived(rayTemplate)) && mlength(f16_f32(rayTemplate.dcolor).xyz) >= 0.00001f;

    // occupy early block
    if (occupyCriteria && !confirmNodeOccupied(prevNonOccupiedBlock)) {
        storeRay(prevNonOccupiedBlock, rayTemplate);
        confirmNode(prevNonOccupiedBlock, SSC(RayActived(rayTemplate)));
        occupyCriteria = false;
    }

    // create block if not occupied 
    createBlockOnce(outNewBlock, occupyCriteria && confirmNodeOccupied(prevNonOccupiedBlock));

    // occupy these block
    if (occupyCriteria && outNewBlock >= 0 && !confirmNodeOccupied(outNewBlock)) {
        storeRay(outNewBlock, rayTemplate);
        confirmNode(outNewBlock, SSC(RayActived(rayTemplate)));
    }

    // recommend or not new block if have
    SB_BARRIER

    bool prevOccopied = confirmNodeOccupied(prevNonOccupiedBlock) && int(outNewBlock) >= 0;
    prevNonOccupiedBlock = allInvoc(prevOccopied) ? int(outNewBlock) : int(prevNonOccupiedBlock);
}


void emitBlock(in int block) {
    SB_BARRIER
    if (anyInvoc(block >= 0)) {
        bool hasIllumination = false;

        [[unroll]]
        for (int tb = 0; tb < int(R_BLOCK_SIZE); tb += int(Wave_Size_RT)) { 
            int bidx = int(tb + Lane_Idx), idx = -1;
            if (bidx < int(R_BLOCK_SIZE)) {
                idx = m16i(blockPreparingHeader(block), int(bidx));
                m16s(idx, blockIndiceHeader(block), int(bidx));
            }
            if (int(bidx) >= 0) {
                bool hasIlm = mlength(f16_f32(rayBlockNodes[block][bidx].data.dcolor).xyz) >= 0.00001f && !SSC(RayActived(rayBlockNodes[block][bidx].data));
                hasIllumination = hasIllumination || anyInvoc(hasIllumination || hasIlm);
            }
        }

        SB_BARRIER
        hasIllumination = hasIllumination || anyInvoc(hasIllumination);
        
        // confirm block or flush
        if (electedInvoc()) {
            if (blockLength(block) >= 1) { 
                confirmBlock(block);
            } else {
                flushBlock(block, hasIllumination); 
            }
        }
    }
}
#endif


#endif
