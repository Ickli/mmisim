#ifndef MMISIM_BASIC_LOGIC_H_
#define MMISIM_BASIC_LOGIC_H_

typedef void(*LogicGateTick)(void* state);

typedef enum {
    LOGIC_GATE_AND,
    LOGIC_GATE_OR,
} LogicGateType;

typedef struct {
    LogicGateTick tick;
    void* state;
    const char* name;
    char* pinFirstIn;
    char* pinFirstOut;
} LogicGate;

typedef struct {
    char in1;
    char in2;
    char out;
    char outn;
    int n;
} BiGateState;

typedef struct {
    char in1;
    char in2;
    char in3;
    char out;
    char outn;
    int n;
} TriGateState;

typedef struct {
    char in1;
    char out;
    char outn;
} UniGateState;

// Returns pointer to scheme
void* Scheme_New();
void Scheme_Free(void* scheme);
// Returns node id
unsigned Scheme_MakeNode(void* scheme, LogicGate* gate);
unsigned Scheme_MakeEdge(void* scheme, int firstNodeId, int secondNodeId, int firstPinOut, int secondPinIn);
void Scheme_EndScheming(void* scheme);

void Scheme_TraverseNew(void* scheme_erased);
void Scheme_PrintCycles(void* scheme);

// Some parameters in loadInfo, that aren't 0, will be used to init gate.
// If during init user passes 'state->in*' field as 1 and it is not bound with anything, it is regarded as vsource 
LogicGate* GateAnd_New(const LogicGate* loadInfo);
LogicGate* GateOr_New(const LogicGate* loadInfo);
LogicGate* GateNot_New(const LogicGate* loadInfo);
LogicGate* GateNor_New(const LogicGate* loadInfo);
LogicGate* GateNand_New(const LogicGate* loadInfo);
LogicGate* GateNand3_New(const LogicGate* loadInfo);
LogicGate* GateXor_New(const LogicGate* loadInfo);

LogicGate* Gates_Allocate();

#endif // MMISIM_BASIC_LOGIC_H_
