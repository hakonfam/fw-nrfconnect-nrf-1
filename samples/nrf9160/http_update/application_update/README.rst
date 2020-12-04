.. _http_application_update_sample:

nRF9160: HTTP application update
################################

.. contents::
   :local:
   :depth: 2

The HTTP application update sample demonstrates how to do a basic FOTA (firmware over-the-air) update.
It uses the :ref:`lib_fota_download` library to download a file from a remote server and write it to flash.


Overview
********

The sample connects to an HTTP server to download a signed firmware image.
The image is generated when building the sample, but you must upload it to a server and configure where it can be downloaded.
See `Specifying the image file`_ for more information.

By default, the image is saved to `MCUboot`_ bootloader secondary slot.
The downloaded image must be signed for use by MCUboot with imgtool.


Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

The sample also requires a signed firmware image that is available for download from an HTTP server.
This image is automatically generated when building the application.

.. include:: /includes/spm.txt


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/http_update/application_update`

.. include:: /includes/build_and_run.txt

The sample is built as a non-secure firmware image for the nrf9160dk_nrf9160ns build target.
Because of this, it automatically includes the :ref:`secure_partition_manager`.
The sample also uses MCUboot, which is automatically built and merged into the final HEX file when building the sample.


Specifying the image file
=========================

Before building the sample, you must specify where the image files will be located.
The sample use two update images, and selects which one to download based on the value of ``CONFIG_APPLICATION_VERSION`` (1 or 2).
If you do not want to host it yourself, you can upload it to a public S3 bucket on Amazon Web Services (AWS).
See `Setting up an AWS S3 bucket`_ for instructions.

To specify the location in |SES|:

1. Select :guilabel:`Project` -> :guilabel:`Configure nRF Connect SDK Project`.
#. Navigate to :guilabel:`HTTP application update sample` and specify the download host name (``CONFIG_DOWNLOAD_HOST``) and the files to download (``CONFIG_DOWNLOAD_FILE_V1`` and ``CONFIG_DOWNLOAD_FILE_V2``).
#. Click :guilabel:`Configure` to save the configuration.

.. include:: /includes/aws_s3_bucket.txt


.. _app_update_hosting

Hosting your image on an AWS S3 Server
--------------------------------------

1. Build the sample with ``CONFIG_APPLICATION_VERSION`` set to 1, create a copy of :file:`build/zephyr/app_update.bin` called `app_update_v1.bin` (or similar)
#. Build the sample with ``CONFIG_APPLICATION_VERSION`` set to 2, create a copy of :file:`build/zephyr/app_update.bin` called `app_update_v2.bin` (or similar)
#. Go to `AWS S3 console`_ and sign in.
#. Go to the bucket you have created.
#. Click :guilabel:`Upload` and select the two files :file:`app_update_v1.bin` and :file:`app_update_v2.bin` (created in the earlier step).
#. Click the files you uploaded in the bucket and check the :guilabel:`Object URL` field to find the download URL for the files.

When specifying the image file, use the ``<bucket-name>.s3.<region>.amazonaws.com`` part of the URL for the download hostname.
Make sure to not include the ``https``.
Specify the rest of the URL as the file names.


Testing
=======

After programming the sample to the board, test it by performing the following steps:

1. Upload the update files as specified in :ref:`app_update_hosting`.
   Then select the file :file:`update.bin` and upload it.
#. Reset your nRF9160 DK to start the application.
#. Open a terminal emulator and observe that an output similar to this is printed::

    SPM: prepare to jump to Non-Secure image
    ***** Booting Zephyr OS v1.13.99 *****

#. Observe that **LED 1** is lit.
   This indicates that version 1 of the application is running.
#. Press **Button 1** on the nRF9160 DK to start the download process and wait until "Download complete" is printed in the terminal.
#. Press the **RESET** button on the board.
   MCUboot will now detect the downloaded image and write it to flash.
   This can take up to a minute and nothing is printed in the terminal while this is processing.
#. Observe that **LED 1** and **LED 2** is lit.
   This indicates that version 2 or higher of the application is running.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`lib_fota_download`
  * :ref:`secure_partition_manager`

From nrfxlib
  * :ref:`nrfxlib:bsdlib`

From Zephyr
  * :ref:`zephyr:flash_api`
  * :ref:`zephyr:logging_api`
  * :ref:`zephyr:gpio_api`

From MCUboot
  * `MCUboot`_
