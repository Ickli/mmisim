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

/* extern */ unsigned long TraverseLimit = 1024;

static void GateAnd_Tick(void* state);
static void GateOr_Tick(void* state);
static void GateNot_Tick(void* state);
static void GateNor_Tick(void* state);
static size_t Scheme_HashUnsigned(void const*);
static bool Scheme_CmpUnsigned(void const*, void const*);

static LogicGate Gates[GATES_MAX_COUNT];
static size_t Gates_Count = 0;

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

void Scheme_Free(void* scheme_erased) {
    LogicScheme* s = (LogicScheme*)scheme_erased;

    const size_t nodeCount = s->nodes.size;

    StremVector_free(&s->nodes);
    StremVector_free(&s->edges);

    for(int i = 0; i < s->cycles.size; ++i) {
        StremVector_free(&StremVectorAt(s->cycles, StremVector, i));
    }

    StremVector_free(&s->cycles);
    StremVector_free(&s->cyclesCheck);
    StremVector_free(&s->dependentOutputs);
    StremQueue_free(&s->nodesQueued);

    for(unsigned nodeInd = 0; nodeInd < nodeCount; ++nodeInd) {
        StremVector* /* unsigned */ cycles 
            = StremHashTable_at(&s->cyclesPerNode, &nodeInd);
        if(cycles != NULL) { StremVector_free(cycles); }
    }

    StremHashTable_free(&s->cyclesPerNode);
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

static size_t Scheme_HashEdgePath(const LogicScheme* sc, const StremVector* /* unsigned */  edgePath) {
    size_t hash = (size_t)-1;
    for(unsigned i = 0; i < edgePath->size; ++i) {
        const LogicScheme_Edge* edge = &StremVectorAt(sc->edges, LogicScheme_Edge, i);
        const unsigned fromNode = edge->fromNodeId;
        hash = (hash << 1) ^ fromNode;
    }
    return hash;
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
    // Scheme_PrintCycle_(s, &edgePath);
    
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

        unsigned long limitCounter = 0;

        while(limitCounter < TraverseLimit && StremQueueSize(s->nodesQueued) != 0) {
            node = StremQueueDeque(s->nodesQueued, LogicScheme_Node*);

            SchemeNode_RememberOutputs(s, node);
            SchemeNode_Tick(s, node);
            Scheme_EnqueChildren(s, node, SchemeNode_HaveOutputsChanged(s, node));
            limitCounter++;
        }

        if(limitCounter == TraverseLimit) {
            // TODO: implement step-by-step
            fprintf(stderr, "Exceeded traverse limit, try execute step-by-step\n");
            abort();
        }
    }
}
