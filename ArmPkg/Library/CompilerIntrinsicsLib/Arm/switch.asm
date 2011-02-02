///------------------------------------------------------------------------------ 
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------



    EXPORT\s\s__ARM_switch8

    AREA\s\sArmSwitch, CODE, READONLY
\s\s
__ARM_switch8
\s\sLDRB\s\s  r12,[lr,#-1]
\s\sCMP\s\s\s\s  r3,r12
\s\sLDRBCC\s\sr3,[lr,r3]
\s\sLDRBCS\s\sr3,[lr,r12]
\s\sADD\s\s\s\s  r12,lr,r3,LSL #1
\s\sBX\s\s\s\s  r12
  
    END
