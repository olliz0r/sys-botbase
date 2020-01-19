#include <switch.h>
#define SEARCH_CHUNK_SIZE 0x40000

extern Handle debughandle;

int attach();
void detach();

void poke(int valSize, u64 addr, u64 val);
u64 peek(u64 addr);