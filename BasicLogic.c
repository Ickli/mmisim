#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strem_structures/strem_vector.h>
#include <strem_structures/strem_ht.h>
#include <BasicLogic.h>
#define GATES_MAX_COUNT 256
#define SCHEMES_MAX_COUNT 8
#define SCHEME_NODE_CAP 4
#define SCHEME_EDGE_CAP 16
#define EDGE_CAP 2
#define CYCLE_MAX_TRAVERSION 8

// TODO: utilize cycle chains because otherwise you risk to cause UB in scheme's work

static void GateAnd_Tick(void* state);
static void GateOr_Tick(void* state);
static void GateNot_Tick(void* state);
static void GateNor_Tick(void* state);
static size_t Scheme_HashUnsigned(void const*);
static bool Scheme_CmpUnsigned(void const*, void const*);

static LogicGate Gates[GATES_MAX_COUNT];
static size_t Gates_Count = 0;
// static size_t FreeGateIds[GATES_MAX_COUNT];
// static size_t FreeGateIds_Count = 0;

// static void Gates_Deallocate(const LogicGate* gate) { }
LogicGate* Gates_Allocate() {
    return &Gates[Gates_Count++];
}

LogicGate* GateAnd_New(const LogicGate* loadInfo) {
    LogicGate* gate = Gates_Allocate();
    BiGateState* state = calloc(1, sizeof(BiGateState));
    gate->tick = GateAnd_Tick;
    gate->state = state;

    gate->pinFirstIn = &state->in1;
    gate->pinFirstOut = &state->out;

    if(loadInfo != NULL) {
        gate->name = loadInfo->name;
        if(loadInfo->state != NULL) {
            *state = *(BiGateState*)loadInfo->state;
        }
    }

    return gate;
}

static void GateAnd_Tick(void* state_erased) {
    BiGateState* state = (BiGateState*)state_erased;
    state->outn = !(state->out = (state->in1 & state->in2));
}

LogicGate* GateOr_New(const LogicGate* loadInfo) {
    LogicGate* gate = Gates_Allocate();
    BiGateState* state = calloc(1, sizeof(BiGateState));
    gate->tick = GateOr_Tick;
    gate->state = state;
    gate->pinFirstIn = &state->in1;
    gate->pinFirstOut = &state->out;

    if(loadInfo != NULL) {
        gate->name = loadInfo->name;
        if(loadInfo->state != NULL) {
            *state = *(BiGateState*)loadInfo->state;
        }
    }

    return gate;
}

static void GateOr_Tick(void* state_erased) {
    BiGateState* state = (BiGateState*)state_erased;
    state->outn = !(state->out = (state->in1 | state->in2));
    // printf("Tick in OR: in1 = %d, in2 = %d, out = %d\n", state->in1, state->in2, state->out);
}

LogicGate* GateNot_New(const LogicGate* loadInfo) {
    LogicGate* gate = Gates_Allocate();
    UniGateState* state = calloc(1, sizeof(UniGateState));
    gate->tick = GateOr_Tick;
    gate->state = state;
    gate->pinFirstIn = &state->in1;
    gate->pinFirstOut = &state->out;
    
    if(loadInfo != NULL) {
        gate->name = loadInfo->name;
        if(loadInfo->state != NULL) {
            *state = *(UniGateState*)loadInfo->state;
        }
    }

    return gate;
}

static void GateNot_Tick(void* state_erased) {
    UniGateState* state = (UniGateState*)state_erased;
    state->outn = (state->out = !state->in1);
}

static int n = 0;
LogicGate* GateNor_New(const LogicGate* loadInfo) {
    LogicGate* gate = Gates_Allocate();
    gate->tick = GateNor_Tick;
    BiGateState* state = calloc(1, sizeof(BiGateState));
    gate->state = state;
    gate->pinFirstIn = &state->in1;
    gate->pinFirstOut = &state->out;

    if(loadInfo != NULL) {
        gate->name = loadInfo->name;
        if(loadInfo->name != NULL) {
            *state = *(BiGateState*)loadInfo->state;
        }
    }
    state->n = n++;

    return gate;
}

static void GateNor_Tick(void* state_erased) {
    BiGateState* state = (BiGateState*)state_erased;
    state->outn = !(state->out = !(state->in1 | state->in2));
    printf("NOR %u: in1 = %d, in2 = %d, out = %d\n", state->n, state->in1, state->in2, state->out);
}

typedef struct {
    StremVector /* unsigned */ inEdgeIndices;
    StremVector /* unsigned */ outEdgeIndices;
    LogicGate* gate;
    int hasTicked;
} LogicScheme_Node;

typedef struct {
    char* fromNodeOutPin;
    char* toNodeInPin;
    unsigned fromNodeId;
    unsigned toNodeId;
    int isResolved;
    int value;
} LogicScheme_Edge;

typedef struct {
    unsigned cycleId;
    // common node between two links
    // the last link has undefined value
    unsigned nodeId; 
} CycleChainLink;

typedef struct {
    StremVector /* CycleChainLink */ chain;
} CycleChain;

typedef struct {
    StremVector /* LogicScheme_Node */ nodes;
    StremVector /* LogicScheme_Edge */ edges;
    StremVector /* StremVector<unsigned> */ cycles;
    StremVector /* char */ cyclesMet;
    StremVector /* StremVector<CycleChain> */ cycleChains;
    // chainPerCycle.size == cycles.size, where each corresponding index contains reffered chain number
    StremVector /* unsigned */ chainPerCycleNums;
    StremHashTable /* unsigned, StremVector<unsigned> */ cyclesPerNode;
} LogicScheme;

static LogicScheme Schemes[SCHEMES_MAX_COUNT];
static size_t Schemes_Count = 0;

StremVector Outs;
StremVector OutsAfter;
bool AreOutsInit = false;

static void Scheme_PrintCycle(const LogicScheme* s, int cycleId);
static void Scheme_PrintCycle_(const LogicScheme* s, const StremVector* edgePath);

void* Scheme_New() {
    LogicScheme* s = &Schemes[Schemes_Count++];
    s->nodes = StremVector_construct(sizeof(LogicScheme_Node), SCHEME_NODE_CAP);
    s->edges = StremVector_construct(sizeof(LogicScheme_Edge), SCHEME_EDGE_CAP);
    s->cycles = StremVector_construct(sizeof(StremVector), SCHEME_EDGE_CAP);
    s->cyclesMet = StremVector_construct(sizeof(char), SCHEME_EDGE_CAP);
    s->cyclesPerNode = StremHashTable_construct(
        sizeof(unsigned), sizeof(StremVector),
        Scheme_HashUnsigned, Scheme_CmpUnsigned
    );

    return (void*)s;
}

unsigned Scheme_MakeNode(void* scheme_erased, LogicGate* gate) {
    LogicScheme* s = (LogicScheme*)scheme_erased;
    StremVector_reserve(&s->nodes, s->nodes.size+1);
    const unsigned id = s->nodes.size;
    s->nodes.size++;
    LogicScheme_Node* node = &StremVectorAt(s->nodes, LogicScheme_Node, id);
    *node = (LogicScheme_Node) {
        StremVector_construct(sizeof(unsigned), EDGE_CAP),
        StremVector_construct(sizeof(unsigned), EDGE_CAP),
        gate
    };

    return id;
}

unsigned Scheme_MakeEdge(void* scheme_erased, int firstNodeId, int secondNodeId, int firstPinOut, int secondPinIn) {
    LogicScheme* s = (LogicScheme*)scheme_erased;
    LogicScheme_Node* fnode = &StremVectorAt(s->nodes, LogicScheme_Node, firstNodeId);
    LogicScheme_Node* snode = &StremVectorAt(s->nodes, LogicScheme_Node, secondNodeId);

    const unsigned edgeId = s->edges.size;
    StremVector_reserve(&s->edges, edgeId+1);
    LogicScheme_Edge* e = &StremVectorAt(s->edges, LogicScheme_Edge, edgeId);
    s->edges.size++;

    e->fromNodeId = firstNodeId;
    e->toNodeId = secondNodeId;
    e->fromNodeOutPin = &fnode->gate->pinFirstOut[firstPinOut];
    e->toNodeInPin = &snode->gate->pinFirstIn[secondPinIn];
    e->isResolved = 0;

    StremVector_push(&fnode->outEdgeIndices, &edgeId, 1);
    StremVector_push(&snode->inEdgeIndices, &edgeId, 1);

    return edgeId;
}

// TODO: update, and fix nested vectors
void Scheme_Free(void* scheme_erased) {
    LogicScheme* s = (LogicScheme*)scheme_erased;
    StremVector_free(&s->nodes);
    StremVector_free(&s->edges);
    StremVector_free(&s->cycles);
    StremVector_free(&s->cyclesMet);
    StremHashTable_free(&s->cyclesPerNode);
}

static unsigned SchemeNode_AreEncycledTogether(LogicScheme* s, unsigned fNodeId, unsigned sNodeId) {
    const StremVector* /* unsigned */ fCycles = StremHashTable_at(&s->cyclesPerNode, &fNodeId);
    if(fCycles == NULL) {
        return false;
    }

    const StremVector* /* unsigned */ sCycles = StremHashTable_at(&s->cyclesPerNode, &sNodeId);
    if(sCycles == NULL) {
        return false;
    }

    memset(s->cyclesMet.content, 0, s->cyclesMet.size*s->cyclesMet.elem_size);
    for(unsigned i = 0; i < fCycles->size; ++i) {
        StremVectorAt(s->cyclesMet, char, StremVectorAt(*fCycles, unsigned, i)) = 1;
    }

    unsigned cycleNum = 0;
    for(unsigned i = 0; i < sCycles->size; ++i) {
        const unsigned cycleId = StremVectorAt(*sCycles, unsigned, i);
        if(StremVectorAt(s->cyclesMet, char, cycleId) != 0) {
            cycleNum = cycleId + 1;
            break;
        }
    }

    return cycleNum;
}


/*
static void SchemeNode_traverse(LogicScheme* s, LogicScheme_Node* n) {
    for(size_t i = 0; i < n->inEdgeIndices.size; ++i) {
        unsigned edgeId = StremVectorAt(n->inEdgeIndices, unsigned, i);
        LogicScheme_Edge* inEdge = &StremVectorAt(
            s->edges, LogicScheme_Edge, edgeId
        );
        // printf("\tCheck for child node %lu\n", i);
        if(!inEdge->isResolved) {
            // printf("\tIs not resolved");
            LogicScheme_Node* fromNode = &StremVectorAt(
                s->nodes, LogicScheme_Node, inEdge->fromNodeId
            );
            SchemeNode_traverse(s, fromNode);
        }
    }
    if(!n->hasTicked) {
        // printf("Ticking\n");
        n->gate->tick(n->gate->state);
        n->hasTicked = 1;
    }

    for(size_t i = 0; i < n->outEdgeIndices.size; ++i) {
        unsigned edgeId = StremVectorAt(n->outEdgeIndices, unsigned, i);
        LogicScheme_Edge* outEdge = &StremVectorAt(
            s->edges, LogicScheme_Edge, edgeId
        );
        outEdge->isResolved = 1;
        *outEdge->toNodeInPin = *outEdge->fromNodeOutPin;
    } 
}
*/

static void SchemeNode_Tick(LogicScheme* s, LogicScheme_Node* n) {
    n->gate->tick(n->gate->state);
    n->hasTicked = 1;

    for(size_t i = 0; i < n->outEdgeIndices.size; ++i) {
        const unsigned edgeId = StremVectorAt(n->outEdgeIndices, unsigned, i);
        LogicScheme_Edge* outEdge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeId);
        outEdge->isResolved = 1;
        *outEdge->toNodeInPin = *outEdge->fromNodeOutPin;
    }
}

static void SchemeNode_CopyNodeOutputs(const LogicScheme* s, const LogicScheme_Node* n, StremVector* /* char */ outputs) {
    const unsigned outEdgeCount = n->outEdgeIndices.size;
    StremVector_reserve(outputs, outEdgeCount);
    outputs->size = outEdgeCount;

    for(unsigned i = 0; i < outEdgeCount; ++i) {
        const unsigned edgeInd = StremVectorAt(n->outEdgeIndices, unsigned, i);
        const LogicScheme_Edge* edge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeInd);
        StremVectorAt(*outputs, char, i) = *edge->fromNodeOutPin;
    }
}

static unsigned SchemeNode_FindEdgeFrom(const LogicScheme* s, const StremVector* edgePath, unsigned fromNodeId) {
    unsigned curEdgeInd = 0;
    for(unsigned i = 0; i < edgePath->size; ++i) {
        const LogicScheme_Edge* startEdge = &StremVectorAt(s->edges, LogicScheme_Edge, i);
        if(startEdge->fromNodeId == fromNodeId) {
            curEdgeInd = i;
            break;
        }
    }
    return curEdgeInd;
}

static void SchemeNode_TraverseCycle(LogicScheme* s, unsigned nodeId, unsigned cycleId);
static bool SchemeNode_TryStabilizeCycle(LogicScheme* s, unsigned nodeId, unsigned cycleId, StremVector* outs, StremVector* outsAfter);

static void SchemeNode_EvaluateInputs(LogicScheme* s, unsigned nodeId, bool evalCycleIfNeeded) {

    LogicScheme_Node* node = &StremVectorAt(s->nodes, LogicScheme_Node, nodeId);
    for(unsigned i = 0; i < node->inEdgeIndices.size; ++i) {
        const unsigned edgeInd = StremVectorAt(node->inEdgeIndices, unsigned, i);
        LogicScheme_Edge* edge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeInd);
        LogicScheme_Node* from = &StremVectorAt(s->nodes, LogicScheme_Node, edge->fromNodeId);

        if(!from->hasTicked) {
            unsigned commonCycleNum = SchemeNode_AreEncycledTogether(s, nodeId, edge->fromNodeId);
            if(commonCycleNum != 0) {
                if(evalCycleIfNeeded) {
                    SchemeNode_TryStabilizeCycle(s, edge->fromNodeId, commonCycleNum - 1, &Outs, &OutsAfter);
                }
            } else {
                SchemeNode_EvaluateInputs(s, edge->fromNodeId, true);
                SchemeNode_Tick(s, from);
            }
        }
    }
}

static bool SchemeNode_TryStabilizeCycle(LogicScheme* s, unsigned nodeId, unsigned cycleId, StremVector* outs, StremVector* outsAfter) {
    StremVector* /* unsigned */ edgePath = &StremVectorAt(s->cycles, StremVector, cycleId);
    // length in edges
    const unsigned cycleLen = edgePath->size;
    unsigned curEdgeIdInd = SchemeNode_FindEdgeFrom(s, edgePath, nodeId);
    // Scheme_PrintCycle_(s, edgePath);

    // printf("TryStabilizeCycle: RUN\n");
    for(int traverse = 0; traverse < CYCLE_MAX_TRAVERSION; ++traverse) {
        for(unsigned i = 0; i < cycleLen; ++i) {
            const unsigned curEdgeInd = StremVectorAt(*edgePath, unsigned, curEdgeIdInd);
            const LogicScheme_Edge* edge = &StremVectorAt(s->edges, LogicScheme_Edge, curEdgeInd);
            LogicScheme_Node* node = &StremVectorAt(s->nodes, LogicScheme_Node, edge->fromNodeId);

            // printf("TryStabilizeCycle: cycleId = %u, curEdgeInd = %u, from = %u\n", cycleId, curEdgeIdInd, edge->fromNodeId);
            SchemeNode_EvaluateInputs(s, edge->fromNodeId, false);
            SchemeNode_CopyNodeOutputs(s, node, outs);
            SchemeNode_Tick(s, node);
            SchemeNode_CopyNodeOutputs(s, node, outsAfter);
            // printf("TryStabilizeCycle: END cycleId = %u\n", cycleId);

            if(traverse != 0 && memcmp(outs->content, outsAfter->content, outs->size * outs->elem_size) == 0) {
                // printf("TryStabilizeCycle: END RUN\n");
                return true;
            }

            curEdgeIdInd = (curEdgeIdInd+1)%cycleLen;
        }
        // printf("TryStabilizeCycle: NEW TRAVERSE CYCLE\n");
    }
    return false;
}

// TODO: this thing is not finished; refactor it to use cycle chain
static void SchemeNode_GetCycleChain(
    LogicScheme* s, 
    StremVector* /* bool */ cyclesInChain,
    unsigned startCycleId,
    CycleChain* chain /* out */
    ) {
    const StremVector* /* unsigned */ edgePath = &StremVectorAt(s->cycles, StremVector, startCycleId);
    const unsigned cycleLen = edgePath->size;
    chain->chain.size = 0;

    cyclesInChain->size = s->cycles.size;

    for(unsigned i = 0; i < cycleLen; ++i) {
        const LogicScheme_Edge* edge = &StremVectorAt(s->edges, LogicScheme_Edge, i);
        StremVector* /* unsigned */ cyclesPerNode = (StremVector*)StremHashTable_at(&s->cyclesPerNode, &edge->fromNodeId);

        for(unsigned cycleIdInd = 0; cycleIdInd < cyclesPerNode->size; ++cycleIdInd) {
            const unsigned cycleId = StremVectorAt(*cyclesPerNode, unsigned, cycleIdInd);
            if(StremVectorAt(*cyclesInChain, bool, cycleId) == false) {
                StremVector_push(&chain->chain, &cycleId, 1); // TODO
                StremVectorAt(*cyclesInChain, bool, cycleId) = true;
            }
        }
    }
}

static void SchemeNode_TraverseCycle(LogicScheme* s, unsigned nodeId, unsigned cycleId) {
    StremVector* /* unsigned */ edgePath = &StremVectorAt(s->cycles, StremVector, cycleId);
    // length in edges
    const unsigned cycleLen = edgePath->size;
    unsigned curEdgeInd = SchemeNode_FindEdgeFrom(s, edgePath, nodeId);

    bool success = SchemeNode_TryStabilizeCycle(s, nodeId, cycleId, &Outs, &OutsAfter);

    if(success) {
        return;
    }

    printf("ERROR: Traversing cycle (");
    Scheme_PrintCycle(s, cycleId);
    printf(") exceeded its limit which is %d\n", CYCLE_MAX_TRAVERSION);
    exit(1);
}

static void SchemeNode_TryStabilizeCycles(LogicScheme* s, const StremVector* /* unsigned */ cycleIds) {
    
}

// TODO: fix to use cycle chains, otherwise risks to cause UB in scheme's work
static void SchemeNode_traverse(LogicScheme* s, unsigned nodeId) {
    LogicScheme_Node* n = &StremVectorAt(s->nodes, LogicScheme_Node, nodeId);

    SchemeNode_EvaluateInputs(s, nodeId, true);

    // may have ticked if is encycled
    if(n->hasTicked) {
        return;
    }

    const StremVector* cyclesPerNode = StremHashTable_at(&s->cyclesPerNode, &nodeId);
    if(cyclesPerNode != NULL) {
        for(unsigned cycleIdInd = 0; cycleIdInd < cyclesPerNode->size; ++cycleIdInd) {
            unsigned cycleId = StremVectorAt(*cyclesPerNode, unsigned, cycleIdInd);
            SchemeNode_TryStabilizeCycle(s, nodeId, cycleId, &Outs, &OutsAfter);
        }
    }

    SchemeNode_Tick(s, n);
}

void Scheme_traverse(void* scheme_erased) {
    LogicScheme* s = (LogicScheme*)scheme_erased;
    
    for(size_t edgeInd = 0; edgeInd < s->edges.size; ++edgeInd) {
        StremVectorAt(s->edges, LogicScheme_Edge, edgeInd).isResolved = 0;
    }
    for(size_t nodeInd = 0; nodeInd < s->nodes.size; ++nodeInd) {
        StremVectorAt(s->nodes, LogicScheme_Node, nodeInd).hasTicked = 0;
    }

    for(size_t nodeInd = 0; nodeInd < s->nodes.size; ++nodeInd) {
        // printf("Start traverse for %lu\n", nodeInd);
        SchemeNode_traverse(s, nodeInd);
    }
}

static size_t Scheme_HashEdgePath(const LogicScheme* sc, const StremVector* /* unsigned */  edgePath) {
    size_t hash = (size_t)-1;
    for(unsigned i = 0; i < edgePath->size; ++i) {
        const LogicScheme_Edge* edge = &StremVectorAt(sc->edges, LogicScheme_Edge, i);
        const unsigned fromNode = edge->fromNodeId;
        hash = (hash << 1) ^ fromNode;
    }
    return hash;
}

static StremVector Scheme_FromLinkedEdgesToVector(
    const LogicScheme* s, 
    const unsigned* linkedEdges, 
    unsigned curEdgeInd,
    unsigned firstCycledNum
    ) {
    StremVector edgePath = StremVector_construct(sizeof(unsigned), 4);

    // at first, curEdgeInd is index, but after subscript we have to convert it from number
    for(;; curEdgeInd = linkedEdges[curEdgeInd] - 1) {
        StremVector_push(&edgePath, &curEdgeInd, 1);
        if(StremVectorAt(s->edges, LogicScheme_Edge, curEdgeInd).fromNodeId == firstCycledNum - 1) {
            break;
        }
        if(linkedEdges[curEdgeInd] == 0) {
            printf("ERROR: FromLinkedEdgesToVector: reached uncycled node\n");
            exit(1);
        }
    }

    return edgePath;
}

static bool Scheme_IsCyclePresent(
    const LogicScheme* s, 
    const StremVector* /* unsigned */ edgePath,
    const StremVector* /* size_t */ cycleHashes,
    size_t* /* out */ hash
    ) {
    *hash = Scheme_HashEdgePath(s, edgePath);

    for(unsigned hashInd = 0; hashInd < cycleHashes->size; ++hashInd) {
        if(*hash == StremVectorAt(*cycleHashes, size_t, hashInd)) {
            const StremVector* /* unsigned */ presentEdgePath = &StremVectorAt(s->cycles, StremVector, hashInd);

            for(unsigned edgeIdInd = 0; edgeIdInd < edgePath->size; ++edgeIdInd) {
                if(StremVectorAt(*edgePath, unsigned, edgeIdInd) 
                    != StremVectorAt(*presentEdgePath, unsigned, edgeIdInd)) {
                    return false;
                }
            }
            
            return true;
        } // if hash == ...
    } // hash: hashes
    return false;
}

static void Scheme_AddCycleIfNotExists(LogicScheme* s, StremVector* cycleHashes, const unsigned* linkedEdges, unsigned curEdgeInd, unsigned firstCycledNum) {
    const unsigned startInd = curEdgeInd;
    StremVector edgePath = Scheme_FromLinkedEdgesToVector(s, linkedEdges, curEdgeInd, firstCycledNum);
    // printf("AddCycle: firstCycledNum = %u\n", firstCycledNum);
    Scheme_PrintCycle_(s, &edgePath);
    putc('\n', stdout);
    
    size_t hash = 0;
    
    // we don't want to duplicate cycles just because some node connects to the other twice
    if(Scheme_IsCyclePresent(s, &edgePath, cycleHashes, &hash)) {
        StremVector_free(&edgePath);
        return;
    }

    const unsigned cycleId = s->cycles.size;

    // at first, curEdgeInd is index, but after subscript we have to convert it from number
    for(;; curEdgeInd = linkedEdges[curEdgeInd] - 1) {
        // printf("AddCycle: iter over edge: %u\n", curEdgeInd);
        const LogicScheme_Edge* curEdge = &StremVectorAt(s->edges, LogicScheme_Edge, curEdgeInd);
        const unsigned nodeInd = curEdge->fromNodeId;

        StremVector* /* unsigned */ cyclesPerNode = (StremVector*)StremHashTable_at(
            &s->cyclesPerNode, &nodeInd);
        if(cyclesPerNode == NULL) {
            StremVector newVec = StremVector_construct(sizeof(unsigned), 4);
            // printf("adding cycle to node %u\n", nodeInd);
            cyclesPerNode = StremHashTable_insert(&s->cyclesPerNode, &nodeInd, &newVec);
        }
        StremVector_push(cyclesPerNode, &cycleId, 1);

        if(curEdge->fromNodeId == firstCycledNum - 1) {
            break;
        }
        if(linkedEdges[curEdgeInd] == 0) {
            printf("ERROR: Scheme_AddCycle: reached uncycled node\n");
            exit(1);
        }
    }

    StremVector_push(&s->cycles, &edgePath, 1);
    StremVector_push(cycleHashes, &hash, 1);

    StremVector_reserve(&s->cyclesMet, s->cyclesMet.size + 1);
    s->cyclesMet.size++; // just bumping 
}

// We use edge numbers (and not indices) in accounting, so adding 1 to it results in 0,
// which means that we can utilize this behaviour and start with no edge
#define TRAV_EDGE_START ((unsigned)-1)

static void Scheme_TraverseDetectCycles(
    LogicScheme* scheme,
    StremVector* /* size_t */ cycleHashes,
    unsigned nodeInd,
    unsigned edgeInd,
    unsigned* nodesInPath,
    unsigned* linkedEdges,
    char* nodesChecked
    ) {
    nodesChecked[nodeInd] = 1;
    nodesInPath[nodeInd] = 1;

    const LogicScheme_Node* parent = &StremVectorAt(scheme->nodes, LogicScheme_Node, nodeInd);

    // printf("DetectCycles: process %u node\n", nodeInd);
    for(unsigned i = 0; i < parent->outEdgeIndices.size; ++i) {
        const unsigned outEdgeInd = StremVectorAt(parent->outEdgeIndices, unsigned, i);
        const LogicScheme_Edge* outEdge = &StremVectorAt(scheme->edges, LogicScheme_Edge, outEdgeInd);

        // printf("DetectCycles:\tchild %u ", outEdge->toNodeId);
        if(nodesInPath[outEdge->toNodeId] != 0) { /* cycle detected */
            const unsigned outCycledNum = nodesInPath[outEdge->toNodeId];

            linkedEdges[outEdgeInd] = edgeInd + 1;
            Scheme_AddCycleIfNotExists(scheme, cycleHashes, linkedEdges, outEdgeInd, outCycledNum);
            linkedEdges[outEdgeInd] = 0;
            printf("(in cycle already)\n");
            continue;
        }
        // printf("(not in)\n");

        linkedEdges[outEdgeInd] = edgeInd + 1;
        if(!nodesChecked[outEdge->toNodeId]) {
            Scheme_TraverseDetectCycles(scheme, cycleHashes, outEdge->toNodeId, outEdgeInd, nodesInPath, linkedEdges, nodesChecked);
        }
        linkedEdges[outEdgeInd] = 0;
    } // edge: edges
    // printf("DetectCycles: END process %u node\n", nodeInd);
    
    nodesInPath[nodeInd] = 0;
}

static void Scheme_DetectCycles(LogicScheme* s) {
    StremVector /* size_t */ cycleHashes = StremVector_construct(sizeof(size_t), 4);
    unsigned* linkedEdges = (unsigned*)calloc(s->edges.size, sizeof(unsigned));
    unsigned* nodesInPath = (unsigned*)calloc(s->nodes.size, sizeof(unsigned));
    char* nodesChecked = (char*)calloc(s->nodes.size, sizeof(char));

    for(unsigned nodeInd = 0; nodeInd < s->nodes.size; ++nodeInd) {
        if(!nodesChecked[nodeInd]) {
            Scheme_TraverseDetectCycles(s, &cycleHashes, nodeInd, TRAV_EDGE_START, nodesInPath, linkedEdges, nodesChecked);
        }
    }

    free(linkedEdges);
    free(nodesInPath);
    free(nodesChecked);
    StremVector_free(&cycleHashes);
}

void Scheme_EndScheming(void* scheme_erased) {
    LogicScheme* s = (LogicScheme*)scheme_erased;
    Scheme_DetectCycles(s);

    if(!AreOutsInit) {
        Outs = StremVector_construct(sizeof(char), 4);
        OutsAfter = StremVector_construct(sizeof(char), 4);
    }
    /*
    StremVector cyclesInChain = StremVector_construct(sizeof(bool), s->cycles.size);
    StremVector chain = StremVector_construct(sizeof(unsigned), 2);

    for(unsigned cycleId = 0; cycleId < s->cycles.size; ++cycleId) {
        SchemeNode_GetCycleChain(s, &cyclesInChain, cycleId, &chain);

        StremVector copiedChain = StremVector_copy(&chain);
        const unsigned cycleChainId = s->cycleChains.size;
        StremVector_push(&s->cycleChains, &copiedChain, 1);

        for(unsigned cycleIdInd = 0; cycleIdInd < chain.size; ++cycleIdInd) {
            const unsigned chainedCycleId = StremVectorAt(chain, unsigned, cycleIdInd);
            StremVectorAt(s->chainPerCycleNums, unsigned, chainedCycleId) = cycleChainId + 1;
        }
    }

    StremVector_free(&cyclesInChain);
    */
}

static void Scheme_PrintCycle_(const LogicScheme* s, const StremVector* edgePath) {
    for(int i = 0; i < edgePath->size; ++i) {
        const unsigned edgeId = StremVectorAt(*edgePath, unsigned, i);
        const LogicScheme_Edge* edge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeId);
        printf("{%u}: ", edgeId);
        if(i == 0) {
            printf("%u -> %u", edge->fromNodeId, edge->toNodeId);
        } else {
            printf(" -> %u", edge->toNodeId);
        }
    }

}

static void Scheme_PrintCycle(const LogicScheme* s, int cycleId) {
    const StremVector* /* unsigned */ edgePath = &StremVectorAt(s->cycles, StremVector, cycleId);
    Scheme_PrintCycle_(s, edgePath);
}

void Scheme_PrintCycles(void* scheme_erased) {
    LogicScheme* s = (LogicScheme*)scheme_erased;

    printf("---\\ CYCLES /---\n");
    for(int i = 0; i < s->cycles.size; ++i) {
        printf("[%d] ", i);
        Scheme_PrintCycle(s, i);
        putc('\n', stdout);
    }
}

static size_t Scheme_HashUnsigned(void const* ptr) {
    return (size_t)(*(unsigned*)ptr);
}
static bool Scheme_CmpUnsigned(void const* fptr, void const* sptr) {
    size_t f = *(unsigned*)fptr;
    size_t s = *(unsigned*)sptr;

    return f == s;
}
