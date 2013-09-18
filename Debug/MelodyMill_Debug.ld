/*
 * GENERATED FILE - DO NOT EDIT
 * (C) Code Red Technologies Ltd, 2008-2013
 * Generated linker script file for STM32F407VE
 * Created from generic_c.ld (vRed Suite 5 v5.2 (4 [Build 2122] [2013-04-29] ))
 * By Red Suite 5 v5.2.4 [Build 2122] [2013-04-29]  on Tue Sep 17 15:52:04 EDT 2013
 */


INCLUDE "MelodyMill_Debug_lib.ld"
INCLUDE "MelodyMill_Debug_mem.ld"

ENTRY(ResetISR)

SECTIONS
{

    /* MAIN TEXT SECTION */    
    .text : ALIGN(4)
    {
        FILL(0xff)
        __vectors_start__ = ABSOLUTE(.) ;
        KEEP(*(.isr_vector))
        
        /* Global Section Table */
        . = ALIGN(4) ;
        __section_table_start = .;
        __data_section_table = .;
        LONG(LOADADDR(.data));
        LONG(    ADDR(.data)) ;
        LONG(  SIZEOF(.data));
        LONG(LOADADDR(.data_RAM2));
        LONG(    ADDR(.data_RAM2)) ;
        LONG(  SIZEOF(.data_RAM2));
        LONG(LOADADDR(.data_RAM3));
        LONG(    ADDR(.data_RAM3)) ;
        LONG(  SIZEOF(.data_RAM3));
        __data_section_table_end = .;
        __bss_section_table = .;
        LONG(    ADDR(.bss));
        LONG(  SIZEOF(.bss));
        LONG(    ADDR(.bss_RAM2));
        LONG(  SIZEOF(.bss_RAM2));
        LONG(    ADDR(.bss_RAM3));
        LONG(  SIZEOF(.bss_RAM3));
        __bss_section_table_end = .;
        __section_table_end = . ;
        /* End of Global Section Table */
        

        *(.after_vectors*)
        
    } >MFlash512
    
    .text : ALIGN(4)    
    {
         *(.text*)
        *(.rodata .rodata.*)
        . = ALIGN(4);
        
    } > MFlash512

    /*
     * for exception handling/unwind - some Newlib functions (in common
     * with C++ and STDC++) use this. 
     */
    .ARM.extab : ALIGN(4)
    {
    	*(.ARM.extab* .gnu.linkonce.armextab.*)
    } > MFlash512
    __exidx_start = .;
    
    .ARM.exidx : ALIGN(4)
    {
    	*(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > MFlash512
    __exidx_end = .;
    
    _etext = .;
        
    
    /* DATA section for PeriphRam16 */
    .data_RAM2 : ALIGN(4)
    {
       FILL(0xff)
    	*(.data.$RAM2*)
    	*(.data.$PeriphRam16*)
       . = ALIGN(4) ;
    } > PeriphRam16 AT>MFlash512
    
    /* DATA section for Ram64 */
    .data_RAM3 : ALIGN(4)
    {
       FILL(0xff)
    	*(.data.$RAM3*)
    	*(.data.$Ram64*)
       . = ALIGN(4) ;
    } > Ram64 AT>MFlash512
    
    /* MAIN DATA SECTION */
    

    .uninit_RESERVED : ALIGN(4)
    {
        KEEP(*(.bss.$RESERVED*))
        . = ALIGN(4) ;
        _end_uninit_RESERVED = .;
    } > Ram112

	
	/* Main DATA section (Ram112) */
	.data : ALIGN(4)
	{
	   FILL(0xff)
	   _data = . ;
	   *(vtable)
	   *(.data*)
	   . = ALIGN(4) ;
	   _edata = . ;
	} > Ram112 AT>MFlash512

    /* BSS section for PeriphRam16 */
    .bss_RAM2 : ALIGN(4)
    {
    	*(.bss.$RAM2*)
    	*(.bss.$PeriphRam16*)
       . = ALIGN(4) ;
    } > PeriphRam16
    /* BSS section for Ram64 */
    .bss_RAM3 : ALIGN(4)
    {
    	*(.bss.$RAM3*)
    	*(.bss.$Ram64*)
       . = ALIGN(4) ;
    } > Ram64

    /* MAIN BSS SECTION */
    .bss : ALIGN(4)
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4) ;
        _ebss = .;
        PROVIDE(end = .);
    } > Ram112
        
    /* NOINIT section for PeriphRam16 */
    .noinit_RAM2 (NOLOAD) : ALIGN(4)
    {
    	*(.noinit.$RAM2*)
    	*(.noinit.$PeriphRam16*)
       . = ALIGN(4) ;
    } > PeriphRam16 
    /* NOINIT section for Ram64 */
    .noinit_RAM3 (NOLOAD) : ALIGN(4)
    {
    	*(.noinit.$RAM3*)
    	*(.noinit.$Ram64*)
       . = ALIGN(4) ;
    } > Ram64 
    
    /* DEFAULT NOINIT SECTION */
    .noinit (NOLOAD): ALIGN(4)
    {
        _noinit = .;
        *(.noinit*) 
         . = ALIGN(4) ;
        _end_noinit = .;
    } > Ram112
    
    PROVIDE(_pvHeapStart = .);
    PROVIDE(_vStackTop = __top_Ram112 - 0);
}
