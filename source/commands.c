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
