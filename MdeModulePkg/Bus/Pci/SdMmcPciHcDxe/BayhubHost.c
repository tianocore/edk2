/** @file
  This driver is used to manage SD/MMC PCI host controllers override function
  for BayHub SD/eMMC host controller.

  Copyright (c) 2018 - 2019, BayHub Tech inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BayhubHost.h"
#include "SdMmcPciHcDxe.h"
#include <Protocol/PciIo.h>

/* Driver global variables*/
UINT16  BhtDeviceId = 0;

EDKII_SD_MMC_OVERRIDE  BhtOverride = {
  EDKII_SD_MMC_OVERRIDE_PROTOCOL_VERSION,
  BhtHostOverrideCapability,
  BhtHostOverrideNotifyPhase
};

/**
  Judgement function for Host type

  @param[in]      PciIo      The PCI IO protocol instance.

  @retval         UNSUPPORT  The host isn't Bayhub host.
  @retval         EMMC_HOST  The host is Bayhub eMMC host.
  @retval         SD_HOST    The host is Bayhub SD host.

**/
UINT8
BhtHostPciSupportType (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  PCI_TYPE00  Pci;
  UINT8       HostType;

  PciIo->Pci.Read (
               PciIo,
               EfiPciIoWidthUint32,
               0,
               sizeof (PCI_TYPE00) / sizeof (UINT32),
               &Pci
               );
  DEBUG ((DEBUG_INFO, "Check device %04x:%04x\n", Pci.Hdr.VendorId, Pci.Hdr.DeviceId));

  // Judgement bayhub device or not
  if (Pci.Hdr.VendorId != 0x1217) {
    HostType = UNSUPPORT;
    goto end;
  }

  BhtDeviceId = Pci.Hdr.DeviceId;

  // Bayhub host only 0x8620 is eMMC host
  switch (Pci.Hdr.DeviceId) {
    case PCI_DEV_ID_SB0:
      HostType = EMMC_HOST;
      break;
    case PCI_DEV_ID_SB1:
    default:
      HostType = SD_HOST;
      break;
  }

end:
  return HostType;
}

/**
  Read specified SD/MMC host controller mmio register

  @param[in]      PciIo        The PCI IO protocol instance.
  @param[in]      Offset       The Offset within the selected BAR to start the
                               memory operation.

  @retval         Value        The value of specified SD/MMC host controller mmio register.

**/
UINT32
BhtMmRead32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset
  )
{
  UINT32  Value;

  PciIo->Mem.Read (PciIo, EfiPciIoWidthUint32, 1, Offset, 1, &Value);
  return Value;
}

/**
  Write specified SD/MMC host controller mmio register

  @param[in]      PciIo        The PCI IO protocol instance.
  @param[in]      Offset       The Offset within the selected BAR to start the
                               memory operation.
  @param[in]      Value        The value to write.

**/
VOID
BhtMmWrite32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Value
  )
{
  PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, 1, Offset, 1, &Value);
}

/**
  Read specified SD/MMC host controller PCIe configure register

  @param[in]      PciIo        The PCI IO protocol instance.
  @param[in]      Offset       The Offset within the selected BAR to start the
                               memory operation.

  @retval         Value        The value of specified SD/MMC host controller PCIe configure register.

**/
UINT32
BhtPciRead32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset
  )
{
  UINT32  Index;
  UINT32  TmpBuff[2];

  if ((BhtDeviceId == PCI_DEV_ID_SDS0) ||
      (BhtDeviceId == PCI_DEV_ID_SDS1) ||
      (BhtDeviceId == PCI_DEV_ID_FJ2) ||
      (BhtDeviceId == PCI_DEV_ID_SB0) ||
      (BhtDeviceId == PCI_DEV_ID_SB1))
  {
    // For Sandstorm, HW implement a mapping method by memory space reg to access PCI reg.
    // Enable mapping

    // Check function conflict
    if ((BhtDeviceId == PCI_DEV_ID_SDS0) ||
        (BhtDeviceId == PCI_DEV_ID_FJ2) ||
        (BhtDeviceId == PCI_DEV_ID_SB0) ||
        (BhtDeviceId == PCI_DEV_ID_SB1))
    {
      Index = 0;
      BhtMmWrite32 (PciIo, BHT_PCR_MAP_EN, 0x40000000);
      while ((BhtMmRead32 (PciIo, BHT_PCR_MAP_EN) & 0x40000000) == 0) {
        if (Index == 5) {
          goto RD_DIS_MAPPING;
        }

        gBS->Stall (1000);
        Index++;
        BhtMmWrite32 (PciIo, BHT_PCR_MAP_EN, 0x40000000);
      }
    } else if (BhtDeviceId == PCI_DEV_ID_SDS1) {
      Index = 0;
      BhtMmWrite32 (PciIo, BHT_PCR_MAP_EN, 0x20000000);
      while ((BhtMmRead32 (PciIo, BHT_PCR_MAP_EN) & 0x20000000) == 0) {
        if (Index == 5) {
          goto RD_DIS_MAPPING;
        }

        gBS->Stall (1000);
        Index++;
        BhtMmWrite32 (PciIo, BHT_PCR_MAP_EN, 0x20000000);
      }
    }

    // Check last operation is complete
    Index = 0;
    while (BhtMmRead32 (PciIo, BHT_PCR_MAP_CTL) & 0xc0000000) {
      if (Index == 5) {
        goto RD_DIS_MAPPING;
      }

      gBS->Stall (1000);
      Index++;
    }

    // Set register address
    TmpBuff[0]  = 0x40000000;
    TmpBuff[0] |= Offset;
    BhtMmWrite32 (PciIo, BHT_PCR_MAP_CTL, TmpBuff[0]);

    // Check read is complete
    Index = 0;
    while (BhtMmRead32 (PciIo, BHT_PCR_MAP_CTL) & 0x40000000) {
      if (Index == 5) {
        goto RD_DIS_MAPPING;
      }

      gBS->Stall (1000);
      Index++;
    }

    // Get PCIR value
    TmpBuff[1] = BhtMmRead32 (PciIo, BHT_PCR_MAP_VAL);

RD_DIS_MAPPING:
    // Disable mapping
    BhtMmWrite32 (PciIo, BHT_PCR_MAP_EN, 0x80000000);
    return TmpBuff[1];
  } else {
    return BhtMmRead32 (PciIo, Offset);
  }
}

/**
  Write specified SD/MMC host controller PCIe configure register

  @param[in]      PciIo        The PCI IO protocol instance.
  @param[in]      Offset       The Offset within the selected BAR to start the
                               memory operation.
  @param[in]      Value        The value to write.

**/
VOID
BhtPciWrite32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Value
  )
{
  UINT32  TmpBuff;
  UINT32  Index;

  if ((BhtDeviceId == PCI_DEV_ID_SDS0) ||
      (BhtDeviceId == PCI_DEV_ID_SDS1) ||
      (BhtDeviceId == PCI_DEV_ID_FJ2) ||
      (BhtDeviceId == PCI_DEV_ID_SB0) ||
      (BhtDeviceId == PCI_DEV_ID_SB1))
  {
    // Enable mapping
    // Check function conflict
    if ((BhtDeviceId == PCI_DEV_ID_SDS0) ||
        (BhtDeviceId == PCI_DEV_ID_FJ2) ||
        (BhtDeviceId == PCI_DEV_ID_SB0) ||
        (BhtDeviceId == PCI_DEV_ID_SB1))
    {
      Index = 0;
      BhtMmWrite32 (PciIo, BHT_PCR_MAP_EN, 0x40000000);
      while ((BhtMmRead32 (PciIo, BHT_PCR_MAP_EN) & 0x40000000) == 0) {
        if (Index == 5) {
          goto WR_DIS_MAPPING;
        }

        gBS->Stall (1000);
        Index++;
        BhtMmWrite32 (PciIo, BHT_PCR_MAP_EN, 0x40000000);
      }
    } else if (BhtDeviceId == PCI_DEV_ID_SDS1) {
      Index = 0;
      BhtMmWrite32 (PciIo, BHT_PCR_MAP_EN, 0x20000000);

      while ((BhtMmRead32 (PciIo, BHT_PCR_MAP_EN) & 0x20000000) == 0) {
        if (Index == 5) {
          goto WR_DIS_MAPPING;
        }

        gBS->Stall (1000);
        Index++;
        BhtMmWrite32 (PciIo, BHT_PCR_MAP_EN, 0x20000000);
      }
    }

    // Enable MEM access
    BhtMmWrite32 (PciIo, BHT_PCR_MAP_VAL, 0x80000000);
    BhtMmWrite32 (PciIo, BHT_PCR_MAP_CTL, 0x800000D0);

    // Check last operation is complete
    Index = 0;
    while (BhtMmRead32 (PciIo, BHT_PCR_MAP_CTL) & 0xc0000000) {
      if (Index == 5) {
        goto WR_DIS_MAPPING;
      }

      gBS->Stall (1000);
      Index++;
    }

    // Set write value
    BhtMmWrite32 (PciIo, BHT_PCR_MAP_VAL, Value);
    // Set register address
    TmpBuff  = 0x80000000;
    TmpBuff |= Offset;
    BhtMmWrite32 (PciIo, BHT_PCR_MAP_CTL, TmpBuff);

    // Check write is complete
    Index = 0;
    while (BhtMmRead32 (PciIo, BHT_PCR_MAP_CTL) & 0x80000000) {
      if (Index == 5) {
        goto WR_DIS_MAPPING;
      }

      gBS->Stall (1000);
      Index++;
    }

WR_DIS_MAPPING:
    // Disable MEM access
    BhtMmWrite32 (PciIo, BHT_PCR_MAP_VAL, 0x80000001);
    BhtMmWrite32 (PciIo, BHT_PCR_MAP_CTL, 0x800000D0);

    // Check last operation is complete
    Index = 0;
    while (BhtMmRead32 (PciIo, BHT_PCR_MAP_CTL) & 0xc0000000) {
      if (Index == 5) {
        break;
      }

      gBS->Stall (1000);
      Index++;
    }

    // Disable function conflict
    // Disable mapping
    BhtMmWrite32 (PciIo, BHT_PCR_MAP_EN, 0x80000000);
  } else {
    BhtMmWrite32 (PciIo, Offset, Value);
  }
}

/**
  Do OR operation with the value of the specified SD/MMC host controller PCIe configure register

  @param[in]      PciIo        The PCI IO protocol instance.
  @param[in]      Offset       The Offset within the selected BAR to start the
                               memory operation.
  @param[in]      Value        The value to do OR operation.

**/
VOID
BhtPciOr32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Value
  )
{
  UINT32  OldValue;

  OldValue = BhtPciRead32 (PciIo, Offset);
  BhtPciWrite32 (PciIo, Offset, Value | OldValue);
}

/**
  Do AND operation with the value of the specified SD/MMC host controller PCIe configure register

  @param[in]      PciIo        The PCI IO protocol instance.
  @param[in]      Offset       The Offset within the selected BAR to start the
                               memory operation.
  @param[in]      Value        The value to do AND operation.

**/
VOID
PciBhtAnd32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Value
  )
{
  UINT32  OldValue;

  OldValue = BhtPciRead32 (PciIo, Offset);
  BhtPciWrite32 (PciIo, Offset, Value & OldValue);
}

/**
  Dump the content of SD/MMC host controller's key Registers.

  @param[in]      PciIo        The PCI IO protocol instance.

**/
VOID
BhtDbgDump (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  if (BhtHostPciSupportType (PciIo) != UNSUPPORT) {
    DEBUG ((DEBUG_INFO, "HOST_CLK_DRIVE_STRENGTH: 0x%x\n", HOST_CLK_DRIVE_STRENGTH));
    DEBUG ((DEBUG_INFO, "HOST_DAT_DRIVE_STRENGTH: 0x%x\n", HOST_DAT_DRIVE_STRENGTH));
    DEBUG ((DEBUG_INFO, "Host register 0x24: 0x%08X\n", BhtMmRead32 (PciIo, SD_MMC_HC_PRESENT_STATE)));
    DEBUG ((DEBUG_INFO, "Host register 0x28: 0x%08X\n", BhtMmRead32 (PciIo, SD_MMC_HC_HOST_CTRL1)));
    DEBUG ((DEBUG_INFO, "Host register 0x2C: 0x%08X\n", BhtMmRead32 (PciIo, SD_MMC_HC_CLOCK_CTRL)));
    DEBUG ((DEBUG_INFO, "Host register 0x30: 0x%08X\n", BhtMmRead32 (PciIo, SD_MMC_HC_NOR_INT_STS)));
    DEBUG ((DEBUG_INFO, "Host register 0x3C: 0x%08X\n", BhtMmRead32 (PciIo, SD_MMC_HC_AUTO_CMD_ERR_STS)));
    DEBUG ((DEBUG_INFO, "Host register 0x110: 0x%08X\n", BhtMmRead32 (PciIo, 0x110)));
    DEBUG ((DEBUG_INFO, "Host register 0x114: 0x%08X\n", BhtMmRead32 (PciIo, 0x114)));
    DEBUG ((DEBUG_INFO, "Host register 0x1A8: 0x%08X\n", BhtMmRead32 (PciIo, 0x1A8)));
    DEBUG ((DEBUG_INFO, "Host register 0x1AC: 0x%08X\n", BhtMmRead32 (PciIo, 0x1AC)));
    DEBUG ((DEBUG_INFO, "Host register 0x1B0: 0x%08X\n", BhtMmRead32 (PciIo, 0x1B0)));
    DEBUG ((DEBUG_INFO, "Host register 0x1CC: 0x%08X\n", BhtMmRead32 (PciIo, 0x1CC)));

    DEBUG ((DEBUG_INFO, "PCR 0x300: 0x%08X\n", BhtPciRead32 (PciIo, 0x300)));
    DEBUG ((DEBUG_INFO, "PCR 0x304: 0x%08X\n", BhtPciRead32 (PciIo, 0x304)));
    DEBUG ((DEBUG_INFO, "PCR 0x328: 0x%08X\n", BhtPciRead32 (PciIo, 0x328)));
    DEBUG ((DEBUG_INFO, "PCR 0x3E4: 0x%08X\n", BhtPciRead32 (PciIo, 0x3e4)));
  }
}

/**
  Configure Bayhub Host before initial SD/MMC host controller

  @param[in] PciIo          The PCI IO protocol instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The host controller is initialized successfully.
  @retval Others            The host controller isn't initialized successfully.

**/
EFI_STATUS
EFIAPI
BhtHostInit (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT8                Slot
  )
{
  UINT16      EmmcVar;
  UINT32      Value32;
  EFI_STATUS  Status;

  if (BhtHostPciSupportType (PciIo) == EMMC_HOST) {
    /* FET on */
    BhtPciOr32 (PciIo, 0xEC, 0x3);
    /* Led on */
    BhtPciOr32 (PciIo, 0xD4, BIT6);
    /* Set 1.8v emmc signaling flag */
    BhtPciOr32 (PciIo, 0x308, BIT4);
  }

  /* Set 200MBaseClock */
  Value32  = BhtPciRead32 (PciIo, 0x304);
  Value32 &= 0x0000FFFF;
  Value32 |= 0x25100000;

  /* Host driver strength setting*/
  EmmcVar  = HOST_CLK_DRIVE_STRENGTH;
  Value32 &= 0xFFFFFF8F;
  Value32 |= ((EmmcVar & 0x7) << 4);
  EmmcVar  = HOST_DAT_DRIVE_STRENGTH;
  Value32 &= 0xFFFFFFF1;
  Value32 |= ((EmmcVar & 0x7) << 1);

  // PCR 0x3E4[22] = 1'b1(Divide the clock to 1/4)
  BhtPciWrite32 (PciIo, 0x304, Value32);
  BhtPciOr32 (PciIo, 0x3E4, BIT22);

  // enable internal clk
  Value32 = BIT0;
  Status  = SdMmcHcOrMmio (PciIo, Slot, SD_MMC_HC_CLOCK_CTRL, sizeof (Value32), &Value32);

  // reset pll start
  Status   = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, TRUE, sizeof (Value32), &Value32);
  Value32 |= BIT12;
  Status   = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, FALSE, sizeof (Value32), &Value32);
  gBS->Stall (1);

  // reset pll end
  Status   = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, TRUE, sizeof (Value32), &Value32);
  Value32 &= ~BIT12;
  Value32 |= BIT18;
  Status   = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, FALSE, sizeof (Value32), &Value32);

  // wait BaseClk stable 0x1CC bit14
  Status = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, TRUE, sizeof (Value32), &Value32);
  while (!(Value32 & BIT14)) {
    gBS->Stall (100);
    Status = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, TRUE, sizeof (Value32), &Value32);
    DEBUG ((DEBUG_INFO, "1CC=0x%08x\n", Value32));
  }

  if (Value32 & BIT18) {
    // Wait 2nd Card Detect debounce Finished by wait twice of debounce max time
    while (1) {
      Status = SdMmcHcRwMmio (PciIo, Slot, SD_MMC_HC_PRESENT_STATE, TRUE, sizeof (Value32), &Value32);
      if (((Value32 >> 16) & 0x01) == ((Value32 >> 18) & 0x01)) {
        break;
      }
    }

    // force pll active end
    Status   = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, TRUE, sizeof (Value32), &Value32);
    Value32 &= ~BIT18;
    Status   = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, FALSE, sizeof (Value32), &Value32);
  }

  return Status;
}

/**
  Do Bayhub Host Voltage after initial SD/MMC host controller

  @param[in] PciIo          The PCI IO protocol instance.
  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The host controller is initialized successfully.
  @retval Others            The host controller isn't initialized successfully.

**/
EFI_STATUS
EFIAPI
BhtHostVoltageSet (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINT8                Slot
  )
{
  EFI_STATUS              Status;
  UINT8                   HostCtrl2;
  UINT16                  ControllerVer;
  SD_MMC_HC_PRIVATE_DATA  Private;

  if (BhtHostPciSupportType (PciIo) == EMMC_HOST) {
    // 1.8V signaling enable
    HostCtrl2 = BIT3;
    Status    = SdMmcHcOrMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
    gBS->Stall (5000);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    /* when disable 1.8v setting for SD card, driver need to delay 10ms
    waiting for card voltage stable. */
    gBS->Stall (10000);
  }

  Status = SdMmcHcRwMmio (PciIo, Slot, SD_MMC_HC_CTRL_VER, TRUE, sizeof (ControllerVer), &ControllerVer);

  ControllerVer &= 0x0f;

  Private.PciIo                   = PciIo;
  Private.BaseClkFreq[Slot]       = 200;
  Private.ControllerVersion[Slot] = ControllerVer;

  Status = SdMmcHcClockSupply (&Private, Slot, 0, TRUE, 400);

  return Status;
}

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
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;

  Status = gBS->HandleProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "open gEfiPciIoProtocolGuid failed at BhtHostOverrideNotifyPhase, Status = 0x%llx\n", Status));
    return Status;
  }

  if (BhtHostPciSupportType (PciIo) == UNSUPPORT) {
    return EFI_SUCCESS;
  }

  switch (PhaseType) {
    case EdkiiSdMmcInitHostPre:
      DEBUG ((DEBUG_INFO, "EdkiiSdMmcInitHostPre\n"));
      Status = BhtHostInit (PciIo, Slot);
      break;
    case EdkiiSdMmcInitHostPost:
      DEBUG ((DEBUG_INFO, "EdkiiSdMmcInitHostPost\n"));
      Status = BhtHostVoltageSet (PciIo, Slot);
      break;
    case EdkiiSdMmcResetPre:
      DEBUG ((DEBUG_INFO, "EdkiiSdMmcResetPre\n"));
      Status = EFI_SUCCESS;
      break;
    case EdkiiSdMmcResetPost:
      DEBUG ((DEBUG_INFO, "EdkiiSdMmcResetPost\n"));
      Status = EFI_SUCCESS;
      break;
    case EdkiiSdMmcUhsSignaling:
      DEBUG ((DEBUG_INFO, "EdkiiSdMmcUhsSignaling\n"));
      Status = EFI_SUCCESS;
      break;
    case EdkiiSdMmcSwitchClockFreqPost:
      DEBUG ((DEBUG_INFO, "EdkiiSdMmcSwitchClockFreqPost\n"));
      BhtDbgDump (PciIo);
      Status = EFI_SUCCESS;
      break;
    case EdkiiSdMmcGetOperatingParam:
      DEBUG ((DEBUG_INFO, "EdkiiSdMmcGetOperatingParam\n"));
      Status = EFI_SUCCESS;
      break;
    default:
      DEBUG ((DEBUG_INFO, "Overridenotify\n"));
      Status = EFI_SUCCESS;
      break;
  }

  return Status;
}

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
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  SD_MMC_HC_SLOT_CAP   *Cap;

  Status = gBS->HandleProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (BhtHostPciSupportType (PciIo) == UNSUPPORT) {
    return EFI_SUCCESS;
  }

  Cap = SdMmcHcSlotCapability;

  Cap->HighSpeed   = 1;
  Cap->Sdr104      = 0;
  Cap->Ddr50       = 0;
  Cap->Sdr50       = 0;
  Cap->Hs400       = 0;
  Cap->BusWidth8   = 1;
  Cap->BaseClkFreq = 200;
  Cap->SysBus64V3  = 0;
  *BaseClkFreq     = Cap->BaseClkFreq;

  return EFI_SUCCESS;
}
