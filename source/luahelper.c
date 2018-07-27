#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <curl/curl.h>

#include "cheat.h"
#include "util.h"

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

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

static int luaRecvLine(lua_State *L)
{
    char res[150];
    int len = recv(sock, &res, 150, 0);
    if (len <= 0)
        res[0] = 0;
    else
        res[len - 1] = 0;
    lua_pushstring(L, res);
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

    memcpy(&res_real, &res, valSizes[valType]);

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
        printf("WARNING: Trying to poke invalid/unsupported type\r\n");
    }

    poke(valSizes[valType], addr, num);
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
    for (int i = 1; i < MemType_CodeWritable; i++)
        if (!strcmp(type, memTypeStrings[i]))
            valType = i;

    int num = lua_tointeger(L, 2);
    MemoryInfo meminfo = getRegionOfType(num, valType);

    lua_pushnumber(L, meminfo.addr);
    lua_pushnumber(L, meminfo.size);

    detach();
    mutexUnlock(&actionLock);
    return 2;
}

static int luaResetSearch(lua_State *L)
{
    mutexLock(&actionLock);
    searchSize = 0;
    mutexUnlock(&actionLock);
    return 0;
}

static int luaSetSearchSize(lua_State *L)
{
    mutexLock(&actionLock);
    searchSize = lua_tonumber(L, 1);
    mutexUnlock(&actionLock);
    return 0;
}

static int luaSearchSection(lua_State *L)
{
    int ret = 1;

    mutexLock(&actionLock);
    attach();
    const char *type = lua_tostring(L, 1);

    u32 regType = MemType_Unmapped;
    for (int i = 1; i < MemType_CodeWritable; i++)
        if (!strcmp(type, memTypeStrings[i]))
            regType = i;
    if (regType == MemType_Unmapped)
    {
        printf("Invalid memory-type!\r\n");
        goto end;
    }

    int index = lua_tonumber(L, 2);
    MemoryInfo meminfo = getRegionOfType(index, regType);

    const char *valTypeStr = lua_tostring(L, 3);

    u32 valType = VAL_NONE;
    for (int i = 0; i < VAL_END; i++)
    {
        if (!strcmp(valTypeStr, valtypes[i]))
            valType = i;
    }

    if (valType == VAL_NONE)
    {
        printf("Invalid value-type!\r\n");
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
        printf("Invalid memory-type!\r\n");
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
        printf("Invalid value-type!\r\n");
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

static int luaGetResultsLenght(lua_State *L)
{
    mutexLock(&actionLock);
    lua_pushnumber(L, searchSize);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaGetResult(lua_State *L)
{
    mutexLock(&actionLock);
    u64 res;
    int index = lua_tonumber(L, 1);
    if (index >= searchSize || index < 0)
    {
        printf("Tried to get result from invalid index\r\n");
        goto end;
    }
    u64 tmp_res = searchArr[index];
    memcpy(&res, &tmp_res, valSizes[search]);

end:
    lua_pushnumber(L, res);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaFreeze(lua_State *L)
{
    mutexLock(&actionLock);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaUnFreeze(lua_State *L)
{
    mutexLock(&actionLock);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaGetFreezeLength(lua_State *L)
{
    mutexLock(&actionLock);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaGetFreeze(lua_State *L)
{
    mutexLock(&actionLock);
    mutexUnlock(&actionLock);
    return 1;
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

    // TODO: Run in thread and terminate it if 'STOP'

    int res = (luaL_loadfile(L, path) || lua_pcall(L, 0, 0, 0));
    if (res)
    {
        printf("%s\r\n", lua_tostring(L, -1));
    }
    lua_close(L);
    mutexLock(&actionLock);
    attach();
    return res;
}
