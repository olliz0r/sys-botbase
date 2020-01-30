#include <switch.h>

extern Handle debughandle;
bool bControllerIsInitialised;
u64 controllerHandle;
HiddbgHdlsDeviceInfo controllerDevice;
HiddbgHdlsState controllerState;

int attach();
void detach();

void poke(u64 addr, u64 size, u8* val);
void peek(u64 addr, u64 size);
void click(HidControllerKeys btn);
void press(HidControllerKeys btn);
void release(HidControllerKeys btn);
void setStickState(int side, int dxVal, int dyVal);