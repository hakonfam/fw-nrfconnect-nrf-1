The nRF53 SoC has some hardware properties which require special handling of the the upgrade procedure of the network core.
This section will discuss the DFU procedure on the network core on the nRF53 SoC; how it is done, and why it must be done in this way.

Motivation:
This section explains why there is a need for special handling when performing FOTA on the nRF53 network core.
The nRF53 SoC contains two cores, the application core and the network core.
Both cores share the same address space.
Both core can access the same RAM regions.
The network core can access the FLASH regions for both app and net core.
The app core can only access tho FLASH regions for the app core, it can not access the FLASH regions for the net core.
All FOTA images are banked in the application flash, for two reasons:
 - The size of the network core flash is 256kB while the application core has 1MB.
 - The logic for downloading the FOTA image resides in the application core, which only has write access to its own FLASH.
Because of this, there is a need for a mechanism which applies an update from the application flash to the network flash.
The overall solution is to let the application core download and verify the update, before it instructs the network core on how to perform the update.
The rest of this section go in to details about this solution, its limitations, and why it was chosen.

Network Core FOTA Overview:
MCUBoot in the application core is used to support FOTA of the network core.
In addition to this, the B0N (ref sample) bootloader is required on the network core.

Once MCUBoot sees a valid update, it will inspect the vector table of the image to identfiy if the update belongs on the network core or not.
If the update does not belong to the network core, MCUBoot will proceed normally, applying the update to the application core.
If the update belongs to the network core, MCUBoot will instruct the bootloader on the network core on how to apply the upgrade.
Note that the network core FOTA image is signed using the same functions and resources as the application core update.

A region of shared RAM is used to communicate the instructions,
MCUBoot will instruct the network core of the start addres, size and exepcted SHA of the update through a "update command" (ref to Doxygen).
Once the update command has been written to the shared RAM region, MCUBoot will trigger execution of the network core.
The network core bootloader (B0N) will inspect the shared RAM region, looking for an update command.
If it sees an update command, it will perform the copy as specifed, and indicate whether the SHA matches or not to the shared RAM region.
Next, the network core will signal the application core, and shut itself off.
Note that even if the copy operation is successful, the network core can not continue operations, since the application might not want to power on the network core from boot.
MCUBoot will inspect the result in the shared RAM region once the network core has signalled that it is done.
If the operation is successful, MCUBoot will write a flag to the secondary slot, indicating that the update has been performed, so that it is not re-attempetd on the next boot.
If the operation is not a success, MCUBoot will retry the operation a configurable amount of time before marking the update as NACK if it did not succeed.
The number of attempts is not retained across reboots, so if power is lost during an update, the procedure is started from scratch.
As a result of this, the solution is still power-fail safe because an aborted update would simply be re-tried.

Note that there is no self check involved in this update procedure, as there is in a normal mcuboot update procedure.
This because B0N does not perform a swap operation, it simply overwrites whatever is in the network core whit the new image.


Generating network core updates:
Update candidates for the network core is automatically generated if the parent image in the application core has 'CONFIG_MCUBOOT_BOOTOLADER=' set.






