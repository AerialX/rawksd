	.section ".init"
	.global _start

	.align	4
	.arm

	.EQU	ios_thread_arg, 6
	.EQU	ios_thread_priority,	0x48
	.EQU	ios_thread_stacksize, 0x2000
	

_start:	
	ldr r2, =ios_thread_stacksize
	ldr sp, =ios_thread_stack
	add sp, sp, r2

	/* Execute main program */
	mov		r0, #0						@ int argc
	mov		r1, #0						@ char *argv[]
	ldr		r3, =main
	bx		r3

	.global _FS_ioctl_ret_
	.thumb_func
_FS_ioctl_ret_:
	.code 16
	.align 2

	ldr r0, = FS_ioctl_vect
	ldr r0, [r0]
	cmp r0, #0
	beq FS_ioctl_ret
	add r2, r0, #0
	ldr r1, = FS_ret
	add r0, r4, #0
	bl FS_hook_ret

FS_ioctl_ret:
	ldr r1, = FS_ret
	ldr r1, [r1]
	pop {r2}
FS_hook_ret:
	bx r2

	.align
	.pool
	

/*******************************************************************************
 *  IOS bss section
 *
 *  This contains the module's thread stack
 *******************************************************************************
 */
	.section ".ios_bss", "a", %nobits
	
	.global ios_thread_stack  /* stack address decrease.. */
ios_thread_stack:
	.space	ios_thread_stacksize
	
	.section ".ios_info_table","ax",%progbits

/*******************************************************************************
 *  IOS info table section
 *
 *  This contains the module's loader information
 *  The stripios tool will find this, and package it nicely for the IOS system
 *******************************************************************************
 */	
	.global ios_info_table
ios_info_table:
	.long	0x0
	.long	0x28		@ numentries * 0x28
	.long	0x6	
	.long	0xB
	.long	ios_thread_arg	@ passed to thread entry func, maybe module id
	.long	0x9
	.long	_start
	.long	0x7D
	.long	ios_thread_priority
	.long	0x7E
	.long	ios_thread_stacksize
	.long	0x7F
	.long	ios_thread_stack


	.pool
	.end
