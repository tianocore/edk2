## @file
#  FDF include file that defines the main macros and sets the dependent PCDs.
#
#  Copyright (C) 2014, Red Hat, Inc.
#  Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

DEFINE BLOCK_SIZE        = 0x1000

#
# A firmware binary built with FD_SIZE_IN_KB=1024, and a firmware binary built
# with FD_SIZE_IN_KB=2048, use the same variable store layout.
#
# Setting FD_SIZE_IN_KB to 4096 results in a different (much larger) variable
# store structure that is incompatible with both of the above-mentioned
# firmware binaries.
#
!if ($(FD_SIZE_IN_KB) == 1024) || ($(FD_SIZE_IN_KB) == 2048)
DEFINE VARS_SIZE         = 0x20000
DEFINE VARS_BLOCKS       = 0x20
DEFINE VARS_LIVE_SIZE    = 0xE000
DEFINE VARS_SPARE_SIZE   = 0x10000
!endif

!if $(FD_SIZE_IN_KB) == 1024
DEFINE FW_BASE_ADDRESS   = 0xFFF00000
DEFINE FW_SIZE           = 0x00100000
DEFINE FW_BLOCKS         = 0x100
DEFINE CODE_BASE_ADDRESS = 0xFFF20000
DEFINE CODE_SIZE         = 0x000E0000
DEFINE CODE_BLOCKS       = 0xE0
DEFINE FVMAIN_SIZE       = 0x000CC000
DEFINE SECFV_OFFSET      = 0x000EC000
DEFINE SECFV_SIZE        = 0x14000
!endif

!if $(FD_SIZE_IN_KB) == 2048
DEFINE FW_BASE_ADDRESS   = 0xFFE00000
DEFINE FW_SIZE           = 0x00200000
DEFINE FW_BLOCKS         = 0x200
DEFINE CODE_BASE_ADDRESS = 0xFFE20000
DEFINE CODE_SIZE         = 0x001E0000
DEFINE CODE_BLOCKS       = 0x1E0
DEFINE FVMAIN_SIZE       = 0x001AC000
DEFINE SECFV_OFFSET      = 0x001CC000
DEFINE SECFV_SIZE        = 0x34000
!endif

!if $(FD_SIZE_IN_KB) == 4096
DEFINE VARS_SIZE         = 0x84000
DEFINE VARS_BLOCKS       = 0x84
DEFINE VARS_LIVE_SIZE    = 0x40000
DEFINE VARS_SPARE_SIZE   = 0x42000

DEFINE FW_BASE_ADDRESS   = 0xFFC00000
DEFINE FW_SIZE           = 0x00400000
DEFINE FW_BLOCKS         = 0x400
DEFINE CODE_BASE_ADDRESS = 0xFFC84000
DEFINE CODE_SIZE         = 0x0037C000
DEFINE CODE_BLOCKS       = 0x37C
DEFINE FVMAIN_SIZE       = 0x00348000
DEFINE SECFV_OFFSET      = 0x003CC000
DEFINE SECFV_SIZE        = 0x34000
!endif

SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFdBaseAddress     = $(FW_BASE_ADDRESS)
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFirmwareFdSize    = $(FW_SIZE)
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFirmwareBlockSize = $(BLOCK_SIZE)

SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageVariableBase = $(FW_BASE_ADDRESS)
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize = $(VARS_LIVE_SIZE)

SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageEventLogBase = gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageVariableBase + gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageEventLogSize = $(BLOCK_SIZE)

SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageFtwWorkingBase = gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageEventLogBase + gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageEventLogSize
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize = $(BLOCK_SIZE)

SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageFtwSpareBase = gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageFtwWorkingBase + gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize = $(VARS_SPARE_SIZE)

DEFINE MEMFD_BASE_ADDRESS = 0x800000
