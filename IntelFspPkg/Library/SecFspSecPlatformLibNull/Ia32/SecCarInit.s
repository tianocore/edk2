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
#  SEC CAR function
#
#------------------------------------------------------------------------------

#-----------------------------------------------------------------------------
#
#  Section:     SecCarInit
#
#  Description: This function initializes the Cache for Data, Stack, and Code
#
#-----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(SecCarInit)
ASM_PFX(SecCarInit):

  #
  # Set up CAR
  #

  xor     %eax, %eax

SecCarInitExit:

  movd       %mm7, %esi                      #RET_ESI
  jmp        *%esi
