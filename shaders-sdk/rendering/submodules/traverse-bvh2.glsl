
// default definitions

#ifndef _CACHE_BINDING
#define _CACHE_BINDING 4
#endif

#ifndef _RAY_TYPE
#define _RAY_TYPE ElectedRay
#endif


//#ifdef ENABLE_AMD_INSTRUCTION_SET // RX Vega may have larger local shared memory and wider bandwidth
//const int stackPageCount = 2;
//const int localStackSize = 16;
//#else
const int stackPageCount = 8;
const int localStackSize = 4;
//#endif

// for structured copying
//struct PagedStack { int stack[localStackSize]; };

// dedicated BVH stack
struct NodeCache { ivec4 stackPages[stackPageCount]; };
layout ( std430, binding = _CACHE_BINDING, set = 0 ) restrict buffer TraverseNodes { NodeCache nodeCache[]; };


// 128-bit payload
int stackPtr = 0, pageIdx = -1, rayID = 0, _r0 = -1;


#ifndef USE_STACKLESS_BVH
//#define lstack localStack[Local_Idx]
//shared ivec4 localStack[WORK_SIZE];
ivec4 lstack = ivec4(-1,-1,-1,-1);

int loadStack(){
    int ptr = --stackPtr; int val = -1;

    // load previous stack page
    if (ptr < 0) { int page = pageIdx--; 
        if (page >= 0) {
            stackPtr = ptr = localStackSize-1;
            lstack = nodeCache[rayID].stackPages[page];
        }
    }

    // fast-stack
    val = lstack.x; lstack = lstack.yzwx;
    return val;
}

void storeStack(in int val){
    int ptr = stackPtr++;

    // store stack to global page, and empty list
    if (ptr >= localStackSize) { int page = ++pageIdx;
        if (page >= 0 && page < stackPageCount) {
            stackPtr = 1, ptr = 0;
            nodeCache[rayID].stackPages[page] = lstack;
        }
    }

    // fast-stack
    lstack = lstack.wxyz; lstack.x = val;
}

bool stackIsFull() { return stackPtr >= localStackSize && pageIdx >= stackPageCount; }
bool stackIsEmpty() { return stackPtr <= 0 && pageIdx < 0; }
#endif



shared _RAY_TYPE rayCache[WORK_SIZE];
#define currentRayTmp rayCache[Local_Idx]


struct GeometrySpace {
    //int axis; mat3 iM;
    vec4 dir;
    vec4 lastIntersection;
};

struct BVHSpace {
    fvec3_ minusOrig, directInv; bvec3_ boxSide;
    float cutOut;
};

struct BvhTraverseState {
    //_RAY_TYPE currentRayTmp;

    int idx, defTriangleID;
    float distMult, diffOffset;

#ifdef USE_STACKLESS_BVH
    uint64_t bitStack;
#endif

    GeometrySpace geometrySpace;
    BVHSpace bvhSpace;
} traverseState;



void doIntersection() {
    bool_ near = bool_(traverseState.defTriangleID >= 0);
    vec2 uv = vec2(0.f.xx);
    float d = intersectTriangle(currentRayTmp.origin.xyz, traverseState.geometrySpace.dir.xyz, traverseState.defTriangleID.x, uv.xy, bool(near.x));
    //float d = intersectTriangle(currentRayTmp.origin.xyz, traverseState.geometrySpace.iM, traverseState.geometrySpace.axis, traverseState.defTriangleID.x, uv.xy, bool(near.x));

    float _nearhit = traverseState.geometrySpace.lastIntersection.z;
    IF (lessF(d, _nearhit)) { traverseState.bvhSpace.cutOut = d * traverseState.distMult; }
    
    // validate hit 
    near &= lessF(d, INFINITY) & lessEqualF(d, _nearhit);
    //near &= lessF(d, _nearhit) | bool_(vorders[traverseState.defTriangleID.x] >= vorders[floatBitsToInt(traverseState.geometrySpace.lastIntersection.w)]); // check z-fight priority
    IF (near.x) traverseState.geometrySpace.lastIntersection = vec4(uv.xy, d.x, intBitsToFloat(traverseState.defTriangleID.x));

    // reset triangle ID 
    traverseState.defTriangleID = -1;
}

void traverseBvh2(in bool_ valid, inout _RAY_TYPE rayIn) {
    currentRayTmp = rayIn;

    vec3 origin = currentRayTmp.origin.xyz;
    vec3 direct = dcts(currentRayTmp.cdirect.xy);
    int eht = floatBitsToInt(currentRayTmp.origin.w);

    // initial state
    traverseState.idx = SSC(valid) ? 0 : -1;
#ifdef USE_STACKLESS_BVH
    traverseState.bitStack = 0ul;
#endif
    traverseState.defTriangleID = -1;
    traverseState.distMult = 1.f;
    traverseState.diffOffset = 0.f;
    traverseState.bvhSpace.cutOut = INFINITY;

    // reset stack
    stackPtr = 0, pageIdx = -1;

    // auto-clean stack from sh*t
#if (!defined(USE_FAST_OFFLOAD) && !defined(USE_STACKLESS_BVH))
    [[unroll]]
    for (int i=0;i<localStackSize;i++) { LOCAL_STACK[i] = -1; }
#endif

    // test constants
    vec3 
        torig = -divW(mult4(GEOMETRY_BLOCK geometryUniform.transform, vec4(origin, 1.0f))).xyz,
        torigTo = divW(mult4(GEOMETRY_BLOCK geometryUniform.transform, vec4(origin+direct, 1.0f))).xyz,
        dirproj = torigTo+torig;

    float dirlenInv = 1.f / max(length(dirproj), 1e-5f);
    dirproj *= dirlenInv; dirproj = 1.f / (max(abs(dirproj), 1e-5f.xxx) * mix(vec3(-1),vec3(1),greaterThanEqual(dirproj,vec3(0.f))));

    // limitation of distance
    bvec3_ bsgn = (bvec3_(sign(dirproj)*ftype_(1.0001f))+true_)>>true_;

    // bvh space precalculations 
    traverseState.bvhSpace.boxSide = bsgn;
    
    /*
    { // calculate longest axis
        vec3 drs = abs(direct); traverseState.geometrySpace.axis = 2;
        if (drs.y >= drs.x && drs.y > drs.z) traverseState.geometrySpace.axis = 1;
        if (drs.x >= drs.z && drs.x > drs.y) traverseState.geometrySpace.axis = 0;
        if (drs.z >= drs.y && drs.z > drs.x) traverseState.geometrySpace.axis = 2;
    }

    // calculate affine matrices
    vec4 vm = vec4(-direct, 1.f) / (traverseState.geometrySpace.axis == 0 ? direct.x : (traverseState.geometrySpace.axis == 1 ? direct.y : direct.z));
    traverseState.geometrySpace.iM = transpose(mat3(
        traverseState.geometrySpace.axis == 0 ? vm.wyz : vec3(1.f,0.f,0.f),
        traverseState.geometrySpace.axis == 1 ? vm.xwz : vec3(0.f,1.f,0.f),
        traverseState.geometrySpace.axis == 2 ? vm.xyw : vec3(0.f,0.f,1.f)
    ));*/

    // continue traversing when needed
    traverseState.geometrySpace.dir = vec4(direct, 1.f);
    traverseState.geometrySpace.lastIntersection = eht > 0 ? hits[eht-1].uvt : vec4(0.f.xx, INFINITY, FINT_NULL);

    // test intersection with main box
    float near = -INFINITY, far = INFINITY;
    const vec2 bndsf2 = vec2(-(1.f+1e-3f), (1.f+1e-3f));
    IF (not(intersectCubeF32Single(torig*dirproj, dirproj, bsgn, mat3x2(bndsf2, bndsf2, bndsf2), near, far))) {
        traverseState.idx = -1;
    }

    float toffset = max(near, 0.f);
    traverseState.bvhSpace.directInv = fvec3_(dirproj);
    traverseState.bvhSpace.minusOrig = fma(fvec3_(torig), fvec3_(dirproj), -fvec3_(toffset).xxx);
    //traverseState.bvhSpace.minusOrig = fvec3_(fma(torig, dirproj, -toffset.xxx));
    traverseState.distMult = 1.f/precIssue(dirlenInv);
    traverseState.diffOffset = toffset;

    IF (lessF(traverseState.geometrySpace.lastIntersection.z, INFINITY)) { 
        traverseState.bvhSpace.cutOut = traverseState.geometrySpace.lastIntersection.z * traverseState.distMult; 
    }

    // begin of traverse BVH 
    const int max_iteraction = 8192;
    ivec2 cnode = traverseState.idx >= 0 ? bvhMeta[traverseState.idx].xy : (-1).xx;

    for (int hi=0;hi<max_iteraction;hi++) {
        IFALL (traverseState.idx < 0) break; // if traverse can't live

        if (traverseState.idx >= 0) { for (;hi<max_iteraction;hi++) {
            bool _continue = false;
            //ivec2 cnode = traverseState.idx >= 0 ? bvhMeta[traverseState.idx].xy : (-1).xx;

            // if not leaf and not wrong
            IF (cnode.y == 2) {
                vec2 nears = INFINITY.xx, fars = INFINITY.xx;
                bvec2_ childIntersect = bvec2_((traverseState.idx >= 0).xx);

                // intersect boxes
                const int _cmp = cnode.x >> 1;
                childIntersect &= intersectCubeDual(traverseState.bvhSpace.minusOrig, traverseState.bvhSpace.directInv, traverseState.bvhSpace.boxSide, 
#ifdef USE_F32_BVH
                    fmat3x4_(bvhBoxes[_cmp][0], bvhBoxes[_cmp][1], bvhBoxes[_cmp][2]),
                    fmat3x4_(vec4(0.f), vec4(0.f), vec4(0.f))
#else
                    fmat3x4_(UNPACK_HF(bvhBoxes[_cmp][0].xy), UNPACK_HF(bvhBoxes[_cmp][1].xy), UNPACK_HF(bvhBoxes[_cmp][2].xy)),
                    fmat3x4_(vec4(0.f), vec4(0.f), vec4(0.f))
#endif
                , nears, fars);
                
                childIntersect &= bool_(cnode.y == 2).xx; // base on fact
                childIntersect &= bvec2_(lessThanEqual(nears+traverseState.diffOffset.xx, traverseState.bvhSpace.cutOut.xx));

                int fmask = (childIntersect.x + childIntersect.y*2)-1; // mask of intersection

                [[flatten]]
                if (fmask >= 0) {
                    _continue = true;

#ifdef USE_STACKLESS_BVH
                    traverseState.bitStack <<= 1;
#endif

                    [[flatten]]
                    if (fmask == 2) { // if both has intersection
                        ivec2 ordered = cnode.xx + (SSC(lessEqualF(nears.x, nears.y)) ? ivec2(0,1) : ivec2(1,0));
                        traverseState.idx = ordered.x;
#ifdef USE_STACKLESS_BVH
                        IF (all(childIntersect)) traverseState.bitStack |= 1ul; 
#else
                        IF (all(childIntersect) & bool_(!stackIsFull())) storeStack(ordered.y);
#endif
                    } else {
                        traverseState.idx = cnode.x + fmask;
                    }

                    cnode = traverseState.idx >= 0 ? bvhMeta[traverseState.idx].xy : (-1).xx;
                }

            } else  
            
            // if leaf, defer for intersection 
            if (cnode.y == 1) {
                if (traverseState.defTriangleID < 0) {
                    traverseState.defTriangleID = cnode.x;
                } else {
                    _continue = true;
                }
            }

#ifdef USE_STACKLESS_BVH
            // stackless 
            if (!_continue) {
                // go to parents so far as possible 
                for (int bi=0;bi<64;bi++) {
                    if ((traverseState.bitStack&1ul)!=0ul || traverseState.bitStack==0ul) break;
                    traverseState.bitStack >>= 1;
                    traverseState.idx = traverseState.idx >= 0 ? bvhMeta[traverseState.idx].z : -1;
                }

                // goto to sibling or break travers
                if (traverseState.bitStack!=0ul && traverseState.idx >= 0) {
                    traverseState.idx += traverseState.idx%2==0?1:-1; traverseState.bitStack &= ~1ul;
                } else {
                    traverseState.idx = -1;
                }

                cnode = traverseState.idx >= 0 ? bvhMeta[traverseState.idx].xy : (-1).xx;
            } _continue = false;
#else
            // stacked 
            if (!_continue) {
                if (!stackIsEmpty()) {
                    traverseState.idx = loadStack();
                } else {
                    traverseState.idx = -1;
                }

                cnode = traverseState.idx >= 0 ? bvhMeta[traverseState.idx].xy : (-1).xx;
            } _continue = false;
#endif

            IFANY ( traverseState.defTriangleID >= 0 || traverseState.idx < 0 ) { SB_BARRIER break; }
        }}

        SB_BARRIER
        
        IFANY (traverseState.defTriangleID >= 0 || traverseState.idx < 0) { SB_BARRIER doIntersection(); }
    }
}
