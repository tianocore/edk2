/**@file
  Xen Platform PEI support

  Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

#include "Platform.h"
#include "Xen.h"

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
  DEBUG ((EFI_D_ERROR, "Detected Xen version %d.%d\n",
          XenVersion >> 16, XenVersion & 0xFFFF));
  mXenInfo.VersionMajor = (UINT16)(XenVersion >> 16);
  mXenInfo.VersionMinor = (UINT16)(XenVersion & 0xFFFF);

  /* TBD: Locate hvm_info and reserve it away. */
  mXenInfo.HvmInfo = NULL;

  BuildGuidDataHob (
    &gEfiXenInfoGuid,
    &mXenInfo,
    sizeof(mXenInfo)
    );

  return EFI_SUCCESS;
}

/**
  Figures out if we are running inside Xen HVM.

  @return UINT32     CPUID index used to connect to HV.

**/
UINT32
XenDetect (
  VOID
  )
{

  UINT32 XenLeaf;
  UINT8 Signature[13];

  for (XenLeaf = 0x40000000; XenLeaf < 0x40010000; XenLeaf += 0x100) {
    AsmCpuid (XenLeaf, NULL, (UINT32 *) &Signature[0],
              (UINT32 *) &Signature[4],
              (UINT32 *) &Signature[8]);
    Signature[12] = '\0';

    if (!AsciiStrCmp ((CHAR8 *) Signature, "XenVMMXenVMM")) {
      return XenLeaf;
    }
  }

  return 0;
}

/**
  Perform Xen PEI initialization.

  @return EFI_SUCCESS     Xen initialized successfully
  @return EFI_NOT_FOUND   Not running under Xen

**/
EFI_STATUS
InitializeXen (
  UINT32 XenLeaf
  )
{
  XenConnect (XenLeaf);

  //
  // Reserve away HVMLOADER reserved memory [0xFC000000,0xFD000000).
  // This needs to match HVMLOADER RESERVED_MEMBASE/RESERVED_MEMSIZE.
  //
  AddReservedMemoryBaseSizeHob (0xFC000000, 0x1000000);

  return EFI_SUCCESS;
}
