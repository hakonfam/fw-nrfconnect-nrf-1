/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x7f000, LENGTH = 0x1000
  RAM (rwx) :  ORIGIN = 0x20000000, LENGTH = 0x10000
  uicr_pinreset_start_address (r) : ORIGIN = 0x10001200, LENGTH = 0x8
}

SECTIONS
{
  .uicr_pinreset_start_address :
  {
    PROVIDE(__start_uicr_pinreset_start_address = .);
    KEEP(*(SORT(.uicr_pinreset_start_address*)))
    PROVIDE(__stop_uicr_pinreset_start_address = .);
  } > uicr_pinreset_start_address
}

SECTIONS
{
  . = ALIGN(4);
  .mem_section_dummy_ram :
  {
  }
  .log_dynamic_data :
  {
    PROVIDE(__start_log_dynamic_data = .);
    KEEP(*(SORT(.log_dynamic_data*)))
    PROVIDE(__stop_log_dynamic_data = .);
  } > RAM

} INSERT AFTER .data;

SECTIONS
{
  .mem_section_dummy_rom :
  {
  }
  .log_const_data :
  {
    PROVIDE(__start_log_const_data = .);
    KEEP(*(SORT(.log_const_data*)))
    PROVIDE(__stop_log_const_data = .);
  } > FLASH
    .nrf_balloc :
  {
    PROVIDE(__start_nrf_balloc = .);
    KEEP(*(.nrf_balloc))
    PROVIDE(__stop_nrf_balloc = .);
  } > FLASH

} INSERT AFTER .text

INCLUDE "nrf5x_common.ld"
