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
//   ReadKr1.s
//
// Abstract:
//
//   Contains assembly code for read Kr1.
//
//--

  .file  "ReadKr1.s"

#include  "IpfMacro.i"

//---------------------------------------------------------------------------------
//++
// AsmReadKr1
//
// This routine is used to get KR1. KR1 is used to store Pei Service Table
// Pointer in archeture.
//
// Arguments : 
//
// On Entry :  None.
//
// Return Value: Pei Services Table.
// 
//--
//----------------------------------------------------------------------------------
PROCEDURE_ENTRY (AsmReadKr1)
        
        mov             r8 = ar.k1;;  // Pei Services Table Pointer
        br.ret.dpnt     b0;;

PROCEDURE_EXIT (AsmReadKr1)
