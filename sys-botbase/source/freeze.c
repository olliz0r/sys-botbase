#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <switch.h>
#include "freeze.h"

FreezeBlock* freezes;

int initFreezes(void)
{
	freezes = calloc(FREEZE_DIC_LENGTH, sizeof(FreezeBlock));
	return 0;
}

int freeFreezes(void)
{
	free(freezes);
	return 0;
}

int findAddrSlot(u64 addr)
{
	for (int i = 0; i < FREEZE_DIC_LENGTH; i++)
	{
		if (freezes[i].address == addr)
			return i;
	}
	
	return -1;
}

int findNextEmptySlot()
{
	return findAddrSlot(0);
}

int addToFreezeMap(u64 addr, u8* v_data, u64 v_size)
{
	// update slot if already exists
	int slot = findAddrSlot(addr);
	if (slot == -1)
		slot = findNextEmptySlot();
	else
		removeFromFreezeMap(addr);
	
	if (slot == -1)
		return 0;
	
	freezes[slot].address = addr;
	freezes[slot].vData = v_data;
	freezes[slot].size = v_size;
	freezes[slot].state = 1;
	
	return slot;
}

int removeFromFreezeMap(u64 addr)
{
	int slot = findAddrSlot(addr);
	if (slot == -1)
		return 0;
	freezes[slot].address = 0;
	freezes[slot].state = 0;
	free(freezes[slot].vData);
	return slot;
}

int getFreezeCount()
{
	int count = 0;
	for (int i = 0; i < FREEZE_DIC_LENGTH; i++)
	{
		if (freezes[i].state != 0)
			++count;
	}
	printf("%02X", count);
	printf("\n");
	return count;
}

// returns false if there was nothing to clear
char clearFreezes()
{
	char clearedOne = 0;
	int i;
	for (i = 0; i < FREEZE_DIC_LENGTH; i++)
	{
		FreezeBlock* curFreeze = &freezes[i];
		if (curFreeze->state != 0)
		{
			removeFromFreezeMap(curFreeze->address);
			clearedOne = 1;
		}
		curFreeze->state = 0;
	}
	return clearedOne;
}