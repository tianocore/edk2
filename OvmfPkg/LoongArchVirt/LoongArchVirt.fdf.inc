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
#Set FVMAIN size
DEFINE FVMAIN_SIZE                = $(FW_SIZE)

#Set Memory layout
DEFINE SEC_PEI_TEMP_RAM_BASE      = 0x10000
DEFINE SEC_PEI_TEMP_RAM_SIZE      = 0x80000
DEFINE DEVICE_TREE_RAM_BASE       = 0x100000
