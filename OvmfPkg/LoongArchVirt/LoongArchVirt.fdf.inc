## @file
#
#  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

DEFINE BLOCK_SIZE                 = 0x1000

############################################################################
# FW total
DEFINE FW_BASE_ADDRESS            = 0x1c000000
DEFINE FW_BLOCKS                  = 0x400
DEFINE FW_SIZE                    = 0x400000

############################################################################
#Flash code layout
#Set Sec size in flash
DEFINE SECFV_SIZE                 = 0x00010000

#Set Pei size in flash
DEFINE PEIFV_SIZE                 = 0x00040000

#Set Dxe size in flash
DEFINE DXEFV_SIZE                 = 0x00350000

#Set FVMAIN size
DEFINE FVMAIN_SIZE                = $(SECFV_SIZE) + $(PEIFV_SIZE) +$(DXEFV_SIZE)

#Set Memory layout
DEFINE SEC_PEI_TEMP_RAM_BASE      = 0x10000
DEFINE SEC_PEI_TEMP_RAM_SIZE      = 0x80000
DEFINE DEVICE_TREE_RAM_BASE       = 0x100000
