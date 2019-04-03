#------------------------------------------------------------------------------
#
# Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Abstract:
#
#   Provide FSP helper function.
#
#------------------------------------------------------------------------------

#
# FspInfoHeaderRelativeOff is patched during build process and initialized to offset of the  AsmGetFspBaseAddress 
# from the FSP Info header. 
#
ASM_GLOBAL ASM_PFX(FspInfoHeaderRelativeOff)
ASM_PFX(FspInfoHeaderRelativeOff):
   #
   # This value will be pached by the build script
   #
   .long    0x012345678

#
# Returns FSP Base Address. 
#
# This function gets the FSP Info Header using relative addressing and returns the FSP Base from the header structure
#
ASM_GLOBAL ASM_PFX(AsmGetFspBaseAddress)
ASM_PFX(AsmGetFspBaseAddress):
   mov    $AsmGetFspBaseAddress, %eax
   sub    FspInfoHeaderRelativeOff, %eax
   add    $0x01C, %eax
   mov    (%eax), %eax
   ret

#
# No stack counter part of AsmGetFspBaseAddress. Return address is in edi.
#
ASM_GLOBAL ASM_PFX(AsmGetFspBaseAddressNoStack)
ASM_PFX(AsmGetFspBaseAddressNoStack):
   mov    $AsmGetFspBaseAddress, %eax
   sub    FspInfoHeaderRelativeOff, %eax
   add    $0x01C, %eax 
   mov    (%eax), %eax
   jmp    *%edi

#
# Returns FSP Info Header. 
#
# This function gets the FSP Info Header using relative addressing and returns it
#
ASM_GLOBAL ASM_PFX(AsmGetFspInfoHeader)
ASM_PFX(AsmGetFspInfoHeader):
   mov    $AsmGetFspBaseAddress, %eax
   sub    FspInfoHeaderRelativeOff, %eax
   ret
   
#
# No stack counter part of AsmGetFspInfoHeader. Return address is in edi.
#
ASM_GLOBAL ASM_PFX(AsmGetFspInfoHeaderNoStack)
ASM_PFX(AsmGetFspInfoHeaderNoStack):
   mov    $AsmGetFspBaseAddress, %eax
   sub    FspInfoHeaderRelativeOff, %eax
   jmp    *%edi
