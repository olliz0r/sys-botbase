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

void poke(int valSize, u64 addr, u64 val)
{
    svcWriteDebugProcessMemory(debughandle, &val, addr, valSize);
}

u64 peek(u64 addr)
{
    u64 out;
    svcReadDebugProcessMemory(&out, debughandle, addr, sizeof(u64));
    return out;
}

MemoryInfo getRegionOfType(int index, u32 type)
{
    MemoryInfo meminfo;
    memset(&meminfo, 0, sizeof(MemoryInfo));

    int curInd = 0;

    u64 lastaddr;
    do
    {
        lastaddr = meminfo.addr;
        u32 pageinfo;
        svcQueryDebugProcessMemory(&meminfo, &pageinfo, debughandle, meminfo.addr + meminfo.size);
        if (meminfo.type == type)
        {
            if (curInd == index)
                return meminfo;
            curInd++;
        }
    } while (lastaddr < meminfo.addr + meminfo.size);
    meminfo.addr = 0;
    return meminfo;
}

u64 getPointerToAddr(int index, u64 addr)
{
#define SEARCH_CHUNK_SIZE 0x40000
    u64 *buf = malloc(SEARCH_CHUNK_SIZE);
    int curInd = 0;
    u64 i = 0;
    MemoryInfo codeRegion = getRegionOfType(i, MemType_CodeMutable);
    while (codeRegion.addr != 0)
    {
        u64 off = 0;
        while (off != codeRegion.size)
        {
            u64 chunkSize = SEARCH_CHUNK_SIZE;
            if (off + chunkSize > codeRegion.size)
                chunkSize = codeRegion.size - off;
            svcReadDebugProcessMemory(buf, debughandle, codeRegion.addr + off, chunkSize);

            for (u64 i = 0; i < chunkSize / sizeof(u64); i++)
            {
                if (buf[i] == addr && curInd++ == index)
                {
                    free(buf);
                    return codeRegion.addr + off + i * sizeof(u64);
                }
            }

            off += chunkSize;
        }

        codeRegion = getRegionOfType(i++, MemType_CodeMutable);
    }

    free(buf);
    return 0;
}

int search = VAL_NONE;
#define SEARCH_ARR_SIZE 200000
u64 searchArr[SEARCH_ARR_SIZE];
int searchSize;

int searchSection(u64 val, u32 valType, MemoryInfo meminfo, void *buffer, u64 bufSize)
{
    int valSize = valSizes[valType];
    u64 off = 0;
    while (off < meminfo.size)
    {
        if (meminfo.size - off < bufSize)
            bufSize = meminfo.size - off;
        svcReadDebugProcessMemory(buffer, debughandle, meminfo.addr + off, bufSize);
        for (u64 i = 0; i < bufSize; i += valSize)
        {
            if (!memcmp(buffer + i, &val, valSize))
            {
                if (searchSize < SEARCH_ARR_SIZE)
                    searchArr[searchSize++] = meminfo.addr + off + i;
                else
                    return 1;
            }
        }
        off += bufSize;
    }
    return 0;
}

int startSearch(u64 val, u32 valType, u32 memtype)
{
    search = valType;

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
        if (meminfo.type == memtype && searchSection(val, valType, meminfo, outbuf, 0x40000))
        {
            free(outbuf);
            return 1;
        }
    } while (lastaddr < meminfo.addr + meminfo.size);

    free(outbuf);
    return 0;
}

int contSearch(u64 val)
{
    int valSize = valSizes[search];
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

            poke(valSizes[freezeTypes[i]], freezeAddrs[i], freezeVals[i]);

            detach();
        }
        mutexUnlock(&actionLock);
        svcSleepThread(5e+8L);
    }
}