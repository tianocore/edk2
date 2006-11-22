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


/**
  Reads the current value of Kr1.

  @return The current value of Kr1.

**/
UINT64
EFIAPI
AsmReadKr1 (
  VOID
  );

/**
  Writes the current value of Kr1.

  @param  Value The 64-bit value to write to Kr1.

**/
VOID
EFIAPI
AsmWriteKr1 (
  IN      UINT64                    Value
  );

/**
  The function returns the pointer to PeiServices.

  The function returns the pointer to PeiServices.
  It will ASSERT() if the pointer to PeiServices is NULL.

  @retval  The pointer to PeiServices.

**/
EFI_PEI_SERVICES **
EFIAPI
GetPeiServicesTablePointer (
  VOID
  )
{
  EFI_PEI_SERVICES  **PeiServices;

  PeiServices = (EFI_PEI_SERVICES **)(UINTN)AsmReadKr1 ();
  ASSERT (PeiServices != NULL);
  return PeiServices;
}

/**
  The constructor function caches the pointer to PEI services.

  The constructor function caches the pointer to PEI services.
  It will always return EFI_SUCCESS.

  @param  FfsHeader   Pointer to FFS header the loaded driver.
  @param  PeiServices Pointer to the PEI services.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PeiServicesTablePointerLibConstructor (
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  IN EFI_PEI_SERVICES     **PeiServices
  )
{
  AsmWriteKr1 ((UINT64)(UINTN)PeiServices);
  return EFI_SUCCESS;
}
