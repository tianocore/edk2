#------------------------------------------------------------------------------
#
# Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
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
