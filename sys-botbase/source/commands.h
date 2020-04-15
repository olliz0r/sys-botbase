#include <switch.h>

extern Handle debughandle;
bool bControllerIsInitialised;
u64 controllerHandle;
HiddbgHdlsDeviceInfo controllerDevice;
HiddbgHdlsState controllerState;
extern u64 buttonClickSleepTime;


void attach();
void detach();
u64 getMainNsoBase(u64 pid);
u64 getHeapBase(Handle handle);
u64 getTitleId(u64 pid);
void getMetaData(u64* heap_base, u64* main_nso_base, u64* titleID);

void poke(u64 offset, u64 size, u8* val);
void peek(u64 offset, u64 size);
void click(HidControllerKeys btn);
void press(HidControllerKeys btn);
void release(HidControllerKeys btn);
void setStickState(int side, int dxVal, int dyVal);