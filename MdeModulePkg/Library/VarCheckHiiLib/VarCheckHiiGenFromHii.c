/** @file
  Var Check Hii generation from Hii Database.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "VarCheckHiiGen.h"

/**
  Generate from Hii Database.

**/
VOID
VarCheckHiiGenFromHiiDatabase (
  VOID
  )
{
  EFI_STATUS                    Status;
  UINTN                         BufferSize;
  VOID                          *Buffer;
  EFI_PHYSICAL_ADDRESS          BufferAddress;
  EFI_HII_DATABASE_PROTOCOL     *HiiDatabase;

  //
  // Locate HII Database protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **) &HiiDatabase);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Call first time with zero buffer length.
  // Should fail with EFI_BUFFER_TOO_SMALL.
  //
  BufferSize = 0;
  Buffer = NULL;
  Status = HiiDatabase->ExportPackageLists (HiiDatabase, 0, &BufferSize, Buffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Allocate buffer to hold the HII Database.
    //
    Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES (BufferSize), &BufferAddress);
    ASSERT_EFI_ERROR (Status);
    Buffer = (VOID *) (UINTN) BufferAddress;

    //
    // Export HII Database into the buffer.
    //
    Status = HiiDatabase->ExportPackageLists (HiiDatabase, 0, &BufferSize, Buffer);
    ASSERT_EFI_ERROR (Status);

    DEBUG ((DEBUG_INFO , "VarCheckHiiGenDxeFromHii - HII Database exported at 0x%x, size = 0x%x\n", Buffer, BufferSize));

#ifdef DUMP_HII_DATA
    DEBUG_CODE (
      DumpHiiDatabase (Buffer, BufferSize);
      );
#endif

    VarCheckParseHiiDatabase (Buffer, BufferSize);

    gBS->FreePages (BufferAddress, EFI_SIZE_TO_PAGES (BufferSize));
  }
}
