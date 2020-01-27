#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include <switch.h>
#include "commands.h"
#include "args.h"
#include "util.h"

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
        fatalLater(rc);
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
        fatalLater(rc);
    rc = fsdevMountSdmc();
    if (R_FAILED(rc))
        fatalLater(rc);
    rc = timeInitialize();
    if (R_FAILED(rc))
        fatalLater(rc);
    rc = socketInitializeDefault();
    if (R_FAILED(rc))
        fatalLater(rc);
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

int sock = -1;

int argmain(int argc, char **argv)
{
    if (argc == 0)
        return 0;


    //peek <address in hex or dec> <amount of bytes in hex or dec>
    if (!strcmp(argv[0], "peek"))
    {
        if(argc != 3)
            return 0;

        attach();
        u64 addr = getHeap() + parseStringToInt(argv[1]);
        u64 size = parseStringToInt(argv[2]);
        peek(addr, size);
        detach();
    }

    //poke <address in hex or dec> <amount of bytes in hex or dec> <data in hex or dec>
    if (!strcmp(argv[0], "poke"))
    {
        if(argc != 3)
            return 0;
        attach();
        u64 addr = getHeap() + parseStringToInt(argv[1]);
        u64 size = 0;
        u8* data = parseStringToByteBuffer(argv[2], &size);
        poke(addr, size, data);
        detach();
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

    if(!strcmp(argv[0], "detachController"))
    {
        hiddbgDetachHdlsVirtualDevice(controllerHandle);
        hiddbgReleaseHdlsWorkBuffer();
        hiddbgExit();
        bControllerIsInitialised = false;
    }

    return 0;
}

int main()
{

    char *linebuf = malloc(sizeof(char) * MAX_LINE_LENGTH);

    int c = sizeof(struct sockaddr_in);
    struct sockaddr_in client;

    while (appletMainLoop())
    {
        int listenfd = setupServerSocket();
        sock = accept(listenfd, (struct sockaddr *)&client, (socklen_t *)&c);
        if (sock <= 0)
        {
            // Accepting fails after sleep for some reason.
            svcSleepThread(1e+9L);
            close(listenfd);
            continue;
        }

        fflush(stdout);
        dup2(sock, STDOUT_FILENO);
        fflush(stderr);
        dup2(sock, STDERR_FILENO);

        while (1)
        {
            int len = recv(sock, linebuf, MAX_LINE_LENGTH, 0);
            if (len < 1)
            {
                break;
            }

            linebuf[len - 1] = 0;
            parseArgs(linebuf, &argmain);

            svcSleepThread(1e+8L);
        }
        close(listenfd);
        detach();
    }

    if (debughandle != 0)
        svcCloseHandle(debughandle);

    return 0;
}
