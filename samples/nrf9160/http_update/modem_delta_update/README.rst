.. _http_application_update_sample:

nRF9160: HTTP modem delta update
################################

.. contents::
   :local:
   :depth: 2

The HTTP modem delta update sample demonstrates how to do a delta update of the modem firmware.
It uses the :ref:`lib_fota_download` library to download a file from a remote server and write it to the modem.


Overview
********

The sample connects to an HTTP server to download a signed modem delta image.
The delta images are part of the official modem firmware relases.
The sample will automatically download the correct delta image to go from base version to test version, or test version to base version, depending on the currently installed version.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

.. include:: /includes/spm.txt


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/http_update/modem_delta_update

.. include:: /includes/build_and_run.txt

The sample is built as a non-secure firmware image for the nrf9160dk_nrf9160ns build target.
Because of this, it automatically includes the :ref:`secure_partition_manager`.

Testing
=======

After programming the sample to the board, test it by performing the following steps:

1. Note the LED pattern (1 or 2 LEDs)
#. Press button 1 to download the delta for the alternative firmware version.
#. Once the download has completed, follow the reboot instructions printed to
   UART.
#. Verify that the LED pattern has changed (1 vs 2)
#. Start over from point 1 again to perform the delta back to the previous
   version.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`lib_fota_download`
  * :ref:`secure_partition_manager`

From nrfxlib
  * :ref:`nrfxlib:nrf_modem`

From Zephyr
  * :ref:`zephyr:flash_api`
  * :ref:`zephyr:logging_api`
  * :ref:`zephyr:gpio_api`
