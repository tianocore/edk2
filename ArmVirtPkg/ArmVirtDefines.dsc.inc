#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

[Defines]

  #
  # Shell can be useful for debugging but should not be enabled for production
  #
  DEFINE BUILD_SHELL             = TRUE

  DEFINE DEBUG_PRINT_ERROR_LEVEL = 0x8000004F

# Dynamic stack cookies pushes the FD size up slightly over 2MB
!if $(TARGET) != NOOPT && $(ARCH) == ARM
  DEFINE FD_SIZE_IN_MB    = 2
!else
  DEFINE FD_SIZE_IN_MB    = 3
!endif

!if $(FD_SIZE_IN_MB) == 2
  DEFINE FD_SIZE          = 0x200000
  DEFINE FD_NUM_BLOCKS    = 0x200
!endif
!if $(FD_SIZE_IN_MB) == 3
  DEFINE FD_SIZE          = 0x300000
  DEFINE FD_NUM_BLOCKS    = 0x300
!endif

# Dynamic stack cookies are not supported on ARM
!if $(ARCH) == ARM
  DEFINE CUSTOM_STACK_CHECK_LIB = STATIC
!else
  DEFINE CUSTOM_STACK_CHECK_LIB = DYNAMIC
!endif
