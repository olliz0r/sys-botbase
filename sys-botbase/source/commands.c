#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "commands.h"
#include "util.h"

Mutex actionLock;

//Controller:
bool bControllerIsInitialised = false;
u64 controllerHandle = 0;
HiddbgHdlsDeviceInfo controllerDevice = {0};
HiddbgHdlsState controllerState = {0};

Handle debughandle = 0;

void attach()
{
    u64 pid = 0;
    pmdmntInitialize();
    pmdmntGetApplicationProcessId(&pid);

    if (debughandle != 0)
        svcCloseHandle(debughandle);

    Result rc = svcDebugActiveProcess(&debughandle, pid);
}

void detach(){
    if (debughandle != 0)
        svcCloseHandle(debughandle);
}

u64 getHeapBaseAddress(){
    MemoryInfo meminfo;
    memset(&meminfo, 0, sizeof(MemoryInfo));

    u64 lastaddr = 0;
    u64 curaddr = 0;
    do
    {
        lastaddr = meminfo.addr;
        u32 pageinfo;
        svcQueryDebugProcessMemory(&meminfo, &pageinfo, debughandle, meminfo.addr + meminfo.size);
        if((meminfo.type & MemType_Heap) == MemType_Heap){
            curaddr = meminfo.addr;
            break;
        }
    } while (lastaddr < meminfo.addr + meminfo.size);
    return curaddr;
}

void initController()
{
    if(bControllerIsInitialised) return;
    //taken from switchexamples github
    hiddbgInitialize();
    // Set the controller type to Pro-Controller, and set the npadInterfaceType.
    controllerDevice.deviceType = HidDeviceType_FullKey3;
    controllerDevice.npadInterfaceType = NpadInterfaceType_Bluetooth;
    // Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
    controllerDevice.singleColorBody = RGBA8_MAXALPHA(255,255,255);
    controllerDevice.singleColorButtons = RGBA8_MAXALPHA(0,0,0);
    controllerDevice.colorLeftGrip = RGBA8_MAXALPHA(230,255,0);
    controllerDevice.colorRightGrip = RGBA8_MAXALPHA(0,40,20);

    // Setup example controller state.
    controllerState.batteryCharge = 4; // Set battery charge to full.
    controllerState.joysticks[JOYSTICK_LEFT].dx = 0x0;
    controllerState.joysticks[JOYSTICK_LEFT].dy = -0x0;
    controllerState.joysticks[JOYSTICK_RIGHT].dx = 0x0;
    controllerState.joysticks[JOYSTICK_RIGHT].dy = -0x0;
    hiddbgAttachHdlsWorkBuffer();
    hiddbgAttachHdlsVirtualDevice(&controllerHandle, &controllerDevice);
    bControllerIsInitialised = true;
}



void poke(u64 offset, u64 size, u8* val)
{
    attach();
    svcWriteDebugProcessMemory(debughandle, val, getHeapBaseAddress() + offset, size);
    detach();
    free(val);
}

void peek(u64 offset, u64 size)
{
    u8 out[size];
    attach();
    svcReadDebugProcessMemory(&out, debughandle, getHeapBaseAddress() + offset, size);
    detach();

    u64 i;
    for (i = 0; i < size; i++)
    {
        printf("%02X", out[i]);
    }
    printf("\n");
}

void click(HidControllerKeys btn)
{
    initController();
    press(btn);
    svcSleepThread(50 * 1e+6L); //50ms = 50 000 000
    release(btn);
}
void press(HidControllerKeys btn)
{
    initController();
    controllerState.buttons |= btn;
    hiddbgSetHdlsState(controllerHandle, &controllerState);
}

void release(HidControllerKeys btn)
{
    initController();
    controllerState.buttons &= ~btn;
    hiddbgSetHdlsState(controllerHandle, &controllerState);
}

void setStickState(int side, int dxVal, int dyVal)
{
    initController();
    controllerState.joysticks[side].dx = dxVal;
    controllerState.joysticks[side].dy = dyVal;
    hiddbgSetHdlsState(controllerHandle, &controllerState);
}