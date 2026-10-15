/** @file
  Mapping the Legacy BIOS WB region is required for X64 when entering compatibility mode.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"
#include <Library/SafeIntLib.h>

#define LEGACY_BIOS_WB_LENGTH  0xA0000

/**
  Maps memory below 640K (legacy BIOS write-back memory) as readable, writeable, and executable.
**/
VOID
MapLegacyBiosMemoryRWX (
  VOID
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Desc;
  EFI_PHYSICAL_ADDRESS             Start;
  UINT64                           Length;
  UINT64                           DescLengthFromStart;

  Status = EFI_SUCCESS;
  Start  = 0x0;

  if (gCpu == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a cannot remap Legacy BIOS memory as RWX because the CPU arch protocol is missing\n",
      __func__
      ));
    return;
  }

  // Ensure that this memory is marked as system memory. If it is not system memory, do not change
  // the memory attributes as we do not want to map something that shouldn't be mapped or map something
  // incorrectly
  while (Start < LEGACY_BIOS_WB_LENGTH) {
    Status = CoreGetMemorySpaceDescriptor (Start, &Desc);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a - Failed to get memory space descriptor for address 0x%llx! Status = %r\n",
        __func__,
        Start,
        Status
        ));
      return;
    }

    // find the length from Start to the end of this descriptor, in the case Start != Desc.BaseAddress
    // these should all be well formed descriptors, but use SafeIntLib functions just to be sure we don't
    // over/underflow
    Status = SafeUint64Add (Desc.BaseAddress, Desc.Length, &DescLengthFromStart);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a - Memory space descriptor has UINT64 overflowing address 0x%llx + length 0x%llx! Status = %r\n",
        __func__,
        Desc.BaseAddress,
        Desc.Length,
        Status
        ));

      // if we fail here this is very bad and means the GCD is malformed
      ASSERT (FALSE);
      return;
    }

    Status = SafeUint64Sub (DescLengthFromStart, Start, &DescLengthFromStart);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a - Memory space descriptor has UINT64 underflowing address 0x%llx + length 0x%llx Start: 0x%llx! Status = %r\n",
        __func__,
        Desc.BaseAddress,
        Desc.Length,
        Start,
        Status
        ));

      // if we fail here this is very bad and means the GCD is malformed
      ASSERT (FALSE);
      return;
    }

    // Ensure we only go up to LEGACY_BIOS_WB_LENGTH here, we know Start is less than it due to while condition
    // We also know Start + DescLengthFromStart won't overflow because above we did a safe subtraction of Start from
    // DescLengthFromStart
    if (Start + DescLengthFromStart > LEGACY_BIOS_WB_LENGTH) {
      Length = LEGACY_BIOS_WB_LENGTH - Start;
    } else {
      Length = DescLengthFromStart;
    }

    // remove this chunk from being remapped if it is not system memory
    if (Desc.GcdMemoryType != EfiGcdMemoryTypeSystemMemory) {
      DEBUG ((
        DEBUG_WARN,
        "%a Not mapping 0x%llx for 0x%llx as RWX because it is not system memory\n",
        __func__,
        Desc.BaseAddress,
        Length
        ));

      // we know this doesn't overflow because we did a safe subtraction of Start from DescLengthFromStart above
      Start += DescLengthFromStart;
      continue;
    }

    // Map the legacy BIOS write-back memory as RWX.
    Status = gCpu->SetMemoryAttributes (
                     gCpu,
                     Start,
                     Length,
                     0
                     );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a failed to map 0x%llx for length 0x%llx as RWX\n",
        __func__,
        Start,
        Length
        ));

      ASSERT_EFI_ERROR (Status);
    }

    // we know this doesn't overflow because we did a safe subtraction of Start from DescLengthFromStart above
    Start += DescLengthFromStart;
  }
}
