#include <switch.h>
#define MAX_LINE_LENGTH 344 * 32 * 2
#define FREEZE_DIC_LENGTH 255

extern u64 mainLoopSleepTime;
extern bool debugResultCodes;
extern u64 freezeAddrMap[FREEZE_DIC_LENGTH];
extern u8* freezeValueMap[FREEZE_DIC_LENGTH];
extern u64 freezeMapSizes[FREEZE_DIC_LENGTH];

int findAddrSlot(u64 addr);
int findNextEmptySlot();
int addToFreezeMap(u64 addr, u8* v_data, u64 v_size);
int removeFromFreezeMap(u64 addr);
int getFreezeCount();
bool clearFreezes();
int setupServerSocket();
u64 parseStringToInt(char* arg);
u8* parseStringToByteBuffer(char* arg, u64* size);
HidControllerKeys parseStringToButton(char* arg);
Result capsscCaptureForDebug(void *buffer, size_t buffer_size, u64 *size); //big thanks to Behemoth from the Reswitched Discord!