.. _lib_fmfu_fdev:

Full modem firmware update from flash device
############################################

.. contents::
   :local:
   :depth: 2

The fmfu_fdev library provides an API for applying a full modem firmware update from a flash device.

Functionality
*************
The modem firmware and metadata stored in the flash device must confirm to a specific serialzation format, see :ref:`lib_dfu_target_full_modem_update`.
The serialized modem firmware contains the hash of the firmware and a signature.
These fields are used to prevalidate of the modem firmware before it is written to the modem, ensuring that the data written corresponds to the data being signed.
Once the modem firmware is prevalidated it is written to the modem using the :file:`nrf_fmfu.h` API.

Serialization
*************
The modem firmware is serialized using the following CDDL scheme.

.. literalinclude:: ../../subsys/dfu/fmfu_fdev/cddl/modem_update.cddl
    :language: c


A generated decoder (:file:`modem_update_decode.c`) is used for parsing data serialized with this format.

Prerequisites
*************
The serialized modem firmware must be stored in the flash device.


The modem library must be de-initialized.


API documentation
*****************

| Header file: :file:`include/dfu/fmfu_fdev.h`
| Source files: :file:`subsys/dfu/fmfu_fdev/src/`

.. doxygengroup:: fmfu_fdev
   :project: nrf
   :members:
