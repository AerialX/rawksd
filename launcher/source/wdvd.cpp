/*
 *  Wii DVD interface API
 *  Copyright (C) 2008 Jeff Epler <jepler@unpythonic.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <ogc/ipc.h>
#include <string.h>
#include <ogc/system.h>
#include "wdvd.h"
#include <malloc.h>

static int di_fd = -1;

static u32 inbuffer[0x10] ATTRIBUTE_ALIGN(32);

int WDVD_Init() { 
    if (di_fd >= 0)
		return 1;

    //di_fd = IOS_Open("/dev/di", 0);
	di_fd = IOS_Open("/dev/do", 0);
    
    return di_fd;
}

bool WDVD_LowClosePartition() {
	inbuffer[0x00] = 0x8C000000;
	return IOS_Ioctl(di_fd, 0x8C, inbuffer, 0x20, 0, 0);
}

bool WDVD_Reset() {
	inbuffer[0x00] = 0x8A000000;
	inbuffer[0x01] = 1;
	return IOS_Ioctl(di_fd, 0x8A, inbuffer, 0x20, inbuffer + 0x08, 0x20);
}

int WDVD_LowUnencryptedRead(void *buf, u32 len, u64 offset) {
	inbuffer[0] = 0x8d000000;
	inbuffer[1] = len;
	inbuffer[2] = offset >> 2;

	int ret = IOS_Ioctl(di_fd, 0x8d, inbuffer, 0x20, buf, len);
	
	return ret;
}


int WDVD_LowRead(void *buf, u32 len, u64 offset) {
	inbuffer[0] = 0x71000000;
	inbuffer[1] = len;
	inbuffer[2] = offset >> 2;

	return IOS_Ioctl(di_fd, 0x71, inbuffer, 0x20, buf, len);
}

int WDVD_LowReadDiskId() {
	inbuffer[0] = 0x70000000;
	return IOS_Ioctl(di_fd, 0x70, inbuffer, 0x20, (void*)0x80000000, 0x20);
}

// ugh. Don't even need this tmd shit.
static u8 tmd_data[0x49E4] ATTRIBUTE_ALIGN(32);
static u8 errorbuffer[0x20] ATTRIBUTE_ALIGN(32);
static u32 commandbuffer[0x08] ATTRIBUTE_ALIGN(32);
int WDVD_LowOpenPartition(u64 offset) {
	
	inbuffer[0] = (u32)commandbuffer;
	inbuffer[1] = 0x20;
	inbuffer[2] = 0;
	inbuffer[3] = 0x24a;
	inbuffer[4] = 0;
	inbuffer[5] = 0;
	inbuffer[6] = (u32)tmd_data;
	inbuffer[7] = 0x49E4;
	inbuffer[8] = (u32)errorbuffer;
	inbuffer[9] = 0x20;
	commandbuffer[0] = 0x8b000000;
	commandbuffer[1] = offset >> 2;

	return IOS_Ioctlv(di_fd, 0x8b, 3, 2, (ioctlv*)inbuffer);
}

void WDVD_Close() {
	IOS_Close(di_fd);
}

bool WDVD_CheckCover() {
	inbuffer[0] = 0x88000000;
	return IOS_Ioctl(di_fd, 0x88, inbuffer, 0x20, inbuffer + 0x20, 0x20);
}

int WDVD_LowReadBCA(void* buffer, u32 length)
{
	inbuffer[0] = 0xDA000000;
	return IOS_Ioctl(di_fd, 0xDA, inbuffer, 0x20, buffer, length);
}

int WDVD_StopMotor(bool eject, bool kill)
{
	inbuffer[0] = 0xE3000000;
	inbuffer[1] = eject ? 1 : 0;
	inbuffer[2] = kill ? 1 : 0;
	return IOS_Ioctl(di_fd, 0xE3, inbuffer, 0x20, inbuffer + 0x20, 0x20);
}

int WDVD_EnableDVD()
{
	// <tueidj> the function is just "int WDVD_EnableDVD()", it calls ioctl 0x8E with inbuffer[0] = 1<<24 ; 0 is disable
	inbuffer[0] = 1 << 24;
	return IOS_Ioctl(di_fd, 0x8E, inbuffer, 0x20, inbuffer + 0x20, 0x20);
}

int WDVD_StartLog()
{
	return IOS_Ioctl(di_fd, 0xCF, inbuffer, 0x20, inbuffer + 0x20, 0x20);
}
