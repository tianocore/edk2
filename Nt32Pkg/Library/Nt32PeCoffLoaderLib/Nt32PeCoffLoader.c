/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  TianoPeCoffLoader.c

Abstract:

  Wrap the Base PE/COFF loader with the PE COFF Protocol


--*/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Guid/PeiPeCoffLoader.h>
#include <Library/PeCoffLoaderLib.h>  
#include <Library/PeiServicesLib.h>


EFI_PEI_PE_COFF_LOADER_PROTOCOL  *mPeiEfiPeiPeCoffLoader = NULL;


EFI_PEI_PE_COFF_LOADER_PROTOCOL *
EFIAPI
GetPeCoffLoaderProtocol (
  )
{
  EFI_STATUS Status;
  
  if (mPeiEfiPeiPeCoffLoader == NULL) {
    Status = PeiServicesLocatePpi(
                              &gEfiPeiPeCoffLoaderGuid,
                              0,
                              NULL,
                              (VOID **) &mPeiEfiPeiPeCoffLoader
                              );
    ASSERT_EFI_ERROR (Status);
   }
  return mPeiEfiPeiPeCoffLoader;
}
