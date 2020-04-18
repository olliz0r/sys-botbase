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

#include <switch.h>
#define MAX_LINE_LENGTH 344 * 32 * 2

extern u64 mainLoopSleepTime;
extern bool debugResultCodes;

int setupServerSocket();
u64 parseStringToInt(char *arg);
u8 *parseStringToByteBuffer(char *arg, u64 *size);
HidControllerKeys parseStringToButton(char *arg);
Result capsscCaptureForDebug(
    void *buffer, size_t buffer_size,
    u64 *size);