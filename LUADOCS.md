# Lua documentation

This is the documentation for the **work in progress** lua-api for sys-netcheat.  
Not everything has been fully tested and there may (or may not) still be issues!

If you encounter any please open an issue and I'll hopefully be able to fix it.

---

## How to run scripts:

You can run lua scripts from both the sd card and **http**-websites.

```
> luarun http://pastebin.com/raw/CX6RSeYH
Hello netcheat!

> luarun /netcheat/hello.lua
Hello from the sd-card!
```

Https is **not** supported yet.

For script-development I recommend opening an local http-server in your script location (for example with `python3 -m http.server`)

```
~> python3 -m http.server &
Serving HTTP on 0.0.0.0 port 8000 (http://0.0.0.0:8000/) ...
~> nc 192.168.178.60 5555
Welcome to netcheat!
This needs debugmode=1 set in your hekate-config!
> luarun http://192.168.178.64:8000/script.lua
Hello from the local network!
```

If you want to terminate an script early (for example because it's stuck in an infinite loop) simply enter `stop` into the console and the script will be canceled.

---

## Types

#### ValTypes

    {"u8", "u16", "u32", "u64"}

#### MemTypes

    {"MemType_Unmapped",
     "MemType_Io",
     "MemType_Normal",
     "MemType_CodeStatic",
     "MemType_CodeMutable",
     "MemType_Heap",
     "MemType_SharedMem",
     "MemType_WeirdMappedMem",
     "MemType_ModuleCodeStatic",
     "MemType_ModuleCodeMutable",
     "MemType_IpcBuffer0",
     "MemType_MappedMemory",
     "MemType_ThreadLocal",
     "MemType_TransferMemIsolated",
     "MemType_TransferMem",
     "MemType_ProcessMem",
     "MemType_Reserved",
     "MemType_IpcBuffer1",
     "MemType_IpcBuffer3",
     "MemType_KernelStack",
     "MemType_CodeReadOnly",
     "MemType_CodeWritable"}

---

## General

#### sleepMS(ms)
Pauses the script for `ms` milliseconds.

#### line = recvLine()
Reads a line from the console-input into the `line` variable.

#### value = peek(valType, address)
Reads the value of type `valType` at `address` into `value`.

#### poke(valType, address, value)
Writes `value` with type `valType` to `address`.

#### address, size = getRegionInfo(memType, n)
Gets the start-`address` and `size` of the `n`th region of type `memType` (starting at `0`!).  
If there is no `n`th region both `address` and `size` will be zero

---

## Search

Since lua isn't an particualarly fast language searching the whole memory of the switch would be really slow if done in pure lua. That's why there are a few helper-functions to allow searching much more quickly.

#### success = startSearch(memType, valType, value)
Searches all sections of type `memType` (`ssearch` in the cli searches `"MemType_Heap"`) for `value` of type `valType`.   
`ret` is normally `0`.  
If the search-buffer is full (too many results found) it will be `1`.

#### success = searchSection(memType, n, valType, value)
Searches the `n`th section of type `memType` (starting at `0`!) for `value` of type `valType`  
`ret` is normally `0`.  
If the search-buffer is full (too many results found) it will be `1`.

#### n = getResultsLength()
Returns the number of results from the last search.

#### val = getResult(n)
Get's the `n`th result from the last search.

---

## Freezes

These functions manage the freezing and unfreezing of values.

#### freeze(valType, address, value)
Freezes the value of type `valType` at `address` to `value`

#### unFreeze(n)
Unfreezes the `n`th frozen value (starting at zero)

#### n = getFreezeLength()
Returns the number of frozen values

#### valType, address, value = getFreeze(n)
Returns the `valType`, `address` and `value` of the `n`th frozen value.