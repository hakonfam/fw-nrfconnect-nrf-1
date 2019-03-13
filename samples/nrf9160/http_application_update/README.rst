.. _http_application_update_sample:

nRF9160: HTTP application update
################################

Th HTTP application update sample demonstrates how to do a basic FOTA (firmware over-the-air) update.
It uses the :ref:`lib_download_client` library to download a file from a remote server and writes it to flash.


Overview
********

The sample connects to an HTTP server to download a firmware image.
You must provide the image file and configure where it can be downloaded.
See `Specifying the image file`_ for more information.

By default, the image is saved to slot 1 (see :ref:`ug_bootloader_architecture`).
The downloaded file is not checked or authenticated, and no further processing is done.



Requirements
************

* The following development board:

  * nRF9160 DK board (PCA10090)

* :ref:`secure_boot` must be programmed on the board.

  The sample is configured to compile and run as a non-secure application on nRF91's Cortex-M33.
  Therefore, it requires the :ref:`secure_boot` bootloader that prepares the required peripherals to be available for the application.
* A firmware image that is available for download from an HTTP server.


Building and running
********************

This sample can be found under :file:`samples/nrf9160/http_application_update` in the |NCS| folder structure.

The sample is built as a non-secure firmware image for the nrf9160_pca10090ns board.
It can be programmed independently from the secure boot firmware.

See :ref:`gs_programming` for information about how to build and program the application.
Note that you must program two different applications as described in `Programming the sample`_.


Specifying the image file
=========================

Before building the sample, you must specify the location of the image file that should be downloaded.

To do so, open the :file:`prj.cnf` file of the sample and add the name of your server and the file to be downloaded::

   CONFIG_HOST="your.server.com"
   CONFIG_RESOURCE="filename.bin"


Programming the sample
======================

When you connect the nRF9160 DK board to your computer, you must first make sure that the :ref:`secure_boot` sample is programmed:

1. Put the **SW5** switch (marked debug/prog) in the **NRF91** position to program the main controller.
#. Build the :ref:`secure_boot` sample for the nrf9160_pca10090 board and program it.
#. Build the HTTP application update sample (this sample) for the nrf9160_pca10090ns board and program it.
#. Verify that the sample was programmed successfully by connecting to the serial port with a terminal emulator (for example, PuTTY) and checking the output.
   See :ref:`putty` for the required settings.


Testing
=======

After programming the sample and all prerequisites to the board, test it by performing the following steps:

1. Reset your nRF9160 DK to start the application.
#. Open a teminal emulator and observe that the kit prints the following information::

   TBD

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`lib_download_client`

From nrfxlib
  * :ref:`nrfxlib:bsdlib`

From Zephyr
  * :ref:`zephyr:flash_interface`
  * :ref:`zephyr:logger`
  * :ref:`zephyr:gpio`
  * ``dfu/mcuboot.h``

In addition, it uses the following samples:

From |NCS|
  * :ref:`secure_boot`
