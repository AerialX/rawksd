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

#pragma once

int WDVD_Init();
void WDVD_Close();
bool WDVD_Reset();
bool WDVD_LowClosePartition();
int WDVD_LowRead(void *buf, u32 len, u64 offset);
int WDVD_LowUnencryptedRead(void *buf, u32 len, u64 offset);
int WDVD_LowReadDiskId();
int WDVD_LowOpenPartition(u64 offset);
int WDVD_LowReadBCA(void* buffer, u32 length);
int WDVD_StopMotor(bool eject, bool kill);
bool WDVD_CheckCover();
int WDVD_EnableDVD();

int WDVD_StartLog();
