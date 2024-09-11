##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

#
# Shell can be useful for debugging but should not be enabled for production.
# Default can be changed on command line with -D FLAG=VALUE.
#
!if $(SECURE_BOOT_ENABLE) == TRUE
  DEFINE BUILD_SHELL             = FALSE
!else
  DEFINE BUILD_SHELL             = TRUE
!endif
