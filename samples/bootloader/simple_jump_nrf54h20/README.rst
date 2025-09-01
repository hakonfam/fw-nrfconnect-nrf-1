.. _simple_jump_bootloader_nrf54h20:

Simple Jump Bootloader for nRF54H20
####################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates a minimal bootloader for the nRF54H20 that performs a simple jump to another firmware image located at a fixed address (0x0e102000).

Overview
********

The simple jump bootloader is designed to:

* Perform basic validation of the target firmware image
* Cleanly transfer execution to the target image
* Provide console output for debugging during the boot process

The bootloader validates the target image by checking:

* Stack pointer appears to be in a valid RAM range
* Reset vector has the Thumb bit set (ARM Cortex-M requirement)
* Reset vector points to a reasonable flash address

Requirements
************

* nRF54H20 development kit
* A valid firmware image programmed at address 0x0e102000

Wiring
******

No additional wiring is required. The sample uses the default console UART pins.

Building and Running
********************

This sample can be found under :file:`samples/bootloader/simple_jump_nrf54h20` in the |NCS| folder structure.

To build the sample, run the following command:

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp

To flash the bootloader to your development kit, run:

.. code-block:: console

   west flash

Sample Output
*************

The following output is printed on the console when the bootloader starts:

.. code-block:: console

   === Simple Jump Bootloader for nRF54H20 ===
   Bootloader started
   Target image address: 0x0e102000
   Target image validation passed
   Preparing to jump...
   Jumping to image at address 0x0e102000
   Stack pointer: 0x2003ff00
   Reset vector: 0x0e102401

If no valid image is found at the target address, the bootloader will output an error and wait:

.. code-block:: console

   === Simple Jump Bootloader for nRF54H20 ===
   Bootloader started
   Target image address: 0x0e102000
   Invalid stack pointer: 0xffffffff
   ERROR: Target image validation failed!
   Bootloader will not jump to invalid image.
   Waiting...

Customization
*************

Target Address
==============

To change the target image address, modify the ``TARGET_IMAGE_ADDRESS`` define in :file:`src/main.c`:

.. code-block:: c

   #define TARGET_IMAGE_ADDRESS    0x0e102000  // Change this address

Validation Checks
=================

Additional validation checks can be added to the ``validate_image()`` function, such as:

* CRC verification
* Digital signature validation
* Magic number checks
* Version validation

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`zephyr:kernel_api`
* :ref:`zephyr:console_api`

Limitations
***********

* The target address is fixed at compile time
* Only basic validation is performed
* No rollback or recovery mechanism
* Assumes the target image follows standard ARM Cortex-M vector table format
