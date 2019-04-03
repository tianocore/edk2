/** @file
Common Lib function for QNC internal network access.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <Uefi.h>

#include <IntelQNCRegs.h>
#include <Library/QNCAccessLib.h>
#include <Library/DebugLib.h>
#include <IndustryStandard/Pci22.h>

UINT32
EFIAPI
QNCPortRead(
  UINT8 Port,
  UINT32 RegAddress
  )
{
  McD0PciCfg32 (QNC_ACCESS_PORT_MEA) = (RegAddress & 0xFFFFFF00);
  McD0PciCfg32 (QNC_ACCESS_PORT_MCR) = MESSAGE_READ_DW (Port, RegAddress);
  return McD0PciCfg32 (QNC_ACCESS_PORT_MDR);
}

VOID
EFIAPI
QNCPortWrite (
  UINT8 Port,
  UINT32 RegAddress,
  UINT32 WriteValue
  )
{
  McD0PciCfg32 (QNC_ACCESS_PORT_MDR) = WriteValue;
  McD0PciCfg32 (QNC_ACCESS_PORT_MEA) = (RegAddress & 0xFFFFFF00);
  McD0PciCfg32 (QNC_ACCESS_PORT_MCR) = MESSAGE_WRITE_DW (Port, RegAddress);
}

UINT32
EFIAPI
QNCAltPortRead (
  UINT8 Port,
  UINT32 RegAddress
  )
{
  McD0PciCfg32 (QNC_ACCESS_PORT_MEA) = (RegAddress & 0xFFFFFF00);
  McD0PciCfg32 (QNC_ACCESS_PORT_MCR) = ALT_MESSAGE_READ_DW (Port, RegAddress);
  return McD0PciCfg32 (QNC_ACCESS_PORT_MDR);
}

VOID
EFIAPI
QNCAltPortWrite (
  UINT8 Port,
  UINT32 RegAddress,
  UINT32 WriteValue
  )
{
  McD0PciCfg32 (QNC_ACCESS_PORT_MDR) = WriteValue;
  McD0PciCfg32 (QNC_ACCESS_PORT_MEA) = (RegAddress & 0xFFFFFF00);
  McD0PciCfg32 (QNC_ACCESS_PORT_MCR) = ALT_MESSAGE_WRITE_DW (Port, RegAddress);
}

UINT32
EFIAPI
QNCPortIORead(
  UINT8 Port,
  UINT32 RegAddress
  )
{
  McD0PciCfg32 (QNC_ACCESS_PORT_MEA) = (RegAddress & 0xFFFFFF00);
  McD0PciCfg32 (QNC_ACCESS_PORT_MCR) = MESSAGE_IO_READ_DW (Port, RegAddress);
  return McD0PciCfg32 (QNC_ACCESS_PORT_MDR);
}

VOID
EFIAPI
QNCPortIOWrite (
  UINT8 Port,
  UINT32 RegAddress,
  UINT32 WriteValue
  )
{
  McD0PciCfg32 (QNC_ACCESS_PORT_MDR) = WriteValue;
  McD0PciCfg32 (QNC_ACCESS_PORT_MEA) = (RegAddress & 0xFFFFFF00);
  McD0PciCfg32 (QNC_ACCESS_PORT_MCR) = MESSAGE_IO_WRITE_DW (Port, RegAddress);
}

RETURN_STATUS
EFIAPI
QNCMmIoWrite (
  UINT32             MmIoAddress,
  QNC_MEM_IO_WIDTH    Width,
  UINT32             DataNumber,
  VOID               *pData
  )
/*++

Routine Description:

  This is for the special consideration for QNC MMIO write, as required by FWG, a reading must be performed after MMIO writing
to ensure the expected write is processed and data is flushed into chipset

Arguments:

  Row -- row number to be cleared ( start from 1 )

Returns:

  EFI_SUCCESS

--*/
{
  RETURN_STATUS  Status;
  UINTN          Index;

  Status = RETURN_SUCCESS;

  for (Index =0; Index < DataNumber; Index++) {
    switch (Width) {
      case QNCMmioWidthUint8:
        QNCMmio8 (MmIoAddress, 0) = ((UINT8 *)pData)[Index];
        if (QNCMmio8 (MmIoAddress, 0) != ((UINT8*)pData)[Index]) {
          Status = RETURN_DEVICE_ERROR;
          break;
        }
        break;

      case QNCMmioWidthUint16:
        QNCMmio16 (MmIoAddress, 0) = ((UINT16 *)pData)[Index];
        if (QNCMmio16 (MmIoAddress, 0) != ((UINT16 *)pData)[Index]) {
          Status = RETURN_DEVICE_ERROR;
          break;
        }
        break;

      case QNCMmioWidthUint32:
        QNCMmio32 (MmIoAddress, 0) = ((UINT32 *)pData)[Index];
        if (QNCMmio32 (MmIoAddress, 0) != ((UINT32 *)pData)[Index]) {
          Status = RETURN_DEVICE_ERROR;
          break;
        }
        break;

      case QNCMmioWidthUint64:
        QNCMmio64 (MmIoAddress, 0) = ((UINT64 *)pData)[Index];
        if (QNCMmio64 (MmIoAddress, 0) != ((UINT64 *)pData)[Index]) {
          Status = RETURN_DEVICE_ERROR;
          break;
        }
        break;

      default:
        break;
    }
  }

  return Status;
}

UINT32
EFIAPI
QncHsmmcRead (
  VOID
  )
{
  return QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HSMMC);
}

VOID
EFIAPI
QncHsmmcWrite (
  UINT32 WriteValue
  )
{
  UINT16  DeviceId;
  UINT32  Data32;

  //
  // Check what Soc we are running on (read Host bridge DeviceId)
  //
  DeviceId = QNCMmPci16(0, MC_BUS, MC_DEV, MC_FUN, PCI_DEVICE_ID_OFFSET);

  if (DeviceId == QUARK2_MC_DEVICE_ID) {
    //
    // Disable HSMMC configuration
    //
    Data32 = QncHsmmcRead ();
    Data32 &= ~SMM_CTL_EN;
    QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HSMMC, Data32);

    //
    // Validate HSMMC configuration is disabled
    //
    Data32 = QncHsmmcRead ();
    ASSERT((Data32 & SMM_CTL_EN) == 0);

    //
    // Enable HSMMC configuration
    //
    WriteValue |= SMM_CTL_EN;
  }

  //
  // Write the register value
  //
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HSMMC, WriteValue);

  if (DeviceId == QUARK2_MC_DEVICE_ID) {
    //
    // Validate HSMMC configuration is enabled
    //
    Data32 = QncHsmmcRead ();
    ASSERT((Data32 & SMM_CTL_EN) != 0);
  }
}

VOID
EFIAPI
QncImrWrite (
  UINT32 ImrBaseOffset,
  UINT32 ImrLow,
  UINT32 ImrHigh,
  UINT32 ImrReadMask,
  UINT32 ImrWriteMask
  )
{
  UINT16  DeviceId;
  UINT32  Data32;

  //
  // Check what Soc we are running on (read Host bridge DeviceId)
  //
  DeviceId = QNCMmPci16(0, MC_BUS, MC_DEV, MC_FUN, PCI_DEVICE_ID_OFFSET);

  //
  // Disable IMR protection
  //
  if (DeviceId == QUARK2_MC_DEVICE_ID) {
    //
    // Disable IMR protection
    //
    Data32 = QNCPortRead (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXL);
    Data32 &= ~IMR_EN;
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXL, Data32);

    //
    // Validate IMR protection is disabled
    //
    Data32 = QNCPortRead (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXL);
    ASSERT((Data32 & IMR_EN) == 0);

    //
    // Update the IMR (IMRXL must be last as it may enable IMR violation checking)
    //
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXRM, ImrReadMask);
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXWM, ImrWriteMask);
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXH, ImrHigh);
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXL, ImrLow);

    //
    // Validate IMR protection is enabled/disabled
    //
    Data32 = QNCPortRead (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXL);
    ASSERT((Data32 & IMR_EN) == (ImrLow & IMR_EN));
  } else {
    //
    // Disable IMR protection (allow all access)
    //
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXRM, (UINT32)IMRX_ALL_ACCESS);
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXWM, (UINT32)IMRX_ALL_ACCESS);

    //
    // Update the IMR (IMRXRM/IMRXWM must be last as they restrict IMR access)
    //
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXL, (ImrLow & ~IMR_EN));
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXH, ImrHigh);
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXRM, ImrReadMask);
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, ImrBaseOffset+QUARK_NC_MEMORY_MANAGER_IMRXWM, ImrWriteMask);
  }
}

VOID
EFIAPI
QncIClkAndThenOr (
  UINT32 RegAddress,
  UINT32 AndValue,
  UINT32 OrValue
  )
{
  UINT32 RegValue;
  //
  // Whenever an iCLK SB register (Endpoint 32h) is being programmed the access
  // should always consist of a READ from the address followed by 2 identical
  // WRITEs to that address.
  //
  RegValue = QNCAltPortRead (QUARK_ICLK_SB_PORT_ID, RegAddress);
  RegValue &= AndValue;
  RegValue |= OrValue;
  QNCAltPortWrite (QUARK_ICLK_SB_PORT_ID, RegAddress, RegValue);
  QNCAltPortWrite (QUARK_ICLK_SB_PORT_ID, RegAddress, RegValue);
}

VOID
EFIAPI
QncIClkOr (
  UINT32 RegAddress,
  UINT32 OrValue
  )
{
  UINT32 RegValue;
  //
  // Whenever an iCLK SB register (Endpoint 32h) is being programmed the access
  // should always consist of a READ from the address followed by 2 identical
  // WRITEs to that address.
  //
  RegValue = QNCAltPortRead (QUARK_ICLK_SB_PORT_ID, RegAddress);
  RegValue |= OrValue;
  QNCAltPortWrite (QUARK_ICLK_SB_PORT_ID, RegAddress, RegValue);
  QNCAltPortWrite (QUARK_ICLK_SB_PORT_ID, RegAddress, RegValue);
}
