#ifndef MMISIM_UTILS_H_
#define MMISIM_UTILS_H_
#include <BasicLogic.h>
#include <stdbool.h>
#include <stddef.h>
// Contains useful, but not essential gates, such as:
//     DC gate, AC gate,
//     Switch - consisting of AND gate and source,
//     AutoSource - whose out value is sampled from static array
//     AutoSwitch - switch, whose source is AutoSource,
//     AutoRecord - whose input value is copied into a static array

// Sources are not meant to be used in cycles;
// They must give new value (1 or 0) only every scheme tick,
// while other elements may tick several times during a scheme one.
typedef struct {
    int n;
    bool isEstablished;
    char out;
} DCState;

typedef struct {
    int n;
    char out;
} ACState;

typedef struct {
    size_t index;
    char out;
} AutoSourceState;

typedef struct {
    size_t index;
    char in;
    char out;
} AutoRecordState;

LogicGate* GateManualDC_New(const LogicGate* loadInfo);
LogicGate* GateManualAC_New(const LogicGate* loadInfo);
LogicGate* GateAutoSource_New(const LogicGate* loadInfo, unsigned* /* out */ sourceIndex);
LogicGate* GateAutoRecord_New(const LogicGate* loadInfo, unsigned* /* out */ recIndex);

void GateAutoSource_SetMany(const char* values, size_t count, size_t off);
void GateAutoSource_SetBitmap(unsigned map, size_t mapOff, size_t dstOff);
void GateAutoRecord_GetMany(char* values, size_t count, size_t off);
void GateAutoRecord_Print(size_t count, size_t off);

// Switch actually consists of two elements: AND gate and source.
// Source is connected to 1'st input of the gate.
// If vc == NULL, creates ManualAC node.
// Returns node index of the gate.
unsigned Scheme_MakeSwitch(void* scheme, LogicGate* vc);
unsigned Scheme_MakeAutoSwitch(void* scheme, unsigned* /* out */ sourceIndex);

#endif // MMISIM_UTILS_H_
