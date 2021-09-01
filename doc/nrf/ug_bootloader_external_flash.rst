.. _ug_bootloader_external_flash:

Using external flash memory partitions
######################################

You can use external flash memory as the storage partition for the secondary slot.
This requires a driver for the external flash memory that supports the following features:

* Single-byte read and write.
* Writing data from the internal flash memory to the external flash memory.

See :ref:`pm_external_flash` for general information about how to set up partitions in the external flash memory using the Partition Manager.

.. note::

   Currently, only MCUboot supports booting images stored in the external memory partition.

To use the external flash memory as storage for the secondary partition, you must set one additional configuration:

* :kconfig:`CONFIG_PM_PARTITION_SIZE_MCUBOOT_SECONDARY` - it specifies the size of the secondary partition.

You have to manually configure the value of :kconfig:`CONFIG_PM_PARTITION_SIZE_MCUBOOT_SECONDARY` to match the value of the automatically deduced size of the primary partition.
CMake gives a warning when the values do not match, and it also provides the correct value.

.. note::

    Currently, you must manually synchronize the value of :kconfig:`CONFIG_PM_PARTITION_SIZE_MCUBOOT_SECONDARY` by setting it for both the parent image and the MCUboot child image.

Both the nRF52840 DK and nRF5340 DK come with an external flash memory that can be used for the secondary slot and can be accessed using the QSPI NOR flash memory driver.
See the test in ``tests/modules/mcuboot/external_flash`` for an example on how to enable this.

