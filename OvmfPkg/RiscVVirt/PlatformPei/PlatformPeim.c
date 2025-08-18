/** @file

  Copyright (c) 2011-2014, ARM Limited. All rights reserved.<BR>
  Copyright (c) 2014-2020, Linaro Limited. All rights reserved.<BR>
  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

//
// The protocols, PPI and GUID definitions for this module
//
#include <Pi/PiBootMode.h>
#include <Ppi/MasterBootMode.h>
#include <Ppi/GuidedSectionExtraction.h>
#include <Ppi/SecHobData.h>
#include <Guid/RiscVSecHobData.h>

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include "../Library/PlatformSecLib/PlatformSecLib.h"

#define PEI_MEMORY_SIZE  SIZE_64MB

EFI_STATUS
EFIAPI
InitializePlatformPeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  );

//
// Module globals
//
CONST EFI_PEI_PPI_DESCRIPTOR  mPpiListBootMode = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMasterBootModePpiGuid,
  NULL
};

CONST EFI_PEI_PPI_DESCRIPTOR  mTpm2DiscoveredPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gOvmfTpmDiscoveredPpiGuid,
  NULL
};

/*
  Install PEI memory.

*/
STATIC
VOID
FindInstallPeiMemory (
  VOID
  )
{
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  VOID                         *HobList;
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_PHYSICAL_ADDRESS         PeiMemoryBase;

  // Get the HOB list from PEI services
  HobList = GetHobList ();
  ASSERT (HobList != NULL);

  //
  // Search the top region
  //
  PeiMemoryBase = 0;

  // Iterate through the HOB list
  Hob.Raw = HobList;
  while (!END_OF_HOB_LIST (Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      ResourceHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)Hob.Raw;
      if (  (ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY)
         && (ResourceHob->PhysicalStart > PeiMemoryBase)
         && (ResourceHob->ResourceLength >= PEI_MEMORY_SIZE))
      {
        PeiMemoryBase = ResourceHob->PhysicalStart + ResourceHob->ResourceLength - PEI_MEMORY_SIZE;
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  ASSERT (PeiMemoryBase != 0);
  ASSERT_EFI_ERROR (PeiServicesInstallPeiMemory (PeiMemoryBase, PEI_MEMORY_SIZE));
}

/**
  Set up TPM resources from FDT.

  @param  FdtBase       Fdt base address

**/
STATIC
VOID
SetupTPMResources (
  VOID  *FdtBase
  )
{
  INT32         Node, Prev;
  INT32         Parent, Depth;
  CONST CHAR8   *Compatible;
  CONST CHAR8   *CompItem;
  INT32         Len;
  INT32         RangesLen;
  CONST UINT8   *RegProp;
  CONST UINT32  *RangesProp;
  UINT64        TpmBase;
  UINT64        TpmBaseSize;

  //
  // Set Parent to suppress incorrect compiler/analyzer warnings.
  //
  Parent = 0;

  for (Prev = Depth = 0; ; Prev = Node) {
    Node = FdtNextNode (FdtBase, Prev, &Depth);
    if (Node < 0) {
      break;
    }

    if (Depth == 1) {
      Parent = Node;
    }

    Compatible = FdtGetProp (FdtBase, Node, "compatible", &Len);

    //
    // Iterate over the NULL-separated items in the compatible string
    //
    for (CompItem = Compatible; CompItem != NULL && CompItem < Compatible + Len;
         CompItem += 1 + AsciiStrLen (CompItem))
    {
      if (AsciiStrCmp (CompItem, "tcg,tpm-tis-mmio") == 0) {
        RegProp = FdtGetProp (FdtBase, Node, "reg", &Len);
        ASSERT (Len == 8 || Len == 16);
        if (Len == 8) {
          TpmBase     = Fdt32ToCpu (*(UINT32 *)RegProp);
          TpmBaseSize = Fdt32ToCpu (*(UINT32 *)((UINT8 *)RegProp + 4));
        } else if (Len == 16) {
          TpmBase     = Fdt64ToCpu (ReadUnaligned64 ((UINT64 *)RegProp));
          TpmBaseSize = Fdt64ToCpu (ReadUnaligned64 ((UINT64 *)((UINT8 *)RegProp + 8)));
        }

        if (Depth > 1) {
          //
          // QEMU/mach-virt may put the TPM on the platform bus, in which case
          // we have to take its 'ranges' property into account to translate the
          // MMIO address. This consists of a <child base, parent base, size>
          // tuple, where the child base and the size use the same number of
          // cells as the 'reg' property above, and the parent base uses 2 cells
          //
          RangesProp = FdtGetProp (FdtBase, Parent, "ranges", &RangesLen);
          ASSERT (RangesProp != NULL);

          //
          // a plain 'ranges' attribute without a value implies a 1:1 mapping
          //
          if (RangesLen != 0) {
            //
            // assume a single translated range with 2 cells for the parent base
            //
            if (RangesLen != Len + 2 * sizeof (UINT32)) {
              DEBUG ((
                DEBUG_WARN,
                "%a: 'ranges' property has unexpected size %d\n",
                __func__,
                RangesLen
                ));
              break;
            }

            if (Len == 8) {
              TpmBase -= Fdt32ToCpu (RangesProp[0]);
            } else {
              TpmBase -= Fdt64ToCpu (ReadUnaligned64 ((UINT64 *)RangesProp));
            }

            //
            // advance RangesProp to the parent bus address
            //
            RangesProp = (UINT32 *)((UINT8 *)RangesProp + Len / 2);
            TpmBase   += Fdt64ToCpu (ReadUnaligned64 ((UINT64 *)RangesProp));
          }
        }

        break;
      }
    }
  }

  if (TpmBase) {
    BuildResourceDescriptorHob (
      EFI_RESOURCE_MEMORY_MAPPED_IO,
      EFI_RESOURCE_ATTRIBUTE_PRESENT     |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED,
      TpmBase,
      ALIGN_VALUE (TpmBaseSize, EFI_PAGE_SIZE)
      );

    ASSERT_EFI_ERROR ((EFI_STATUS)PcdSet64S (PcdTpmBaseAddress, TpmBase));
    PeiServicesInstallPpi (&mTpm2DiscoveredPpi);
  }
}

/*
  Entry point of this module.

  @param  FileHandle            Handle of the file being invoked.
  @param  PeiServices           Pointer to the PEI Services Table.

  @retval EFI_SUCCESS           The entry point executes successfully.
  @retval Others                Some error occurs during the execution of this function.
*/
EFI_STATUS
EFIAPI
InitializePlatformPeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS              Status;
  VOID                    *Hob;
  RISCV_SEC_HANDOFF_DATA  *SecData;
  EFI_GUID                SecHobDataGuid = RISCV_SEC_HANDOFF_HOB_GUID;

  DEBUG ((DEBUG_LOAD | DEBUG_INFO, "Platform PEIM Loaded\n"));

  Status = PeiServicesSetBootMode (BOOT_WITH_FULL_CONFIGURATION);
  ASSERT_EFI_ERROR (Status);

  Hob = GetFirstGuidHob (&SecHobDataGuid);
  ASSERT (Hob != NULL);
  SecData = GET_GUID_HOB_DATA (Hob);

  //
  // Do all platform initializations
  //
  MemoryInitialization (SecData->FdtPointer);
  CpuInitialization (SecData->FdtPointer);
  PlatformInitialization (SecData->FdtPointer);
  SetupTPMResources (SecData->FdtPointer);

  //
  // Install PEI memory
  //
  FindInstallPeiMemory ();

  //
  // Reinstall HOB after memory initialization
  //
  BuildGuidDataHob (
    &SecHobDataGuid,     // GUID for the HOB
    &SecData,            // Pointer to the data
    sizeof (SecData)     // Size of the data
    );

  Status = PeiServicesInstallPpi (&mPpiListBootMode);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
