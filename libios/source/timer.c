/*
	Custom IOS Module (FAT)

	Copyright (C) 2008 neimod.
	Copyright (C) 2009 WiiGator.
	Copyright (C) 2009 Waninkoko.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "syscalls.h"
#include "gctypes.h"
#include "mem.h"

#define TIMER_MSGS 8

static osqueue_t queuehandle = -1;
static ostimer_t timerId     = -1;

void Timer_Init(void)
{
	void *queuespace = NULL;

	if (queuehandle >= 0)
		return;

	queuespace = Alloc(TIMER_MSGS*sizeof(u32));
	if (queuespace==NULL)
		return;

	queuehandle = os_message_queue_create(queuespace, TIMER_MSGS);

	timerId = os_create_timer(0, 0, queuehandle, 0x666);

	os_stop_timer(timerId);
}

void Timer_Sleep(u32 time)
{
	u32 message;

	// not initialized
	if (timerId<0 || queuehandle<0)
		return;

	os_message_queue_send(queuehandle, 0x555, 0);

	os_restart_timer(timerId, time, 0);

	while (1) {
		os_message_queue_receive(queuehandle, &message, 0);

		if (message == 0x555)
			break;
	}

	os_message_queue_receive(queuehandle, &message, 0);

	os_stop_timer(timerId);
}
