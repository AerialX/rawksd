#include <ppc-asm.h>

#define sr0		0
#define sr1		1
#define sr2		2
#define sr3		3
#define sr4		4
#define sr5		5
#define sr6		6
#define sr7		7
#define sr8		8
#define sr9		9
#define sr10		10
#define sr11		11
#define sr12		12
#define sr13		13
#define sr14		14
#define sr15		15

#define hid0		1008

#define	ibat0U		528
#define	ibat0L		529	
#define	ibat1U		530	
#define	ibat1L		531	
#define	ibat2U		532	
#define	ibat2L		533	
#define	ibat3U		534	
#define	ibat4U		560
#define	ibat4L		561
#define	ibat5U		562

#define	dbat0U		536
#define	dbat0L		537	
#define	dbat1U		538	
#define	dbat1L		539	
#define	dbat2U		540	
#define	dbat2L		541	
#define	dbat3U		542	
#define dbat4U		568
#define dbat4L		569
#define dbat5U		570

#define MSR_DR		0x00000010
#define MSR_IR		0x00000020

	.text
	.section .stub

	.extern _start
	.globl _stub_entry
_stub_entry:
	lis     sp, 0x0011
	ori     sp, sp, 0xCC64
	isync
	mtspr hid0, sp
	isync
	li      r4, 0x2000
	mtmsr   r4
	isync
	
	li      r0, 0
	mtspr ibat0U, r0
	mtspr ibat1U, r0
	mtspr ibat2U, r0
	mtspr ibat3U, r0
	mtspr ibat4U, r0
	mtspr ibat5U, r0
	mtspr dbat0U, r0
	mtspr dbat1U, r0
	mtspr dbat2U, r0
	mtspr dbat3U, r0
	mtspr dbat4U, r0
	mtspr dbat5U, r0
	isync
	
	lis     r4, 0x8000
	mtsr   sr0, r4
	mtsr   sr1, r4
	mtsr   sr2, r4
	mtsr   sr3, r4
	mtsr   sr4, r4
	mtsr   sr5, r4
	mtsr   sr6, r4
	mtsr   sr7, r4
	mtsr   sr8, r4
	mtsr   sr9, r4
	mtsr  sr10, r4
	mtsr  sr11, r4
	mtsr  sr12, r4
	mtsr  sr13, r4
	mtsr  sr14, r4
	mtsr  sr15, r4
	isync
	
	li      r4, 2
	lis     r3, 0x8000
	addi    r3, r3, 0x1FFF
	isync
	mtspr dbat0L, r4
	mtspr dbat0U, r3
	isync
	mtspr ibat0L, r4
	mtspr ibat0U, r3
	isync
	
	addis   r4, r4, 0x1000  #0x10000002
	addis   r3, r3, 0x1000  #0x90001FFF
	isync
	mtspr dbat4L, r4
	mtspr dbat4U, r3
	isync
	mtspr ibat4L, r4
	mtspr ibat4U, r3
	isync

	li      r4, 0x2A
	addis   r3, r3, 0x3000  #0xC0001FFF
	isync
	mtspr dbat1L, r4
	mtspr dbat1U, r3
	isync
	mtspr ibat1L, r4
	mtspr ibat1U, r3
	isync
	
	addis   r4, r4, 0x1000  #0x1000002A
	addis   r3, r3, 0x1000  #0xD0001FFF
	isync
	mtspr dbat2L, r4
	mtspr dbat2U, r3
	isync
	mtspr ibat2L, r4
	mtspr ibat2U, r3
	isync
	
	mfmsr   r3
	ori     r3, r3, MSR_DR|MSR_IR
	mtsrr1  r3
	lis     r3, _start@h
	ori     r3, r3, _start@l
	mtsrr0  r3
	rfi
	