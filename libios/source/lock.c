#include "lock.h"
#include "syscalls.h"

#define LOCK_MSG 0x5AFEC0DE

lock_t InitializeLock(void *ptr, u32 initial, u32 max) {
	u32 i;
	osqueue_t queue = os_message_queue_create(ptr, max);
	if (queue >= 0) {
		for (i=0; i < max-initial; i++)
			os_message_queue_send(queue, LOCK_MSG, 0);
	}

	return queue;
}

void GetLock(lock_t lock) {
	u32 msg = 0;
	do {
		if (os_message_queue_receive(lock, &msg, 0))
			break;
	} while (msg != LOCK_MSG);
}

void ReleaseLock(lock_t lock) {
	os_message_queue_send(lock, LOCK_MSG, 0);
}

void DestroyLock(lock_t lock) {
	os_message_queue_destroy(lock);
}
