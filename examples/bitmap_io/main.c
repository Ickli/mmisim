#include <stdlib.h>
#include <stdio.h>

#include <BasicLogic.h>
#include <Utils.h>

#define TIMES 4
#define LEDS_COUNT 3
#define KEYS_COUNT 2
void print_leds() {
    char leds[LEDS_COUNT];
    GateAutoRecord_GetMany(leds, LEDS_COUNT, 0);

    printf("led: ");
    for(int i = 0; i < LEDS_COUNT; i++) {
        printf("%d ", leds[i]);
    }
    printf("\n");
}

void set_ins(int value) {
    char leds[KEYS_COUNT];
    for(int i = 0; i < KEYS_COUNT; i++) {
        leds[i] = !!(value & (1 << i));
    }
    GateAutoSource_SetMany(leds, KEYS_COUNT, 0);
}

int main() {
    DCState dcinfostate = { 0, true, 1 };
    LogicGate dcinfo = {0};
    dcinfo.state = &dcinfostate;

    void* s = Scheme_New();

    unsigned fkey = Scheme_MakeAutoSwitch(s, NULL);
    unsigned skey = Scheme_MakeAutoSwitch(s, NULL);
    unsigned dc = Scheme_MakeNode(s, GateManualDC_New(&dcinfo));
    unsigned and = Scheme_MakeNode(s, GateAnd_New(NULL));
    unsigned fled = Scheme_MakeNode(s, GateAutoRecord_New(NULL, NULL));
    unsigned sled = Scheme_MakeNode(s, GateAutoRecord_New(NULL, NULL));
    unsigned tled = Scheme_MakeNode(s, GateAutoRecord_New(NULL, NULL));

    // Main
    Scheme_MakeEdge(s, dc, fkey, 0, 0);
    Scheme_MakeEdge(s, dc, skey, 0, 0);
    Scheme_MakeEdge(s, fkey, and, 0, 0);
    Scheme_MakeEdge(s, skey, and, 0, 1);
    Scheme_MakeEdge(s, and, tled, 0, 0);

    // LEDs
    Scheme_MakeEdge(s, fkey, fled, 0, 0);
    Scheme_MakeEdge(s, skey, sled, 0, 0);

    for(int i = 0; i < TIMES; ++i) {
        GateAutoSource_SetBitmap(i, 0, 0);
        Scheme_TraverseNew(s);
        printf("leds: ");
        GateAutoRecord_Print(LEDS_COUNT, 0);
    }
 
    return 0;
}
