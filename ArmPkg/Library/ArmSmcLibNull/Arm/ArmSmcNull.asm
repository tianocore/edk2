//
//  Copyright (c) 2012-2013, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

    EXPORT ArmCallSmc
    EXPORT ArmCallSmcArg1
    EXPORT ArmCallSmcArg2
    EXPORT ArmCallSmcArg3

    AREA   ArmSmc, CODE, READONLY

ArmCallSmc
  bx     lr

// Arg1 in R1
ArmCallSmcArg1
  bx     lr

ArmCallSmcArg2
  bx     lr

ArmCallSmcArg3
  bx     lr

  END
