## @file
#  Definitions of Flash definition file on RiscVVirt RISC-V platform
#
#  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#  Copyright (c) 2022, Ventana Micro Systems Inc. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
[Defines]
DEFINE BLOCK_SIZE                  = 0x1000

DEFINE PFLASH0_BASE                = 0x20000000
DEFINE PFLASH1_BASE                = 0x22000000

DEFINE CODE_BASE_ADDRESS           = $(PFLASH0_BASE)
DEFINE CODE_SIZE                   = 0x00800000
DEFINE CODE_BLOCKS                 = 0x800

DEFINE VARS_BASE_ADDRESS            = $(PFLASH1_BASE)
DEFINE VARS_SIZE                    = 0x000C0000
DEFINE VARS_BLOCK_SIZE              = 0x40000
DEFINE VARS_BLOCKS                  = 0x3

#
# EFI Variable memory region.
# The total size of EFI Variable FD must include
# all of sub regions of EFI Variable
#
DEFINE VARS_OFFSET                   = 0x00000000
DEFINE VARS_LIVE_SIZE                = 0x00040000
DEFINE VARS_FTW_WORKING_OFFSET       = $(VARS_OFFSET) + $(VARS_LIVE_SIZE)
DEFINE VARS_FTW_WORKING_SIZE         = 0x00040000
DEFINE VARS_FTW_SPARE_OFFSET         = $(VARS_FTW_WORKING_OFFSET) + $(VARS_FTW_WORKING_SIZE)
DEFINE VARS_FTW_SPARE_SIZE           = 0x00040000

SET gUefiCpuPkgTokenSpaceGuid.PcdCpuCoreCrystalClockFrequency = 10000000
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecPeiTempRamBase = 0x83FF0000
SET gUefiOvmfPkgTokenSpaceGuid.PcdOvmfSecPeiTempRamSize = 0x00010000
