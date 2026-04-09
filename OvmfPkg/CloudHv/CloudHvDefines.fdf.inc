## @file
#  FDF include file that defines the main macros and sets the dependent PCDs.
#
#  Copyright (C) 2014, Red Hat, Inc.
#  Copyright (c) 2006 - 2022, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

DEFINE BLOCK_SIZE        = 0x1000
DEFINE VARS_OFFSET       = 0

DEFINE VARS_SIZE         = 0x84000
DEFINE VARS_BLOCKS       = 0x84
DEFINE VARS_LIVE_SIZE    = 0x40000
DEFINE VARS_SPARE_SIZE   = 0x42000

DEFINE FW_BASE_ADDRESS   = 0x004FFFD0
DEFINE FW_SIZE           = 0x00400000
DEFINE FW_BLOCKS         = 0x400
DEFINE CODE_BASE_ADDRESS = 0x00583FD0
DEFINE CODE_SIZE         = 0x0037C000
DEFINE CODE_BLOCKS       = 0x37C
DEFINE FVMAIN_SIZE       = 0x00348000
DEFINE SECFV_OFFSET      = 0x003CC000
DEFINE SECFV_SIZE        = 0x34000

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

# The OVMF WorkArea contains a fixed size header followed by the actual data.
# The size of header is accessed through a fixed PCD in the reset vector code.
# The value need to be kept in sync with the any changes to the Confidential
# Computing Work Area header defined in the Include/WorkArea.h
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfConfidentialComputingWorkAreaHeader  = 4

SET gUefiOvmfPkgTokenSpaceGuid.PcdCfvBase           = $(FW_BASE_ADDRESS)
SET gUefiOvmfPkgTokenSpaceGuid.PcdCfvRawDataOffset  = $(VARS_OFFSET)
SET gUefiOvmfPkgTokenSpaceGuid.PcdCfvRawDataSize    = $(VARS_SIZE)

SET gUefiOvmfPkgTokenSpaceGuid.PcdBfvBase           = $(CODE_BASE_ADDRESS)
SET gUefiOvmfPkgTokenSpaceGuid.PcdBfvRawDataOffset  = $(VARS_SIZE)
SET gUefiOvmfPkgTokenSpaceGuid.PcdBfvRawDataSize    = $(CODE_SIZE)

!if $(SMM_REQUIRE) == TRUE
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64 = gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageVariableBase
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase = gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageFtwWorkingBase
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase   = gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageFtwSpareBase
!endif

DEFINE MEMFD_BASE_ADDRESS = 0x800000
