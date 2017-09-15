/** @file

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License which accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <IndustryStandard/Vtd.h>
#include <Ppi/VtdInfo.h>

#include "IntelVTdPmrPei.h"

extern VTD_INFO                *mVTdInfo;

/**
  Get protected low memory alignment.

  @param VtdUnitBaseAddress The base address of the VTd engine.

  @return protected low memory alignment.
**/
UINT32
GetPlmrAlignment (
  IN UINTN         VtdUnitBaseAddress
  )
{
  UINT32        Data32;

  MmioWrite32 (VtdUnitBaseAddress + R_PMEN_LOW_BASE_REG, 0xFFFFFFFF);
  Data32 = MmioRead32 (VtdUnitBaseAddress + R_PMEN_LOW_BASE_REG);
  Data32 = ~Data32 + 1;

  return Data32;
}

/**
  Get protected high memory alignment.

  @param VtdUnitBaseAddress The base address of the VTd engine.

  @return protected high memory alignment.
**/
UINT64
GetPhmrAlignment (
  IN UINTN         VtdUnitBaseAddress
  )
{
  UINT64        Data64;
  UINT8         HostAddressWidth;

  HostAddressWidth = mVTdInfo->HostAddressWidth;

  MmioWrite64 (VtdUnitBaseAddress + R_PMEN_HIGH_BASE_REG, 0xFFFFFFFFFFFFFFFF);
  Data64 = MmioRead64 (VtdUnitBaseAddress + R_PMEN_HIGH_BASE_REG);
  Data64 = ~Data64 + 1;
  Data64 = Data64 & (LShiftU64 (1, HostAddressWidth) - 1);

  return Data64;
}

/**
  Get protected low memory alignment.

  @param EngineMask         The mask of the VTd engine to be accessed.

  @return protected low memory alignment.
**/
UINT32
GetLowMemoryAlignment (
  IN UINT64        EngineMask
  )
{
  UINTN         Index;
  UINT32        Alignment;
  UINT32        FinalAlignment;

  FinalAlignment = 0;
  for (Index = 0; Index < mVTdInfo->VTdEngineCount; Index++) {
    if ((EngineMask & LShiftU64(1, Index)) == 0) {
      continue;
    }
    Alignment = GetPlmrAlignment ((UINTN)mVTdInfo->VTdEngineAddress[Index]);
    if (FinalAlignment < Alignment) {
      FinalAlignment = Alignment;
    }
  }
  return FinalAlignment;
}

/**
  Get protected high memory alignment.

  @param EngineMask         The mask of the VTd engine to be accessed.

  @return protected high memory alignment.
**/
UINT64
GetHighMemoryAlignment (
  IN UINT64        EngineMask
  )
{
  UINTN         Index;
  UINT64        Alignment;
  UINT64        FinalAlignment;

  FinalAlignment = 0;
  for (Index = 0; Index < mVTdInfo->VTdEngineCount; Index++) {
    if ((EngineMask & LShiftU64(1, Index)) == 0) {
      continue;
    }
    Alignment = GetPhmrAlignment ((UINTN)mVTdInfo->VTdEngineAddress[Index]);
    if (FinalAlignment < Alignment) {
      FinalAlignment = Alignment;
    }
  }
  return FinalAlignment;
}

/**
  Enable PMR in the VTd engine.

  @param VtdUnitBaseAddress The base address of the VTd engine.

  @retval EFI_SUCCESS      The PMR is enabled.
  @retval EFI_UNSUPPORTED  The PMR is not supported.
**/
EFI_STATUS
EnablePmr (
  IN UINTN         VtdUnitBaseAddress
  )
{
  UINT32        Reg32;
  VTD_CAP_REG   CapReg;

  CapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_CAP_REG);
  if (CapReg.Bits.PLMR == 0 || CapReg.Bits.PHMR == 0) {
    return EFI_UNSUPPORTED;
  }

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_PMEN_ENABLE_REG);
  if ((Reg32 & BIT0) == 0) {
    MmioWrite32 (VtdUnitBaseAddress + R_PMEN_ENABLE_REG, BIT31);
    do {
      Reg32 = MmioRead32 (VtdUnitBaseAddress + R_PMEN_ENABLE_REG);
    } while((Reg32 & BIT0) == 0);
  }

  return EFI_SUCCESS;
}

/**
  Disable PMR in the VTd engine.

  @param VtdUnitBaseAddress The base address of the VTd engine.

  @retval EFI_SUCCESS      The PMR is disabled.
  @retval EFI_UNSUPPORTED  The PMR is not supported.
**/
EFI_STATUS
DisablePmr (
  IN UINTN         VtdUnitBaseAddress
  )
{
  UINT32        Reg32;
  VTD_CAP_REG   CapReg;

  CapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_CAP_REG);
  if (CapReg.Bits.PLMR == 0 || CapReg.Bits.PHMR == 0) {
    return EFI_UNSUPPORTED;
  }

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_PMEN_ENABLE_REG);
  if ((Reg32 & BIT0) != 0) {
    MmioWrite32 (VtdUnitBaseAddress + R_PMEN_ENABLE_REG, 0x0);
    do {
      Reg32 = MmioRead32 (VtdUnitBaseAddress + R_PMEN_ENABLE_REG);
    } while((Reg32 & BIT0) != 0);
  }

  return EFI_SUCCESS;
}

/**
  Set PMR region in the VTd engine.

  @param VtdUnitBaseAddress The base address of the VTd engine.
  @param LowMemoryBase      The protected low memory region base.
  @param LowMemoryLength    The protected low memory region length.
  @param HighMemoryBase     The protected high memory region base.
  @param HighMemoryLength   The protected high memory region length.

  @retval EFI_SUCCESS      The PMR is set to protected region.
  @retval EFI_UNSUPPORTED  The PMR is not supported.
**/
EFI_STATUS
SetPmrRegion (
  IN UINTN         VtdUnitBaseAddress,
  IN UINT32        LowMemoryBase,
  IN UINT32        LowMemoryLength,
  IN UINT64        HighMemoryBase,
  IN UINT64        HighMemoryLength
  )
{
  VTD_CAP_REG   CapReg;
  UINT32        PlmrAlignment;
  UINT64        PhmrAlignment;

  DEBUG ((DEBUG_INFO, "VtdUnitBaseAddress - 0x%x\n", VtdUnitBaseAddress));

  CapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_CAP_REG);
  if (CapReg.Bits.PLMR == 0 || CapReg.Bits.PHMR == 0) {
    DEBUG ((DEBUG_ERROR, "PLMR/PHMR unsupported\n"));
    return EFI_UNSUPPORTED;
  }

  PlmrAlignment = GetPlmrAlignment (VtdUnitBaseAddress);
  DEBUG ((DEBUG_INFO, "PlmrAlignment - 0x%x\n", PlmrAlignment));
  PhmrAlignment = GetPhmrAlignment (VtdUnitBaseAddress);
  DEBUG ((DEBUG_INFO, "PhmrAlignment - 0x%lx\n", PhmrAlignment));

  if ((LowMemoryBase    != ALIGN_VALUE(LowMemoryBase, PlmrAlignment)) ||
      (LowMemoryLength  != ALIGN_VALUE(LowMemoryLength, PlmrAlignment)) ||
      (HighMemoryBase   != ALIGN_VALUE(HighMemoryBase, PhmrAlignment)) ||
      (HighMemoryLength != ALIGN_VALUE(HighMemoryLength, PhmrAlignment))) {
    DEBUG ((DEBUG_ERROR, "PLMR/PHMR alignment issue\n"));
    return EFI_UNSUPPORTED;
  }

  if (LowMemoryBase == 0 && LowMemoryLength == 0) {
    LowMemoryBase = 0xFFFFFFFF;
  }
  if (HighMemoryBase == 0 && HighMemoryLength == 0) {
    HighMemoryBase = 0xFFFFFFFFFFFFFFFF;
  }

  MmioWrite32 (VtdUnitBaseAddress + R_PMEN_LOW_BASE_REG,    LowMemoryBase);
  MmioWrite32 (VtdUnitBaseAddress + R_PMEN_LOW_LIMITE_REG,  LowMemoryBase + LowMemoryLength - 1);
  MmioWrite64 (VtdUnitBaseAddress + R_PMEN_HIGH_BASE_REG,   HighMemoryBase);
  MmioWrite64 (VtdUnitBaseAddress + R_PMEN_HIGH_LIMITE_REG, HighMemoryBase + HighMemoryLength - 1);

  return EFI_SUCCESS;
}

/**
  Set DMA protected region.

  @param EngineMask         The mask of the VTd engine to be accessed.
  @param LowMemoryBase      The protected low memory region base.
  @param LowMemoryLength    The protected low memory region length.
  @param HighMemoryBase     The protected high memory region base.
  @param HighMemoryLength   The protected high memory region length.

  @retval EFI_SUCCESS      The DMA protection is set.
  @retval EFI_UNSUPPORTED  The DMA protection is not set.
**/
EFI_STATUS
SetDmaProtectedRange (
  IN UINT64        EngineMask,
  IN UINT32        LowMemoryBase,
  IN UINT32        LowMemoryLength,
  IN UINT64        HighMemoryBase,
  IN UINT64        HighMemoryLength
  )
{
  UINTN       Index;
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "SetDmaProtectedRange(0x%lx) - [0x%x, 0x%x] [0x%lx, 0x%lx]\n", EngineMask, LowMemoryBase, LowMemoryLength, HighMemoryBase, HighMemoryLength));

  for (Index = 0; Index < mVTdInfo->VTdEngineCount; Index++) {
    if ((EngineMask & LShiftU64(1, Index)) == 0) {
      continue;
    }
    DisablePmr ((UINTN)mVTdInfo->VTdEngineAddress[Index]);
    Status = SetPmrRegion (
               (UINTN)mVTdInfo->VTdEngineAddress[Index],
               LowMemoryBase,
               LowMemoryLength,
               HighMemoryBase,
               HighMemoryLength
               );
    if (EFI_ERROR(Status)) {
      return Status;
    }
    Status = EnablePmr ((UINTN)mVTdInfo->VTdEngineAddress[Index]);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Diable DMA protection.

  @param EngineMask         The mask of the VTd engine to be accessed.

  @retval DMA protection is disabled.
**/
EFI_STATUS
DisableDmaProtection (
  IN UINT64        EngineMask
  )
{
  UINTN       Index;
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "DisableDmaProtection\n"));

  for (Index = 0; Index < mVTdInfo->VTdEngineCount; Index++) {
    if ((EngineMask & LShiftU64(1, Index)) == 0) {
      continue;
    }
    Status = DisablePmr ((UINTN)mVTdInfo->VTdEngineAddress[Index]);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}
