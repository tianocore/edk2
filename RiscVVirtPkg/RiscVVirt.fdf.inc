## @file
#  RISC-V flash definition file on EFI/Framework Open Virtual Machine Firmware (OVMF) platform
#
#  Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution. The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##
DEFINE BLOCK_SIZE        = 0x1000
DEFINE VARS_SIZE         = 0x20000
DEFINE VARS_BLOCKS       = 0x20

DEFINE FW_BASE_ADDRESS   = 0xFF800000
DEFINE FW_SIZE           = 0x00800000
DEFINE FW_BLOCKS         = 0x800
#
# FF800000-FF820000 is used by variable.
#
DEFINE CODE_BASE_ADDRESS = 0xFF820000
DEFINE CODE_SIZE         = 0x007E0000
DEFINE CODE_BLOCKS       = 0x7E0
DEFINE FVMAIN_BASE       = 0x0560000
DEFINE FVMAIN_SIZE       = 0x0018C000
DEFINE PEIFV_OFFSET      = 0x06EC000
DEFINE PEIFV_SIZE        = 0xE0000
DEFINE SECFV_OFFSET      = 0x007CC000
DEFINE SECFV_SIZE        = 0x34000

SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFdBaseAddress     = $(FW_BASE_ADDRESS)
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFirmwareFdSize    = $(FW_SIZE)
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFirmwareBlockSize = $(BLOCK_SIZE)

