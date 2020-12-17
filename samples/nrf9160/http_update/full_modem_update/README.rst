.. _http_full_modem_update_sample:

nRF9160: HTTP full modem update
###############################

.. contents::
   :local:
   :depth: 2

The HTTP full modem update sample demonstrates how to perform a full firmware update of the modem (as opposed to a delta update).
The sample downloads a modem firmware signed by Nordic and performs the firmware update of the modem.

Overview
********

An external flash with minimum 2MB of free space is required to perform a full modem update.
Hence, only version 0.14.0 and later of the nrf9160DK support this sample as the prior versions do not have external flash.

The sample connects to an HTTP server to download a signed modem firmware.
It uses the :ref:`lib_fota_download` library to download the modem firmware from a remote server and writes it to the external flash.
After the modem firmware has been stored in external flash, the :ref:`lib_fmfu_fdev` library is used to prevalidate the update and write it to the modem.
Two different versions can be downloaded, depending on what version is currently installed.
The version which is not currently installed is selected.

Requirements
************

The sample supports the following development kit, version 0.14.0 or higher:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

.. include:: /includes/spm.txt


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/application_update/full_modem_update`

.. include:: /includes/build_and_run.txt

The sample is built as a non-secure firmware image for the ``nrf9160dk_nrf9160ns`` build target.
Because of this, it automatically includes the :ref:`secure_partition_manager`.

Testing
=======

After programming the sample to the board, test it by performing the following steps:

1. Start the application and wait for a prompt for pressing a button.
#. Press the button to start the update procedure.
#. Once the download is completed, the modem update procedure will begin automatically.
#. Reset the board.
#. Observe that the LED pattern has changed.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`lib_fota_download`
  * :ref:`lib_dfu_target`
  * :ref:`lib_dfu_target_stream`
  * :ref:`lib_dfu_target_full_modem_update`
  * :ref:`lib_fmfu_fdev`
  * :ref:`secure_partition_manager`

From nrfxlib
  * :ref:`nrfxlib:nrf_modem`

From Zephyr
  * :ref:`zephyr:flash_api`
  * :ref:`zephyr:stream_api`
  * :ref:`zephyr:logging_api`
  * :ref:`zephyr:gpio_api`
