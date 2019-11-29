#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

get_domain(${BOARD} domain)

if(CONFIG_ARM_FIRMWARE_USES_SECURE_ENTRY_FUNCS)
  zephyr_sources(secure_services_ns.c)

  add_dependencies(zephyr_interface ${domain}_spm_subimage)
endif()
