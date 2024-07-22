/** @file
  Arm Monitor Library that chooses the conduit based on the PSCI node in the
  device tree provided by QEMU.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2024, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <Library/ArmHvcLib.h>
#include <Library/ArmMonitorLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>

typedef enum {
  SmcccConduitUnknown,
  SmcccConduitSmc,
  SmcccConduitHvc,
} SMCCC_CONDUIT;

/**
  Discover the SMCCC conduit by parsing the PSCI device tree node.

  @return the discovered SMCCC conduit
**/
STATIC
SMCCC_CONDUIT
DiscoverSmcccConduit (
  VOID
  )
{
  VOID                *DeviceTreeBase;
  INT32               Node, Prev;
  INT32               Len;
  CONST FDT_PROPERTY  *Compatible;
  CONST CHAR8         *CompatibleItem;
  CONST FDT_PROPERTY  *Prop;

  DeviceTreeBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  ASSERT (FdtCheckHeader (DeviceTreeBase) == 0);

  //
  // Enumerate all FDT nodes looking for the PSCI node and capture the conduit
  //
  for (Prev = 0; ; Prev = Node) {
    Node = FdtNextNode (DeviceTreeBase, Prev, NULL);
    if (Node < 0) {
      break;
    }

    Compatible = FdtGetProperty (DeviceTreeBase, Node, "compatible", &Len);
    if (Compatible == NULL) {
      continue;
    }

    //
    // Iterate over the NULL-separated items in the compatible string
    //
    for (CompatibleItem = Compatible->Data; CompatibleItem < Compatible->Data + Len;
         CompatibleItem += 1 + AsciiStrLen (CompatibleItem))
    {
      if (AsciiStrCmp (CompatibleItem, "arm,psci-0.2") != 0) {
        continue;
      }

      Prop = FdtGetProperty (DeviceTreeBase, Node, "method", NULL);
      if (Prop == NULL) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Missing PSCI method property\n",
          __func__
          ));

        return SmcccConduitUnknown;
      }

      if (AsciiStrnCmp (Prop->Data, "hvc", 3) == 0) {
        return SmcccConduitHvc;
      } else if (AsciiStrnCmp (Prop->Data, "smc", 3) == 0) {
        return SmcccConduitSmc;
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Unknown PSCI method \"%a\"\n",
          __func__,
          Prop
          ));

        return SmcccConduitUnknown;
      }
    }
  }

  return SmcccConduitUnknown;
}

/** Monitor call.

  An HyperVisor Call (HVC) or System Monitor Call (SMC) will be issued
  depending on the default conduit.

  @param [in,out]  Args    Arguments for the HVC/SMC.
**/
VOID
EFIAPI
ArmMonitorCall (
  IN OUT ARM_MONITOR_ARGS  *Args
  )
{
  switch (DiscoverSmcccConduit ()) {
    case SmcccConduitHvc:
      ArmCallHvc ((ARM_HVC_ARGS *)Args);
      break;

    case SmcccConduitSmc:
      ArmCallSmc ((ARM_SMC_ARGS *)Args);
      break;

    default:
      ASSERT (FALSE);
  }
}
