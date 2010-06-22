/*******************************************************************************
 *
 * syscalls.s - IOS syscalls assembly file
 *
 *******************************************************************************
 *
 *
 * v1.0 - 26 July 2008				- initial release by neimod
 * v1.1 - 5 September 2008			- prepared for public release
 *
 */

	.section ".text"
	.align	4
	.arm

.macro syscall vec_sys
	.long 0xE6000010 +(\vec_sys<<5)
	bx lr
.endm
 
	.code 32
	.global os_thread_create
os_thread_create:
	syscall 0x0

 	.code 32
	.global os_get_thread_id
os_get_thread_id:
	syscall 0x3

	.code 32
	.global os_get_parent_thread_id
os_get_parent_thread_id:
	syscall 0x4

	.code 32
	.global os_thread_continue
os_thread_continue:
	syscall 0x5

	.code 32
	.global os_thread_stop
os_thread_stop:
	syscall 0x6

	.code 32
	.global os_thread_get_priority
os_thread_get_priority:
	syscall 0x8

	.code 32
	.global os_thread_set_priority
os_thread_set_priority:
	syscall 0x9

	.code 32
	.global os_message_queue_create
os_message_queue_create:
	syscall 0xA

	.code 32
	.global os_message_queue_destroy
os_message_queue_destroy:
	syscall 0xB
	
	.code 32
	.global os_message_queue_send
os_message_queue_send:
	syscall 0xC

	.code 32
	.global os_message_queue_send_now
os_message_queue_send_now:
	syscall 0xD

	.code 32
	.global os_message_queue_receive
os_message_queue_receive:
	syscall 0xE

	.global os_create_timer
os_create_timer:
	syscall 0x11

	.code 32
	.global os_restart_timer 
os_restart_timer:
	syscall 0x12

	.code 32
	.global os_stop_timer  
os_stop_timer:
	syscall 0x13

	.code 32
	.global os_destroy_timer 
os_destroy_timer:
	syscall 0x14

	.code 32
	.global os_time_now
os_time_now:
	syscall 0x15

	.code 32
	.global os_heap_create
os_heap_create:
	syscall 0x16

	.code 32
	.global os_heap_destroy
os_heap_destroy:
	syscall 0x17

	.code 32
	.global os_heap_alloc
os_heap_alloc:
	syscall 0x18

	.code 32
	.global os_heap_alloc_aligned
os_heap_alloc_aligned:
	syscall 0x19

	.code 32
	.global os_heap_free
os_heap_free:
	syscall 0x1A

	.code 32
	.global os_device_register
os_device_register:
	syscall 0x1B

	.code 32
	.global os_open
os_open:
	syscall 0x1C

	.code 32
	.global os_close
os_close:
	syscall 0x1D

	.code 32
	.global os_read
os_read:
	syscall 0x1E

	.code 32
	.global os_write
os_write:
	syscall 0x1F

	.code 32
	.global os_seek
os_seek:
	syscall 0x20

	.code 32
	.global os_ioctl
os_ioctl:
	syscall 0x21

	.code 32
	.global os_ioctlv
os_ioctlv:
	syscall 0x22
	
	.code 32
	.global os_open_async
os_open_async:
	syscall 0x23
	
	.code 32
	.global os_close_async
os_close_async:
	syscall 0x24
	
	.code 32
	.global os_read_async
os_read_async:
	syscall 0x25
	
	.code 32
	.global os_write_async
os_write_async:
	syscall 0x26
	
	.code 32
	.global os_seek_async
os_seek_async:
	syscall 0x27
	
	.code 32
	.global os_ioctl_async
os_ioctl_async:
	syscall 0x28
	
	.code 32
	.global os_ioctlv_async
os_ioctlv_async:
	syscall 0x29

	.code 32
	.global os_message_queue_ack
os_message_queue_ack:
	syscall 0x2A

	.code 32
	.global os_sync_before_read
os_sync_before_read:
	syscall 0x3F

	.code 32
	.global os_sync_after_write
os_sync_after_write:
	syscall 0x40
	
	.code 32
	.global os_debug_print
os_debug_print:
	syscall 0x4B

	.code 32
	.global os_syscall_50
os_syscall_50:
	syscall 0x50

	.code 32
	.global os_poke_gpios
os_poke_gpios:
	syscall 0x56
	
	.code 32
	.global os_create_key
os_create_key:
	syscall 0x5B
	
	.code 32
	.global os_destroy_key
os_destroy_key:
	syscall 0x5C
	
	.code 32
	.global os_bind_ecc_public_keypair
os_bind_ecc_public_keypair:
	syscall 0x5F
	
	.code 32
	.global os_calc_ecdh_shared
os_calc_ecdh_shared:
	syscall 0x61

	.code 32
	.global os_get_key
os_get_key:
	syscall 0x63
	
	.code 32
	.global os_aes_encrypt
os_aes_encrypt:
	syscall 0x69
	
	.code 32
	.global os_aes_decrypt
os_aes_decrypt:
	syscall 0x6B

	.code 32
	.global os_puts
os_puts:
	adds r1,r0,#0
	movs R0,#4
	svc 0xAB
	bx lr
	
	.pool
	.end


