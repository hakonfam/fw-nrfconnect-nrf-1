.. _lib_dfu_ctx_mcuboot:

DFU Context Handler
###################

The DFU Context Handler library provides a common API for different types of firmware upgardes.
It is used to allow components dealing with firmware updates to only operate against a single interface.


Supported variants
******************
The supported variants must implement a set of functions defined in :file:`subsys/dfu/src/dfu_context_handler.c`.
Currently supported variants are listed below:

Modem
====
modemlol


MCUBoot style
=============
mcuboot styyle

API documentation
*****************

| Header file: :file:`subsys/dfu/include/dfu_ctx_mcuboot.h`
| Source file: :file:`subsys/dfu/src/dfu_ctx_mcuboot.c`

.. doxygengroup:: dfu_ctx_mcuboot
   :project: nrf
   :members:
