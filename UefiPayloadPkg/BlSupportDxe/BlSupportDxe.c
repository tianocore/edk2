/** @file
  This driver will setup PCDs for DXE phase from HOBs
  and initialise architecture-specific settings and resources.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "BlSupportDxe.h"
#include <Guid/TpmInstance.h>
#include <Library/PcdLib.h>

STATIC
BOOLEAN
TryGetWideAspectCappedViewportWidth (
  IN  UINT32  HorizontalResolution,
  IN  UINT32  VerticalResolution,
  IN  UINT32  CapAspectWidth,
  IN  UINT32  CapAspectHeight,
  OUT UINT32  *ViewportWidth
  )
{
  UINT64  CandidateWidth;

  if ((ViewportWidth == NULL) ||
      (HorizontalResolution == 0) ||
      (VerticalResolution == 0) ||
      (CapAspectWidth == 0) ||
      (CapAspectHeight == 0))
  {
    return FALSE;
  }

  if (((UINT64)HorizontalResolution * CapAspectHeight) <= ((UINT64)VerticalResolution * CapAspectWidth)) {
    return FALSE;
  }

  CandidateWidth = ((UINT64)VerticalResolution * CapAspectWidth) / CapAspectHeight;
  CandidateWidth &= ~1ULL;
  if ((CandidateWidth == 0) || (CandidateWidth >= HorizontalResolution)) {
    return FALSE;
  }

  *ViewportWidth = (UINT32)CandidateWidth;
  return TRUE;
}

/**
  Main entry for the bootloader support DXE module.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
BlDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;
  EFI_HOB_GUID_TYPE          *GuidHob;
  EFI_PEI_GRAPHICS_INFO_HOB  *GfxInfo;
  ACPI_BOARD_INFO            *AcpiBoardInfo;
  FIRMWARE_INFO              *FirmwareInfo;
  EFI_SYSTEM_RESOURCE_TABLE  *Esrt;
  EFI_SYSTEM_RESOURCE_ENTRY  *Esre;
  UINT32                     HorizontalResolution;
  UINT32                     VerticalResolution;
  UINT32                     ThresholdH;
  UINT32                     ThresholdV;
  UINT32                     ViewportWidth;

  //
  // Find the frame buffer information and update PCDs
  //
  GuidHob = GetFirstGuidHob (&gEfiGraphicsInfoHobGuid);
  if (GuidHob != NULL) {
    GfxInfo              = (EFI_PEI_GRAPHICS_INFO_HOB *)GET_GUID_HOB_DATA (GuidHob);
    HorizontalResolution = GfxInfo->GraphicsMode.HorizontalResolution;
    VerticalResolution   = GfxInfo->GraphicsMode.VerticalResolution;

    if (FeaturePcdGet (PcdFspGopBasicHiDpiSupport)) {
      ThresholdH = PcdGet32 (PcdFspGopBasicHiDpiScaleThresholdHorizontal);
      ThresholdV = PcdGet32 (PcdFspGopBasicHiDpiScaleThresholdVertical);

      if ((HorizontalResolution >= ThresholdH) && (VerticalResolution >= ThresholdV) &&
          ((HorizontalResolution & 1) == 0) && ((VerticalResolution & 1) == 0))
      {
        if (FeaturePcdGet (PcdFspGopBasicHiDpiWideAspectCapSupport) &&
            TryGetWideAspectCappedViewportWidth (
              HorizontalResolution,
              VerticalResolution,
              PcdGet32 (PcdFspGopBasicHiDpiWideAspectCapWidth),
              PcdGet32 (PcdFspGopBasicHiDpiWideAspectCapHeight),
              &ViewportWidth
              ))
        {
          HorizontalResolution = ViewportWidth;
        }

        HorizontalResolution /= 2;
        VerticalResolution   /= 2;
      }
    }

    Status = PcdSet32S (PcdVideoHorizontalResolution, HorizontalResolution);
    ASSERT_EFI_ERROR (Status);
    Status = PcdSet32S (PcdVideoVerticalResolution, VerticalResolution);
    ASSERT_EFI_ERROR (Status);
    Status = PcdSet32S (PcdSetupVideoHorizontalResolution, HorizontalResolution);
    ASSERT_EFI_ERROR (Status);
    Status = PcdSet32S (PcdSetupVideoVerticalResolution, VerticalResolution);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Set PcdPciExpressBaseAddress and PcdPciExpressBaseSize by HOB info
  //
  GuidHob = GetFirstGuidHob (&gUefiAcpiBoardInfoGuid);
  if (GuidHob != NULL) {
    AcpiBoardInfo = (ACPI_BOARD_INFO *)GET_GUID_HOB_DATA (GuidHob);
    Status        = PcdSet64S (PcdPciExpressBaseAddress, AcpiBoardInfo->PcieBaseAddress);
    ASSERT_EFI_ERROR (Status);
    Status = PcdSet64S (PcdPciExpressBaseSize, AcpiBoardInfo->PcieBaseSize);
    ASSERT_EFI_ERROR (Status);
  }

  Status = BlArchAdditionalOps (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);

  //
  // Build and publish a single-entry ESRT if a bootloader provided us with
  // details about currently running firmware
  //
  GuidHob = GetFirstGuidHob (&gEfiFirmwareInfoHobGuid);
  if (GuidHob != NULL) {
    FirmwareInfo = (FIRMWARE_INFO *)GET_GUID_HOB_DATA (GuidHob);

    Esrt = AllocateZeroPool (sizeof (EFI_SYSTEM_RESOURCE_TABLE) + sizeof (EFI_SYSTEM_RESOURCE_ENTRY));
    ASSERT (Esrt != NULL);

    Esrt->FwResourceVersion  = EFI_SYSTEM_RESOURCE_TABLE_FIRMWARE_RESOURCE_VERSION;
    Esrt->FwResourceCount    = 1;
    Esrt->FwResourceCountMax = 1;

    Esre = (EFI_SYSTEM_RESOURCE_ENTRY *)&Esrt[1];
    CopyMem (&Esre->FwClass, &FirmwareInfo->Type, sizeof (EFI_GUID));
    Esre->FwType                   = ESRT_FW_TYPE_SYSTEMFIRMWARE;
    Esre->FwVersion                = FirmwareInfo->Version;
    Esre->LowestSupportedFwVersion = FirmwareInfo->LowestSupportedVersion;

    Status = gBS->InstallConfigurationTable (&gEfiSystemResourceTableGuid, Esrt);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
