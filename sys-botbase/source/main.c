#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include <switch.h>
#include "commands.h"
#include "args.h"
#include "util.h"
#include "freeze.h"
#include <poll.h>

#define TITLE_ID 0x430000000000000B
#define HEAP_SIZE 0x001000000
#define THREAD_SIZE 0x20000

Thread freezeThread, touchThread, keyboardThread;

// prototype thread functions to give the illusion of cleanliness
void sub_freeze(void *arg);
void sub_touch(void *arg);
void sub_key(void *arg);

// locks for thread
Mutex freezeMutex, touchMutex, keyMutex;

// events for releasing or idling threads
u8 freeze_thr_state = 0; 
// key and touch events currently being processed
KeyData currentKeyEvent = {0};
TouchData currentTouchEvent = {0};

// we aren't an applet
u32 __nx_applet_type = AppletType_None;

// we override libnx internals to do a minimal init
void __libnx_initheap(void)
{
	static u8 inner_heap[HEAP_SIZE];
    extern void* fake_heap_start;
    extern void* fake_heap_end;

    // Configure the newlib heap.
    fake_heap_start = inner_heap;
    fake_heap_end   = inner_heap + sizeof(inner_heap);
}

void __appInit(void)
{
    Result rc;
    svcSleepThread(20000000000L);
    rc = smInitialize();
    if (R_FAILED(rc))
        fatalThrow(rc);
    if (hosversionGet() == 0) {
        rc = setsysInitialize();
        if (R_SUCCEEDED(rc)) {
            SetSysFirmwareVersion fw;
            rc = setsysGetFirmwareVersion(&fw);
            if (R_SUCCEEDED(rc))
                hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
            setsysExit();
        }
    }
    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalThrow(rc);
    rc = fsdevMountSdmc();
    if (R_FAILED(rc))
        fatalThrow(rc);
    rc = timeInitialize();
    if (R_FAILED(rc))
        fatalThrow(rc);
    rc = pmdmntInitialize();
	if (R_FAILED(rc)) {
        fatalThrow(rc);
	}
    rc = ldrDmntInitialize();
	if (R_FAILED(rc)) {
		fatalThrow(rc);
	}
    rc = pminfoInitialize();
	if (R_FAILED(rc)) {
		fatalThrow(rc);
	}
    rc = socketInitializeDefault();
    if (R_FAILED(rc))
        fatalThrow(rc);

    rc = capsscInitialize();
    if (R_FAILED(rc))
        fatalThrow(rc);
}

void __appExit(void)
{
    fsdevUnmountAll();
    fsExit();
    smExit();
    audoutExit();
    timeExit();
    socketExit();
}

u64 mainLoopSleepTime = 50;
u64 freezeRate = 3;
bool debugResultCodes = false;

bool echoCommands = false;

void makeTouch(HidTouchState* state, u64 sequentialCount, u64 holdTime, bool hold)
{
    mutexLock(&touchMutex);
    memset(&currentTouchEvent, 0, sizeof currentTouchEvent);
    currentTouchEvent.states = state;
    currentTouchEvent.sequentialCount = sequentialCount;
    currentTouchEvent.holdTime = holdTime;
    currentTouchEvent.hold = hold;
    currentTouchEvent.state = 1;
    mutexUnlock(&touchMutex);
}

void makeKeys(HiddbgKeyboardAutoPilotState* states, u64 sequentialCount)
{
    mutexLock(&keyMutex);
    memset(&currentKeyEvent, 0, sizeof currentKeyEvent);
    currentKeyEvent.states = states;
    currentKeyEvent.sequentialCount = sequentialCount;
    currentKeyEvent.state = 1;
    mutexUnlock(&keyMutex);
}

int argmain(int argc, char **argv)
{
    if (argc == 0)
        return 0;


    //peek <address in hex or dec> <amount of bytes in hex or dec>
    if (!strcmp(argv[0], "peek"))
    {
        if(argc != 3)
            return 0;

        MetaData meta = getMetaData();

        u64 offset = parseStringToInt(argv[1]);
        u64 size = parseStringToInt(argv[2]);
        peek(meta.heap_base + offset, size);
    }

    if (!strcmp(argv[0], "peekAbsolute"))
    {
        if(argc != 3)
            return 0;

        u64 offset = parseStringToInt(argv[1]);
        u64 size = parseStringToInt(argv[2]);
        peek(offset, size);
    }

    if (!strcmp(argv[0], "peekMain"))
    {
        if(argc != 3)
            return 0;

        MetaData meta = getMetaData();

        u64 offset = parseStringToInt(argv[1]);
        u64 size = parseStringToInt(argv[2]);
        peek(meta.main_nso_base + offset, size);
    }

    //poke <address in hex or dec> <amount of bytes in hex or dec> <data in hex or dec>
    if (!strcmp(argv[0], "poke"))
    {
        if(argc != 3)
            return 0;
            
        MetaData meta = getMetaData();

        u64 offset = parseStringToInt(argv[1]);
        u64 size = 0;
        u8* data = parseStringToByteBuffer(argv[2], &size);
        poke(meta.heap_base + offset, size, data);
        free(data);
    } 
    
    if (!strcmp(argv[0], "pokeAbsolute"))
    {
        if(argc != 3)
            return 0;

        u64 offset = parseStringToInt(argv[1]);
        u64 size = 0;
        u8* data = parseStringToByteBuffer(argv[2], &size);
        poke(offset, size, data);
        free(data);
    }
        
    if (!strcmp(argv[0], "pokeMain"))
    {
        if(argc != 3)
            return 0;
            
        MetaData meta = getMetaData();

        u64 offset = parseStringToInt(argv[1]);
        u64 size = 0;
        u8* data = parseStringToByteBuffer(argv[2], &size);
        poke(meta.main_nso_base + offset, size, data);
        free(data);
    } 

    //click <buttontype>
    if (!strcmp(argv[0], "click"))
    {
        if(argc != 2)
            return 0;
        HidControllerKeys key = parseStringToButton(argv[1]);
        click(key);
    }

    //hold <buttontype>
    if (!strcmp(argv[0], "press"))
    {
        if(argc != 2)
            return 0;
        HidControllerKeys key = parseStringToButton(argv[1]);
        press(key);
    }

    //release <buttontype>
    if (!strcmp(argv[0], "release"))
    {
        if(argc != 2)
            return 0;
        HidControllerKeys key = parseStringToButton(argv[1]);
        release(key);
    }

    //setStick <left or right stick> <x value> <y value>
    if (!strcmp(argv[0], "setStick"))
    {
        if(argc != 4)
            return 0;
        
        int side = 0;
        if(!strcmp(argv[1], "LEFT")){
            side = JOYSTICK_LEFT;
        }else if(!strcmp(argv[1], "RIGHT")){
            side = JOYSTICK_RIGHT;
        }else{
            return 0;
        }

        int dxVal = strtol(argv[2], NULL, 0);
        if(dxVal > JOYSTICK_MAX) dxVal = JOYSTICK_MAX; //0x7FFF
        if(dxVal < JOYSTICK_MIN) dxVal = JOYSTICK_MIN; //-0x8000
        int dyVal = strtol(argv[3], NULL, 0);
        if(dyVal > JOYSTICK_MAX) dyVal = JOYSTICK_MAX;
        if(dyVal < JOYSTICK_MIN) dyVal = JOYSTICK_MIN;

        setStickState(side, dxVal, dyVal);
    }

    //detachController
    if(!strcmp(argv[0], "detachController"))
    {
        Result rc = hiddbgDetachHdlsVirtualDevice(controllerHandle);
        if (R_FAILED(rc) && debugResultCodes)
            printf("hiddbgDetachHdlsVirtualDevice: %d\n", rc);
        rc = hiddbgReleaseHdlsWorkBuffer();
        if (R_FAILED(rc) && debugResultCodes)
            printf("hiddbgReleaseHdlsWorkBuffer: %d\n", rc);
        hiddbgExit();
        bControllerIsInitialised = false;
    }

    //configure <mainLoopSleepTime or buttonClickSleepTime> <time in ms>
    if(!strcmp(argv[0], "configure")){
        if(argc != 3)
            return 0;

        if(!strcmp(argv[1], "mainLoopSleepTime")){
            u64 time = parseStringToInt(argv[2]);
            mainLoopSleepTime = time;
        }

        if(!strcmp(argv[1], "buttonClickSleepTime")){
            u64 time = parseStringToInt(argv[2]);
            buttonClickSleepTime = time;
        }

        if(!strcmp(argv[1], "echoCommands")){
            u64 shouldActivate = parseStringToInt(argv[2]);
            echoCommands = shouldActivate != 0;
        }

        if(!strcmp(argv[1], "printDebugResultCodes")){
            u64 shouldActivate = parseStringToInt(argv[2]);
            debugResultCodes = shouldActivate != 0;
        }
        
        if(!strcmp(argv[1], "keySleepTime")){
            u64 keyTime = parseStringToInt(argv[2]);
            keyPressSleepTime = keyTime;
        }

        if(!strcmp(argv[1], "fingerDiameter")){
            u32 fDiameter = (u32) parseStringToInt(argv[2]);
            fingerDiameter = fDiameter;
        }

        if(!strcmp(argv[1], "pollRate")){
            u64 fPollRate = parseStringToInt(argv[2]);
            pollRate = fPollRate;
        }

        if(!strcmp(argv[1], "freezeRate")){
            u64 fFreezeRate = parseStringToInt(argv[2]);
            freezeRate = fFreezeRate;
        }
    }

    if(!strcmp(argv[0], "getTitleID")){
        MetaData meta = getMetaData();
        printf("%016lX\n", meta.titleID);
    }

    if(!strcmp(argv[0], "getSystemLanguage")){
        //thanks zaksa
        setInitialize();
        u64 languageCode = 0;   
        SetLanguage language = SetLanguage_ENUS;
        setGetSystemLanguage(&languageCode);   
        setMakeLanguage(languageCode, &language);
        printf("%d\n", language);
    }
 
    if(!strcmp(argv[0], "getMainNsoBase")){
        MetaData meta = getMetaData();
        printf("%016lX\n", meta.main_nso_base);
    }
    
    if(!strcmp(argv[0], "getBuildID")){
        MetaData meta = getMetaData();
        printf("%02x%02x%02x%02x%02x%02x%02x%02x\n", meta.buildID[0], meta.buildID[1], meta.buildID[2], meta.buildID[3], meta.buildID[4], meta.buildID[5], meta.buildID[6], meta.buildID[7]);

    }

    if(!strcmp(argv[0], "getHeapBase")){
        MetaData meta = getMetaData();
        printf("%016lX\n", meta.heap_base);
    }

    if(!strcmp(argv[0], "pixelPeek")){
        //errors with 0x668CE, unless debugunit flag is patched
        u64 bSize = 0x7D000;
        char* buf = malloc(bSize); 
        u64 outSize = 0;

        Result rc = capsscCaptureForDebug(buf, bSize, &outSize);

        if (R_FAILED(rc) && debugResultCodes)
            printf("capssc, 1204: %d\n", rc);
        
        u64 i;
        for (i = 0; i < outSize; i++)
        {
            printf("%02X", buf[i]);
        }
        printf("\n");

        free(buf);
    }

    if(!strcmp(argv[0], "getVersion")){
        printf("1.7\n");
    }
	
	// follow pointers and print absolute offset (little endian, flip it yourself if required)
	// pointer <first (main) jump> <additional jumps> !!do not add the last jump in pointerexpr here, add it yourself!!
	if (!strcmp(argv[0], "pointer"))
	{
		if(argc < 2)
            return 0;
		s64 jumps[argc-1];
		for (int i = 1; i < argc; i++)
			jumps[i-1] = parseStringToSignedLong(argv[i]);
		u64 solved = followMainPointer(jumps, argc-1);
		printf("%016lX\n", solved);
	}

    // pointerPeek <read size> <first (main) jump> <additional jumps> <final jump in pointerexpr>
    if (!strcmp(argv[0], "pointerPeek"))
	{
		if(argc < 3)
            return 0;
            
        u64 finalJump = parseStringToSignedLong(argv[argc-1]);
		u64 size = parseStringToInt(argv[1]);
        u64 count = argc - 3;
		s64 jumps[count];
		for (int i = 2; i < argc-1; i++)
			jumps[i-2] = parseStringToSignedLong(argv[i]);
		u64 solved = followMainPointer(jumps, count);
        solved += finalJump;
        peek(solved, size);
	}
	
	// add to freeze map
	if (!strcmp(argv[0], "freeze"))
    {
        if(argc != 3)
            return 0;
		
		MetaData meta = getMetaData();
		
        u64 offset = parseStringToInt(argv[1]);
        u64 size = 0;
        u8* data = parseStringToByteBuffer(argv[2], &size);
        addToFreezeMap(offset, data, size, meta.titleID);
    }
	
	// remove from freeze map
	if (!strcmp(argv[0], "unFreeze"))
    {
        if(argc != 2)
            return 0;
		
        u64 offset = parseStringToInt(argv[1]);
        removeFromFreezeMap(offset);
    }
	
	// get count of offsets being frozen
	if (!strcmp(argv[0], "freezeCount"))
	{
		getFreezeCount(true);
	}
	
	// clear all freezes
	if (!strcmp(argv[0], "freezeClear"))
	{
		clearFreezes();
		freeze_thr_state = 2;
	}

    //touch followed by arrayof: <x in the range 0-1280> <y in the range 0-720>. Array is sequential taps, not different fingers. Functions in its own thread, but will not allow the call again while running. tapcount * pollRate * 2
    if (!strcmp(argv[0], "touch"))
	{
        if(argc < 3 || argc % 2 == 0)
            return 0;

        u32 count = (argc-1)/2;
		HidTouchState* state = calloc(count, sizeof(HidTouchState));
        u32 i, j = 0;
        for (i = 0; i < count; ++i)
        {
            state[i].diameter_x = state[i].diameter_y = fingerDiameter;
            state[i].x = (u32) parseStringToInt(argv[++j]);
            state[i].y = (u32) parseStringToInt(argv[++j]);
        }

        makeTouch(state, count, pollRate * 1e+6L, false);
	}

    //touchHold <x in the range 0-1280> <y in the range 0-720> <time in milliseconds (must be at least 15ms)>. Functions in its own thread, but will not allow the call again while running. pollRate + holdtime
    if(!strcmp(argv[0], "touchHold")){
        if(argc != 4)
            return 0;

        HidTouchState* state = calloc(1, sizeof(HidTouchState));
        state->diameter_x = state->diameter_y = fingerDiameter;
        state->x = (u32) parseStringToInt(argv[1]);
        state->y = (u32) parseStringToInt(argv[2]);
        u64 time = parseStringToInt(argv[3]);
        makeTouch(state, 1, time * 1e+6L, false);
    }

    //touchDraw followed by arrayof: <x in the range 0-1280> <y in the range 0-720>. Array is vectors of where finger moves to, then removes the finger. Functions in its own thread, but will not allow the call again while running. (vectorcount * pollRate * 2) + pollRate
    if (!strcmp(argv[0], "touchDraw"))
	{
        if(argc < 3 || argc % 2 == 0)
            return 0;

        u32 count = (argc-1)/2;
		HidTouchState* state = calloc(count, sizeof(HidTouchState));
        u32 i, j = 0;
        for (i = 0; i < count; ++i)
        {
            state[i].diameter_x = state[i].diameter_y = fingerDiameter;
            state[i].x = (u32) parseStringToInt(argv[++j]);
            state[i].y = (u32) parseStringToInt(argv[++j]);
        }

        makeTouch(state, count, pollRate * 1e+6L * 2, true);
	}

    //key followed by arrayof: <HidKeyboardKey> to be pressed in sequential order
    //thank you Red (hp3721) for this functionality
    if (!strcmp(argv[0], "key"))
	{
        if (argc < 2)
            return 0;
        
        u64 count = argc-1;
        HiddbgKeyboardAutoPilotState* keystates = calloc(count, sizeof (HiddbgKeyboardAutoPilotState));
        u64 i;
        for (i = 0; i < count; i++)
        {
            u8 key = (u8) parseStringToInt(argv[i+1]);
            if (key < 4 || key > 231)
                continue;
            keystates[i].keys[key / 64] = 1UL << key;
            keystates[i].modifiers = 1024UL; //numlock
        }

        makeKeys(keystates, count);
    }

    //keyMod followed by arrayof: <HidKeyboardKey> <HidKeyboardModifier>(without the bitfield shift) to be pressed in sequential order
    if (!strcmp(argv[0], "keyMod"))
	{
        if (argc < 3 || argc % 2 == 0)
            return 0;

        u32 count = (argc-1)/2;
        HiddbgKeyboardAutoPilotState* keystates = calloc(count, sizeof (HiddbgKeyboardAutoPilotState));
        u64 i, j = 0;
        for (i = 0; i < count; i++)
        {
            u8 key = (u8) parseStringToInt(argv[++j]);
            if (key < 4 || key > 231)
                continue;
            keystates[i].keys[key / 64] = 1UL << key;
            keystates[i].modifiers = BIT((u8) parseStringToInt(argv[++j]));
        }

        makeKeys(keystates, count);
    }

    //keyMulti followed by arrayof: <HidKeyboardKey> to be pressed at the same time.
    if (!strcmp(argv[0], "keyMulti"))
	{
        if (argc < 2)
            return 0;
        
        u64 count = argc-1;
        HiddbgKeyboardAutoPilotState* keystate = calloc(1, sizeof (HiddbgKeyboardAutoPilotState));
        u64 i;
        for (i = 0; i < count; i++)
        {
            u8 key = (u8) parseStringToInt(argv[i+1]);
            if (key < 4 || key > 231)
                continue;
            keystate[0].keys[key / 64] |= 1UL << key;
        }

        makeKeys(keystate, 1);
    }

    return 0;
}

void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    if (*fd_count == *fd_size) {
        *fd_size *= 2;

        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN;

    (*fd_count)++;
}

void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}

int main()
{
    char *linebuf = malloc(sizeof(char) * MAX_LINE_LENGTH);

    int c = sizeof(struct sockaddr_in);
    struct sockaddr_in client;

    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    int listenfd = setupServerSocket();
    pfds[0].fd = listenfd;
    pfds[0].events = POLLIN;
    fd_count = 1;

    int newfd;
	
	Result rc;
	int fr_count = 0;
	
    initFreezes();

	// freeze thread
	mutexInit(&freezeMutex);
	rc = threadCreate(&freezeThread, sub_freeze, (void*)&freeze_thr_state, NULL, THREAD_SIZE, 0x2C, -2); 
	if (R_SUCCEEDED(rc))
        rc = threadStart(&freezeThread);

    // touch thread
    mutexInit(&touchMutex);
    rc = threadCreate(&touchThread, sub_touch, (void*)&currentTouchEvent, NULL, THREAD_SIZE, 0x2C, -2); 
    if (R_SUCCEEDED(rc))
        rc = threadStart(&touchThread);

    // key thread
    mutexInit(&keyMutex);
    rc = threadCreate(&keyboardThread, sub_key, (void*)&currentKeyEvent, NULL, THREAD_SIZE, 0x2C, -2); 
    if (R_SUCCEEDED(rc))
        rc = threadStart(&keyboardThread);
    
	
    while (appletMainLoop())
    {
        poll(pfds, fd_count, -1);
		mutexLock(&freezeMutex);
        for(int i = 0; i < fd_count; i++) 
        {
            if (pfds[i].revents & POLLIN) 
            {
                if (pfds[i].fd == listenfd) 
                {
                    newfd = accept(listenfd, (struct sockaddr *)&client, (socklen_t *)&c);
                    if(newfd != -1)
                    {
                        add_to_pfds(&pfds, newfd, &fd_count, &fd_size);
                    }else{
                        svcSleepThread(1e+9L);
                        close(listenfd);
                        listenfd = setupServerSocket();
                        pfds[0].fd = listenfd;
                        pfds[0].events = POLLIN;
                        break;
                    }
                }
                else
                {
                    bool readEnd = false;
                    int readBytesSoFar = 0;
                    while(!readEnd){
                        int len = recv(pfds[i].fd, &linebuf[readBytesSoFar], 1, 0);
                        if(len <= 0)
                        {
                            close(pfds[i].fd);
                            del_from_pfds(pfds, i, &fd_count);
                            readEnd = true;
                        }
                        else
                        {
                            readBytesSoFar += len;
                            if(linebuf[readBytesSoFar-1] == '\n'){
                                readEnd = true;
                                linebuf[readBytesSoFar - 1] = 0;

                                fflush(stdout);
                                dup2(pfds[i].fd, STDOUT_FILENO);

                                parseArgs(linebuf, &argmain);

                                if(echoCommands){
                                    printf("%s\n",linebuf);
                                }
                            }
                        }
                    }
                }
            }
        }
		fr_count = getFreezeCount(false);
		if (fr_count == 0)
			freeze_thr_state = 2;
		mutexUnlock(&freezeMutex);
        svcSleepThread(mainLoopSleepTime * 1e+6L);
    }
	
	if (R_SUCCEEDED(rc))
    {
	    freeze_thr_state = 1;
		threadWaitForExit(&freezeThread);
        threadClose(&freezeThread);
        currentTouchEvent.state = 3;
        threadWaitForExit(&touchThread);
        threadClose(&touchThread);
        currentKeyEvent.state = 3;
        threadWaitForExit(&keyboardThread);
        threadClose(&keyboardThread);
	}
	
	clearFreezes();
    freeFreezes();
	
    return 0;
}

void sub_freeze(void *arg)
{
	u64 heap_base;
	u64 tid_now = 0;
	u64 pid = 0;
	bool wait_su = false;
	int freezecount = 0;
	
	IDLE:while (freezecount == 0)
	{
		if (*(u8*)arg == 1)
			break;
		
		// do nothing
		svcSleepThread(1e+9L);
		freezecount = getFreezeCount(false);
	}
	
	while (1)
	{
		if (*(u8*)arg == 1)
			break;
		
		if (*(u8*)arg == 2) // no freeze
		{
			mutexLock(&freezeMutex);
			freeze_thr_state = 0;
			mutexUnlock(&freezeMutex); // stupid but it works so is it really stupid? (yes)
			freezecount = 0;
			wait_su = false;
			goto IDLE;
		}
		
		mutexLock(&freezeMutex);
		attach();
		heap_base = getHeapBase(debughandle);
		pmdmntGetApplicationProcessId(&pid);
		tid_now = getTitleId(pid);
		detach();
		
		// don't freeze on startup of new tid to remove any chance of save corruption
		if (tid_now == 0)
		{
			mutexUnlock(&freezeMutex);
			svcSleepThread(1e+10L);
			wait_su = true;
			continue;
		}
		
		if (wait_su)
		{
			mutexUnlock(&freezeMutex);
			svcSleepThread(3e+10L);
			wait_su = false;
			mutexLock(&freezeMutex);
		}
		
		if (heap_base > 0)
		{
			attach();
			for (int j = 0; j < FREEZE_DIC_LENGTH; j++)
			{
				if (freezes[j].state == 1 && freezes[j].titleId == tid_now)
				{
					writeMem(heap_base + freezes[j].address, freezes[j].size, freezes[j].vData);
				}
			}
			detach();
		}
		
		mutexUnlock(&freezeMutex);
		svcSleepThread(freezeRate * 1e+6L);
		tid_now = 0;
		pid = 0;
	}
}

void sub_touch(void *arg)
{
    while (1)
    {
        TouchData* touchPtr = (TouchData*)arg;
        if (touchPtr->state == 1)
        {
            mutexLock(&touchMutex); // don't allow any more assignments to the touch var (will lock the main thread)
            touch(touchPtr->states, touchPtr->sequentialCount, touchPtr->holdTime, touchPtr->hold);
            free(touchPtr->states);
            touchPtr->state = 0;
            mutexUnlock(&touchMutex);
        }

        svcSleepThread(1e+6L);

        if (touchPtr->state == 3)
            break;
    }
}

void sub_key(void *arg)
{
    while (1)
    {
        KeyData* keyPtr = (KeyData*)arg;
        if (keyPtr->state == 1)
        {
            mutexLock(&keyMutex); 
            key(keyPtr->states, keyPtr->sequentialCount);
            free(keyPtr->states);
            keyPtr->state = 0;
            mutexUnlock(&keyMutex);
        }

        svcSleepThread(1e+6L);

        if (keyPtr->state == 3)
            break;
    }
}
