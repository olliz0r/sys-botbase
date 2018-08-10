#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <poll.h>

#include <curl/curl.h>

#include "cheat.h"
#include "util.h"

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

Semaphore done;


size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

CURLcode dlUrlToFile(char *url, char *path)
{
    CURL *curl;
    FILE *fp;
    CURLcode res = CURLE_FAILED_INIT;
    curl = curl_easy_init();
    if (curl)
    {
        fp = fopen(path, "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    else
        return CURLE_FAILED_INIT;
    return res;
}

void luaInit()
{
    mkdir("/netcheat", 0700);
}

char *line;

static int luaRecvLine(lua_State *L)
{
    mutexLock(&actionLock);
    while (strlen(line) == 0)
    {
        mutexUnlock(&actionLock);
        svcSleepThread(200000000);
        mutexLock(&actionLock);

        if(semaphoreTryWait(&done)) {
            semaphoreSignal(&done);
            break;
        }
    }
    lua_pushstring(L, line);
    line[0] = 0;
    mutexUnlock(&actionLock);
    return 1;
}

static int luaSleepMS(lua_State *L)
{
    u64 len = lua_tonumber(L, 1) * 1000000;
    svcSleepThread(len);
    return 0;
}

static int luaPeek(lua_State *L)
{
    mutexLock(&actionLock);
    attach();
    const char *type = lua_tostring(L, 1);
    u64 addr = lua_tonumber(L, 2);

    u64 res = peek(addr);
    u64 res_real = 0;

    int valType = VAL_NONE;
    for (int i = 1; i < VAL_END; i++)
        if (!strcmp(type, valtypes[i]))
            valType = i;

    if (valType == VAL_NONE)
    {
        luaL_error(L, "Error: Trying to peek invalid/unsupported type\r\n");
    } else {
        memcpy(&res_real, &res, valSizes[valType]);
    }

    lua_pushnumber(L, res_real);
    detach();
    mutexUnlock(&actionLock);
    return 1;
}

static int luaPoke(lua_State *L)
{
    mutexLock(&actionLock);
    attach();
    const char *type = lua_tostring(L, 1);
    u64 addr = lua_tonumber(L, 2);
    u64 num = lua_tonumber(L, 3);

    int valType = VAL_NONE;
    for (int i = 1; i < VAL_END; i++)
        if (!strcmp(type, valtypes[i]))
            valType = i;

    if (valType == VAL_NONE)
    {
        luaL_error(L, "Error: Trying to poke invalid/unsupported type\r\n");
    } else {
        poke(valSizes[valType], addr, num);
    }

    detach();
    mutexUnlock(&actionLock);
    return 0;
}

static int luaGetRegionInfo(lua_State *L)
{
    mutexLock(&actionLock);
    attach();
    const char *type = lua_tostring(L, 1);

    u32 valType = MemType_Unmapped;
    for (int i = 1; i <= MemType_CodeWritable; i++)
        if (!strcmp(type, memTypeStrings[i]))
            valType = i;
    if (valType == MemType_Unmapped)
    {
        luaL_error(L, "Invalid valType!");
        goto end;
    }

    int num = lua_tointeger(L, 2);
    MemoryInfo meminfo = getRegionOfType(num, valType);

    lua_pushnumber(L, meminfo.addr);
    lua_pushnumber(L, meminfo.size);

end:
    detach();
    mutexUnlock(&actionLock);
    return 2;
}

static int luaSearchSection(lua_State *L)
{
    int ret = 1;

    mutexLock(&actionLock);
    searchSize = 0;

    attach();
    const char *type = lua_tostring(L, 1);

    u32 regType = MemType_Unmapped;
    for (int i = 1; i < MemType_CodeWritable; i++)
        if (!strcmp(type, memTypeStrings[i]))
            regType = i;
    if (regType == MemType_Unmapped)
    {
        luaL_error(L, "Invalid memory-type!\r\n");
        goto end;
    }

    int index = lua_tonumber(L, 2);
    MemoryInfo meminfo = getRegionOfType(index, regType);
    if(meminfo.size == 0)
        goto end;
    

    const char *valTypeStr = lua_tostring(L, 3);

    u32 valType = VAL_NONE;
    for (int i = 0; i < VAL_END; i++)
    {
        if (!strcmp(valTypeStr, valtypes[i]))
            valType = i;
    }

    if (valType == VAL_NONE)
    {
        luaL_error(L, "Invalid value-type!\r\n");
        goto end;
    }

    u64 val = lua_tonumber(L, 4);

    void *buffer = malloc(SEARCH_CHUNK_SIZE);

    ret = searchSection(val, valType, meminfo, buffer, SEARCH_CHUNK_SIZE);

    free(buffer);
end:
    lua_pushnumber(L, ret);
    detach();
    mutexUnlock(&actionLock);
    return 1;
}

static int luaStartSearch(lua_State *L)
{
    int ret = 1;
    mutexLock(&actionLock);
    attach();
    const char *regTypeStr = lua_tostring(L, 1);

    u32 regType = MemType_Unmapped;
    for (int i = 1; i < MemType_CodeWritable; i++)
        if (!strcmp(regTypeStr, memTypeStrings[i]))
            regType = i;
    if (regType == MemType_Unmapped)
    {
        luaL_error(L, "Invalid memory-type!\r\n");
        goto end;
    }

    const char *valTypeStr = lua_tostring(L, 2);

    u32 valType = VAL_NONE;
    for (int i = 0; i < VAL_END; i++)
    {
        if (!strcmp(valTypeStr, valtypes[i]))
            valType = i;
    }

    if (valType == VAL_NONE)
    {
        luaL_error(L, "Invalid value-type!\r\n");
        goto end;
    }

    u64 val = lua_tonumber(L, 3);

    ret = startSearch(val, valType, regType);

end:
    lua_pushnumber(L, ret);
    detach();
    mutexUnlock(&actionLock);
    return 1;
}

static int luaContSearch(lua_State *L)
{
    mutexLock(&actionLock);
    attach();
    int newVal = lua_tonumber(L, 1);
    contSearch(newVal);
    detach();
    mutexUnlock(&actionLock);
    return 0;
}

static int luaGetResultsLength(lua_State *L)
{
    mutexLock(&actionLock);
    lua_pushnumber(L, searchSize);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaGetResult(lua_State *L)
{
    mutexLock(&actionLock);
    int index = lua_tonumber(L, 1);
    if (index >= searchSize || index < 0)
    {
        luaL_error(L, "Tried to get result from invalid index\r\n");
        goto end;
    }

end:
    lua_pushnumber(L, searchArr[index]);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaFreeze(lua_State *L)
{
    mutexLock(&actionLock);
    attach();
    const char *type = lua_tostring(L, 1);
    u64 addr = lua_tonumber(L, 2);
    u64 num = lua_tonumber(L, 3);

    int valType = VAL_NONE;
    for (int i = 1; i < VAL_END; i++)
        if (!strcmp(type, valtypes[i]))
            valType = i;

    if (valType == VAL_NONE)
    {
        luaL_error(L, "Trying to poke invalid/unsupported type\r\n");
        goto end;
    }

    freezeAdd(addr, valType, num);
end:
    detach();
    mutexUnlock(&actionLock);
    return 0;
}

static int luaUnFreeze(lua_State *L)
{
    mutexLock(&actionLock);
    int index = lua_tonumber(L, 1);
    if (index >= numFreezes || index < 0)
    {
        luaL_error(L, "Trying to access invalid value!\r\n");
        goto end;
    }
    freezeDel(index);
end:
    mutexUnlock(&actionLock);
    return 0;
}

static int luaGetFreezeLength(lua_State *L)
{
    mutexLock(&actionLock);
    lua_pushnumber(L, numFreezes);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaGetFreeze(lua_State *L)
{
    mutexLock(&actionLock);
    int index = lua_tonumber(L, 1);
    if (index >= numFreezes || index < 0)
    {
        luaL_error(L, "Trying to access invalid value!\r\n");
        goto end;
    }

    lua_pushstring(L, valtypes[freezeTypes[index]]);
    lua_pushnumber(L, freezeAddrs[index]);
    lua_pushnumber(L, freezeVals[index]);

end:
    mutexUnlock(&actionLock);
    return 1;
}


char *runPath;
void luaRunner(void *LState)
{
    lua_State *L = (lua_State *)LState;

    int res = (luaL_loadfile(L, runPath) || lua_pcall(L, 0, 0, 0));
    if (res)
    {
        printf("%s\r\n", lua_tostring(L, -1));
    }
    semaphoreSignal(&done);
}

void luaHookFunc(lua_State *L, lua_Debug *ar)
{
    if (ar->event == LUA_HOOKLINE)
        if (semaphoreTryWait(&done))
            luaL_error(L, "Sucessfully terminated lua-script!");
    svcSleepThread(1);
}

int luaRunPath(char *path)
{
    detach();
    mutexUnlock(&actionLock);
    if (!strncmp(path, "http://", 7))
    {
        CURLcode res = dlUrlToFile(path, "/netcheat/dl.lua");
        if (res != CURLE_OK)
        {
            printf("Failed to dl file :/\r\n");
            return 1;
        }

        path = "/netcheat/dl.lua";
    }
    runPath = path;

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushcfunction(L, luaRecvLine);
    lua_setglobal(L, "recvLine");

    lua_pushcfunction(L, luaSleepMS);
    lua_setglobal(L, "sleepMS");

    lua_pushcfunction(L, luaPeek);
    lua_setglobal(L, "peek");

    lua_pushcfunction(L, luaPoke);
    lua_setglobal(L, "poke");

    lua_pushcfunction(L, luaGetRegionInfo);
    lua_setglobal(L, "getRegionInfo");

    lua_pushcfunction(L, luaSearchSection);
    lua_setglobal(L, "searchSection");

    lua_pushcfunction(L, luaStartSearch);
    lua_setglobal(L, "startSearch");

    lua_pushcfunction(L, luaContSearch);
    lua_setglobal(L, "contSearch");

    lua_pushcfunction(L, luaGetResultsLength);
    lua_setglobal(L, "getResultsLength");

    lua_pushcfunction(L, luaGetResult);
    lua_setglobal(L, "getResult");

    lua_pushcfunction(L, luaFreeze);
    lua_setglobal(L, "freeze");

    lua_pushcfunction(L, luaUnFreeze);
    lua_setglobal(L, "unfreeze");

    lua_pushcfunction(L, luaGetFreezeLength);
    lua_setglobal(L, "getFreezeLength");

    lua_pushcfunction(L, luaGetFreeze);
    lua_setglobal(L, "getFreeze");

    line = malloc(MAX_LINE_LENGTH);
    line[0] = 0;

    semaphoreInit(&done, 0);

    lua_sethook(L, &luaHookFunc, LUA_MASKLINE, 0);

    Thread luaThread;
    Result rc = threadCreate(&luaThread, luaRunner, L, 0x4000, 49, 3);
    if (R_FAILED(rc))
        fatalLater(rc);
    threadStart(&luaThread);

    line = malloc(MAX_LINE_LENGTH);
    line[0] = 0;

    struct pollfd fd;
    fd.fd = sock;
    fd.events = POLLIN;
    int ret;

    while (!semaphoreTryWait(&done))
    {
        svcSleepThread(300000000L);
        ret = poll(&fd, 1, 200);
        mutexLock(&actionLock);
        switch (ret)
        {
        case -1:
            semaphoreSignal(&done);
            mutexUnlock(&actionLock);
            goto done;
            break;
        case 0:
            break;
        default:
        {
            int len = recv(sock, line, MAX_LINE_LENGTH, 0);
            if (len <= 0)
            {
                printf("Receive failed!\r\n");
                semaphoreSignal(&done);
                mutexUnlock(&actionLock);
                goto done;
            }
            line[len - 1] = 0;
            if (!strcmp(line, "stop"))
            {
                printf("Aborting lua-script!\r\n");
                line[0] = 0;
                semaphoreSignal(&done);
                mutexUnlock(&actionLock);
                goto done;
            }
            break;
        }

        }
        mutexUnlock(&actionLock);
    }

done:

    threadWaitForExit(&luaThread);

    lua_close(L);

    free(line);

    return 0;
}
