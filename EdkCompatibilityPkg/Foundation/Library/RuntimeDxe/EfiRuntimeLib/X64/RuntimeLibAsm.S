#------------------------------------------------------------------------------
#
# Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   RuntimeLibAsm.S
#
# Abstract:
#
#
#------------------------------------------------------------------------------
#include <EfiBind.h>

.text

.globl ASM_PFX(EfiCpuFlushCache)
#------------------------------------------------------------------------------
#EFI_STATUS
#EfiCpuFlushCache (
#  IN EFI_PHYSICAL_ADDRESS          Start,
#  IN UINT64                        Length
#  );
#------------------------------------------------------------------------------

ASM_PFX(EfiCpuFlushCache):
    wbinvd
    mov $0, %rax
    ret

