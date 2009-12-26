#pragma once

#include <gctypes.h>
#include "ipc.h"

#ifdef __cplusplus
	extern "C" {
#endif

/*  0 */ u32  os_thread_create( u32 (*entry)(void* arg), void* arg, void* stack, u32 stacksize, u32 priority, s32 autostart);

/*  3 */ int   os_get_thread_id(void);
/*  4 */ int   os_get_parent_thread_id(void);
/*  5 */ int   os_thread_continue(int id);
/*  6 */ int   os_thread_stop(int id);

/*  8 */ int   os_thread_get_priority(void);
/*  9 */ void  os_thread_set_priority(int id, u32 priority);
/*  A */ u32   os_message_queue_create(void* ptr, u32 id);

/*  C */ int   os_message_queue_send(int queue, u32 message, int flags);
/*  D */ int   os_message_queue_send_now(int queue, u32 message, int flags);
/*  E */ u32   os_message_queue_receive(u32 queue, u32* message, u32 flags);
/*  F */ int   os_register_event_handler(int device, int queue, int message);
/* 10 */ int   os_unregister_event_handler(int device);
/* 11 */ s32   os_create_timer(s32 time_us, s32 repeat_time_us, s32 message_queue, s32 message);
/* 12 */ s32   os_restart_timer(s32 timer_id, s32 time_us);
/* 13 */ s32   os_stop_timer(s32 timer_id);
/* 14 */ s32   os_destroy_timer(s32 time_id);
/* 15 */ s32   os_timer_now(s32 time_id);
/* 16 */ u32   os_heap_create(void* ptr, u32 size);
/* 17 */ u32   os_heap_destroy(u32 heap);
/* 18 */ void* os_heap_alloc(u32 heap, u32 size);
/* 19 */ void* os_heap_alloc_aligned(int heap, int size, int align);
/* 1A */ void  os_heap_free(u32 heap, void* ptr);
/* 1B */ u32   os_device_register(const char* devicename, u32 queuehandle);
/* 1C */ s32   os_open(const char* device, s32 mode);
/* 1D */ s32   os_close(s32 fd);
/* 1E */ s32   os_read(s32 fd, void* buffer, s32 length);
/* 1F */ s32   os_write(s32 fd, const void* buffer, s32 length);
/* 20 */ s32   os_seek(s32 fd, s32 where, s32 whence);
/* 21 */ s32   os_ioctl(s32 fd, s32 request, void* buffer_in, s32 bytes_in, void* buffer_io, s32 bytes_io);
/* 22 */ s32   os_ioctlv(s32 fd, s32 request, s32 bytes_in, s32 bytes_out, ioctlv* vector);

/* 2A */ void  os_message_queue_ack(void* message, s32 result);

/* 34 */ int   os_software_IRQ(int dev);

/* 3F */ void  os_sync_before_read(void* ptr, u32 size);
/* 40 */ void  os_sync_after_write(const void* ptr, u32 size);

/* 50 */ void  os_syscall_50(u32 unknown);

/* 56 */ void  os_poke_gpios(u32 reg, u32 value);

/* 63 */ int   os_get_key(int keyid, void* buffer);

/* 69 */ int   os_aes_encrypt(int keyid, void *iv, void *in, s32 len, void *out);

/* 6B */ int   os_aes_decrypt(int keyid, void *iv, void *in, s32 len, void *out);

s32 IOS_IoctlvFormat(s32 fd, s32 ioctl, const char *format, ...);

#ifdef __cplusplus
   }
#endif
