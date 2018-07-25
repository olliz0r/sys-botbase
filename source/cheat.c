#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cheat.h"
#include "util.h"

Handle debughandle = 0;

Mutex actionLock;

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
        printf("Couldn't open the process (Error: %x)\r\n"
               "Make sure that you actually started a game.\r\n",
               rc);
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

int poke(char *type, u64 addr, u64 val)
{

    if (!strcmp(type, "u8"))
    {
        u8 realval = (u8)val;
        svcWriteDebugProcessMemory(debughandle, &realval, addr, sizeof(u8));
    }
    else if (!strcmp(type, "u16"))
    {
        u16 realval = (u16)val;
        svcWriteDebugProcessMemory(debughandle, &realval, addr, sizeof(u16));
    }
    else if (!strcmp(type, "u32"))
    {
        u32 realval = (u32)val;
        svcWriteDebugProcessMemory(debughandle, &realval, addr, sizeof(u32));
    }
    else if (!strcmp(type, "u64"))
    {
        u64 realval = val;
        svcWriteDebugProcessMemory(debughandle, &realval, addr, sizeof(u64));
    }
    else
    {
        return 1;
    }
    return 0;
}

u64 peek(u64 addr)
{
    u64 out;
    svcReadDebugProcessMemory(&out, debughandle, addr, sizeof(u64));
    return out;
}

int search = VAL_NONE;
#define SEARCH_ARR_SIZE 200000
u64 searchArr[SEARCH_ARR_SIZE];
int searchSize;

int startSearch(u64 val, u32 valType, u32 memtype)
{
    search = valType;
    int valSize = pow(2, valType - 1);

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
        if (meminfo.type == memtype)
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

                for (u64 i = 0; i < chunksize; i += valSize)
                {
                    if (!memcmp(outbuf + i, &val, valSize) && searchSize < SEARCH_ARR_SIZE)
                    {
                        searchArr[searchSize++] = curaddr + i;
                    }
                }

                curaddr += chunksize;
            }
        }
    } while (lastaddr < meminfo.addr + meminfo.size && searchSize < SEARCH_ARR_SIZE);
    if (searchSize >= SEARCH_ARR_SIZE)
    {
        return 1;
    }

    free(outbuf);

    return 0;
}

int contSearch(u64 val)
{
    int valSize = pow(2, search - 1);
    int newSearchSize = 0;
    if (search == VAL_NONE)
    {
        printf("You need to start a search first!");
        return 1;
    }

    for (int i = 0; i < searchSize; i++)
    {
        u64 newVal = peek(searchArr[i]);
        if (!memcmp(&val, &newVal, valSize))
        {
            searchArr[newSearchSize++] = searchArr[i];
        }
    }

    searchSize = newSearchSize;
    return 0;
}

#define FREEZE_LIST_LEN 100
u64 freezeAddrs[FREEZE_LIST_LEN];
int freezeTypes[FREEZE_LIST_LEN];
u64 freezeVals[FREEZE_LIST_LEN];
int numFreezes = 0;

void freezeList()
{
    for (int i = 0; i < numFreezes; i++)
    {
        printf("%d) %lx (%s) = %ld\r\n", i, freezeAddrs[i], valtypes[freezeTypes[i]], freezeVals[i]);
    }
}

void freezeAdd(u64 addr, int type, u64 value)
{
    if (numFreezes >= FREEZE_LIST_LEN)
    {
        printf("Can't add any more frozen values!\r\n"
               "Please remove some of the old ones!\r\n");
    }
    freezeAddrs[numFreezes] = addr;
    freezeTypes[numFreezes] = type;
    freezeVals[numFreezes] = value;
    numFreezes++;
}

void freezeDel(int index)
{
    if (numFreezes <= index)
    {
        printf("That number doesn't exit!");
    }
    numFreezes--;
    for (int i = index; i < numFreezes; i++)
    {
        freezeAddrs[i] = freezeAddrs[i + 1];
        freezeTypes[i] = freezeTypes[i + 1];
        freezeVals[i] = freezeVals[i + 1];
    }
}

void freezeLoop()
{
    while (1)
    {
        mutexLock(&actionLock);
        for (int i = 0; i < numFreezes; i++)
        {
            if (attach())
            {
                printf("The process apparently died. Cleaning the freezes up!\r\n");
                while (numFreezes > 0)
                {
                    freezeDel(0);
                }
                break;
            }

            poke(valtypes[freezeTypes[i]], freezeAddrs[i], freezeVals[i]);

            detach();
        }
        mutexUnlock(&actionLock);
        svcSleepThread(5e+8L);
    }
}