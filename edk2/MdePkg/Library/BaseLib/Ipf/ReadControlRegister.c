/** @file
  Implementation of Control Register reading functions on Itanium platform.

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
// Loop up table for Index and corresponding control register access routines. 
//
GLOBAL_REMOVE_IF_UNREFERENCED REGISTER_ENTRY mControlRegisterAccessEntries[] = {
  {IPF_CONTROL_REGISTER_DCR, AsmReadControlRegisterDcr},
  {IPF_CONTROL_REGISTER_ITM, AsmReadControlRegisterItm},
  {IPF_CONTROL_REGISTER_IVA, AsmReadControlRegisterIva},
  {IPF_CONTROL_REGISTER_PTA, AsmReadControlRegisterPta},
  {IPF_CONTROL_REGISTER_IPSR, AsmReadControlRegisterIpsr},
  {IPF_CONTROL_REGISTER_ISR, AsmReadControlRegisterIsr},
  {IPF_CONTROL_REGISTER_IIP, AsmReadControlRegisterIip},
  {IPF_CONTROL_REGISTER_IFA, AsmReadControlRegisterIfa},
  {IPF_CONTROL_REGISTER_ITIR, AsmReadControlRegisterItir},
  {IPF_CONTROL_REGISTER_IIPA, AsmReadControlRegisterIipa},
  {IPF_CONTROL_REGISTER_IFS, AsmReadControlRegisterIfs},
  {IPF_CONTROL_REGISTER_IIM, AsmReadControlRegisterIim},
  {IPF_CONTROL_REGISTER_IHA, AsmReadControlRegisterIha},
  {IPF_CONTROL_REGISTER_LID, AsmReadControlRegisterLid},
  {IPF_CONTROL_REGISTER_IVR, AsmReadControlRegisterIvr},
  {IPF_CONTROL_REGISTER_TPR, AsmReadControlRegisterTpr},
  {IPF_CONTROL_REGISTER_EOI, AsmReadControlRegisterEoi},
  {IPF_CONTROL_REGISTER_IRR0, AsmReadControlRegisterIrr0},
  {IPF_CONTROL_REGISTER_IRR1, AsmReadControlRegisterIrr1},
  {IPF_CONTROL_REGISTER_IRR2, AsmReadControlRegisterIrr2},
  {IPF_CONTROL_REGISTER_IRR3, AsmReadControlRegisterIrr3},
  {IPF_CONTROL_REGISTER_ITV, AsmReadControlRegisterItv},
  {IPF_CONTROL_REGISTER_PMV, AsmReadControlRegisterPmv},
  {IPF_CONTROL_REGISTER_CMCV, AsmReadControlRegisterCmcv},
  {IPF_CONTROL_REGISTER_LRR0, AsmReadControlRegisterLrr0},
  {IPF_CONTROL_REGISTER_LRR1, AsmReadControlRegisterLrr1}
};


/**
  Reads a 64-bit control register.

  Reads and returns the control register specified by Index. The valid Index valued are defined
  above in "Related Definitions".
  If Index is invalid then 0xFFFFFFFFFFFFFFFF is returned.  This function is only available on IPF.

  @param  Index                     The index of the control register to read.

  @return The control register specified by Index.

**/
UINT64
EFIAPI
AsmReadControlRegister (
  IN UINT64  Index
  )
{
  UINTN   Item;

  for (Item = 0; Item < sizeof (mControlRegisterAccessEntries) / sizeof (mControlRegisterAccessEntries[0]); Item++) {
    if (mControlRegisterAccessEntries[Item].Index == Index) {
      return mControlRegisterAccessEntries[Item].Function ();
    }
  }

  return 0xFFFFFFFFFFFFFFFF;
}
