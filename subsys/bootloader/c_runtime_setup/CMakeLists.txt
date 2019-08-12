#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
zephyr_library_named(kernel_init)
zephyr_library_sources(${ZEPHYR_BASE}/kernel/init.c)
zephyr_library_sources(zephyr_dummy_interrupt_handlers.c)
zephyr_library_compile_definitions(z_cstart=UNUSED_z_cstart)
add_dependencies(${IMAGE}kernel_init ${IMAGE}offsets_h)
