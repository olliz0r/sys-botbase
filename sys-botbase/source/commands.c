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
HidDeviceType controllerInitializedType = HidDeviceType_FullKey3;
HiddbgHdlsHandle controllerHandle = {0};
HiddbgHdlsDeviceInfo controllerDevice = {0};
HiddbgHdlsState controllerState = {0};

//Keyboard:
HiddbgKeyboardAutoPilotState dummyKeyboardState = {0};

Handle debughandle = 0;
u64 buttonClickSleepTime = 50;
u64 keyPressSleepTime = 25;
u64 pollRate = 17; // polling is linked to screen refresh rate (system UI) or game framerate. Most cases this is 1/60 or 1/30
u32 fingerDiameter = 50;
HiddbgHdlsSessionId sessionId = {0};
bool initflag=0;
u8 *workmem = NULL;
size_t workmem_size = 0x1000;

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
    u64 heap_base = 0;
    Result rc = svcGetInfo(&heap_base, InfoType_HeapRegionAddress, debughandle, 0);
    if (R_FAILED(rc) && debugResultCodes)
        printf("svcGetInfo: %d\n", rc);

    return heap_base;
}

u64 getTitleId(u64 pid){
    u64 titleId = 0;
    Result rc = pminfoGetProgramId(&titleId, pid);
    if (R_FAILED(rc) && debugResultCodes)
        printf("pminfoGetProgramId: %d\n", rc);
    return titleId;
}

u64 GetTitleVersion(u64 pid){
    u64 titleV = 0;
    s32 out;

    Result rc = nsInitialize();
	if (R_FAILED(rc)) 
        fatalThrow(rc);

    NsApplicationContentMetaStatus *MetaStatus = malloc(sizeof(NsApplicationContentMetaStatus[100U]));
    rc = nsListApplicationContentMetaStatus(getTitleId(pid), 0, MetaStatus, 100, &out);
    if (R_FAILED(rc) && debugResultCodes)
        printf("nsListApplicationContentMetaStatus: %d\n", rc);
    for (int i = 0; i < out; i++) {
        if (titleV < MetaStatus[i].version) titleV = MetaStatus[i].version;
    }

    free(MetaStatus);
    nsExit();

    return (titleV / 0x10000);
}

u64 getoutsize(NsApplicationControlData* buf){
    Result rc = nsInitialize();
    if (R_FAILED(rc))
        fatalThrow(rc);
    u64 outsize = 0;
    u64 pid = 0;
    pmdmntGetApplicationProcessId(&pid);
    rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, getTitleId(pid), buf, sizeof(NsApplicationControlData), &outsize);
    if (R_FAILED(rc)) {
        printf("nsGetApplicationControlData() failed: 0x%x\n", rc);
    }
    nsExit();
    return outsize;
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
    meta.titleVersion = GetTitleVersion(pid);
    getBuildID(&meta, pid);

    detach();
    return meta;
}

bool getIsProgramOpen(u64 id)
{
    u64 pid = 0;    
    Result rc = pmdmntGetProcessId(&pid, id);
    if (pid == 0 || R_FAILED(rc))
        return false;

    return true;
}

void initController()
{
    if(bControllerIsInitialised) return;
    //taken from switchexamples github
    Result rc = hiddbgInitialize();
	
	//old
    //if (R_FAILED(rc) && debugResultCodes)
    //printf("hiddbgInitialize: %d\n", rc);
    
	//new
	if (R_FAILED(rc) && debugResultCodes) {
        printf("hiddbgInitialize(): 0x%x\n", rc);
    }
    else {
        workmem = aligned_alloc(0x1000, workmem_size);
        if (workmem) initflag = 1;
        else printf("workmem alloc failed\n");
    }    
	
	// Set the controller type to Pro-Controller, and set the npadInterfaceType.
    controllerDevice.deviceType = controllerInitializedType;
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

    rc = hiddbgAttachHdlsWorkBuffer(&sessionId, workmem, workmem_size);
    if (R_FAILED(rc) && debugResultCodes)
        printf("hiddbgAttachHdlsWorkBuffer: %d\n", rc);
    rc = hiddbgAttachHdlsVirtualDevice(&controllerHandle, &controllerDevice);
    if (R_FAILED(rc) && debugResultCodes)
        printf("hiddbgAttachHdlsVirtualDevice: %d\n", rc);
    //init a dummy keyboard state for assignment between keypresses
    dummyKeyboardState.keys[3] = 0x800000000000000UL; // Hackfix found by Red: an unused key press (KBD_MEDIA_CALC) is required to allow sequential same-key presses. bitfield[3]
    bControllerIsInitialised = true;
}

void detachController()
{
    initController();

    Result rc = hiddbgDetachHdlsVirtualDevice(controllerHandle);
    if (R_FAILED(rc) && debugResultCodes)
        printf("hiddbgDetachHdlsVirtualDevice: %d\n", rc);
    rc = hiddbgReleaseHdlsWorkBuffer(sessionId);
    if (R_FAILED(rc) && debugResultCodes)
        printf("hiddbgReleaseHdlsWorkBuffer: %d\n", rc);
    hiddbgExit();
	free(workmem);
    bControllerIsInitialised = false;

    sessionId.id = 0;
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

void peekInfinite(u64 offset, u64 size)
{
    u64 sizeRemainder = size;
    u64 totalFetched = 0;
    u64 i;
    u8 *out = malloc(sizeof(u8) * MAX_LINE_LENGTH);

    attach();
    while (sizeRemainder > 0)
    {
        u64 thisBuffersize = sizeRemainder > MAX_LINE_LENGTH ? MAX_LINE_LENGTH : sizeRemainder;
        sizeRemainder -= thisBuffersize;
        readMem(out, offset + totalFetched, thisBuffersize);
        for (i = 0; i < thisBuffersize; i++)
        {
            printf("%02X", out[i]);
        }

        totalFetched += thisBuffersize;
    }
    detach();
    printf("\n");
    free(out);
}

void peekMulti(u64* offset, u64* size, u64 count)
{
    u64 totalSize = 0;
    for (int i = 0; i < count; i++)
        totalSize += size[i];
    
    u8 *out = malloc(sizeof(u8) * totalSize);
    u64 ofs = 0;
    attach();
    for (int i = 0; i < count; i++)
    {
        readMem(out + ofs, offset[i], size[i]);
        ofs += size[i];
    }
    detach();

    u64 i;
    for (i = 0; i < totalSize; i++)
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

void click(HidNpadButton btn)
{
    initController();
    press(btn);
    svcSleepThread(buttonClickSleepTime * 1e+6L);
    release(btn);
}

void press(HidNpadButton btn)
{
    initController();
    controllerState.buttons |= btn;
    Result rc = hiddbgSetHdlsState(controllerHandle, &controllerState);
    if (R_FAILED(rc) && debugResultCodes)
        printf("hiddbgSetHdlsState: %d\n", rc);
}

void release(HidNpadButton btn)
{
    initController();
    controllerState.buttons &= ~btn;
    Result rc = hiddbgSetHdlsState(controllerHandle, &controllerState);
    if (R_FAILED(rc) && debugResultCodes)
        printf("hiddbgSetHdlsState: %d\n", rc);
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
    hiddbgSetHdlsState(controllerHandle, &controllerState);
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

u64 followMainPointer(s64* jumps, size_t count) 
{
	u64 offset;
    u64 size = sizeof offset;
	u8 *out = malloc(size);
	MetaData meta = getMetaData(); 
	
	attach();
	readMem(out, meta.main_nso_base + jumps[0], size);
	offset = *(u64*)out;
	int i;
    for (i = 1; i < count; ++i)
	{
		readMem(out, offset + jumps[i], size);
		offset = *(u64*)out;
        // this traversal resulted in an error
        if (offset == 0)
            break;
	}
	detach();
	free(out);
	
    return offset;
}

void touch(HidTouchState* state, u64 sequentialCount, u64 holdTime, bool hold, u8* token)
{
    initController();
    state->delta_time = holdTime; // only the first touch needs this for whatever reason
    for (u32 i = 0; i < sequentialCount; i++)
    {
        hiddbgSetTouchScreenAutoPilotState(&state[i], 1);
        svcSleepThread(holdTime);
        if (!hold)
        {
            hiddbgSetTouchScreenAutoPilotState(NULL, 0);
            svcSleepThread(pollRate * 1e+6L);
        }

        if ((*token) == 1)
            break;
    }

    if(hold) // send finger release event
    {
        hiddbgSetTouchScreenAutoPilotState(NULL, 0);
        svcSleepThread(pollRate * 1e+6L);
    }
    
    hiddbgUnsetTouchScreenAutoPilotState();
}

void key(HiddbgKeyboardAutoPilotState* states, u64 sequentialCount)
{
    initController();
    HiddbgKeyboardAutoPilotState tempState = {0};
    u32 i;
    for (i = 0; i < sequentialCount; i++)
    {
        memcpy(&tempState.keys, states[i].keys, sizeof(u64) * 4);
        tempState.modifiers = states[i].modifiers;
        hiddbgSetKeyboardAutoPilotState(&tempState);
        svcSleepThread(keyPressSleepTime * 1e+6L);

        if (i != (sequentialCount-1))
        {
            if (memcmp(states[i].keys, states[i+1].keys, sizeof(u64) * 4) == 0 && states[i].modifiers == states[i+1].modifiers)
            {
                hiddbgSetKeyboardAutoPilotState(&dummyKeyboardState);
                svcSleepThread(pollRate * 1e+6L);
            }
        }
        else
        {
            hiddbgSetKeyboardAutoPilotState(&dummyKeyboardState);
            svcSleepThread(pollRate * 1e+6L);
        }
    }

    hiddbgUnsetKeyboardAutoPilotState();
}

void clickSequence(char* seq, u8* token)
{
    const char delim = ','; // used for chars and sticks
    const char startWait = 'W';
    const char startPress = '+';
    const char startRelease = '-';
    const char startLStick = '%';
    const char startRStick = '&';
    char* command = strtok(seq, &delim);
    HidNpadButton currKey = {0};
    u64 currentWait = 0;

    initController();
    while (command != NULL)
    {
        if ((*token) == 1)
            break;

        if (!strncmp(command, &startLStick, 1))
        {
            // l stick
            s64 x = parseStringToSignedLong(&command[1]);
            if(x > JOYSTICK_MAX) x = JOYSTICK_MAX; 
            if(x < JOYSTICK_MIN) x = JOYSTICK_MIN; 
            s64 y = 0;
            command = strtok(NULL, &delim);
            if (command != NULL)
                y = parseStringToSignedLong(command);
            if(y > JOYSTICK_MAX) y = JOYSTICK_MAX;
            if(y < JOYSTICK_MIN) y = JOYSTICK_MIN;
            setStickState(JOYSTICK_LEFT, (s32)x, (s32)y);
        }
        else if (!strncmp(command, &startRStick, 1))
        {
            // r stick
            s64 x = parseStringToSignedLong(&command[1]);
            if(x > JOYSTICK_MAX) x = JOYSTICK_MAX; 
            if(x < JOYSTICK_MIN) x = JOYSTICK_MIN; 
            s64 y = 0;
            command = strtok(NULL, &delim);
            if (command != NULL)
                y = parseStringToSignedLong(command);
            if(y > JOYSTICK_MAX) y = JOYSTICK_MAX;
            if(y < JOYSTICK_MIN) y = JOYSTICK_MIN;
            setStickState(JOYSTICK_RIGHT, (s32)x, (s32)y);
        }
        else if (!strncmp(command, &startPress, 1))
        {
            // press
            currKey = parseStringToButton(&command[1]);
            press(currKey);
        }  
        else if (!strncmp(command, &startRelease, 1))
        {
            // release
            currKey = parseStringToButton(&command[1]);
            release(currKey);
        }   
        else if (!strncmp(command, &startWait, 1))
        {
            // wait
            currentWait = parseStringToInt(&command[1]);
            svcSleepThread(currentWait * 1e+6l);
        }
        else
        {
            // click
            currKey = parseStringToButton(command);
            press(currKey);
            svcSleepThread(buttonClickSleepTime * 1e+6L);
            release(currKey);
        }

        command = strtok(NULL, &delim);
    }
}
