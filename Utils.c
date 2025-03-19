#include <Utils.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <strem_structures/strem_vector.h>

void GateManualDC_Tick(void* state_erased) {
    DCState* state = (DCState*)state_erased;
    if(!state->isEstablished) {
        int out;

        printf("Provide 0 or 1 for %d'th DC: ", state->n);
        scanf("%d", &out);
        state->out = (char)out;
        state->isEstablished = true;
    }
}

void GateManualAC_Tick(void* state_erased) {
    ACState* state = (ACState*)state_erased;
    int out;

    printf("Provide 0 or 1 for %d'th AC: ", state->n);
    scanf("%d", &out);
    state->out = (char)out;
}

LogicGate* GateManualDC_New(const LogicGate* loadInfo) {
    LogicGate* gate = Gates_Allocate();
    DCState* state = calloc(1, sizeof(DCState));
    gate->tick = GateManualDC_Tick;
    gate->state = state;
    gate->pinFirstIn = NULL;
    gate->pinFirstOut = &state->out;

    if(loadInfo != NULL) {
        gate->name = loadInfo->name;
        if(loadInfo->state != NULL) {
            *state = *(DCState*)loadInfo->state;
        }
    }

    return gate;
}

LogicGate* GateManualAC_New(const LogicGate* loadInfo) {
    LogicGate* gate = Gates_Allocate();
    ACState* state = calloc(1, sizeof(ACState));
    gate->tick = GateManualAC_Tick;
    gate->state = state;
    gate->pinFirstIn = NULL;
    gate->pinFirstOut = &state->out;

    if(loadInfo != NULL) {
        gate->name = loadInfo->name;
        if(loadInfo->state != NULL) {
            *state = *(ACState*)loadInfo->state;
        }
    }

    return gate;
}

unsigned Scheme_MakeSwitch(void* scheme, LogicGate* vc) {
    if(vc == NULL) {
        vc = GateManualAC_New(NULL);
    }

    LogicGate* and = GateAnd_New(NULL);
    unsigned andn = Scheme_MakeNode(scheme, and);
    unsigned vcn = Scheme_MakeNode(scheme, vc);

    Scheme_MakeEdge(scheme, vcn, andn, 0, 1);

    return andn;
}

// AutoSourceOutputs
static StremVector /* char */ ASOut;
static bool AreASOutInit = false;

// AutoRecordInputs
static StremVector /* char */ ARIn;
static bool AreARInInit = false;

static void InitASOutOnce() {
    if(!AreASOutInit) {
        ASOut = StremVector_construct(sizeof(char), 4);
        AreASOutInit = true;
    }
    StremVector_reserve(&ASOut, 4);
}

static void InitARInOnce() {
    if(!AreARInInit) {
        ARIn = StremVector_construct(sizeof(char), 4);
        AreARInInit = true;
    }
    StremVector_reserve(&ARIn, 4);
}

void GateAutoSource_Tick(void* state_erased) {
    AutoSourceState* state = (AutoSourceState*)state_erased;
    state->out = StremVectorAt(ASOut, char, state->index);
}

LogicGate* GateAutoSource_New(const LogicGate* loadInfo, unsigned* sourceIndex) {
    InitASOutOnce();

    LogicGate* gate = Gates_Allocate();
    AutoSourceState* state = calloc(1, sizeof(AutoSourceState));
    gate->tick = GateAutoSource_Tick;
    gate->state = state;
    gate->pinFirstIn = NULL;
    gate->pinFirstOut = &state->out;

    if(loadInfo != NULL) {
        gate->name = loadInfo->name;
    }

    state->index = ASOut.size;
    if(sourceIndex != NULL) {
        *sourceIndex = state->index;
    }

    StremVector_reserve(&ASOut, ASOut.size+1);
    ASOut.size++;

    return gate;
}

void GateAutoSource_SetMany(const char* values, size_t count, size_t off) {
    InitASOutOnce();

    const size_t size = off + count;
    if(ASOut.size < size) {
        ASOut.size = size;
    }

    void* dst = (void*)((char*)ASOut.content + off);
    memcpy(dst, (void*)values, count*sizeof(char));
}

void GateAutoSource_SetBitmap(unsigned map, size_t mapOff, size_t dstOff) {
    for(size_t i = mapOff; i < sizeof(map); ++i) {
        StremVectorAt(ASOut, char, dstOff + i) = !!(map & (1 << i));
    }
}

unsigned Scheme_MakeAutoSwitch(void* scheme, unsigned* sourceIndex) {
    unsigned asnode = Scheme_MakeNode(scheme, GateAutoSource_New(NULL, sourceIndex));
    unsigned andnode = Scheme_MakeNode(scheme, GateAnd_New(NULL));

    Scheme_MakeEdge(scheme, asnode, andnode, 0, 1);

    return andnode;
}

void GateAutoRecord_Tick(void* state_erased) {
    AutoRecordState* state = (AutoRecordState*)state_erased;
    state->out = state->in;
    StremVectorAt(ARIn, char, state->index) = state->in;
}

LogicGate* GateAutoRecord_New(const LogicGate* loadInfo, unsigned* recIndex) {
    InitARInOnce();

    LogicGate* gate = Gates_Allocate();
    AutoRecordState* state = calloc(1, sizeof(AutoRecordState));
    gate->tick = GateAutoRecord_Tick;
    gate->state = state;
    gate->pinFirstIn = &state->in;
    gate->pinFirstOut = &state->out;

    if(loadInfo != NULL) {
        gate->name = loadInfo->name;
    }

    state->index = ARIn.size;
    if(recIndex != NULL) {
        *recIndex = state->index;
    }

    StremVector_reserve(&ARIn, ARIn.size+1);
    ARIn.size++;

    return gate;
}

void GateAutoRecord_GetMany(char* values, size_t count, size_t off) {
    const size_t size = off + count;
    if(size > ARIn.size) {
        printf("ERROR: GateAutoRecord_GetMany: out of bounds access\n");
        exit(1);
    }

    void* src = (void*)((char*)ARIn.content + off);
    memcpy(values, src, count*sizeof(char));
}

void GateAutoRecord_Print(size_t count, size_t off) {
    const size_t size = off + count;
    if(size > ARIn.size) {
        printf("ERROR: GateAutoRecord_Print: out of bounds access\n");
        exit(1);
    }
    
    for(size_t i = 0; i < count; ++i) {
        printf("%d ", StremVectorAt(ARIn, char, off + i));
    }
    printf("\n");
}
