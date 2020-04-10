#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
zephyr_library()
zephyr_library_sources(bsdlib.c)
zephyr_library_sources(bsd_os.c)
zephyr_library_sources(nrf91_sockets.c)

message("Adding pm.yml for bsdlib for image ${IMAGE_NAME}")
add_partition_manager_config(pm.yml)
