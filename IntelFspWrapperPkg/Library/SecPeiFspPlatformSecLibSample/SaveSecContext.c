/** @file
  Sample to provide SaveSecContext function.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <PiPei.h>
#include <Library/DebugLib.h>

#include <Ppi/TopOfTemporaryRam.h>
#include <Ppi/SecPlatformInformation.h>

/**
  Save BIST value before call FspInit.

  @param[in] Bist   BIST value.
**/
VOID
AsmSaveBistValue (
  IN UINT32  Bist
  );

/**
  Save Ticker value before call FspInit.

  @param[in] Ticker   Ticker value.
**/
VOID
AsmSaveTickerValue (
  IN UINT64  Ticker
  );

/**
  Save SEC context before call FspInit.

  @param[in] PeiServices  Pointer to PEI Services Table.
**/
VOID
EFIAPI
SaveSecContext (
  IN CONST EFI_PEI_SERVICES                     **PeiServices
  )
{
  UINT32      *Bist;
  UINT64      *Ticker;
  UINT32      Size;
  UINT32      Count;
  UINT32      TopOfTemporaryRam;
  VOID        *TopOfTemporaryRamPpi;
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "SaveSecContext - 0x%x\n", PeiServices));

  Status = (*PeiServices)->LocatePpi (
                             PeiServices,
                             &gTopOfTemporaryRamPpiGuid,
                             0,
                             NULL,
                             (VOID **) &TopOfTemporaryRamPpi
                             );
  if (EFI_ERROR (Status)) {
    return ;
  }

  DEBUG ((DEBUG_INFO, "TopOfTemporaryRamPpi - 0x%x\n", TopOfTemporaryRamPpi));

  //
  // The entries of BIST information, together with the number of them,
  // reside in the bottom of stack, left untouched by normal stack operation.
  // This routine copies the BIST information to the buffer pointed by
  // PlatformInformationRecord for output.
  //
  // |--------------| <- TopOfTemporaryRam
  // |Number of BSPs|
  // |--------------|
  // |     BIST     |
  // |--------------|
  // |     ....     |
  // |--------------|
  // |  TSC[63:32]  |
  // |--------------|
  // |  TSC[31:00]  |
  // |--------------|
  //

  TopOfTemporaryRam = (UINT32)(UINTN)TopOfTemporaryRamPpi - sizeof(UINT32);
  TopOfTemporaryRam -= sizeof(UINT32) * 2;
  DEBUG ((DEBUG_INFO, "TopOfTemporaryRam - 0x%x\n", TopOfTemporaryRam));
  Count             = *(UINT32 *)(UINTN)(TopOfTemporaryRam - sizeof(UINT32));
  DEBUG ((DEBUG_INFO, "Count - 0x%x\n", Count));
  Size              = Count * sizeof (IA32_HANDOFF_STATUS);
  DEBUG ((DEBUG_INFO, "Size - 0x%x\n", Size));

  Bist   = (UINT32 *)(UINTN)(TopOfTemporaryRam - sizeof(UINT32) - Size);
  DEBUG ((DEBUG_INFO, "Bist - 0x%x\n", *Bist));
  Ticker = (UINT64 *)(UINTN)(TopOfTemporaryRam - sizeof(UINT32) - Size - sizeof(UINT64));
  DEBUG ((DEBUG_INFO, "Ticker - 0x%lx\n", *Ticker));

  // Just need record BSP
  AsmSaveBistValue (*Bist);
  AsmSaveTickerValue (*Ticker);
}
