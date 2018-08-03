#include <switch.h>
#define SEARCH_CHUNK_SIZE 0x40000

extern Handle debughandle;
enum
{
    VAL_NONE,
    VAL_U8,
    VAL_U16,
    VAL_U32,
    VAL_U64,
    VAL_END
};
static char *valtypes[] = {"none", "u8", "u16", "u32", "u64"};
static int valSizes[] = {0, 1, 2, 4, 8};

static char *memTypeStrings[] = {"MemType_Unmapped",
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
                                 "MemType_CodeWritable"};

extern int search;
#define SEARCH_ARR_SIZE 200000
extern u64 searchArr[SEARCH_ARR_SIZE];
extern int searchSize;

int attach();
void detach();

void poke(int valSize, u64 addr, u64 val);
u64 peek(u64 addr);

int startSearch(u64 val, u32 valType, u32 memtype);
int contSearch(u64 val);


#define FREEZE_LIST_LEN 100

void freezeList();
void freezeAdd(u64 addr, int type, u64 value);
void freezeDel(int index);
void freezeLoop();

extern int numFreezes;
extern u64 freezeAddrs[FREEZE_LIST_LEN];
extern int freezeTypes[FREEZE_LIST_LEN];
extern u64 freezeVals[FREEZE_LIST_LEN];


MemoryInfo getRegionOfType(int index, u32 type);
int searchSection(u64 val, u32 valType, MemoryInfo meminfo, void *buffer, u64 bufSize);
