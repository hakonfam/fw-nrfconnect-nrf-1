.. _ug_bootloader_external_flash:

Using external flash memory partitions
######################################

You can use external flash memory as the storage partition for the secondary slot, using a driver for the external flash memory, like the QSPI NOR flash memory driver, that supports the following features:

* Single-byte read and write.
* Writing data from the internal flash memory to the external flash memory.

See :ref:`pm_external_flash` for general information about how to set up partitions in the external flash memory using the Partition Manager.

.. note::

   Currently, only MCUboot supports booting images stored in the external memory partition.

Both the nRF52840 DK and nRF5340 DK come with an external flash memory that can be used for the secondary slot and can be accessed using the QSPI NOR flash memory driver.
See the test in :file:`tests/modules/mcuboot/external_flash` for an example on how to enable this.
