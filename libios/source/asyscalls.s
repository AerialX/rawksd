	.section ".text"
	.align	4
	.arm

.macro syscall number, name
	.code 32
	.global \name
	.type \name STT_FUNC
\name:
	.long 0xE6000010 +(\number<<5)
	bx lr
.endm

syscall 0x00 os_thread_create
syscall 0x03 os_get_thread_id
syscall 0x04 os_get_parent_thread_id
syscall 0x05 os_thread_continue
syscall 0x06 os_thread_stop
syscall 0x07 os_thread_yield
syscall 0x08 os_thread_get_priority
syscall 0x09 os_thread_set_priority
syscall 0x0A os_message_queue_create
syscall 0x0B os_message_queue_destroy
syscall 0x0C os_message_queue_send
syscall 0x0D os_message_queue_send_now
syscall 0x0E os_message_queue_receive
syscall 0x11 os_create_timer
syscall 0x12 os_restart_timer
syscall 0x13 os_stop_timer
syscall 0x14 os_destroy_timer
syscall 0x15 os_time_now
syscall 0x16 os_heap_create
syscall 0x17 os_heap_destroy
syscall 0x18 os_heap_alloc
syscall 0x19 os_heap_alloc_aligned
syscall 0x1A os_heap_free
syscall 0x1B os_device_register
syscall 0x1C os_open
syscall 0x1D os_close
syscall 0x1E os_read
syscall 0x1F os_write
syscall 0x20 os_seek
syscall 0x21 os_ioctl
syscall 0x22 os_ioctlv
syscall 0x23 os_open_async
syscall 0x24 os_close_async
syscall 0x25 os_read_async
syscall 0x26 os_write_async
syscall 0x27 os_seek_async
syscall 0x28 os_ioctl_async
syscall 0x29 os_ioctlv_async
syscall 0x2A os_message_queue_ack
syscall 0x3F os_sync_before_read
syscall 0x40 os_sync_after_write
syscall 0x4B os_debug_print
syscall 0x50 os_syscall_50
syscall 0x56 os_poke_gpios
syscall 0x5B os_create_key
syscall 0x5C os_destroy_key
syscall 0x5D os_init_key
syscall 0x5F os_set_sig_info
syscall 0x61 os_calc_ecdh_shared
syscall 0x63 os_get_4byte_key
syscall 0x69 os_aes_encrypt
syscall 0x6B os_aes_decrypt
syscall 0x77 os_crash

	.code 32
	.global os_puts
	.type os_puts STT_FUNC
os_puts:
	adds r1,r0,#0
	movs R0,#4
	svc 0xAB
	bx lr
	
	.pool
	.end


