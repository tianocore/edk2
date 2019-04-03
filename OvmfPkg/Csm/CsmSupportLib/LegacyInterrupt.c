/** @file
  Legacy Interrupt Support

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LegacyInterrupt.h"

//
// Handle for the Legacy Interrupt Protocol instance produced by this driver
//
STATIC EFI_HANDLE mLegacyInterruptHandle = NULL;

//
// Legacy Interrupt Device number (0x01 on piix4, 0x1f on q35/mch)
//
STATIC UINT8      mLegacyInterruptDevice;

//
// The Legacy Interrupt Protocol instance produced by this driver
//
STATIC EFI_LEGACY_INTERRUPT_PROTOCOL mLegacyInterrupt = {
  GetNumberPirqs,
  GetLocation,
  ReadPirq,
  WritePirq
};

STATIC UINT8 PirqReg[MAX_PIRQ_NUMBER] = { PIRQA, PIRQB, PIRQC, PIRQD, PIRQE, PIRQF, PIRQG, PIRQH };


/**
  Return the number of PIRQs supported by this chipset.

  @param[in]  This         Pointer to LegacyInterrupt Protocol
  @param[out] NumberPirqs  The pointer to return the max IRQ number supported

  @retval EFI_SUCCESS   Max PIRQs successfully returned

**/
EFI_STATUS
EFIAPI
GetNumberPirqs (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  OUT UINT8                          *NumberPirqs
  )
{
  *NumberPirqs = MAX_PIRQ_NUMBER;

  return EFI_SUCCESS;
}


/**
  Return PCI location of this device.
  $PIR table requires this info.

  @param[in]   This                - Protocol instance pointer.
  @param[out]  Bus                 - PCI Bus
  @param[out]  Device              - PCI Device
  @param[out]  Function            - PCI Function

  @retval  EFI_SUCCESS   Bus/Device/Function returned

**/
EFI_STATUS
EFIAPI
GetLocation (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  OUT UINT8                          *Bus,
  OUT UINT8                          *Device,
  OUT UINT8                          *Function
  )
{
  *Bus      = LEGACY_INT_BUS;
  *Device   = mLegacyInterruptDevice;
  *Function = LEGACY_INT_FUNC;

  return EFI_SUCCESS;
}


/**
  Builds the PCI configuration address for the register specified by PirqNumber

  @param[in]  PirqNumber - The PIRQ number to build the PCI configuration address for

  @return  The PCI Configuration address for the PIRQ
**/
UINTN
GetAddress (
  UINT8  PirqNumber
  )
{
  return PCI_LIB_ADDRESS(
          LEGACY_INT_BUS,
          mLegacyInterruptDevice,
          LEGACY_INT_FUNC,
          PirqReg[PirqNumber]
          );
}

/**
  Read the given PIRQ register

  @param[in]  This        Protocol instance pointer
  @param[in]  PirqNumber  The Pirq register 0 = A, 1 = B etc
  @param[out] PirqData    Value read

  @retval EFI_SUCCESS   Decoding change affected.
  @retval EFI_INVALID_PARAMETER   Invalid PIRQ number

**/
EFI_STATUS
EFIAPI
ReadPirq (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  IN  UINT8                          PirqNumber,
  OUT UINT8                          *PirqData
  )
{
  if (PirqNumber >= MAX_PIRQ_NUMBER) {
    return EFI_INVALID_PARAMETER;
  }

  *PirqData = PciRead8 (GetAddress (PirqNumber));
  *PirqData = (UINT8) (*PirqData & 0x7f);

  return EFI_SUCCESS;
}


/**
  Write the given PIRQ register

  @param[in]  This        Protocol instance pointer
  @param[in]  PirqNumber  The Pirq register 0 = A, 1 = B etc
  @param[out] PirqData    Value to write

  @retval EFI_SUCCESS   Decoding change affected.
  @retval EFI_INVALID_PARAMETER   Invalid PIRQ number

**/
EFI_STATUS
EFIAPI
WritePirq (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  IN  UINT8                          PirqNumber,
  IN  UINT8                          PirqData
  )
{
  if (PirqNumber >= MAX_PIRQ_NUMBER) {
    return EFI_INVALID_PARAMETER;
  }

  PciWrite8 (GetAddress (PirqNumber), PirqData);
  return EFI_SUCCESS;
}


/**
  Initialize Legacy Interrupt support

  @retval EFI_SUCCESS   Successfully initialized

**/
EFI_STATUS
LegacyInterruptInstall (
  VOID
  )
{
  UINT16      HostBridgeDevId;
  EFI_STATUS  Status;

  //
  // Make sure the Legacy Interrupt Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED(NULL, &gEfiLegacyInterruptProtocolGuid);

  //
  // Query Host Bridge DID to determine platform type, then set device number
  //
  HostBridgeDevId = PcdGet16 (PcdOvmfHostBridgePciDevId);
  switch (HostBridgeDevId) {
    case INTEL_82441_DEVICE_ID:
      mLegacyInterruptDevice = LEGACY_INT_DEV_PIIX4;
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      mLegacyInterruptDevice = LEGACY_INT_DEV_Q35;
      break;
    default:
      DEBUG ((EFI_D_ERROR, "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __FUNCTION__, HostBridgeDevId));
      ASSERT (FALSE);
      return EFI_UNSUPPORTED;
  }

  //
  // Make a new handle and install the protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mLegacyInterruptHandle,
                  &gEfiLegacyInterruptProtocolGuid,
                  &mLegacyInterrupt,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  return Status;
}

