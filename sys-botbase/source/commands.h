#include <switch.h>
#include "dmntcht.h"

extern Handle debughandle;
bool bControllerIsInitialised;
u64 controllerHandle;
HiddbgHdlsDeviceInfo controllerDevice;
HiddbgHdlsState controllerState;
extern u64 buttonClickSleepTime;

extern DmntCheatProcessMetadata metaData;

void attach();

void poke(u64 offset, u64 size, u8* val);
void peek(u64 offset, u64 size);
void click(HidControllerKeys btn);
void press(HidControllerKeys btn);
void release(HidControllerKeys btn);
void setStickState(int side, int dxVal, int dyVal);