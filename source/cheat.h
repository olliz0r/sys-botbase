#include <switch.h>

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
static int valSizes[] = {0, 1, 2, 4};

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

void freezeList();
void freezeAdd(u64 addr, int type, u64 value);
void freezeDel(int index);
void freezeLoop();

MemoryInfo getRegionOfType(int index, u32 type);
u64 getPointerToAddr(int index, u64 addr);