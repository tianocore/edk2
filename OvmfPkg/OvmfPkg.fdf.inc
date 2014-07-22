## @file
#  FDF include file that defines the main macros and sets the dependent PCDs.
#
#  Copyright (C) 2014, Red Hat, Inc.
#  Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials are licensed and made available
#  under the terms and conditions of the BSD License which accompanies this
#  distribution. The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR
#  IMPLIED.
#
##

DEFINE BLOCK_SIZE        = 0x1000
DEFINE VARS_SIZE         = 0x20000
DEFINE VARS_BLOCKS       = 0x20

!ifdef $(FD_SIZE_1MB)

DEFINE FW_BASE_ADDRESS   = 0xFFF00000
DEFINE FW_SIZE           = 0x00100000
DEFINE FW_BLOCKS         = 0x100
DEFINE CODE_BASE_ADDRESS = 0xFFF20000
DEFINE CODE_SIZE         = 0x000E0000
DEFINE CODE_BLOCKS       = 0xE0
DEFINE FVMAIN_SIZE       = 0x000CC000
DEFINE SECFV_OFFSET      = 0x000EC000
DEFINE SECFV_SIZE        = 0x14000

!else

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

SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFdBaseAddress     = $(FW_BASE_ADDRESS)
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFirmwareFdSize    = $(FW_SIZE)
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFirmwareBlockSize = $(BLOCK_SIZE)

SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageVariableBase = $(FW_BASE_ADDRESS)
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize = 0xE000

SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageEventLogBase = gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageVariableBase + gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageEventLogSize = $(BLOCK_SIZE)

SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageFtwWorkingBase = gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageEventLogBase + gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageEventLogSize
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize = $(BLOCK_SIZE)

SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageFtwSpareBase = gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFlashNvStorageFtwWorkingBase + gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize
SET gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize = 0x10000
