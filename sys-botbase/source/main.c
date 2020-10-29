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
#include <poll.h>

#define TITLE_ID 0x430000000000000B
#define HEAP_SIZE 0x000540000

// we aren't an applet
u32 __nx_applet_type = AppletType_None;

// setup a fake heap (we don't need the heap anyway)
char fake_heap[HEAP_SIZE];

// we override libnx internals to do a minimal init
void __libnx_initheap(void)
{
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    // setup newlib fake heap
    fake_heap_start = fake_heap;
    fake_heap_end = fake_heap + HEAP_SIZE;
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
bool debugResultCodes = false;

bool echoCommands = false;

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
        if(dxVal > JOYSTICK_MAX) dxVal = JOYSTICK_MAX;
        if(dxVal < JOYSTICK_MIN) dxVal = JOYSTICK_MIN;

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
    }

    if(!strcmp(argv[0], "getTitleID")){
        MetaData meta = getMetaData();
        printf("%16lX\n", meta.titleID);
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
        printf("%16lX\n", meta.main_nso_base);
    }
    
    if(!strcmp(argv[0], "getBuildID")){
        MetaData meta = getMetaData();
        printf("%02x%02x%02x%02x%02x%02x%02x%02x\n", meta.buildID[0], meta.buildID[1], meta.buildID[2], meta.buildID[3], meta.buildID[4], meta.buildID[5], meta.buildID[6], meta.buildID[7]);

    }

    if(!strcmp(argv[0], "getHeapBase")){
        MetaData meta = getMetaData();
        printf("%16lX\n", meta.heap_base);
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
        printf("1.6\n");
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
    while (appletMainLoop())
    {
        poll(pfds, fd_count, -1);
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
        svcSleepThread(mainLoopSleepTime * 1e+6L);
    }

    return 0;
}
