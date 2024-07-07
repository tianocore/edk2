/**@file
  PlatformId Event HOB creation

  Copyright (c) 2024, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Guid/TcgEventHob.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PrintLib.h>
#include <Library/QemuFwCfgLib.h>

#define DPREFIX  "sp800155evts: "

/**
 * Creates an EFI_HOB_TYPE_GUID_EXTENSION HOB for a given SP800155 event.
 * Associates the string data with gTcg800155PlatformIdEventHobGuid. Any
 * unused bytes or out-of-bounds event sizes are considered corrupted and
 * are discarded.
**/
STATIC
VOID
PlatformIdRegisterSp800155 (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN       UINT8             *Evt,
  IN       UINTN             EvtSize
  )
{
  EFI_STATUS         Status;
  VOID               *Hob;
  EFI_HOB_GUID_TYPE  *GuidHob;
  UINT8              *EvtDest;

  Status = (*PeiServices)->CreateHob (
                             PeiServices,
                             EFI_HOB_TYPE_GUID_EXTENSION,
                             sizeof (EFI_HOB_GUID_TYPE) + (UINT16)EvtSize,
                             &Hob
                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, DPREFIX "GUID HOB creation failed, skipping\n"));
    return;
  }

  GuidHob = (EFI_HOB_GUID_TYPE *)Hob;
  CopyGuid (&GuidHob->Name, &gTcg800155PlatformIdEventHobGuid);
  EvtDest = (UINT8 *)GET_GUID_HOB_DATA (Hob);
  CopyMem (EvtDest, Evt, EvtSize);
  // Fill the remaining HOB padding bytes with 0s.
  SetMem (EvtDest + EvtSize, GET_GUID_HOB_DATA_SIZE (Hob) - EvtSize, 0);
}

/**
 * Reads the given path from the fw_cfg file and registers it as an
 * EFI_HOB_GUID_EXTENSION HOB with gTcg800155PlatformIdEventHobGuid.
 * Returns FALSE iff the file does not exist.
**/
BOOLEAN
PlatformIdRegisterEvent (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN CONST CHAR8             *Path
  )
{
  EFI_STATUS            Status;
  UINTN                 NumPages;
  EFI_PHYSICAL_ADDRESS  Pages;
  FIRMWARE_CONFIG_ITEM  FdtItem;
  UINTN                 FdtSize;
  UINT8                 *Evt;

  Status = QemuFwCfgFindFile (Path, &FdtItem, &FdtSize);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (FdtSize > MAX_UINT16 - sizeof (EFI_HOB_GUID_TYPE)) {
    DEBUG ((DEBUG_ERROR, DPREFIX "Eventdata too large for HOB, skipping\n"));
    return TRUE;
  }

  NumPages = EFI_SIZE_TO_PAGES (FdtSize);
  Status   = (*PeiServices)->AllocatePages (
                               PeiServices,
                               EfiBootServicesData,
                               NumPages,
                               &Pages
                               );
  if (EFI_ERROR (Status)) {
    return TRUE;
  }

  Evt = (UINT8 *)(UINTN)Pages;
  QemuFwCfgSelectItem (FdtItem);
  QemuFwCfgReadBytes (FdtSize, Evt);
  PlatformIdRegisterSp800155 (PeiServices, Evt, FdtSize);

  Status = (*PeiServices)->FreePages (PeiServices, Pages, NumPages);
  ASSERT_EFI_ERROR (Status);
  return TRUE;
}

VOID
PlatformIdInitialization (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  UINTN  Index;
  CHAR8  Path[64];

  for (Index = 0; ; Index++) {
    AsciiSPrint (Path, sizeof (Path), "opt/org.tianocode/sp800155evt/%d", Index);
    if (!PlatformIdRegisterEvent (PeiServices, Path)) {
      break;
    }
  }
}
