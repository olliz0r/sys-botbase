/*
 * Copyright (c) 2020 sys-botbase
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 3, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <switch.h>

extern Handle debughandle;
bool bControllerIsInitialised;
u64 controllerHandle;
HiddbgHdlsDeviceInfo controllerDevice;
HiddbgHdlsState controllerState;
extern u64 buttonClickSleepTime;

typedef struct {
  u64 main_nso_base;
  u64 heap_base;
  u64 titleID;
  u8 buildID[0x20];
} MetaData;

void attach();
void detach();
u64 getMainNsoBase(u64 pid);
u64 getHeapBase(Handle handle);
u64 getTitleId(u64 pid);
void getBuildID(MetaData *meta, u64 pid);
MetaData getMetaData(void);

void poke(u64 offset, u64 size, u8 *val);
void peek(u64 offset, u64 size);
void click(HidControllerKeys btn);
void press(HidControllerKeys btn);
void release(HidControllerKeys btn);
void setStickState(int side, int dxVal, int dyVal);
