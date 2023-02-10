/** @file
This driver is used to manage SD/MMC PCI host controllers override function
for BayHub BH720 eMMC host controller.

Copyright (c) 2018 - 2019, BayHub Tech inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BAYHUB_HOST_H_
#define BAYHUB_HOST_H_

#include <Uefi.h>
#include <Protocol/SdMmcOverride.h>
#include <Protocol/PciIo.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Guid/DebugMask.h>

// O2/BHT add BAR1 for PCIR mapping registers
// These registers is defined by O2/BHT, but we may follow name definition rule.
#define BHT_PCR_MAP_VAL  0x200    /* PCI CFG Space Register Mapping Value Register */
#define BHT_PCR_MAP_CTL  0x204    /* PCI CFG Space Register Mapping Control Register */
#define BHT_PCR_MAP_EN   0x208    /* PCI CFG Space Register Mapping Enable Register */
#define BHT_GPIOCTL      0x210    /* GPIO control register*/

#define HOST_CLK_DRIVE_STRENGTH  2
#define HOST_DAT_DRIVE_STRENGTH  2
#define HS200_ALLPASS_PHASE      0
#define HS100_ALLPASS_PHASE      6

#define UNSUPPORT  0
#define SD_HOST    1
#define EMMC_HOST  2

#define PCI_DEV_ID_RJ    0x8320
#define PCI_DEV_ID_SDS0  0x8420
#define PCI_DEV_ID_SDS1  0x8421
#define PCI_DEV_ID_FJ2   0x8520
#define PCI_DEV_ID_SB0   0x8620
#define PCI_DEV_ID_SB1   0x8621
#define PCI_DEV_ID_SE2   0x8720

/**
  Override function for SDHCI capability bits

  @param[in]      ControllerHandle      The EFI_HANDLE of the controller.
  @param[in]      Slot                  The 0 based slot index.
  @param[in,out]  SdMmcHcSlotCapability The SDHCI capability structure.
  @param[in,out]  BaseClkFreq           The base clock frequency value that
                                        optionally can be updated.

  @retval EFI_SUCCESS           The override function completed successfully.
  @retval EFI_NOT_FOUND         The specified controller or slot does not exist.
  @retval EFI_INVALID_PARAMETER SdMmcHcSlotCapability is NULL

**/
EFI_STATUS
EFIAPI
BhtHostOverrideCapability (
  IN      EFI_HANDLE  ControllerHandle,
  IN      UINT8       Slot,
  IN OUT  VOID        *SdMmcHcSlotCapability,
  IN OUT  UINT32      *BaseClkFreq
  );

/**
  Override function for SDHCI controller operations

  @param[in]      ControllerHandle      The EFI_HANDLE of the controller.
  @param[in]      Slot                  The 0 based slot index.
  @param[in]      PhaseType             The type of operation and whether the
                                        hook is invoked right before (pre) or
                                        right after (post)
  @param[in,out]  PhaseData             The pointer to a phase-specific data.

  @retval EFI_SUCCESS           The override function completed successfully.
  @retval EFI_NOT_FOUND         The specified controller or slot does not exist.
  @retval EFI_INVALID_PARAMETER PhaseType is invalid

**/
EFI_STATUS
EFIAPI
BhtHostOverrideNotifyPhase (
  IN      EFI_HANDLE               ControllerHandle,
  IN      UINT8                    Slot,
  IN      EDKII_SD_MMC_PHASE_TYPE  PhaseType,
  IN OUT  VOID                     *PhaseData
  );

extern EDKII_SD_MMC_OVERRIDE  BhtOverride;

#endif
