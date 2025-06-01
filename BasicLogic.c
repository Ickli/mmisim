#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strem_structures/strem_vector.h>
#include <strem_structures/strem_ht.h>
#include <strem_structures/strem_queue.h>
#include <BasicLogic.h>
#define GATES_MAX_COUNT 256
#define SCHEMES_MAX_COUNT 8
#define SCHEME_NODE_CAP 4
#define SCHEME_EDGE_CAP 16
#define EDGE_CAP 2
#define CYCLE_MAX_TRAVERSION 8

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
}

void GateNand_Tick(void* erased) {
    BiGateState* s = (BiGateState*)erased;
    s->outn = !(s->out = !(s->in1 & s->in2));
}

void GateNand3_Tick(void* erased) {
    TriGateState* s = (TriGateState*)erased;
    s->outn = !(s->out = !(s->in1 & s->in2 & s->in3));
}

int nnand = 0;
int nnand3 = 0;
LogicGate* GateNand_New(const LogicGate* loadInfo) {
    LogicGate* gate = Gates_Allocate();
    gate->tick = GateNand_Tick;
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
    state->n = nnand++;

    return gate;
}

LogicGate* GateNand3_New(const LogicGate* loadInfo) {
    LogicGate* gate = Gates_Allocate();
    gate->tick = GateNand3_Tick;
    TriGateState* state = calloc(1, sizeof(TriGateState));
    gate->state = state;
    gate->pinFirstIn = &state->in1;
    gate->pinFirstOut = &state->out;

    if(loadInfo != NULL) {
        gate->name = loadInfo->name;
        if(loadInfo->name != NULL) {
            *state = *(TriGateState*)loadInfo->state;
        }
    }
    state->n = nnand3++;

    return gate;
}

void GateXor_Tick(void* state_erased) {
    BiGateState* s = (BiGateState*)state_erased;
    s->outn = !(s->out = s->in1 ^ s->in2);
}

LogicGate* GateXor_New(const LogicGate* loadInfo) {
    LogicGate* gate = Gates_Allocate();
    gate->tick = GateXor_Tick;
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
    state->n = nnand++;

    return gate;
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
    StremVector /* LogicScheme_Node */ nodes;
    StremVector /* LogicScheme_Edge */ edges;
    StremVector /* StremVector<unsigned> */ cycles;
    StremVector /* char */ cyclesCheck;
    StremQueue /* LogicScheme_Node* */ nodesQueued;
    // stack, unused currently
    StremVector /* unsigned */ cyclesBeingProcessed;
    // stack, unused currently
    StremVector /* unsigned */ edgesToTraverse;
    StremVector /* char */ dependentOutputs;
    StremHashTable /* unsigned, StremVector<unsigned> */ cyclesPerNode;
} LogicScheme;

static LogicScheme Schemes[SCHEMES_MAX_COUNT];
static size_t Schemes_Count = 0;

StremVector Outs;
StremVector OutsAfter;
StremVector GeneralOuts;
StremVector GeneralOutsAfter;
bool AreOutsInit = false;

static void Scheme_PrintCycle(const LogicScheme* s, int cycleId);
static void Scheme_PrintCycle_(const LogicScheme* s, const StremVector* edgePath);

void* Scheme_New() {
    LogicScheme* s = &Schemes[Schemes_Count++];
    s->nodes = StremVector_construct(sizeof(LogicScheme_Node), SCHEME_NODE_CAP);
    s->edges = StremVector_construct(sizeof(LogicScheme_Edge), SCHEME_EDGE_CAP);
    s->cycles = StremVector_construct(sizeof(StremVector), SCHEME_EDGE_CAP);
    s->cyclesCheck = StremVector_construct(sizeof(char), SCHEME_EDGE_CAP);
    s->cyclesBeingProcessed = StremVector_construct(sizeof(unsigned), SCHEME_EDGE_CAP);
    s->edgesToTraverse = StremVector_construct(sizeof(unsigned), SCHEME_EDGE_CAP);
    s->dependentOutputs = StremVector_construct(sizeof(char), SCHEME_EDGE_CAP);
    s->nodesQueued = StremQueue_construct(sizeof(LogicScheme_Node*), SCHEME_EDGE_CAP);
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
    StremVector_free(&s->cyclesCheck);
    StremVector_free(&s->cyclesBeingProcessed);
    StremVector_free(&s->edgesToTraverse);
    StremVector_free(&s->dependentOutputs);
    StremQueue_free(&s->nodesQueued);
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

    memset(s->cyclesCheck.content, 0, s->cyclesCheck.size*s->cyclesCheck.elem_size);
    for(unsigned i = 0; i < fCycles->size; ++i) {
        StremVectorAt(s->cyclesCheck, char, StremVectorAt(*fCycles, unsigned, i)) = 1;
    }

    unsigned cycleNum = 0;
    for(unsigned i = 0; i < sCycles->size; ++i) {
        const unsigned cycleId = StremVectorAt(*sCycles, unsigned, i);
        if(StremVectorAt(s->cyclesCheck, char, cycleId) != 0) {
            cycleNum = cycleId + 1;
            break;
        }
    }

    return cycleNum;
}

static void SchemeNode_Tick(LogicScheme* s, LogicScheme_Node* n) {
    const size_t ind = n - ((LogicScheme_Node*)s->nodes.content);
    n->gate->tick(n->gate->state);
    n->hasTicked = 1;

    for(size_t i = 0; i < n->outEdgeIndices.size; ++i) {
        const unsigned edgeId = StremVectorAt(n->outEdgeIndices, unsigned, i);
        LogicScheme_Edge* outEdge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeId);
        outEdge->isResolved = 1;
        *outEdge->toNodeInPin = *outEdge->fromNodeOutPin;
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
    // printf("construct cycle vec %u first cycled num {\n", firstCycledNum);
    for(;; curEdgeInd = linkedEdges[curEdgeInd] - 1) {
        // printf("%u edge ind, from node %u\n", curEdgeInd, StremVectorAt(s->edges, LogicScheme_Edge, curEdgeInd).fromNodeId);
        StremVector_push(&edgePath, &curEdgeInd, 1);
        if(StremVectorAt(s->edges, LogicScheme_Edge, curEdgeInd).fromNodeId == firstCycledNum - 1) {
            break;
        }
        if(linkedEdges[curEdgeInd] == 0) {
            printf("ERROR: FromLinkedEdgesToVector: reached uncycled node\n");
            exit(1);
        }
    }
    // printf("} end cycle vec\n");

    return edgePath;
}

static bool Scheme_IsCyclePresent(
    const LogicScheme* s, 
    const StremVector* /* unsigned */ edgePath,
    const StremVector* /* size_t */ cycleHashes,
    size_t* /* out */ hash
    ) {
    *hash = Scheme_HashEdgePath(s, edgePath);

    bool areEqual = false;
    for(unsigned hashInd = 0; !areEqual && hashInd < cycleHashes->size; ++hashInd) {
        if(*hash == StremVectorAt(*cycleHashes, size_t, hashInd)) {
            const StremVector* /* unsigned */ presentEdgePath = &StremVectorAt(s->cycles, StremVector, hashInd);
            Scheme_PrintCycle_(s, presentEdgePath);

            areEqual = presentEdgePath->size == edgePath->size;
            for(unsigned edgeIdInd = 0; areEqual && edgeIdInd < presentEdgePath->size; ++edgeIdInd) {
                areEqual = (StremVectorAt(*edgePath, unsigned, edgeIdInd) == StremVectorAt(*presentEdgePath, unsigned, edgeIdInd));
            }
        } // if hash == ...
    } // hash: hashes, while not areEqual
    return areEqual;
}

static void Scheme_AddCycleIfNotExists(LogicScheme* s, StremVector* cycleHashes, const unsigned* linkedEdges, unsigned curEdgeInd, unsigned firstCycledNum) {
    const unsigned startInd = curEdgeInd;
    StremVector edgePath = Scheme_FromLinkedEdgesToVector(s, linkedEdges, curEdgeInd, firstCycledNum);
    // printf("AddCycle: firstCycledNum = %u\n", firstCycledNum);
    Scheme_PrintCycle_(s, &edgePath);
    
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

    StremVector_reserve(&s->cyclesCheck, s->cyclesCheck.size + 1);
    s->cyclesCheck.size++; // just bumping 
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

    // printf("DetectCycles: process %u node %u edge{\n", nodeInd, edgeInd);
    for(unsigned i = 0; i < parent->outEdgeIndices.size; ++i) {
        const unsigned outEdgeInd = StremVectorAt(parent->outEdgeIndices, unsigned, i);
        const LogicScheme_Edge* outEdge = &StremVectorAt(scheme->edges, LogicScheme_Edge, outEdgeInd);

        // printf("DetectCycles:\tchild %u ", outEdge->toNodeId);
        if(nodesInPath[outEdge->toNodeId] != 0) { /* cycle detected */
            // printf("Find cycle start = %u\n", outEdge->toNodeId);
            const unsigned outCycledNum = outEdge->toNodeId + 1;

            linkedEdges[outEdgeInd] = edgeInd + 1;
            Scheme_AddCycleIfNotExists(scheme, cycleHashes, linkedEdges, outEdgeInd, outCycledNum);
            linkedEdges[outEdgeInd] = 0;
            // printf("(in cycle already)\n");
            continue;
        }
        // printf("(not in)\n");

        linkedEdges[outEdgeInd] = edgeInd + 1;
        Scheme_TraverseDetectCycles(scheme, cycleHashes, outEdge->toNodeId, outEdgeInd, nodesInPath, linkedEdges, nodesChecked);
        linkedEdges[outEdgeInd] = 0;
    } // edge: edges
    // printf("} DetectCycles: END process %u node\n", nodeInd);
    
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
        GeneralOuts = StremVector_construct(sizeof(char), 4);
        GeneralOutsAfter = StremVector_construct(sizeof(char), 4);
    }
}

static void Scheme_PrintCycle_(const LogicScheme* s, const StremVector* edgePath) {
    for(int i = edgePath->size - 1; i >= 0; --i) {
        const unsigned edgeId = StremVectorAt(*edgePath, unsigned, i);
        const LogicScheme_Edge* edge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeId);
        printf("{%u}: ", edgeId);
        if(i == edgePath->size - 1) {
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

/******************* NEW LOGIC *********************/
static void Scheme_TraverseInputs(LogicScheme* s, unsigned nodeId);
static void Scheme_RememberDependentOutputs(LogicScheme* s, unsigned cycleId);

static void Scheme_AddUnmetCyclesToSubstack(LogicScheme* s, size_t subStart, unsigned nodeId) {
    const StremVector* /* unsigned */ cycles = StremHashTable_at(&s->cyclesPerNode, &nodeId);
    for(unsigned i = 0; i < cycles->size; ++i) {
        bool cycleFound = false;
        const unsigned cycleId = StremVectorAt(*cycles, unsigned, i);
        for(unsigned j = subStart; j < s->cyclesBeingProcessed.size; ++j) {
            if(cycleId == StremVectorAt(s->cyclesBeingProcessed, unsigned, j)) {
                cycleFound = true;
                break;
            }
        }

        if(!cycleFound) {
            StremVector_push(&s->cyclesBeingProcessed, &cycleId, 1);
            Scheme_RememberDependentOutputs(s, cycleId);
        }
    }
}

static unsigned Scheme_GetDependentNode(const LogicScheme* s, unsigned cycleId) {
    const StremVector* edgePath = &StremVectorAt(s->cycles, StremVector, cycleId);
    const LogicScheme_Edge* edge = &StremVectorAt(s->edges, LogicScheme_Edge, StremVectorAt(*edgePath, unsigned, edgePath->size - 1));
    return edge->fromNodeId;
}

static void Scheme_RememberDependentOutputs(LogicScheme* s, unsigned cycleId) {
    const LogicScheme_Node* node = &StremVectorAt(s->nodes, LogicScheme_Node, Scheme_GetDependentNode(s, cycleId));

    for(unsigned i = 0; i < node->outEdgeIndices.size; ++i) {
        const unsigned edgeInd = StremVectorAt(node->outEdgeIndices, unsigned, i);
        const LogicScheme_Edge* outEdge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeInd);
        StremVector_push(&s->dependentOutputs, outEdge->toNodeInPin, 1);
    }
}

static void Scheme_TickDependentOutputs_(LogicScheme* s, size_t cycleSubstackStart) {
    for(unsigned cycleIdInd = cycleSubstackStart; cycleIdInd < s->cyclesBeingProcessed.size; ++cycleIdInd) {
        const unsigned cycleId = StremVectorAt(s->cyclesBeingProcessed, unsigned, cycleIdInd);
        // const StremVector* cycle = &StremVectorAt(s->cycles, StremVector, cycleId);

        LogicScheme_Node* depNode = &StremVectorAt(s->nodes, LogicScheme_Node, Scheme_GetDependentNode(s, cycleId));
        SchemeNode_Tick(s, depNode);
    }
}

static void Scheme_TickDependentOutputs(LogicScheme* s, size_t cycleSubstackStart) {
    for(unsigned cycleIdInd = cycleSubstackStart; cycleIdInd < s->cyclesBeingProcessed.size; ++cycleIdInd) {
        const unsigned cycleId = StremVectorAt(s->cyclesBeingProcessed, unsigned, cycleIdInd);
        const StremVector* cycle = &StremVectorAt(s->cycles, StremVector, cycleId);

        // Traversing whole cycle and not solely dependent nodes, because in-mid node may need
        // to propagate new out value to the dep.node, but has already ticked in cycle traversion.
        //
        // traverse from end to start of a stack -> traverse from start to end of a cycle,
        // because cycle is reversed in the stack.
        // traverse toNode and not fromNode, so the last traversed node will be dependent 
        // node of the cycle.
        for(int edgeIdInd = cycle->size - 1; edgeIdInd >= 0; --edgeIdInd) {
            const unsigned edgeId = StremVectorAt(*cycle, unsigned, edgeIdInd);
            LogicScheme_Edge* edge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeId);

            LogicScheme_Node* toNode = &StremVectorAt(s->nodes, LogicScheme_Node, edge->toNodeId);
            SchemeNode_Tick(s, toNode);
        } 
    }
}

static void Scheme_RefreshDependentOutputs(LogicScheme* s, size_t cycleSubstackStart, size_t outSubstackStart) {
    size_t curOutPos = outSubstackStart;

    for(unsigned cycleIdInd = cycleSubstackStart; cycleIdInd < s->cyclesBeingProcessed.size; ++cycleIdInd) {
        const unsigned cycleId = StremVectorAt(s->cyclesBeingProcessed, unsigned, cycleIdInd);
        // const StremVector* cycle = &StremVectorAt(s->cycles, StremVector, cycleId);

        LogicScheme_Node* depNode = &StremVectorAt(s->nodes, LogicScheme_Node, Scheme_GetDependentNode(s, cycleId));

        for(unsigned i = 0; i < depNode->outEdgeIndices.size; ++i) {
            const unsigned edgeInd = StremVectorAt(depNode->outEdgeIndices, unsigned, i);
            const LogicScheme_Edge* outEdge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeInd);

            StremVectorAt(s->dependentOutputs, char, curOutPos) = *outEdge->toNodeInPin;

            curOutPos++;
        }
    }
}

// returns true if outputs have not changed.
// otherwise, returns false and sets firstUnmatchedDepNode
static bool Scheme_CmpDependentOutputs(LogicScheme* s, size_t cycleSubstackStart, size_t outSubstackStart, LogicScheme_Node** firstUnmatchedDepNode) {
    size_t curOutPos = outSubstackStart;

    for(unsigned cycleIdInd = cycleSubstackStart; cycleIdInd < s->cyclesBeingProcessed.size; ++cycleIdInd) {
        const unsigned cycleId = StremVectorAt(s->cyclesBeingProcessed, unsigned, cycleIdInd);
        // const StremVector* cycle = &StremVectorAt(s->cycles, StremVector, cycleId);

        LogicScheme_Node* depNode = &StremVectorAt(s->nodes, LogicScheme_Node, Scheme_GetDependentNode(s, cycleId));

        for(unsigned i = 0; i < depNode->outEdgeIndices.size; ++i) {
            const unsigned edgeInd = StremVectorAt(depNode->outEdgeIndices, unsigned, i);
            const LogicScheme_Edge* outEdge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeInd);

            if(StremVectorAt(s->dependentOutputs, char, curOutPos) != *outEdge->toNodeInPin) {
                // printf("Output has changed for node %u of cycle %u\n", outEdge->fromNodeId, cycleId);
                *firstUnmatchedDepNode = depNode;
                return false;
            }

            curOutPos++;
        }
    }

    return true;
}

static size_t StartSubstack(StremVector* stack) {
    return stack->size;
}

static void EndSubstack(StremVector* stack, size_t substackStart) {
#ifdef DEBUG
    if(substackStart > stack->size) {
        printf("ERROR: EndSubstack: substackStart (%lu) > stack size (%lu)\n", substackStart, stack->size);
        exit(1);
    }
#endif
    stack->size = substackStart;
}

static bool Scheme_IsCycleBeingProcessed(const LogicScheme* s, size_t batchStart, unsigned cycleId) {
    for(size_t i = batchStart; i < s->cyclesBeingProcessed.size; ++i) {
        if(StremVectorAt(s->cyclesBeingProcessed, unsigned, i) == cycleId) {
            return true;
        }
    }
    return false;
}

static void Scheme_TraverseCycledNew(LogicScheme* s, unsigned nodeId) {
    Scheme_TraverseInputs(s, nodeId);

    // i don't like the idea of allocating smth per call, so use slices
    size_t edgeStackStart = StartSubstack(&s->edgesToTraverse);
    size_t cycleStackStart = StartSubstack(&s->cyclesBeingProcessed);
    size_t depOutputsStart = StartSubstack(&s->dependentOutputs);

    int traverse = 0;
    LogicScheme_Node* curnode = &StremVectorAt(s->nodes, LogicScheme_Node, nodeId);
    Scheme_TraverseInputs(s, nodeId);
    SchemeNode_Tick(s, curnode);
    curnode->hasTicked = traverse + 1;
    Scheme_AddUnmetCyclesToSubstack(s, cycleStackStart, nodeId);

    for(traverse = 0; traverse < CYCLE_MAX_TRAVERSION; ++traverse) {
        StremVector_push(&s->edgesToTraverse, curnode->outEdgeIndices.content, curnode->outEdgeIndices.size);

        while(s->edgesToTraverse.size != edgeStackStart) {
            const unsigned edgeInd = StremVectorPopBack(s->edgesToTraverse, unsigned);
            const LogicScheme_Edge* edge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeInd);

            curnode = &StremVectorAt(s->nodes, LogicScheme_Node, edge->toNodeId);

            if(!SchemeNode_AreEncycledTogether(s, edge->fromNodeId, edge->toNodeId)) {
                continue;
            }

            // if traversed on this iteration, skip
            if(curnode->hasTicked == traverse + 1) {
                continue;
            }

            Scheme_TraverseInputs(s, edge->toNodeId);
            SchemeNode_Tick(s, curnode);
            curnode->hasTicked = traverse + 1;

            if(traverse == 0) {
                Scheme_AddUnmetCyclesToSubstack(s, cycleStackStart, edge->toNodeId);
            }
            StremVector_push(&s->edgesToTraverse, curnode->outEdgeIndices.content, curnode->outEdgeIndices.size);
        }

        // Ticks whole cycles
        Scheme_TickDependentOutputs(s, cycleStackStart);
        if(Scheme_CmpDependentOutputs(s, cycleStackStart, depOutputsStart, &curnode)) {
            break;
        }

        // TODO: try to not refresh those outputs which are assured to be equal by CmpDependentOutputs
        Scheme_RefreshDependentOutputs(s, cycleStackStart, depOutputsStart);
    }

    if(traverse == CYCLE_MAX_TRAVERSION) {
        printf("WARNING: Scheme_TraverseCyclesNew: max traversion count exceeded\n");
    }

    EndSubstack(&s->edgesToTraverse, edgeStackStart);
    EndSubstack(&s->cyclesBeingProcessed, cycleStackStart);
    EndSubstack(&s->dependentOutputs, depOutputsStart);
}

static void Scheme_TraverseInputs(LogicScheme* s, unsigned nodeId) {
    LogicScheme_Node* n = &StremVectorAt(s->nodes, LogicScheme_Node, nodeId);

    if(n->hasTicked) {
        return;
    }

    for(unsigned edgeIdInd = 0; edgeIdInd < n->inEdgeIndices.size; ++edgeIdInd) {
        unsigned edgeId = StremVectorAt(n->inEdgeIndices, unsigned, edgeIdInd);
        LogicScheme_Edge* edge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeId);
        // printf("??? %p\n", edge->fromNodeOutPin);

        LogicScheme_Node* fromNode = &StremVectorAt(s->nodes, LogicScheme_Node, edge->fromNodeId);
        if(fromNode->hasTicked) {
            continue;
        }

        bool isFromCycled = (StremHashTable_at(&s->cyclesPerNode, &edge->fromNodeId) != NULL);
        if(!isFromCycled) {
            Scheme_TraverseInputs(s, edge->fromNodeId);
            SchemeNode_Tick(s, fromNode);
        } else if(!SchemeNode_AreEncycledTogether(s, nodeId, edge->fromNodeId)) {
            Scheme_TraverseCycledNew(s, edge->fromNodeId);
        }
    }
}

static void Scheme_EnqueChildren(LogicScheme* s, const LogicScheme_Node* n, bool outputsHaveChanged) {
    for(unsigned edgeIdInd = 0; edgeIdInd < n->outEdgeIndices.size; ++edgeIdInd) {
        const unsigned edgeId = StremVectorAt(n->outEdgeIndices, unsigned, edgeIdInd);
        const LogicScheme_Edge* outEdge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeId);
        const LogicScheme_Node* toNode = &StremVectorAt(s->nodes, LogicScheme_Node, outEdge->toNodeId);

        if(outputsHaveChanged || !toNode->hasTicked) {
            StremQueue_insert(&s->nodesQueued, &toNode);
        }
    }
}

static void SchemeNode_RememberOutputs(LogicScheme* s, const LogicScheme_Node* n) {
    StremVector_reserve(&s->dependentOutputs, n->outEdgeIndices.size);
    s->dependentOutputs.size = n->outEdgeIndices.size;
    
    for(unsigned edgeIdInd = 0; edgeIdInd < n->outEdgeIndices.size; ++edgeIdInd) {
        const unsigned edgeId = StremVectorAt(n->outEdgeIndices, unsigned, edgeIdInd);
        const LogicScheme_Edge* outEdge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeId);
        StremVectorAt(s->dependentOutputs, char, edgeIdInd) = *outEdge->fromNodeOutPin;
    }
}

static bool SchemeNode_HaveOutputsChanged(const LogicScheme* s, const LogicScheme_Node* n) {
    for(unsigned edgeIdInd = 0; edgeIdInd < n->outEdgeIndices.size; ++edgeIdInd) {
        const unsigned edgeId = StremVectorAt(n->outEdgeIndices, unsigned, edgeIdInd);
        const LogicScheme_Edge* outEdge = &StremVectorAt(s->edges, LogicScheme_Edge, edgeId);
        if(StremVectorAt(s->dependentOutputs, char, edgeIdInd) != *outEdge->fromNodeOutPin) {
            return true;
        }
    }
    return false;
}

void Scheme_TraverseNew(void* scheme_erased) {
    LogicScheme* s = (LogicScheme*)scheme_erased;

    for(size_t nodeInd = 0; nodeInd < s->nodes.size; ++nodeInd) {
        StremVectorAt(s->nodes, LogicScheme_Node, nodeInd).hasTicked = 0;
    }

    for(unsigned nodeId = 0; nodeId < s->nodes.size; ++nodeId) {
        LogicScheme_Node* node = &StremVectorAt(s->nodes, LogicScheme_Node, nodeId);
        // may have ticked while traversing cycles: 
        // it is encycled with prev cycled ones or just depends on cycled nodes
        if(node->hasTicked) {
            continue;
        }

        SchemeNode_Tick(s, node);
        Scheme_EnqueChildren(s, node, true);

        while(StremQueueSize(s->nodesQueued) != 0) {
            node = StremQueueDeque(s->nodesQueued, LogicScheme_Node*);

            SchemeNode_RememberOutputs(s, node);
            SchemeNode_Tick(s, node);
            Scheme_EnqueChildren(s, node, SchemeNode_HaveOutputsChanged(s, node));
        }
    }
}
