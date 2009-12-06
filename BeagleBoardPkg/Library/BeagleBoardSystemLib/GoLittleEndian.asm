//------------------------------------------------------------------------------ 
//
// Copyright (c) 2008-2009 Apple Inc. All rights reserved.
//
// All rights reserved. This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------

  EXPORT  GoLittleEndian
  PRESERVE8
  AREA    Ebl, CODE, READONLY

// r0 is target address
GoLittleEndian
  // Switch to SVC Mode
  mov   r2,#0xD3        // SVC mode
  msr   CPSR_c,r2       // Switch modes
  
  bx  r0
  
  END
