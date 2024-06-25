.. platform_key_loader

Platform Key Loader
###################

.. contents::
   :local:
   :depth: 2

The Platform Key Loader (PKL) is an application that interfaces with the Secure Domain Firmware (SDFW) to provision platform keys.

Motivation
==========

The Secure Domain Firmware (SDFW) is responsible for handling platform keys on behalf of local domains.
These keys are stored in a manner that makes it hard to serialize in build time.
Hence, all platform keys must be provisioned in run time.

Operation
=========

All platform keys to install are included in the image binary for the PKL at build time.
When provisioning the device, the PKL image and its associated assets must be programmed and executed.

The PKL triggers a ``psa_import_key`` operation for all configured keys.
Each imported key is verified by using it in a cryptographic operation.

Building the PKL
----------------

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
