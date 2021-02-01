#include <switch.h>
#define FREEZE_DIC_LENGTH 255

typedef struct {
	char state;
	u64 address;
	u8* vData;
	u64 size;
} FreezeBlock;

extern FreezeBlock* freezes;

int initFreezes(void);
int freeFreezes(void);
int findAddrSlot(u64 addr);
int findNextEmptySlot();
int addToFreezeMap(u64 addr, u8* v_data, u64 v_size);
int removeFromFreezeMap(u64 addr);
int getFreezeCount();
char clearFreezes();