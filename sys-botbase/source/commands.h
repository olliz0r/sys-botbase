#include <switch.h>
#define POLLMIN 17000000L // polling is linked to screen refresh rate (system UI) or game framerate. Most cases this is 1/60 or 1/30

extern Handle debughandle;
extern bool bControllerIsInitialised;
extern HiddbgHdlsHandle controllerHandle;
extern HiddbgHdlsDeviceInfo controllerDevice;
extern HiddbgHdlsState controllerState;
extern HiddbgKeyboardAutoPilotState dummyKeyboardState;
extern u64 buttonClickSleepTime;
extern u64 keyPressSleepTime;
extern u32 fingerDiameter;

typedef struct {
    u64 main_nso_base;
    u64 heap_base;
    u64 titleID;
    u8 buildID[0x20];
} MetaData;

typedef struct {
    HidTouchState* states;
    u64 sequentialCount;
    u64 holdTime;
    bool hold;
    u8 state;
} TouchData;

typedef struct {
    HiddbgKeyboardAutoPilotState* states;
    u64 sequentialCount;
    u8 state;
} KeyData;

void attach();
void detach();
u64 getMainNsoBase(u64 pid);
u64 getHeapBase(Handle handle);
u64 getTitleId(u64 pid);
void getBuildID(MetaData* meta, u64 pid);
MetaData getMetaData(void);

void poke(u64 offset, u64 size, u8* val);
void writeMem(u64 offset, u64 size, u8* val);
void peek(u64 offset, u64 size);
void readMem(u8* out, u64 offset, u64 size);
void click(HidControllerKeys btn);
void press(HidControllerKeys btn);
void release(HidControllerKeys btn);
void setStickState(int side, int dxVal, int dyVal);
void reverseArray(u8* arr, int start, int end);
u64 followMainPointer(s64* jumps, size_t count);
void touch(HidTouchState* state, u64 sequentialCount, u64 holdTime, bool hold);
void key(HiddbgKeyboardAutoPilotState* states, u64 sequentialCount);