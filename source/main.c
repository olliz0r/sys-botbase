#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <switch.h>
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

Handle debughandle = 0;
enum
{
    SEARCH_NONE,
    SEARCH_U32,
    SEARCH_U64
};

int search = SEARCH_NONE;
#define SEARCH_ARR_SIZE 20000
u64 searchArr[SEARCH_ARR_SIZE];
int searchSize;

int sock = -1;

int attach()
{
    u64 pids[300];
    u32 numProc;
    svcGetProcessList(&numProc, pids, 300);
    u64 pid = pids[numProc - 1];

    Result rc = svcDebugActiveProcess(&debughandle, pid);
    if (R_FAILED(rc))
    {
        printf("Couldn't open the process (Error: %x)\n"
               "Make sure that you actually started a game.\n", rc);
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

        u32 u32query = 0;
        u64 u64query = 0;

        if (!strcmp(argv[1], "u32"))
        {
            search = SEARCH_U32;
            u32query = strtoul(argv[2], NULL, 10);
        }
        else if (!strcmp(argv[1], "u64"))
        {
            search = SEARCH_U64;
            u64query = strtoull(argv[2], NULL, 10);
        }
        else
            goto help;

        MemoryInfo meminfo;
        memset(&meminfo, 0, sizeof(MemoryInfo));

        searchSize = 0;

        u64 lastaddr = 0;
        void *outbuf = malloc(0x40000);
        do
        {
            lastaddr = meminfo.addr;
            u32 pageinfo;
            svcQueryDebugProcessMemory(&meminfo, &pageinfo, debughandle, meminfo.addr + meminfo.size);
            if (meminfo.type == MemType_Heap)
            {
                u64 curaddr = meminfo.addr;
                u64 chunksize = 0x40000;

                while (curaddr < meminfo.addr + meminfo.size)
                {
                    if (curaddr + chunksize > meminfo.addr + meminfo.size)
                    {
                        chunksize = meminfo.addr + meminfo.size - curaddr;
                    }

                    svcReadDebugProcessMemory(outbuf, debughandle, curaddr, chunksize);

                    if (search == SEARCH_U32)
                    {
                        u32 *u32buf = (u32 *)outbuf;
                        for (u64 i = 0; i < chunksize / sizeof(u32); i++)
                        {
                            if (u32buf[i] == u32query && searchSize < SEARCH_ARR_SIZE)
                            {
                                printf("Got a hit at %lx!\n", curaddr + i * sizeof(u32));
                                searchArr[searchSize++] = curaddr + i * sizeof(u32);
                            }
                        }
                    }

                    if (search == SEARCH_U64)
                    {
                        u64 *u64buf = (u64 *)outbuf;
                        for (u64 i = 0; i < chunksize / sizeof(u64); i++)
                        {
                            if (u64buf[i] == u64query && searchSize < SEARCH_ARR_SIZE)
                            {
                                printf("Got a hit at %lx!\n", curaddr + i * sizeof(u64));
                                searchArr[searchSize++] = curaddr + i * sizeof(u32);
                            }
                        }
                    }

                    curaddr += chunksize;
                }
            }
        } while (lastaddr < meminfo.addr + meminfo.size && searchSize < SEARCH_ARR_SIZE);
        if(searchSize >= SEARCH_ARR_SIZE) {
            printf("There might be more after this, try getting the variable to a number that's less 'common'\n");
        }
        free(outbuf);

        return 0;
    }

    if (!strcmp(argv[0], "csearch"))
    {
        if (argc != 2)
            goto help;
        if (search == SEARCH_NONE)
        {
            printf("You need to start a search first!");
            return 0;
        }

        u32 u32NewVal = 0;
        u64 u64NewVal = 0;
        if (search == SEARCH_U32)
        {
            u32NewVal = strtoul(argv[1], NULL, 10);
        }
        else if (search == SEARCH_U64)
        {
            u64NewVal = strtoull(argv[1], NULL, 10);
        }

        u64 newSearchSize = 0;
        for (int i = 0; i < searchSize; i++)
        {
            if (search == SEARCH_U32)
            {
                u32 val;
                svcReadDebugProcessMemory(&val, debughandle, searchArr[i], sizeof(u32));
                if (val == u32NewVal)
                {
                    printf("Got a hit at %lx!\n", searchArr[i]);
                    searchArr[newSearchSize++] = searchArr[i];
                }
            }
            if (search == SEARCH_U64)
            {
                u64 val;
                svcReadDebugProcessMemory(&val, debughandle, searchArr[i], sizeof(u64));
                if (val == u64NewVal)
                {
                    printf("Got a hit at %lx!\n", searchArr[i]);
                    searchArr[newSearchSize++] = searchArr[i];
                }
            }
        }

        searchSize = newSearchSize;
        return 0;
    }

    if (!strcmp(argv[0], "poke"))
    {
        if (argc != 4)
            goto help;

        u64 addr = strtoull(argv[1], NULL, 16);

        if (!strcmp(argv[2], "u32"))
        {
            u32 val = strtoul(argv[3], NULL, 10);
            svcWriteDebugProcessMemory(debughandle, &val, addr, sizeof(u32));
        }
        else if (!strcmp(argv[2], "u64"))
        {
            u64 val = strtoull(argv[3], NULL, 10);
            svcWriteDebugProcessMemory(debughandle, &val, addr, sizeof(u64));
        }
        else
            goto help;
        return 0;
    }

help:
    printf("Commands:\n"
           "    help                       | Shows this text\n"
           "    ssearch u32/u64 value      | Starts a search with 'value' as the starting-value\n"
           "    csearch value              | Searches the hits of the last search for the new value\n"
           "    poke address u32/u64 value | Sets the memory at address to value\n");
    return 0;
}

int main()
{

    int listenfd = setupServerSocket();

    char *linebuf = malloc(sizeof(char) * MAX_LINE_LENGTH);

    int c = sizeof(struct sockaddr_in);
    struct sockaddr_in client;

    while (appletMainLoop()) {
        sock = accept(listenfd, (struct sockaddr *)&client, (socklen_t*)&c);
        if(sock <= 0) {
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

        printf("Welcome to netcheat!\n"
               "This needs fullsvcperm=1 and debugmode=1 set in your hekate-config!\n");

        while (1)
        {
            write(sock, "> ", 2);

            int len = recv(sock, linebuf, MAX_LINE_LENGTH, 0);
            if(len < 1) {
                break;
            }

            linebuf[len - 1] = 0;

            if (attach())
                continue;

            parseArgs(linebuf, &argmain);
            
            detach();

            svcSleepThread(1e+7L);
        }
        detach();
    }

    if (debughandle != 0)
        svcCloseHandle(debughandle);
    
    return 0;
}
