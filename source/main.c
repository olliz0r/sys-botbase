#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include <switch.h>
#include "cheat.h"
#include "args.h"
#include "util.h"
#include "luahelper.h"

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

void registerFspLr()
{
    if (kernelAbove400())
        return;

    Result rc = fsprInitialize();
    if (R_FAILED(rc))
        fatalLater(rc);

    u64 pid;
    svcGetProcessId(&pid, CUR_PROCESS_HANDLE);

    rc = fsprRegisterProgram(pid, TITLE_ID, FsStorageId_NandSystem, NULL, 0, NULL, 0);
    if (R_FAILED(rc))
        fatalLater(rc);
    fsprExit();
}

void __appInit(void)
{
    Result rc;
    svcSleepThread(20000000000L);
    rc = smInitialize();
    if (R_FAILED(rc))
        fatalLater(rc);
    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalLater(rc);
    registerFspLr();
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

#define MAX_LINE_LENGTH 300

int sock = -1;

int argmain(int argc, char **argv)
{
    if (argc == 0)
        return 0;

    if (!strcmp(argv[0], "help"))
    {
        goto help;
    }

    if (!strcmp(argv[0], "ssearch"))
    {
        if (argc != 3)
            goto help;

        int searchType = VAL_NONE;
        for (int i = 1; i < VAL_END; i++)
        {
            if (!strcmp(argv[1], valtypes[i]))
                searchType = i;
        }

        if (searchType == VAL_NONE)
            goto help;

        u64 val = strtoull(argv[2], NULL, 10);

        printf("Starting search, this might take a while...\r\n");
        int res = startSearch(val, searchType, MemType_Heap);

        for (int i = 0; i < 100 && i < searchSize; i++)
            printf("Hit at %lx!\r\n", searchArr[i]);

        if (searchSize > 100)
            printf("...\r\n");

        if (res)
            printf("There are too many hits to process, try getting the variable to a number that's less 'common'\r\n");

        return 0;
    }

    if (!strcmp(argv[0], "csearch"))
    {
        if (argc != 2)
            goto help;
        if (search == VAL_NONE)
        {
            printf("You need to start a search first!");
            return 0;
        }

        u64 newVal = 0;

        newVal = strtoull(argv[1], NULL, 10);
        contSearch(newVal);

        for (int i = 0; i < 100 && i < searchSize; i++)
            printf("Hit at %lx!\r\n", searchArr[i]);

        if (searchSize > 100)
            printf("...\r\n");

        return 0;
    }

    if (!strcmp(argv[0], "poke"))
    {
        if (argc != 4)
            goto help;
        u64 addr = strtoull(argv[1], NULL, 16);
        u64 val = strtoull(argv[3], NULL, 10);

        int valType = VAL_NONE;
        for (int i = 1; i < VAL_END; i++)
            if (!strcmp(argv[2], valtypes[i]))
                valType = i;
        if (valType == VAL_NONE)
            goto help;

        poke(valSizes[valType], addr, val);

        return 0;
    }

    if (!strcmp(argv[0], "lfreeze"))
    {
        freezeList();
        return 0;
    }

    if (!strcmp(argv[0], "dfreeze"))
    {
        if (argc != 2)
            goto help;
        u32 index = strtoul(argv[1], NULL, 10);
        freezeDel(index);
        return 0;
    }

    if (!strcmp(argv[0], "afreeze"))
    {
        if (argc != 4)
            goto help;
        u64 addr = strtoull(argv[1], NULL, 16);
        u64 val = strtoull(argv[3], NULL, 10);

        int valType = VAL_NONE;
        for (int i = 1; i < VAL_END; i++)
            if (!strcmp(argv[2], valtypes[i]))
                valType = i;
        if (valType == VAL_NONE)
            goto help;

        freezeAdd(addr, valType, val);

        return 0;
    }

    if (!strcmp(argv[0], "luarun"))
    {
        if (argc != 2)
            goto help;
        if (luaRunPath(argv[1]))
        {
            printf("Something went wrong while trying to run the lua-script :/\r\n");
        }
        return 0;
    }

help:
    printf("Commands:\r\n"
           "    help                                 | Shows this text\r\n"
           "    ssearch u8/u16/u32/u64 value         | Starts a search with 'value' as the starting-value\r\n"
           "    csearch value                        | Searches the hits of the last search for the new value\r\n"
           "    poke address u8/u16/u32/u64 value    | Sets the memory at address to value\r\n"
           "    afreeze address u8/u16/u32/u64 value | Freezes the memory at address to value\r\n"
           "    lfreeze                              | Lists all frozen values\r\n"
           "    dfreeze index                        | Unfreezes the memory at index\r\n"
           "    luarun path/url                      | Runs lua script at path or url (http:// only)\r\n");
    return 0;
}

int main()
{
    int listenfd = setupServerSocket();

    char *linebuf = malloc(sizeof(char) * MAX_LINE_LENGTH);

    int c = sizeof(struct sockaddr_in);
    struct sockaddr_in client;

    mutexInit(&actionLock);

    luaInit();

    Thread freezeThread;
    Result rc = threadCreate(&freezeThread, freezeLoop, NULL, 0x4000, 49, 3);
    if (R_FAILED(rc))
        fatalLater(rc);
    rc = threadStart(&freezeThread);
    if (R_FAILED(rc))
        fatalLater(rc);

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

        printf("Welcome to netcheat!\r\n"
               "This needs fullsvcperm=1 and debugmode=1 set in your hekate-config!\r\n");

        while (1)
        {
            write(sock, "> ", 2);

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
