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

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

CURLcode dlUrlToFile(char* url, char* path) {
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

static int luaRecvLine(lua_State *L) {
    char res[150];
    int len = recv(sock, &res, 150, 0);
    if(len <= 0)
        res[0] = 0;
    else
        res[len-1] = 0;
    lua_pushstring(L, res);
    return 1;
}

static int luaSleepMS(lua_State *L) {
    u64 len = lua_tonumber(L, 1) * 1000000;
    svcSleepThread(len);
    return 1;
}

static int luaPeek8(lua_State *L) {
    mutexLock(&actionLock); attach();
    u64 addr = lua_tonumber(L, 1);
    u8 res = (u8) peek(addr);
    lua_pushnumber(L, res);
    detach(); mutexUnlock(&actionLock);
    return 1;
}

static int luaPeek16(lua_State *L) {
    mutexLock(&actionLock); attach();
    u64 addr = lua_tonumber(L, 1);
    u16 res = (u16) peek(addr);
    lua_pushnumber(L, res);
    detach(); mutexUnlock(&actionLock);
    return 1;
}

static int luaPeek32(lua_State *L) {
    mutexLock(&actionLock); attach();
    u64 addr = lua_tonumber(L, 1);
    u32 res = (u32) peek(addr);
    lua_pushnumber(L, res);
    detach(); mutexUnlock(&actionLock);
    return 1;
}

static int luaPeek64(lua_State *L) {
    mutexLock(&actionLock); attach();
    u64 addr = lua_tonumber(L, 1);
    u64 res = (u64) peek(addr);
    lua_pushnumber(L, res);
    detach(); mutexUnlock(&actionLock);
    return 1;
}

static int luaPoke8(lua_State *L) {
    mutexLock(&actionLock); attach();
    u64 addr = lua_tonumber(L, 1);
    u64 num = lua_tonumber(L, 2);
    poke(1, addr, num);
    detach(); mutexUnlock(&actionLock);
    return 1;
}

static int luaPoke16(lua_State *L) {
    mutexLock(&actionLock); attach();
    u64 addr = lua_tonumber(L, 1);
    u64 num = lua_tonumber(L, 2);
    poke(2, addr, num);
    detach(); mutexUnlock(&actionLock);
    return 1;
}

static int luaPoke32(lua_State *L) {
    mutexLock(&actionLock); attach();
    u64 addr = lua_tonumber(L, 1);
    u64 num = lua_tonumber(L, 2);
    poke(4, addr, num);
    detach(); mutexUnlock(&actionLock);
    return 1;
}

static int luaPoke64(lua_State *L) {
    mutexLock(&actionLock); attach();
    u64 addr = lua_tonumber(L,1);
    u64 num = lua_tonumber(L, 2);
    poke(8, addr, num);
    detach(); mutexUnlock(&actionLock);
    return 1;
}

static int luaGetHeapRegionBase(lua_State *L) {
    mutexLock(&actionLock); attach();
    detach(); mutexUnlock(&actionLock);
    return 1;
}

static int luaGetHeapRegionSize(lua_State *L) {
    mutexLock(&actionLock); attach();
    detach(); mutexUnlock(&actionLock);
    return 1;
}

static int luaStartSearch(lua_State *L) {
    mutexLock(&actionLock); attach();
    detach(); mutexUnlock(&actionLock);
    return 1;
}

static int luaContSearch(lua_State *L) {
    mutexLock(&actionLock); attach();
    detach(); mutexUnlock(&actionLock);
    return 1;
}

static int luaGetResultsLenght(lua_State *L) {
    mutexLock(&actionLock);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaGetResult(lua_State *L) {
    mutexLock(&actionLock);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaFreeze(lua_State *L) {
    mutexLock(&actionLock);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaUnFreeze(lua_State *L) {
    mutexLock(&actionLock);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaGetFreezeLength(lua_State *L) {
    mutexLock(&actionLock);
    mutexUnlock(&actionLock);
    return 1;
}

static int luaGetFreeze(lua_State *L) {
    mutexLock(&actionLock);
    mutexUnlock(&actionLock);
    return 1;
}

int luaRunPath(char *path)
{
    detach();
    mutexUnlock(&actionLock);
    if(!strncmp(path, "http://", 7)) {
        CURLcode res = dlUrlToFile(path, "/netcheat/dl.lua");
        if(res != CURLE_OK) {
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

    lua_pushcfunction(L, luaPeek8); 
    lua_setglobal(L, "peek8"); 
    lua_pushcfunction(L, luaPeek16);
    lua_setglobal(L, "peek16");
    lua_pushcfunction(L, luaPeek32);
    lua_setglobal(L, "peek32");
    lua_pushcfunction(L, luaPeek64);
    lua_setglobal(L, "peek64");

    lua_pushcfunction(L, luaPoke8);
    lua_setglobal(L, "poke8");
    lua_pushcfunction(L, luaPoke16);
    lua_setglobal(L, "poke16");
    lua_pushcfunction(L, luaPoke32);
    lua_setglobal(L, "poke32");
    lua_pushcfunction(L, luaPoke64);
    lua_setglobal(L, "poke64");


    // TODO: Run in thread and terminate it if 'STOP'

	int res = (luaL_loadfile(L, path) || lua_pcall(L, 0, 0, 0));
    if(res) {
        printf("%s\r\n", lua_tostring(L, -1));
    }
	lua_close(L);
    mutexLock(&actionLock);
    attach();
    return res;
}

