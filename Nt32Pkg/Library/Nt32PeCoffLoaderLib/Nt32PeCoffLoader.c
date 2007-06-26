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



//
// Include common header file for this module.
//
#include "CommonHeader.h"

EFI_PEI_PE_COFF_LOADER_PROTOCOL  *mPeiEfiPeiPeCoffLoader;

EFI_STATUS
EFIAPI
PeCoffLoaderConstructor (
  IN EFI_FFS_FILE_HEADER      *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = (*PeiServices)->LocatePpi (
                            PeiServices,
                            &gEfiPeiPeCoffLoaderGuid,
                            0,
                            NULL,
                            &mPeiEfiPeiPeCoffLoader
                            );
  return Status;
}

EFI_PEI_PE_COFF_LOADER_PROTOCOL *
EFIAPI
GetPeCoffLoaderProtocol (
  )
{
  return mPeiEfiPeiPeCoffLoader;
}
