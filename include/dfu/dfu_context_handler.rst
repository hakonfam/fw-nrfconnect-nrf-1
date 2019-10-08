.. _lib_dfu_context_handler:

DFU Context Handler
###################

The DFU Context Handler library provides a common API for different types of firmware upgardes.
It is used to allow components dealing with firmware updates to only operate against a single interface.


Supported variants
******************
The supported variants must implement a set of functions defined in :file:`subsys/dfu/src/dfu_context_handler.c`.
Currently supported variants are listed below:
:ref:`lib_dfu_ctx_mcuboot`
:ref:`lib_dfu_ctx_modem`

API documentation
*****************

| Header file: :file:`include/dfu/dfu_context_handler.h`
| Source files: :file:`subsys/dfu/src/`

.. doxygengroup:: dfu_ctx_handler
   :project: nrf
   :members:
