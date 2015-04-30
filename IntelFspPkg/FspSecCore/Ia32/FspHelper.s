#------------------------------------------------------------------------------
#
# Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Abstract:
#
#   Provide FSP helper function.
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(FspInfoHeaderRelativeOff)
ASM_PFX(FspInfoHeaderRelativeOff):
   #
   # This value will be pached by the build script
   #
   .long    0x012345678


ASM_GLOBAL ASM_PFX(AsmGetFspBaseAddress)
ASM_PFX(AsmGetFspBaseAddress):
   mov    $AsmGetFspBaseAddress, %eax
   sub    $FspInfoHeaderRelativeOff, %eax
   add    $0x01C, %eax
   mov    (%eax), %eax
   ret

ASM_GLOBAL ASM_PFX(AsmGetFspInfoHeader)
ASM_PFX(AsmGetFspInfoHeader):
   mov    $AsmGetFspBaseAddress, %eax
   sub    $FspInfoHeaderRelativeOff, %eax
   ret
