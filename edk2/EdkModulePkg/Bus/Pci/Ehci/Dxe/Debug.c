/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    Debug.c

Abstract:


Revision History
--*/


#include "Ehci.h"


VOID
DumpEHCIPortsStatus (
  IN USB2_HC_DEV    *HcDev
  )
{
  UINT8       PortNumber;
  UINT8       Index;
  UINT32      Value;

  ReadEhcCapabiltiyReg (
     HcDev,
     HCSPARAMS,
     &Value
     );

  PortNumber = (UINT8) (Value & HCSP_NPORTS);

  for (Index = 0; Index < PortNumber; Index++) {
    ReadEhcOperationalReg (
      HcDev,
      PORTSC + 4 * Index,
      &Value
      );
     DEBUG((gEHCDebugLevel, "Port[%d] = 0x%x\n", Index, Value));
  }


}


