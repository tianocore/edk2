#
#  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
#  Copyright (c) 2014-2017, Linaro Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#

#include <AsmMacroIoLib.h>

//EFI_PHYSICAL_ADDRESS
//GetPhysAddrTop (
//  VOID
//  );
ASM_FUNC(ArmGetPhysAddrTop)
  mov   r0, #0x00000000
  mov   r1, #0x10000
  bx    lr
