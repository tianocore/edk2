/** @file

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <IndustryStandard/Vtd.h>
#include <Ppi/VtdInfo.h>

#include "IntelVTdPmrPei.h"

/**
  Flush VTD page table and context table memory.

  This action is to make sure the IOMMU engine can get final data in memory.

  @param[in]  Base              The base address of memory to be flushed.
  @param[in]  Size              The size of memory in bytes to be flushed.
**/
VOID
FlushPageTableMemory (
  IN UINTN  Base,
  IN UINTN  Size
  )
{
  WriteBackDataCacheRange ((VOID *)Base, Size);
}

/**
  Flush VTd engine write buffer.

  @param VtdUnitBaseAddress The base address of the VTd engine.
**/
VOID
FlushWriteBuffer (
  IN UINTN  VtdUnitBaseAddress
  )
{
  UINT32      Reg32;
  VTD_CAP_REG CapReg;

  CapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_CAP_REG);

  if (CapReg.Bits.RWBF != 0) {
    Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
    MmioWrite32 (VtdUnitBaseAddress + R_GCMD_REG, Reg32 | B_GMCD_REG_WBF);
    do {
      Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
    } while ((Reg32 & B_GSTS_REG_WBF) != 0);
  }
}

/**
  Invalidate VTd context cache.

  @param VtdUnitBaseAddress The base address of the VTd engine.
**/
EFI_STATUS
InvalidateContextCache (
  IN UINTN  VtdUnitBaseAddress
  )
{
  UINT64  Reg64;

  Reg64 = MmioRead64 (VtdUnitBaseAddress + R_CCMD_REG);
  if ((Reg64 & B_CCMD_REG_ICC) != 0) {
    DEBUG ((DEBUG_ERROR,"ERROR: InvalidateContextCache: B_CCMD_REG_ICC is set for VTD(%x)\n",VtdUnitBaseAddress));
    return EFI_DEVICE_ERROR;
  }

  Reg64 &= ((~B_CCMD_REG_ICC) & (~B_CCMD_REG_CIRG_MASK));
  Reg64 |= (B_CCMD_REG_ICC | V_CCMD_REG_CIRG_GLOBAL);
  MmioWrite64 (VtdUnitBaseAddress + R_CCMD_REG, Reg64);

  do {
    Reg64 = MmioRead64 (VtdUnitBaseAddress + R_CCMD_REG);
  } while ((Reg64 & B_CCMD_REG_ICC) != 0);

  return EFI_SUCCESS;
}

/**
  Invalidate VTd IOTLB.

  @param VtdUnitBaseAddress The base address of the VTd engine.
**/
EFI_STATUS
InvalidateIOTLB (
  IN UINTN  VtdUnitBaseAddress
  )
{
  UINT64       Reg64;
  VTD_ECAP_REG ECapReg;

  ECapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_ECAP_REG);

  Reg64 = MmioRead64 (VtdUnitBaseAddress + (ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
  if ((Reg64 & B_IOTLB_REG_IVT) != 0) {
    DEBUG ((DEBUG_ERROR,"ERROR: InvalidateIOTLB: B_IOTLB_REG_IVT is set for VTD(%x)\n", VtdUnitBaseAddress));
    return EFI_DEVICE_ERROR;
  }

  Reg64 &= ((~B_IOTLB_REG_IVT) & (~B_IOTLB_REG_IIRG_MASK));
  Reg64 |= (B_IOTLB_REG_IVT | V_IOTLB_REG_IIRG_GLOBAL);
  MmioWrite64 (VtdUnitBaseAddress + (ECapReg.Bits.IRO * 16) + R_IOTLB_REG, Reg64);

  do {
    Reg64 = MmioRead64 (VtdUnitBaseAddress + (ECapReg.Bits.IRO * 16) + R_IOTLB_REG);
  } while ((Reg64 & B_IOTLB_REG_IVT) != 0);

  return EFI_SUCCESS;
}

/**
  Enable DMAR translation.

  @param VtdUnitBaseAddress The base address of the VTd engine.
  @param RootEntryTable     The address of the VTd RootEntryTable.

  @retval EFI_SUCCESS           DMAR translation is enabled.
  @retval EFI_DEVICE_ERROR      DMAR translation is not enabled.
**/
EFI_STATUS
EnableDmar (
  IN UINTN  VtdUnitBaseAddress,
  IN UINTN  RootEntryTable
  )
{
  UINT32    Reg32;

  DEBUG((DEBUG_INFO, ">>>>>>EnableDmar() for engine [%x] \n", VtdUnitBaseAddress));

  DEBUG((DEBUG_INFO, "RootEntryTable 0x%x \n", RootEntryTable));
  MmioWrite64 (VtdUnitBaseAddress + R_RTADDR_REG, (UINT64)(UINTN)RootEntryTable);

  MmioWrite32 (VtdUnitBaseAddress + R_GCMD_REG, B_GMCD_REG_SRTP);

  DEBUG((DEBUG_INFO, "EnableDmar: waiting for RTPS bit to be set... \n"));
  do {
    Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  } while((Reg32 & B_GSTS_REG_RTPS) == 0);

  //
  // Init DMAr Fault Event and Data registers
  //
  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_FEDATA_REG);

  //
  // Write Buffer Flush before invalidation
  //
  FlushWriteBuffer (VtdUnitBaseAddress);

  //
  // Invalidate the context cache
  //
  InvalidateContextCache (VtdUnitBaseAddress);

  //
  // Invalidate the IOTLB cache
  //
  InvalidateIOTLB (VtdUnitBaseAddress);

  //
  // Enable VTd
  //
  MmioWrite32 (VtdUnitBaseAddress + R_GCMD_REG, B_GMCD_REG_TE);
  DEBUG((DEBUG_INFO, "EnableDmar: Waiting B_GSTS_REG_TE ...\n"));
  do {
    Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  } while ((Reg32 & B_GSTS_REG_TE) == 0);

  DEBUG ((DEBUG_INFO,"VTD () enabled!<<<<<<\n"));

  return EFI_SUCCESS;
}

/**
  Disable DMAR translation.

  @param VtdUnitBaseAddress The base address of the VTd engine.

  @retval EFI_SUCCESS           DMAR translation is disabled.
  @retval EFI_DEVICE_ERROR      DMAR translation is not disabled.
**/
EFI_STATUS
DisableDmar (
  IN UINTN  VtdUnitBaseAddress
  )
{
  UINT32    Reg32;

  DEBUG((DEBUG_INFO, ">>>>>>DisableDmar() for engine [%x] \n", VtdUnitBaseAddress));

  //
  // Write Buffer Flush before invalidation
  //
  FlushWriteBuffer (VtdUnitBaseAddress);

  //
  // Disable VTd
  //
  MmioWrite32 (VtdUnitBaseAddress + R_GCMD_REG, B_GMCD_REG_SRTP);
  do {
    Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  } while((Reg32 & B_GSTS_REG_RTPS) == 0);

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  DEBUG((DEBUG_INFO, "DisableDmar: GSTS_REG - 0x%08x\n", Reg32));

  MmioWrite64 (VtdUnitBaseAddress + R_RTADDR_REG, 0);

  DEBUG ((DEBUG_INFO,"VTD () Disabled!<<<<<<\n"));

  return EFI_SUCCESS;
}

/**
  Enable VTd translation table protection.

  @param VTdInfo            The VTd engine context information.
  @param EngineMask         The mask of the VTd engine to be accessed.
**/
VOID
EnableVTdTranslationProtection (
  IN VTD_INFO      *VTdInfo,
  IN UINT64        EngineMask
  )
{
  UINTN       Index;
  VOID        *RootEntryTable;

  DEBUG ((DEBUG_INFO, "EnableVTdTranslationProtection - 0x%lx\n", EngineMask));

  RootEntryTable = AllocatePages (1);
  ASSERT (RootEntryTable != NULL);
  if (RootEntryTable == NULL) {
    DEBUG ((DEBUG_INFO, " EnableVTdTranslationProtection : OutOfResource\n"));
    return ;
  }

  ZeroMem (RootEntryTable, EFI_PAGES_TO_SIZE(1));
  FlushPageTableMemory ((UINTN)RootEntryTable, EFI_PAGES_TO_SIZE(1));

  for (Index = 0; Index < VTdInfo->VTdEngineCount; Index++) {
    if ((EngineMask & LShiftU64(1, Index)) == 0) {
      continue;
    }
    EnableDmar ((UINTN)VTdInfo->VTdEngineAddress[Index], (UINTN)RootEntryTable);
  }

  return ;
}

/**
  Disable VTd translation table protection.

  @param VTdInfo            The VTd engine context information.
  @param EngineMask         The mask of the VTd engine to be accessed.
**/
VOID
DisableVTdTranslationProtection (
  IN VTD_INFO      *VTdInfo,
  IN UINT64        EngineMask
  )
{
  UINTN       Index;

  DEBUG ((DEBUG_INFO, "DisableVTdTranslationProtection - 0x%lx\n", EngineMask));

  for (Index = 0; Index < VTdInfo->VTdEngineCount; Index++) {
    if ((EngineMask & LShiftU64(1, Index)) == 0) {
      continue;
    }
    DisableDmar ((UINTN)VTdInfo->VTdEngineAddress[Index]);
  }

  return ;
}
