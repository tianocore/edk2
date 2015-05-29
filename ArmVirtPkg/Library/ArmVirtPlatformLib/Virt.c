/** @file
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
*  Copyright (c) 2014, Linaro Limited. All rights reserved.
*  Copyright (c) 2014, Red Hat, Inc.
*
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/IoLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <ArmPlatform.h>
#include <libfdt.h>
#include <Pi/PiBootMode.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>

/**
  Return the current Boot Mode

  This function returns the boot reason on the platform

  @return   Return the current Boot Mode of the platform

**/
EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  )
{
  return BOOT_WITH_FULL_CONFIGURATION;
}

/**
  This function is called by PrePeiCore, in the SEC phase.
**/
RETURN_STATUS
ArmPlatformInitialize (
  IN  UINTN                     MpId
  )
{
  //
  // We are relying on ArmPlatformInitializeSystemMemory () being called from
  // InitializeMemory (), which only occurs if the following feature is disabled
  //
  ASSERT (!FeaturePcdGet (PcdSystemMemoryInitializeInSec));
  return RETURN_SUCCESS;
}

/**
  Initialize the system (or sometimes called permanent) memory

  This memory is generally represented by the DRAM.

  This function is called from InitializeMemory() in MemoryInitPeim, in the PEI
  phase.
**/
VOID
ArmPlatformInitializeSystemMemory (
  VOID
  )
{
  VOID         *DeviceTreeBase;
  INT32        Node, Prev;
  UINT64       NewBase;
  UINT64       NewSize;
  CONST CHAR8  *Type;
  INT32        Len;
  CONST UINT64 *RegProp;

  NewBase = 0;
  NewSize = 0;

  DeviceTreeBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  ASSERT (DeviceTreeBase != NULL);

  //
  // Make sure we have a valid device tree blob
  //
  ASSERT (fdt_check_header (DeviceTreeBase) == 0);

  //
  // Look for a memory node
  //
  for (Prev = 0;; Prev = Node) {
    Node = fdt_next_node (DeviceTreeBase, Prev, NULL);
    if (Node < 0) {
      break;
    }

    //
    // Check for memory node
    //
    Type = fdt_getprop (DeviceTreeBase, Node, "device_type", &Len);
    if (Type && AsciiStrnCmp (Type, "memory", Len) == 0) {
      //
      // Get the 'reg' property of this node. For now, we will assume
      // two 8 byte quantities for base and size, respectively.
      //
      RegProp = fdt_getprop (DeviceTreeBase, Node, "reg", &Len);
      if (RegProp != 0 && Len == (2 * sizeof (UINT64))) {

        NewBase = fdt64_to_cpu (ReadUnaligned64 (RegProp));
        NewSize = fdt64_to_cpu (ReadUnaligned64 (RegProp + 1));

        //
        // Make sure the start of DRAM matches our expectation
        //
        ASSERT (FixedPcdGet64 (PcdSystemMemoryBase) == NewBase);
        PcdSet64 (PcdSystemMemorySize, NewSize);

        DEBUG ((EFI_D_INFO, "%a: System RAM @ 0x%lx - 0x%lx\n",
               __FUNCTION__, NewBase, NewBase + NewSize - 1));
      } else {
        DEBUG ((EFI_D_ERROR, "%a: Failed to parse FDT memory node\n",
               __FUNCTION__));
      }
      break;
    }
  }

  //
  // We need to make sure that the machine we are running on has at least
  // 128 MB of memory configured, and is currently executing this binary from
  // NOR flash. This prevents a device tree image in DRAM from getting
  // clobbered when our caller installs permanent PEI RAM, before we have a
  // chance of marking its location as reserved or copy it to a freshly
  // allocated block in the permanent PEI RAM in the platform PEIM.
  //
  ASSERT (NewSize >= SIZE_128MB);
  ASSERT (
    (((UINT64)PcdGet64 (PcdFdBaseAddress) +
      (UINT64)PcdGet32 (PcdFdSize)) <= NewBase) ||
    ((UINT64)PcdGet64 (PcdFdBaseAddress) >= (NewBase + NewSize)));
}

VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  *PpiListSize = 0;
  *PpiList = NULL;
}
