/** @file
  DXE Reset System Library Shutdown API implementation for OVMF.

  Copyright (C) 2020, Red Hat, Inc.
  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>                   // BIT13

#include <IndustryStandard/Xen/sched.h>
#include <Library/BaseLib.h>        // CpuDeadLoop()
#include <Library/DebugLib.h>       // ASSERT()
#include <Library/IoLib.h>          // IoOr16()
#include <Library/PcdLib.h>         // PcdGet16()
#include <Library/ResetSystemLib.h> // ResetShutdown()
#include <Library/XenHypercallLib.h>
#include <OvmfPlatforms.h>          // PIIX4_PMBA_VALUE

STATIC UINT16  mAcpiPmBaseAddress;

EFI_STATUS
EFIAPI
DxeResetInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT16  HostBridgeDevId;

  HostBridgeDevId = PcdGet16 (PcdOvmfHostBridgePciDevId);
  switch (HostBridgeDevId) {
    case INTEL_82441_DEVICE_ID:
      mAcpiPmBaseAddress = PIIX4_PMBA_VALUE;
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      mAcpiPmBaseAddress = ICH9_PMBASE_VALUE;
      break;
    default:
      //
      // Fallback to using hypercall.
      // Necessary for PVH guest, but should work for HVM guest.
      //
      mAcpiPmBaseAddress = 0xffff;
      break;
  }

  return EFI_SUCCESS;
}

/**
  Calling this function causes the system to enter a power state equivalent
  to the ACPI G2/S5 or G3 states.

  System shutdown should not return, if it returns, it means the system does
  not support shut down reset.
**/
VOID
EFIAPI
ResetShutdown (
  VOID
  )
{
  if (mAcpiPmBaseAddress != 0xffff) {
    IoBitFieldWrite16 (mAcpiPmBaseAddress + 4, 10, 13, 0);
    IoOr16 (mAcpiPmBaseAddress + 4, BIT13);
  } else {
    INTN                ReturnCode;
    XEN_SCHED_SHUTDOWN  ShutdownOp = {
      .Reason = XEN_SHED_SHUTDOWN_POWEROFF,
    };
    ReturnCode = XenHypercallSchedOp (XEN_SCHEDOP_SHUTDOWN, &ShutdownOp);
    ASSERT (ReturnCode == 0);
  }

  CpuDeadLoop ();
}
