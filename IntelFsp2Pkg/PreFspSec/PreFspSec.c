/** @file

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PreFspSec.h"
#include "Guid/FspHeaderFile.h"
#include <Library/PeCoffLib.h>
#include <Library/BaseMemoryLib.h>

/**
  Relocate Pe/Te Image

  @param[in] ImageBaseAddress   Image base address

  @retval EFI_SUCCESS           Image is relocated successfully
  @retval Others                Image is not relocated successfully
**/
EFI_STATUS
RelocatePeTeImage (
  UINT64  ImageBaseAddress
  )
{
  RETURN_STATUS                 Status;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;

  ZeroMem (&ImageContext, sizeof (ImageContext));

  ImageContext.Handle    = (VOID *)ImageBaseAddress;
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)ImageBaseAddress;

  //
  // rebase the image
  //
  Status = PeCoffLoaderRelocateImage (&ImageContext);

  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  This function will patch the Sec Core and Pei Core in current FSP.
**/
VOID
EFIAPI
FspPatchItself (
  )
{
  UINT64           FspBase;
  UINT64           SecCoreImageBase;
  UINT64           PeiCoreImageBase;
  FSP_INFO_HEADER  *FspInfoHeader;
  UINT64           Delta;
  EFI_STATUS       Status;

  FspBase       = AsmGetRuntimeFspBaseAddress ();
  FspInfoHeader = (FSP_INFO_HEADER *)(UINTN)AsmGetFspInfoHeader ();
  ASSERT (FspInfoHeader->Signature == FSP_INFO_HEADER_SIGNATURE);
  Delta = FspBase - (UINT64)FspInfoHeader->ImageBase;
  if (Delta == 0) {
    //
    // No need to patch FSP
    //
    return;
  }

  //
  // Fix up FspInfoHeader->ImageBase
  //
  FspInfoHeader->ImageBase = (UINT32)FspBase;
  if (FspInfoHeader->ImageBase != (UINT32)FspBase) {
    DEBUG ((DEBUG_WARN, "Current FSP area can not be changed. Maybe it is in flash\n"));
    return;
  }

  //
  // Get SecCore image, and rebase it
  //
  SecCoreImageBase = AsmGetRuntimeSecCoreAddress ();
  if (SecCoreImageBase != 0) {
    Status = RelocatePeTeImage (SecCoreImageBase);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Sec Core is relocated successfully\n"));
    } else {
      DEBUG ((DEBUG_WARN, "Sec Core is not relocated. May have issue later\n"));
    }
  }

  //
  // Get PeiCore image, and rebase it
  //
  PeiCoreImageBase = AsmGetRuntimePeiCoreAddress ();
  if (PeiCoreImageBase != 0) {
    Status = RelocatePeTeImage (PeiCoreImageBase);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Pei Core is relocated successfully\n"));
    } else {
      DEBUG ((DEBUG_INFO, "Pei Core is not relocated. May have issue later\n"));
    }
  }
}
