.. _nc_bootloader:

nRF5340 Network core  bootloader
################################

This bootloader sample implements an immutable first stage bootloader that has the capability to download update the application firmware on the network core.
It also performs flash protection of both itself an the application.

Overview
********

The network core bootloader sample provides a scheme for transporting an
already verified and authenticated firmware upgrade from the application core
flash to the network core flash, as well as performing flash protection.

This is accomplished by the following steps:

1. Lock the flash.
     The bootloader sample locks the flash that contains the sample bootloader and its configuration.
     Locking is done using the ACL peripheral.
     For details on locking, see the :ref:`fprotect_readme` driver.

#. Verify the next stage in the boot chain.
     After selecting the image to be booted next, the bootloader sample verifies its validity using one of the provisioned public keys hashes.
     The image for the next boot stage has a set of metadata associated with it.
     This metadata contains the full public key corresponding to the private key that was used to sign the firmware.
     The bootloader sample checks the public key against a set of provisioned keys.
     Note that to save space, only the hashes of the provisioned keys are stored, and only the hashes of the keys are compared.
     If the public key in the metadata matches one of the valid provisioned public key hashes, the image is considered valid.
     All public key hashes at lower indices than the matching hash are permanently invalidated at this point, which means that images can no longer be validated with those public keys.
     For example, if an image is successfully validated with the public key at index 2, the public keys 0 and 1 are invalidated.
     This mechanism can be used to decommission broken keys.
     If the public key does not match any of the still valid provisioned hashes, validation fails.

  #. Boot the application in the network core.
       After possibly performing a firmware update, and enabling flash protection, the network core bootloader uninitializes all peripherals that it used and boots the application.

Provisioning
============

The public key hashes are not compiled in with the source code of the bootloader sample.
Instead, they must be stored in a separate memory region through a process called *provisioning*.

By default, the bootloader sample will automatically generate and provision public key hashes directly into the bootloader HEX file, based on the specified private key and additional public keys.
Alternatively, to facilitate the manufacturing process of a device with the bootloader sample, it is possible to decouple this process and program the sample HEX file and the HEX file containing the public key hashes separately.
If you choose to do so, use the Python scripts in ``scripts\bootloader`` to create and provision the keys manually.

   .. note::
      On nRF9160, the provisioning data is held in the OTP region in UICR.
      Because of this, you must erase the UICR before programming the bootloader.
      On nRF9160, the UICR can only be erased by erasing the whole chip.
      To do so on the command line, call ``west flash`` with the ``--erase`` option.
      This will erase the whole chip before programming the new image.
      In |SES|, choose :guilabel:`Target` > :guilabel:`Connect J-Link` and then :guilabel:`Target` > :guilabel:`Erase All` to erase the whole chip.


Requirements
************


  * |nRF5340DK|

.. _bootloader_build_and_run:

Building and running
********************

The source code of the sample can be found under :file:`samples/nrf5340/netboot/` in the |NCS| folder structure.

The most common use case for the bootloader sample is to be included as a child image in a multi-image build, rather than being built stand-alone.
TODO explain that you can set SECURE_BOOT=y for the network core application.
This sample is included automatically if the application in the nrf5340 application core has the :option:`CONFIG_BOOTLOADER_MCUBOOT` option set.

Testing
=======

To test the network core bootloader sample, enable :option:`CONFIG_BOOTLOADER_MCUBOOT` in the application core :file:`prj.conf`.
Then test it by performing the following steps:

#. |connect_terminal|
#. Reset the board.
#. Observe that the application starts as expected.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`doc_fw_info`
* :ref:`fprotect_readme`
* ``include/bl_validation.h``
* ``include/bl_crypto.h``
* ``subsys/bootloader/include/provision.h``

The sample also uses drivers from nrfx.
