/**@file
  Xen Platform PEI support

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Guid/XenInfo.h>
#include <IndustryStandard/E820.h>
#include <Library/ResourcePublicationLib.h>
#include <Library/MtrrLib.h>

#include "Platform.h"
#include "Xen.h"

BOOLEAN mXen = FALSE;

STATIC UINT32 mXenLeaf = 0;

EFI_XEN_INFO mXenInfo;

/**
  Returns E820 map provided by Xen

  @param Entries      Pointer to E820 map
  @param Count        Number of entries

  @return EFI_STATUS
**/
EFI_STATUS
XenGetE820Map (
  EFI_E820_ENTRY64 **Entries,
  UINT32 *Count
  )
{
  EFI_XEN_OVMF_INFO *Info =
    (EFI_XEN_OVMF_INFO *)(UINTN) OVMF_INFO_PHYSICAL_ADDRESS;

  if (AsciiStrCmp ((CHAR8 *) Info->Signature, "XenHVMOVMF")) {
    return EFI_NOT_FOUND;
  }

  ASSERT (Info->E820 < MAX_ADDRESS);
  *Entries = (EFI_E820_ENTRY64 *)(UINTN) Info->E820;
  *Count = Info->E820EntriesCount;

  return EFI_SUCCESS;
}

/**
  Connects to the Hypervisor.

  @param  XenLeaf     CPUID index used to connect.

  @return EFI_STATUS

**/
EFI_STATUS
XenConnect (
  UINT32 XenLeaf
  )
{
  UINT32 Index;
  UINT32 TransferReg;
  UINT32 TransferPages;
  UINT32 XenVersion;

  AsmCpuid (XenLeaf + 2, &TransferPages, &TransferReg, NULL, NULL);
  mXenInfo.HyperPages = AllocatePages (TransferPages);
  if (!mXenInfo.HyperPages) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < TransferPages; Index++) {
    AsmWriteMsr64 (TransferReg,
                   (UINTN) mXenInfo.HyperPages +
                   (Index << EFI_PAGE_SHIFT) + Index);
  }

  AsmCpuid (XenLeaf + 1, &XenVersion, NULL, NULL, NULL);
  DEBUG ((DEBUG_ERROR, "Detected Xen version %d.%d\n",
          XenVersion >> 16, XenVersion & 0xFFFF));
  mXenInfo.VersionMajor = (UINT16)(XenVersion >> 16);
  mXenInfo.VersionMinor = (UINT16)(XenVersion & 0xFFFF);

  BuildGuidDataHob (
    &gEfiXenInfoGuid,
    &mXenInfo,
    sizeof(mXenInfo)
    );

  return EFI_SUCCESS;
}

/**
  Figures out if we are running inside Xen HVM.

  @retval TRUE   Xen was detected
  @retval FALSE  Xen was not detected

**/
BOOLEAN
XenDetect (
  VOID
  )
{
  UINT8 Signature[13];

  if (mXenLeaf != 0) {
    return TRUE;
  }

  Signature[12] = '\0';
  for (mXenLeaf = 0x40000000; mXenLeaf < 0x40010000; mXenLeaf += 0x100) {
    AsmCpuid (mXenLeaf,
              NULL,
              (UINT32 *) &Signature[0],
              (UINT32 *) &Signature[4],
              (UINT32 *) &Signature[8]);

    if (!AsciiStrCmp ((CHAR8 *) Signature, "XenVMMXenVMM")) {
      mXen = TRUE;
      return TRUE;
    }
  }

  mXenLeaf = 0;
  return FALSE;
}


VOID
XenPublishRamRegions (
  VOID
  )
{
  EFI_E820_ENTRY64  *E820Map;
  UINT32            E820EntriesCount;
  EFI_STATUS        Status;

  if (!mXen) {
    return;
  }

  DEBUG ((DEBUG_INFO, "Using memory map provided by Xen\n"));

  //
  // Parse RAM in E820 map
  //
  E820EntriesCount = 0;
  Status = XenGetE820Map (&E820Map, &E820EntriesCount);

  ASSERT_EFI_ERROR (Status);

  if (E820EntriesCount > 0) {
    EFI_E820_ENTRY64 *Entry;
    UINT32 Loop;

    for (Loop = 0; Loop < E820EntriesCount; Loop++) {
      Entry = E820Map + Loop;

      //
      // Only care about RAM
      //
      if (Entry->Type != EfiAcpiAddressRangeMemory) {
        continue;
      }

      AddMemoryBaseSizeHob (Entry->BaseAddr, Entry->Length);

      MtrrSetMemoryAttribute (Entry->BaseAddr, Entry->Length, CacheWriteBack);
    }
  }
}


/**
  Perform Xen PEI initialization.

  @return EFI_SUCCESS     Xen initialized successfully
  @return EFI_NOT_FOUND   Not running under Xen

**/
EFI_STATUS
InitializeXen (
  VOID
  )
{
  RETURN_STATUS PcdStatus;

  if (mXenLeaf == 0) {
    return EFI_NOT_FOUND;
  }

  XenConnect (mXenLeaf);

  //
  // Reserve away HVMLOADER reserved memory [0xFC000000,0xFD000000).
  // This needs to match HVMLOADER RESERVED_MEMBASE/RESERVED_MEMSIZE.
  //
  AddReservedMemoryBaseSizeHob (0xFC000000, 0x1000000, FALSE);

  PcdStatus = PcdSetBoolS (PcdPciDisableBusEnumeration, TRUE);
  ASSERT_RETURN_ERROR (PcdStatus);

  return EFI_SUCCESS;
}
