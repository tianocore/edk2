#------------------------------------------------------------------------------ 
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

.text
.align 2

.globl ASM_PFX(GccSemihostCall)
INTERWORK_FUNC(GccSemihostCall)

/*
  Semihosting operation request mechanism
  
  SVC  0x123456  in ARM state (for all architectures)
  SVC  0xAB in Thumb state (excluding ARMv7-M)
  BKPT 0xAB for ARMv7-M (Thumb-2 only)
  
  R0 - operation type 
  R1 - block containing all other parametes 
 */
ASM_PFX(GccSemihostCall):
  svc     #0x123456  
  bx      lr


