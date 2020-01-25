#include <switch.h>
#define MAX_LINE_LENGTH 300

extern int sock;

void fatalLater(Result err);
int setupServerSocket();
u64 getHeap();
u64 parseStringToInt(char* arg);
u8* parseStringToByteBuffer(char* arg);
HidControllerKeys parseStringToButton(char* arg);