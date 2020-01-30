#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "commands.h"
#include "util.h"

Handle debughandle = 0;

Mutex actionLock;

//Controller:
bool bControllerIsInitialised = false;
u64 controllerHandle = 0;
HiddbgHdlsDeviceInfo controllerDevice = {0};
HiddbgHdlsState controllerState = {0};


int attach()
{
    if (debughandle != 0)
        svcCloseHandle(debughandle);
    u64 pids[300];
    u32 numProc;
    svcGetProcessList(&numProc, pids, 300);
    u64 pid = pids[numProc - 1];

    Result rc = svcDebugActiveProcess(&debughandle, pid);
    if (R_FAILED(rc))
    {
        return 1;
    }
    return 0;
}

void detach()
{
    if (debughandle != 0)
        svcCloseHandle(debughandle);
    debughandle = 0;
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



void poke(u64 addr, u64 size, u8* val)
{
    Result rc = svcWriteDebugProcessMemory(debughandle, val, addr, size);
    free(val);
}

void peek(u64 addr, u64 size)
{
    u8 out[size];
    Result rc = svcReadDebugProcessMemory(&out, debughandle, addr, size);

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