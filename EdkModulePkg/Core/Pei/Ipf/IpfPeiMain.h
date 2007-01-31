/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IpfPeiMain.h

Abstract:

  Definition of IPF specific function

Revision History

--*/

#ifndef _IPF_PEI_MAIN_H_
#define _IPF_PEI_MAIN_H_

#include <PeiMain.h>

SAL_RETURN_REGS
GetHandOffStatus (
  VOID
  )
/*++

Routine Description:

   This routine is called by all processors simultaneously, to get some hand-off
   status that has been captured by IPF dispatcher and recorded in kernel registers.
  
Arguments :

  On Entry :  None.

Returns:

   Lid, R20Status.

--*/

;

#endif
