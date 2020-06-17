Due to the small amount of available flash for the Recovery Bootloader,
some optimization in MDK has been made. This optimization removes all 
FTPAN fixes from the system_nrf52 file and removes unused interrupts 
from the interrupts vector table stored inside startup file. 

For GCC toolchain, use pregenerated Makefile and linker file, which use
custom MDK files. For Keil and IAR projects, genproject generates projects
with default startup and system files. 

These files needs to be replaced manualy, otherwise code will not fit the flash page:
- find toolchain - specific startup file in custom_mdk directory,
- replace startup and system files in project with custom MDK files.
- compile project, after changes it should fit into one flash page.

