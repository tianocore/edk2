/*++

Copyright (c) 2006 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  PeiServicesTablePointer.c

Abstract:

  PEI Services Table Pointer Library.
  
--*/

#include "InternalPeiServicesTablePointer.h"

/**
  
  The function returns the pointer to PeiServicee following
  PI1.0.
  
  For IA32, the four-bytes field immediately prior to new IDT
  base addres is used to save the EFI_PEI_SERVICES**.
  For x64, the eight-bytes field immediately prior to new IDT
  base addres is used to save the EFI_PEI_SERVICES**.

  @retval  The pointer to PeiServices.

**/
EFI_PEI_SERVICES **
EFIAPI
GetPeiServicesTablePointer (
  VOID
  )
{
  EFI_PEI_SERVICES  **PeiServices;

  PeiServices = (EFI_PEI_SERVICES **) AsmPeiSevicesTablePointer ();
  ASSERT (PeiServices != NULL);
  return PeiServices;
}

