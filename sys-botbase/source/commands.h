#include <switch.h>

extern Handle debughandle;
extern bool bControllerIsInitialised;
extern HidDeviceType controllerInitializedType;
extern HiddbgHdlsHandle controllerHandle;
extern HiddbgHdlsDeviceInfo controllerDevice;
extern HiddbgHdlsState controllerState;
extern HiddbgKeyboardAutoPilotState dummyKeyboardState;
extern u64 buttonClickSleepTime;
extern u64 keyPressSleepTime;
extern u64 pollRate;
extern u32 fingerDiameter;

typedef struct {
    u64 main_nso_base;
    u64 heap_base;
    u64 titleID;
    u64 titleVersion;
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

#define JOYSTICK_LEFT 0
#define JOYSTICK_RIGHT 1

void attach();
void detach();
void detachController();
u64 getMainNsoBase(u64 pid);
u64 getHeapBase(Handle handle);
u64 getTitleId(u64 pid);
u64 GetTitleVersion(u64 pid);
u64 getoutsize(NsApplicationControlData* buf);
void getBuildID(MetaData* meta, u64 pid);
MetaData getMetaData(void);
bool getIsProgramOpen(u64 id);

void poke(u64 offset, u64 size, u8* val);
void writeMem(u64 offset, u64 size, u8* val);
void peek(u64 offset, u64 size);
void peekInfinite(u64 offset, u64 size);
void peekMulti(u64* offset, u64* size, u64 count);
void readMem(u8* out, u64 offset, u64 size);
void click(HidNpadButton btn);
void press(HidNpadButton btn);
void release(HidNpadButton btn);
void setStickState(int side, int dxVal, int dyVal);
void reverseArray(u8* arr, int start, int end);
u64 followMainPointer(s64* jumps, size_t count);
void touch(HidTouchState* state, u64 sequentialCount, u64 holdTime, bool hold, u8* token);
void key(HiddbgKeyboardAutoPilotState* states, u64 sequentialCount);
void clickSequence(char* seq, u8* token);
