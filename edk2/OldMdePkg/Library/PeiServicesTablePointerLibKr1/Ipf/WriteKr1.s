//++
//
// Copyright (c) 2006 Intel Corporation. All rights reserved
// This software and associated documentation (if any) is furnished
// under a license and may only be used or copied in accordance
// with the terms of the license. Except as permitted by such
// license, no part of this software or documentation may be
// reproduced, stored in a retrieval system, or transmitted in any
// form or by any means without the express written consent of
// Intel Corporation.
//
//
// Module Name:
//
//   WriteKr1.s
//
// Abstract:
//
//   Contains assembly code for write Kr1.
//
//--

  .file  "WriteKr1.s"

#include  "IpfMacro.i"

//---------------------------------------------------------------------------------
//++
// AsmWriteKr1
//
// This routine is used to Write KR1. KR1 is used to store Pei Service Table
// Pointer in archeture.
//
// Arguments : r32  Pei Services Table Pointer
//
// On Entry :  None.
//
// Return Value: None.
// 
//--
//----------------------------------------------------------------------------------
PROCEDURE_ENTRY (AsmWriteKr1)
        
        mov             ar.k1 = r32;;  // Pei Services Table Pointer
        br.ret.dpnt     b0;;

PROCEDURE_EXIT (AsmWriteKr1)
 