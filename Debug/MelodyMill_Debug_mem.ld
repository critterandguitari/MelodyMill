/*
 * GENERATED FILE - DO NOT EDIT
 * (C) Code Red Technologies Ltd, 2008-2013
 * Linker script memory definitions
 * Created from LinkMemoryTemplate
 * By Red Suite 5 v5.2.4 [Build 2122] [2013-04-29]  on Thu Jul 11 05:16:40 EDT 2013)
*/

MEMORY
{
  /* Define each memory region */
  MFlash512 (rx) : ORIGIN = 0x8000000, LENGTH = 0x80000 /* 512k */
  Ram112 (rwx) : ORIGIN = 0x20000000, LENGTH = 0x1c000 /* 112k */
  PeriphRam16 (rwx) : ORIGIN = 0x2001c000, LENGTH = 0x4000 /* 16k */
  Ram64 (rwx) : ORIGIN = 0x10000000, LENGTH = 0x10000 /* 64k */

}
  /* Define a symbol for the top of each memory region */
  __top_MFlash512 = 0x8000000 + 0x80000;
  __top_Ram112 = 0x20000000 + 0x1c000;
  __top_PeriphRam16 = 0x2001c000 + 0x4000;
  __top_Ram64 = 0x10000000 + 0x10000;

