/** @file
  Implementation of Application Register reading functions on Itanium platform.

  Copyright (c) 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BaseLibInternals.h"


//
// Loop up table for Index and corresponding application register access routines. 
//
GLOBAL_REMOVE_IF_UNREFERENCED REGISTER_ENTRY mApplicationRegisterAccessEntries[] = {
  {IPF_APPLICATION_REGISTER_K0, AsmReadApplicationRegisterK0},
  {IPF_APPLICATION_REGISTER_K1, AsmReadApplicationRegisterK1},
  {IPF_APPLICATION_REGISTER_K2, AsmReadApplicationRegisterK2},
  {IPF_APPLICATION_REGISTER_K3, AsmReadApplicationRegisterK3},
  {IPF_APPLICATION_REGISTER_K4, AsmReadApplicationRegisterK4},
  {IPF_APPLICATION_REGISTER_K5, AsmReadApplicationRegisterK5},
  {IPF_APPLICATION_REGISTER_K6, AsmReadApplicationRegisterK6},
  {IPF_APPLICATION_REGISTER_K7, AsmReadApplicationRegisterK7},
  {IPF_APPLICATION_REGISTER_RSC, AsmReadApplicationRegisterRsc},
  {IPF_APPLICATION_REGISTER_BSP, AsmReadApplicationRegisterBsp},
  {IPF_APPLICATION_REGISTER_BSPSTORE, AsmReadApplicationRegisterBspstore},
  {IPF_APPLICATION_REGISTER_RNAT, AsmReadApplicationRegisterRnat},
  {IPF_APPLICATION_REGISTER_FCR, AsmReadApplicationRegisterFcr},
  {IPF_APPLICATION_REGISTER_EFLAG, AsmReadApplicationRegisterEflag},
  {IPF_APPLICATION_REGISTER_CSD, AsmReadApplicationRegisterCsd},
  {IPF_APPLICATION_REGISTER_SSD, AsmReadApplicationRegisterSsd},
  {IPF_APPLICATION_REGISTER_CFLG, AsmReadApplicationRegisterCflg},
  {IPF_APPLICATION_REGISTER_FSR, AsmReadApplicationRegisterFsr},
  {IPF_APPLICATION_REGISTER_FIR, AsmReadApplicationRegisterFir},
  {IPF_APPLICATION_REGISTER_FDR, AsmReadApplicationRegisterFdr},
  {IPF_APPLICATION_REGISTER_CCV, AsmReadApplicationRegisterCcv},
  {IPF_APPLICATION_REGISTER_UNAT, AsmReadApplicationRegisterUnat},
  {IPF_APPLICATION_REGISTER_FPSR, AsmReadApplicationRegisterFpsr},
  {IPF_APPLICATION_REGISTER_ITC, AsmReadApplicationRegisterItc},
  {IPF_APPLICATION_REGISTER_PFS, AsmReadApplicationRegisterPfs},
  {IPF_APPLICATION_REGISTER_LC, AsmReadApplicationRegisterLc},
  {IPF_APPLICATION_REGISTER_EC, AsmReadApplicationRegisterEc}
};


/**
  Reads a 64-bit application register.

  Reads and returns the application register specified by Index. The valid Index valued are defined
  above in "Related Definitions".
  If Index is invalid then 0xFFFFFFFFFFFFFFFF is returned.  This function is only available on IPF.

  @param  Index                     The index of the application register to read.

  @return The application register specified by Index.

**/
UINT64
EFIAPI
AsmReadApplicationRegister (
  IN UINT64  Index
  )
{
  UINTN   Item;

  for (Item = 0; Item < sizeof (mApplicationRegisterAccessEntries) / sizeof (mApplicationRegisterAccessEntries[0]); Item++) {
    if (mApplicationRegisterAccessEntries[Item].Index == Index) {
      return mApplicationRegisterAccessEntries[Item].Function ();
    }
  }

  return 0xFFFFFFFFFFFFFFFF;
}
