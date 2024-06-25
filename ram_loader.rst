.. _sdfw_ramloader:

Local Domain Key Loader
#######################

.. contents::
   :local:
   :depth: 2

The Local Domain Key Loader (LDKL) is an application that interfaces with the Secure Domain Firmware (SDFW) to provision local domain keys to Haltium devices.

Motivation
==========

The SDFW is responsible for handling keys on behalf of local domains.
These local domain keys are stored in a manner that makes it hard to serialize in build time.
Hence, all keys handled by the SDFW must be provisioned in run time.

Operation
=========

All local domain keys to install are included in the image binary for the LDKL at build time.
When provisioning the device, the LDKL can be executed from either volatile memory or non-volatile memory.
This choice is made when compiling the LDKL application.
The UICR corresponding to the LDKL must be programmed for the LDKL to be able to execute.

The LDKL triggers a ``psa_import_key`` operation for all configured keys.
Each imported key is verified by using it in a cryptographic operation.
If the verification fails, a known value is written to a static offset of the chosen local domain TCM.

Building the LDKL
-----------------

Enable the option ``CONFIG_LDKL`` when building an NCS sample to build the LDKL.
This will include the ``sysbuild`` image ``ldkl``.
Configure this sample to select what keys to provision, and if it should be executed from volatile or non-volatile memory.

The build target determines what local domain the LDKL will be executed from.
This does not limit what local domains the LDKL can provisions keys for, that is only limited by the Local Domain Life Cycle State (LDLCS).

Running the LDKL
----------------

Programming the LDKL will result in some assets being overwritten.
These assets must be stored before and restored after LDKL execution for them not to be lost.
The local domain UICR and MRAM image are examples of such assets.

All local domains being provisioned must be in life cycle state ``PRoT Provisioning`` for the LDKL to be able to install the keys.
