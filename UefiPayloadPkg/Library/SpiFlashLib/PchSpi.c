/** @file

  Copyright (c) 2017-2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "SpiCommon.h"

/**
  Acquire SPI MMIO BAR.

  @param[in] PchSpiBase           PCH SPI PCI Base Address

  @retval                         Return SPI BAR Address

**/
UINT32
AcquireSpiBar0 (
  IN  UINTN  PchSpiBase
  )
{
  return MmioRead32 (PchSpiBase + R_SPI_BASE) & ~(B_SPI_BAR0_MASK);
}

/**
  Release SPI MMIO BAR. Do nothing.

  @param[in] PchSpiBase           PCH SPI PCI Base Address

**/
VOID
ReleaseSpiBar0 (
  IN  UINTN  PchSpiBase
  )
{
}

/**
  This function is to enable/disable BIOS Write Protect in SMM phase.

  @param[in] EnableSmmSts        Flag to Enable/disable Bios write protect

**/
VOID
CpuSmmDisableBiosWriteProtect (
  IN  BOOLEAN  EnableSmmSts
  )
{
  UINT32  Data32;

  if (EnableSmmSts) {
    //
    // Disable BIOS Write Protect in SMM phase.
    //
    Data32 = MmioRead32 ((UINTN)(0xFED30880)) | (UINT32)(BIT0);
    AsmWriteMsr32 (0x000001FE, Data32);
  } else {
    //
    // Enable BIOS Write Protect in SMM phase
    //
    Data32 = MmioRead32 ((UINTN)(0xFED30880)) & (UINT32)(~BIT0);
    AsmWriteMsr32 (0x000001FE, Data32);
  }

  //
  // Read FED30880h back to ensure the setting went through.
  //
  Data32 = MmioRead32 (0xFED30880);
}

/**
  This function is a hook for Spi to disable BIOS Write Protect.

  @param[in] PchSpiBase           PCH SPI PCI Base Address
  @param[in] CpuSmmBwp            Need to disable CPU SMM Bios write protection or not

  @retval EFI_SUCCESS             The protocol instance was properly initialized
  @retval EFI_ACCESS_DENIED       The BIOS Region can only be updated in SMM phase

**/
EFI_STATUS
EFIAPI
DisableBiosWriteProtect (
  IN  UINTN  PchSpiBase,
  IN  UINT8  CpuSmmBwp
  )
{
  //
  // Write clear BC_SYNC_SS prior to change WPD from 0 to 1.
  //
  MmioOr8 (PchSpiBase + R_SPI_BCR + 1, (B_SPI_BCR_SYNC_SS >> 8));

  //
  // Enable the access to the BIOS space for both read and write cycles
  //
  MmioOr8 (PchSpiBase + R_SPI_BCR, B_SPI_BCR_BIOSWE);

  if (CpuSmmBwp != 0) {
    CpuSmmDisableBiosWriteProtect (TRUE);
  }

  return EFI_SUCCESS;
}

/**
  This function is a hook for Spi to enable BIOS Write Protect.

  @param[in] PchSpiBase           PCH SPI PCI Base Address
  @param[in] CpuSmmBwp            Need to disable CPU SMM Bios write protection or not

**/
VOID
EFIAPI
EnableBiosWriteProtect (
  IN  UINTN  PchSpiBase,
  IN  UINT8  CpuSmmBwp
  )
{
  //
  // Disable the access to the BIOS space for write cycles
  //
  MmioAnd8 (PchSpiBase + R_SPI_BCR, (UINT8)(~B_SPI_BCR_BIOSWE));

  if (CpuSmmBwp != 0) {
    CpuSmmDisableBiosWriteProtect (FALSE);
  }
}

/**
  This function disables SPI Prefetching and caching,
  and returns previous BIOS Control Register value before disabling.

  @param[in] PchSpiBase           PCH SPI PCI Base Address

  @retval                         Previous BIOS Control Register value

**/
UINT8
SaveAndDisableSpiPrefetchCache (
  IN  UINTN  PchSpiBase
  )
{
  UINT8  BiosCtlSave;

  BiosCtlSave = MmioRead8 (PchSpiBase + R_SPI_BCR) & B_SPI_BCR_SRC;

  MmioAndThenOr32 (
    PchSpiBase + R_SPI_BCR, \
    (UINT32)(~B_SPI_BCR_SRC), \
    (UINT32)(V_SPI_BCR_SRC_PREF_DIS_CACHE_DIS <<  B_SPI_BCR_SRC)
    );

  return BiosCtlSave;
}

/**
  This function updates BIOS Control Register with the given value.

  @param[in] PchSpiBase           PCH SPI PCI Base Address
  @param[in] BiosCtlValue         BIOS Control Register Value to be updated

**/
VOID
SetSpiBiosControlRegister (
  IN  UINTN  PchSpiBase,
  IN  UINT8  BiosCtlValue
  )
{
  MmioAndThenOr8 (PchSpiBase + R_SPI_BCR, (UINT8) ~B_SPI_BCR_SRC, BiosCtlValue);
}
