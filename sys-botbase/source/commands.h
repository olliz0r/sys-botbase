#include <switch.h>

extern Handle debughandle;
extern bool bControllerIsInitialised;
extern HiddbgHdlsHandle cHandle;
extern HiddbgHdlsDeviceInfo controllerDevice;
extern HiddbgHdlsState controllerState;
extern u64 buttonClickSleepTime;

typedef struct {
    u64 main_nso_base;
    u64 heap_base;
    u64 titleID;
    u8 buildID[0x20];
} MetaData;

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
u64 followMainPointer(u64* jumps, size_t count);