#include <switch.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "commands.h"
#include "util.h"

//Controller:
bool bControllerIsInitialised = false;
HiddbgHdlsHandle cHandle = {0};
HiddbgHdlsDeviceInfo controllerDevice = {0};
HiddbgHdlsState controllerState = {0};

Handle debughandle = 0;
u64 buttonClickSleepTime = 50;

void attach()
{
    u64 pid = 0;
    Result rc = pmdmntGetApplicationProcessId(&pid);
    if (R_FAILED(rc) && debugResultCodes)
        printf("pmdmntGetApplicationProcessId: %d\n", rc);

    if (debughandle != 0)
        svcCloseHandle(debughandle);

    rc = svcDebugActiveProcess(&debughandle, pid);
    if (R_FAILED(rc) && debugResultCodes)
        printf("svcDebugActiveProcess: %d\n", rc);
}

void detach(){
    if (debughandle != 0)
        svcCloseHandle(debughandle);
}

u64 getMainNsoBase(u64 pid){
    LoaderModuleInfo proc_modules[2];
    s32 numModules = 0;
    Result rc = ldrDmntGetProcessModuleInfo(pid, proc_modules, 2, &numModules);
    if (R_FAILED(rc) && debugResultCodes)
        printf("ldrDmntGetProcessModuleInfo: %d\n", rc);

    LoaderModuleInfo *proc_module = 0;
    if(numModules == 2){
        proc_module = &proc_modules[1];
    }else{
        proc_module = &proc_modules[0];
    }
    return proc_module->base_address;
}

u64 getHeapBase(Handle handle){
    MemoryInfo meminfo;
    memset(&meminfo, 0, sizeof(MemoryInfo));
    u64 heap_base = 0;
    u64 lastaddr = 0;
    do
    {
        lastaddr = meminfo.addr;
        u32 pageinfo;
        svcQueryDebugProcessMemory(&meminfo, &pageinfo, handle, meminfo.addr + meminfo.size);
        if((meminfo.type & MemType_Heap) == MemType_Heap){
            heap_base = meminfo.addr;
            break;
        }
    } while (lastaddr < meminfo.addr + meminfo.size);

    return heap_base;
}

u64 getTitleId(u64 pid){
    u64 titleId = 0;
    Result rc = pminfoGetProgramId(&titleId, pid);
    if (R_FAILED(rc) && debugResultCodes)
        printf("pminfoGetProgramId: %d\n", rc);
    return titleId;
}

void getBuildID(MetaData* meta, u64 pid){
    LoaderModuleInfo proc_modules[2];
    s32 numModules = 0;
    Result rc = ldrDmntGetProcessModuleInfo(pid, proc_modules, 2, &numModules);
    if (R_FAILED(rc) && debugResultCodes)
        printf("ldrDmntGetProcessModuleInfo: %d\n", rc);

    LoaderModuleInfo *proc_module = 0;
    if(numModules == 2){
        proc_module = &proc_modules[1];
    }else{
        proc_module = &proc_modules[0];
    }
    memcpy(meta->buildID, proc_module->build_id, 0x20);
}

MetaData getMetaData(){
    MetaData meta;
    attach();
    u64 pid = 0;    
    Result rc = pmdmntGetApplicationProcessId(&pid);
    if (R_FAILED(rc) && debugResultCodes)
        printf("pmdmntGetApplicationProcessId: %d\n", rc);
    
    meta.main_nso_base = getMainNsoBase(pid);
    meta.heap_base =  getHeapBase(debughandle);
    meta.titleID = getTitleId(pid);
    getBuildID(&meta, pid);

    detach();
    return meta;
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
    controllerDevice.npadInterfaceType = HidNpadInterfaceType_Bluetooth;
    // Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
    controllerDevice.singleColorBody = RGBA8_MAXALPHA(255,255,255);
    controllerDevice.singleColorButtons = RGBA8_MAXALPHA(0,0,0);
    controllerDevice.colorLeftGrip = RGBA8_MAXALPHA(230,255,0);
    controllerDevice.colorRightGrip = RGBA8_MAXALPHA(0,40,20);

    // Setup example controller state.
    controllerState.battery_level = 4; // Set battery charge to full.
    controllerState.analog_stick_l.x = 0x0;
    controllerState.analog_stick_l.y = -0x0;
    controllerState.analog_stick_r.x = 0x0;
    controllerState.analog_stick_r.y = -0x0;
    rc = hiddbgAttachHdlsWorkBuffer();
    if (R_FAILED(rc) && debugResultCodes)
        printf("hiddbgAttachHdlsWorkBuffer: %d\n", rc);
    rc = hiddbgAttachHdlsVirtualDevice(&cHandle, &controllerDevice);
    if (R_FAILED(rc) && debugResultCodes)
        printf("hiddbgAttachHdlsVirtualDevice: %d\n", rc);
    bControllerIsInitialised = true;
}


void poke(u64 offset, u64 size, u8* val)
{
    attach();
    writeMem(offset, size, val);
    detach();
}

void writeMem(u64 offset, u64 size, u8* val)
{
	Result rc = svcWriteDebugProcessMemory(debughandle, val, offset, size);
    if (R_FAILED(rc) && debugResultCodes)
        printf("svcWriteDebugProcessMemory: %d\n", rc);
}

void peek(u64 offset, u64 size)
{
    u8 *out = malloc(sizeof(u8) * size);
    attach();
    readMem(out, offset, size);
    detach();

    u64 i;
    for (i = 0; i < size; i++)
    {
        printf("%02X", out[i]);
    }
    printf("\n");
    free(out);
}

void readMem(u8* out, u64 offset, u64 size)
{
	Result rc = svcReadDebugProcessMemory(out, debughandle, offset, size);
    if (R_FAILED(rc) && debugResultCodes)
        printf("svcReadDebugProcessMemory: %d\n", rc);
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
    hiddbgSetHdlsState(cHandle, &controllerState);
}

void release(HidControllerKeys btn)
{
    initController();
    controllerState.buttons &= ~btn;
    hiddbgSetHdlsState(cHandle, &controllerState);
}

void setStickState(int side, int dxVal, int dyVal)
{
    initController();
	if (side == JOYSTICK_LEFT)
	{
		controllerState.analog_stick_l.x = dxVal;
		controllerState.analog_stick_l.y = dyVal;
	}
	else
	{
		controllerState.analog_stick_r.x = dxVal;
		controllerState.analog_stick_r.y = dyVal;
	}
    hiddbgSetHdlsState(cHandle, &controllerState);
}

void reverseArray(u8* arr, int start, int end)
{
    int temp;
    while (start < end)
    {
        temp = arr[start];   
        arr[start] = arr[end];
        arr[end] = temp;
        start++;
        end--;
    }   
} 

u64 followMainPointer(u64* jumps, size_t count) 
{
	u64 offset;
    u64 size = sizeof offset;
	u8 *out = malloc(size);
	MetaData meta = getMetaData(); // double attach but let's deal with micro-optimizations later
	
	attach();
	readMem(out, meta.main_nso_base + jumps[0], size);
	offset = *(u64*)out;
	int i;
    for (i = 1; i < count; ++i)
	{
		readMem(out, jumps[i] + offset, size);
		offset = *(u64*)out;
	}
	detach();
	free(out);
	
    return offset;
}

