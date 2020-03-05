#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "commands.h"
#include "util.h"
#include "dmntcht.h"

Mutex actionLock;

//Controller:
bool bControllerIsInitialised = false;
u64 controllerHandle = 0;
HiddbgHdlsDeviceInfo controllerDevice = {0};
HiddbgHdlsState controllerState = {0};

Handle debughandle = 0;
u64 buttonClickSleepTime = 50;

DmntCheatProcessMetadata metaData;

void attach()
{
    Result rc = dmntchtInitialize();
    if (R_FAILED(rc) && debugResultCodes)
        printf("dmntchtInitialize: %d\n", rc);
    rc = dmntchtForceOpenCheatProcess();
    if (R_FAILED(rc) && debugResultCodes)
        printf("dmntchtForceOpenCheatProcess: %d\n", rc);
    rc = dmntchtGetCheatProcessMetadata(&metaData);
    if (R_FAILED(rc) && debugResultCodes)
        printf("dmntchtGetCheatProcessMetadata: %d\n", rc);
}

void initController()
{
    if(bControllerIsInitialised) return;
    //taken from switchexamples github
    Result rc = hiddbgInitialize();
    if (R_FAILED(rc) && debugResultCodes)
        printf("hiddbgInitialize: %d\n", rc);
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
    rc = hiddbgAttachHdlsWorkBuffer();
    if (R_FAILED(rc) && debugResultCodes)
        printf("hiddbgAttachHdlsWorkBuffer: %d\n", rc);
    rc = hiddbgAttachHdlsVirtualDevice(&controllerHandle, &controllerDevice);
    if (R_FAILED(rc) && debugResultCodes)
        printf("hiddbgAttachHdlsVirtualDevice: %d\n", rc);
    bControllerIsInitialised = true;
}



void poke(u64 offset, u64 size, u8* val)
{
    Result rc = dmntchtWriteCheatProcessMemory(offset, val, size);
    if (R_FAILED(rc) && debugResultCodes)
        printf("dmntchtWriteCheatProcessMemory: %d\n", rc);
}

void peek(u64 offset, u64 size)
{
    u8 out[size];
    Result rc = dmntchtReadCheatProcessMemory(offset, &out, size);
    if (R_FAILED(rc) && debugResultCodes)
        printf("dmntchtReadCheatProcessMemory: %d\n", rc);

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
    svcSleepThread(buttonClickSleepTime * 1e+6L);
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