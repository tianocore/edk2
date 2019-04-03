/** @file

This driver is responsible for the registration of child drivers
and the abstraction of the QNC SMI sources.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "QNCSmmHelpers.h"

//
// Help handle porting bit shifts to IA-64.
//
#define BIT_ZERO 0x00000001


VOID
QNCSmmPublishDispatchProtocols(
  VOID
  )
{
  UINTN      Index;
  EFI_STATUS Status;

  //
  // Install protocol interfaces.
  //
  for (Index = 0; Index < NUM_PROTOCOLS; Index++) {
    Status = gSmst->SmmInstallProtocolInterface (
                 &mPrivateData.InstallMultProtHandle,
                      mPrivateData.Protocols[Index].Guid,
                      EFI_NATIVE_INTERFACE,
                      &mPrivateData.Protocols[Index].Protocols.Generic
                 );

  ASSERT_EFI_ERROR (Status);
}
}

EFI_STATUS
QNCSmmInitHardware(
  VOID
  )
/*++

Routine Description:

  Initialize bits that aren't necessarily related to an SMI source.

Dependencies:

  gSmst - SMM System Table; contains an entry for SMM CPU IO

Returns:

  EFI_SUCCESS.  Asserts, otherwise.

--*/
{
  EFI_STATUS Status;

  //
  // Clear all SMIs
  //
  QNCSmmClearSmi();

  Status = QNCSmmEnableGlobalSmiBit ();
  ASSERT_EFI_ERROR (Status);

  //
  // Be *really* sure to clear all SMIs
  //
  QNCSmmClearSmi ();

  return EFI_SUCCESS;
}

EFI_STATUS
QNCSmmEnableGlobalSmiBit (
  VOID
  )
/*++

Routine Description:

  Enables the QNC to generate SMIs. Note that no SMIs will be generated
  if no SMI sources are enabled. Conversely, no enabled SMI source will
  generate SMIs if SMIs are not globally enabled. This is the main
  switchbox for SMI generation.

Arguments:

  None

Returns:

  EFI_SUCCESS.
  Asserts, otherwise.

--*/
{
  UINT32        NewValue;

  //
  // Enable SMI globally
  //
  NewValue = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC);
  NewValue |= SMI_EN;
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC, NewValue);

  return EFI_SUCCESS;
}

EFI_STATUS
QNCSmmClearSmi(
  VOID
  )
/*++

Routine Description:

  Clears the SMI after all SMI source have been processed.
  Note that this function will not work correctly (as it is
  written) unless all SMI sources have been processed.
  A revision of this function could manually clear all SMI
  status bits to guarantee success.

Returns:

  EFI_SUCCESS.
  Asserts, otherwise.

--*/
{
  BOOLEAN EosSet;
  BOOLEAN SciEn;

  UINT32 Pm1Cnt = 0;
  UINT16 Pm1Sts = 0;
  UINT32 Gpe0Sts = 0;
  UINT32 SmiSts  = 0;

  //
  // Determine whether an ACPI OS is present (via the SCI_EN bit)
  //
  Pm1Cnt = IoRead32(PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C);
  SciEn = (BOOLEAN)((Pm1Cnt & B_QNC_PM1BLK_PM1C_SCIEN) == B_QNC_PM1BLK_PM1C_SCIEN);

  if (SciEn == FALSE) {

    //
    // Clear any SMIs that double as SCIs (when SCI_EN==0)
    //
    Pm1Sts = (B_QNC_PM1BLK_PM1S_WAKE | B_QNC_PM1BLK_PM1S_PCIEWSTS | B_QNC_PM1BLK_PM1S_RTC | B_QNC_PM1BLK_PM1S_GLOB | B_QNC_PM1BLK_PM1S_TO);

    Gpe0Sts = B_QNC_GPE0BLK_GPE0S_ALL;

    IoOr16((PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1S), Pm1Sts);
    IoOr32(((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_GPE0S), Gpe0Sts);
  }

  //
  // Clear all SMIs that are unaffected by SCI_EN
  //
  SmiSts = IoRead32((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_SMIS);
  SmiSts |= B_QNC_GPE0BLK_SMIS_ALL;
  IoWrite32(((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_SMIS), SmiSts);

  //
  // Try to clear the EOS bit. ASSERT on an error
  //
  EosSet = QNCSmmSetAndCheckEos();
  ASSERT (EosSet);

  return EFI_SUCCESS;
}

BOOLEAN
QNCSmmSetAndCheckEos(
  VOID
  )
{
  //
  // Reset the QNC to generate subsequent SMIs
  //
  IoOr32(((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_SMIS), B_QNC_GPE0BLK_SMIS_EOS);
    return TRUE;
}

BOOLEAN
QNCSmmGetSciEn(
  )
{
  BOOLEAN SciEn;
  UINT32 Pm1Cnt;

  //
  // Determine whether an ACPI OS is present (via the SCI_EN bit)
  //
  Pm1Cnt = IoRead32(PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C);

  SciEn = (BOOLEAN)((Pm1Cnt & B_QNC_PM1BLK_PM1C_SCIEN) == B_QNC_PM1BLK_PM1C_SCIEN);

  return SciEn;
}

//
// These may or may not need to change w/ the QNC version; they're highly IA-32 dependent, though.
//

BOOLEAN
ReadBitDesc (
  CONST QNC_SMM_BIT_DESC  *BitDesc
  )
{
  UINT64           Register;
  UINT32           PciBus;
  UINT32           PciDev;
  UINT32           PciFun;
  UINT32           PciReg;
  BOOLEAN          BitWasOne;

  ASSERT (BitDesc != NULL );
  ASSERT (!IS_BIT_DESC_NULL( *BitDesc ) );

  Register  = 0;
  BitWasOne = FALSE;

  switch (BitDesc->Reg.Type) {

  case ACPI_ADDR_TYPE:
    //
    // Double check that we correctly read in the acpi base address
    //
    ASSERT ((PcdGet16 (PcdPm1blkIoBaseAddress) != 0x0) && ((PcdGet16 (PcdPm1blkIoBaseAddress) & 0x1) != 0x1) );

    switch (BitDesc->SizeInBytes) {

    case 0:
      //
      // Chances are that this field didn't get initialized.
      // Check your assignments to bit descriptions.
      //
      ASSERT (FALSE );
      break;

      case 1:
      Register = (UINT64) IoRead8 (PcdGet16 (PcdPm1blkIoBaseAddress) + BitDesc->Reg.Data.acpi);
        break;

      case 2:
      Register = (UINT64) IoRead16 (PcdGet16 (PcdPm1blkIoBaseAddress) + BitDesc->Reg.Data.acpi);
        break;

      case 4:
      Register = (UINT64) IoRead32 (PcdGet16 (PcdPm1blkIoBaseAddress) + BitDesc->Reg.Data.acpi);
        break;

      default:
        //
        // Unsupported or invalid register size
        //
        ASSERT (FALSE );
        break;
      };

    if ((Register & LShiftU64 (BIT_ZERO, BitDesc->Bit)) != 0) {
        BitWasOne = TRUE;
      } else {
        BitWasOne = FALSE;
      }
    break;

  case GPE_ADDR_TYPE:
      //
    // Double check that we correctly read in the gpe base address
      //
    ASSERT (((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) != 0x0) && (((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) & 0x1) != 0x1) );

    switch (BitDesc->SizeInBytes) {

    case 0:
      //
      // Chances are that this field didn't get initialized.
      // Check your assignments to bit descriptions.
      //
      ASSERT (FALSE );
      break;

    case 1:
      Register = (UINT64) IoRead8 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + BitDesc->Reg.Data.gpe);
      break;

    case 2:
      Register = (UINT64) IoRead16 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + BitDesc->Reg.Data.gpe);
      break;

    case 4:
      Register = (UINT64) IoRead32 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + BitDesc->Reg.Data.gpe);
      break;

    default:
      //
      // Unsupported or invalid register size
      //
      ASSERT (FALSE );
      break;
    };

    if ((Register & LShiftU64 (BIT_ZERO, BitDesc->Bit)) != 0) {
        BitWasOne = TRUE;
      } else {
        BitWasOne = FALSE;
      }
    break;

  case MEMORY_MAPPED_IO_ADDRESS_TYPE:
    //
    // Read the register, and it with the bit to read
    //

    //
    // This code does not support reads greater then 64 bits
    //
    ASSERT (BitDesc->SizeInBytes <= 8);
    CopyMem (&Register, BitDesc->Reg.Data.Mmio, BitDesc->SizeInBytes);
    Register &= LShiftU64 (BIT0, BitDesc->Bit);
    if (Register) {
      BitWasOne = TRUE;
    } else {
      BitWasOne = FALSE;
    }
    break;

  case PCI_ADDR_TYPE:
    PciBus = BitDesc->Reg.Data.pci.Fields.Bus;
    PciDev = BitDesc->Reg.Data.pci.Fields.Dev;
    PciFun = BitDesc->Reg.Data.pci.Fields.Fnc;
    PciReg = BitDesc->Reg.Data.pci.Fields.Reg;
    switch (BitDesc->SizeInBytes) {

    case 0:
      //
      // Chances are that this field didn't get initialized.
      // Check your assignments to bit descriptions.
      ASSERT (FALSE );
      break;

    case 1:
      Register = (UINT64) PciRead8 (PCI_LIB_ADDRESS (PciBus, PciDev, PciFun, PciReg));
      break;

    case 2:
      Register = (UINT64) PciRead16 (PCI_LIB_ADDRESS (PciBus, PciDev, PciFun, PciReg));
      break;

    case 4:
      Register = (UINT64) PciRead32 (PCI_LIB_ADDRESS (PciBus, PciDev, PciFun, PciReg));
      break;

    default:
      //
      // Unsupported or invalid register size
      //
      ASSERT (FALSE );
      break;
    };

    if ((Register & LShiftU64 (BIT_ZERO, BitDesc->Bit)) != 0) {
        BitWasOne = TRUE;
    } else {
      BitWasOne = FALSE;
    }
    break;

  default:
    //
    // This address type is not yet implemented
    //
    ASSERT (FALSE );
    break;
  };

  return BitWasOne;
}

VOID
WriteBitDesc (
  CONST QNC_SMM_BIT_DESC   *BitDesc,
  CONST BOOLEAN           ValueToWrite
  )
{
  UINT64           Register;
  UINT64           AndVal;
  UINT64           OrVal;
  UINT32           PciBus;
  UINT32           PciDev;
  UINT32           PciFun;
  UINT32           PciReg;

  ASSERT (BitDesc != NULL);
  ASSERT (!IS_BIT_DESC_NULL(*BitDesc));

  AndVal = ~(BIT_ZERO << (BitDesc->Bit));
  OrVal  = ((UINT32)ValueToWrite) << (BitDesc->Bit);

  switch (BitDesc->Reg.Type) {

  case ACPI_ADDR_TYPE:
    //
    // Double check that we correctly read in the acpi base address
    //
    ASSERT ((PcdGet16 (PcdPm1blkIoBaseAddress) != 0x0) && ((PcdGet16 (PcdPm1blkIoBaseAddress) & 0x1) != 0x1));

    switch (BitDesc->SizeInBytes) {

    case 0:
      //
      // Chances are that this field didn't get initialized.
      // Check your assignments to bit descriptions.
      //
      ASSERT (FALSE );
      break;

    case 1:
      IoAndThenOr8 (PcdGet16 (PcdPm1blkIoBaseAddress) + BitDesc->Reg.Data.acpi, (UINT8)AndVal, (UINT8)OrVal);
      break;

    case 2:
      IoAndThenOr16 (PcdGet16 (PcdPm1blkIoBaseAddress) + BitDesc->Reg.Data.acpi, (UINT16)AndVal, (UINT16)OrVal);
      break;

    case 4:
      IoAndThenOr32 (PcdGet16 (PcdPm1blkIoBaseAddress) + BitDesc->Reg.Data.acpi, (UINT32)AndVal, (UINT32)OrVal);
      break;

    default:
      //
      // Unsupported or invalid register size
      //
      ASSERT (FALSE );
      break;
    };
    break;

  case GPE_ADDR_TYPE:
    //
    // Double check that we correctly read in the gpe base address
    //
    ASSERT (((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) != 0x0) && (((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) & 0x1) != 0x1));

    switch (BitDesc->SizeInBytes) {

    case 0:
      //
      // Chances are that this field didn't get initialized.
      // Check your assignments to bit descriptions.
      //
      ASSERT (FALSE );
      break;

    case 1:
      IoAndThenOr8 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + BitDesc->Reg.Data.gpe, (UINT8)AndVal, (UINT8)OrVal);
      break;

    case 2:
      IoAndThenOr16 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + BitDesc->Reg.Data.gpe, (UINT16)AndVal, (UINT16)OrVal);
      break;

    case 4:
      IoAndThenOr32 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + BitDesc->Reg.Data.gpe, (UINT32)AndVal, (UINT32)OrVal);
      break;

    default:
      //
      // Unsupported or invalid register size
      //
      ASSERT (FALSE );
      break;
    };
      break;

  case MEMORY_MAPPED_IO_ADDRESS_TYPE:
    //
    // Read the register, or it with the bit to set, then write it back.
    //

    //
    // This code does not support writes greater then 64 bits
    //
    ASSERT (BitDesc->SizeInBytes <= 8);
    CopyMem (&Register, BitDesc->Reg.Data.Mmio, BitDesc->SizeInBytes);
    Register &= AndVal;
    Register |= OrVal;
    CopyMem (BitDesc->Reg.Data.Mmio, &Register, BitDesc->SizeInBytes);
    break;

  case PCI_ADDR_TYPE:
    PciBus = BitDesc->Reg.Data.pci.Fields.Bus;
    PciDev = BitDesc->Reg.Data.pci.Fields.Dev;
    PciFun = BitDesc->Reg.Data.pci.Fields.Fnc;
    PciReg = BitDesc->Reg.Data.pci.Fields.Reg;
    switch (BitDesc->SizeInBytes) {

    case 0:
      //
      // Chances are that this field didn't get initialized -- check your assignments
      // to bit descriptions.
      //
      ASSERT (FALSE );
      break;

    case 1:
      PciAndThenOr8 (PCI_LIB_ADDRESS (PciBus, PciDev, PciFun, PciReg), (UINT8) AndVal, (UINT8) OrVal);
      break;

    case 2:
      PciAndThenOr16 (PCI_LIB_ADDRESS (PciBus, PciDev, PciFun, PciReg), (UINT16) AndVal, (UINT16) OrVal);
      break;

    case 4:
      PciAndThenOr32 (PCI_LIB_ADDRESS (PciBus, PciDev, PciFun, PciReg), (UINT32) AndVal, (UINT32) OrVal);
      break;

    default:
      //
      // Unsupported or invalid register size
      //
      ASSERT (FALSE );
      break;
    };
    break;

    default:
    //
    // This address type is not yet implemented
    //
    ASSERT (FALSE );
    break;
  };
}
