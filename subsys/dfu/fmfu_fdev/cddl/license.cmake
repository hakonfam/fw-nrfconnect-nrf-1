#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(LICENSE "\
/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

")

foreach (file ${FILES})
  file(READ ${file} SOURCE_CONTENT)
  file(WRITE ${file} "${LICENSE}${SOURCE_CONTENT}")
endforeach()
