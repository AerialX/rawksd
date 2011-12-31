OUTPUT_FORMAT("elf32-bigarm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
 
__ram_end__                     = 0x13788000;

MEMORY {
	ios_info_table	: ORIGIN = 0x0, LENGTH = 0xD4
	exe(rwx)	: ORIGIN = 0x13740000, LENGTH = 0x8000
	ram(rw)		: ORIGIN = 0x13748000, LENGTH = 0x40000
}
 
SECTIONS
{
    .debug_aranges 0 : { *(.debug_aranges) } .debug_pubnames 0 : { *(.debug_pubnames) } .debug_info 0 : { *(.debug_info) } .debug_abbrev 0 : { *(.debug_abbrev) } .debug_line 0 : { *(.debug_line) } .debug_frame 0 : { *(.debug_frame) } .debug_str 0 : { *(.debug_str) } .debug_loc 0 : { *(.debug_loc) } .debug_macinfo 0 : { *(.debug_macinfo) } .note.arm.ident 0 : { KEEP (*(.note.arm.ident)) }
    .ios_info_table 0 :  {
		. = .;
		KEEP (*(.ios_info_table))
	} > ios_info_table
    .init : {
		. = .;
		KEEP (*(.init))
	} > exe
    .text ALIGN (0x4) : { 
    	*(.text*) 
    	*(.gnu.warning) 
    	*(.gnu.linkonce.t.*) 
    	*(.init) 
    	*(.glue_7) 
    	*(.glue_7t)
	} > exe
    .data : {
		KEEP( *(.ios_data) )
		*(.data*)
		*(.data1)
		*(.gnu.linkonce.d.*)
		. = ALIGN (4);
		__CTOR_LIST__ = ABSOLUTE (.);
		KEEP (*(SORT (.ctors*)))
		__CTOR_END__ = ABSOLUTE (.);
		__DTOR_LIST__ = ABSOLUTE (.);
		KEEP (*(SORT (.dtors*)))
		__DTOR_END__ = ABSOLUTE (.);
		*(.dynamic)
		*(.sdata*)
		*(.gnu.linkonce.s.*)
		. = ALIGN (4);
		*(.2ram.*)
	} > ram 
    .fini : {
		. = .;
		*(.fini)
	} > ram
     .rodata ALIGN (0x4) : { 
    	. = .; 
    	*(.rodata*) 
    	*(.gnu.linkonce.r.*)
	} > ram
    .rodata1 ALIGN (0x4) : {
		. = .; *(.rodata1)
	} > ram
    .fixup ALIGN (0x4) : {
		. = .; *(.fixup)
	} > ram
    .gcc_except_table ALIGN (0x4) : {
		. = .;
		*(.gcc_except_table)
	} > ram
    .got ALIGN (0x4) : {
		*(.got.plt)
		*(.got)
	} > ram
    .bss ALIGN (0x4) : {
		*(.scommon)
		*(.dynsbss)
		*(.sbss*)
		*(.gnu.linkonce.sb.*)
		*(.dynbss)
		*(.bss*)
		*(.gnu.linkonce.b.*)
		*(COMMON)
		KEEP( *(.ios_bss) )
		. = ALIGN (8);
		PROVIDE(HEAP_START = .);
		. = ABSOLUTE(__ram_end__);
	} > ram    
}

PROVIDE(HEAP_END = __ram_end__);