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

#define TITLE_ID 0x420000000000000F
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


    if (!strcmp(argv[0], "examplecommand"))
    {
        if(argc != 3)
            return 0;

        return 0;
    }

    return 0;
}

int main()
{
    int listenfd = setupServerSocket();

    char *linebuf = malloc(sizeof(char) * MAX_LINE_LENGTH);

    int c = sizeof(struct sockaddr_in);
    struct sockaddr_in client;

    mutexInit(&actionLock);

    while (appletMainLoop())
    {
        sock = accept(listenfd, (struct sockaddr *)&client, (socklen_t *)&c);
        if (sock <= 0)
        {
            // Accepting fails after sleep for some reason.
            svcSleepThread(1e+9L);
            close(listenfd);
            listenfd = setupServerSocket();
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

            mutexLock(&actionLock);
            if (attach())
            {
                mutexUnlock(&actionLock);
                continue;
            }

            parseArgs(linebuf, &argmain);

            detach();
            mutexUnlock(&actionLock);

            svcSleepThread(1e+8L);
        }
        detach();
    }

    if (debughandle != 0)
        svcCloseHandle(debughandle);

    return 0;
}
