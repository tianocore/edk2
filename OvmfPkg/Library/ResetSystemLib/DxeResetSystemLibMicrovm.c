/** @file
  Reset System Library functions for OVMF

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>                   // BIT1
#include <PiDxe.h>

#include <Library/BaseLib.h>        // CpuDeadLoop()
#include <Library/DebugLib.h>       // ASSERT()
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>          // IoWrite8()
#include <Library/ResetSystemLib.h> // ResetCold()
#include <Library/TimerLib.h>       // MicroSecondDelay()
#include <Library/UefiRuntimeLib.h> // EfiGoneVirtual()
#include <OvmfPlatforms.h>          // PIIX4_PMBA_VALUE

EFI_STATUS
EFIAPI
DxeResetSystemLibMicrovmConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN                            Address = MICROVM_GED_MMIO_BASE;
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;

  DEBUG ((DEBUG_INFO, "%a: start\n", __FUNCTION__));

  Status = gDS->GetMemorySpaceDescriptor (Address, &Descriptor);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: GetMemorySpaceDescriptor failed\n", __FUNCTION__));
    return RETURN_UNSUPPORTED;
  }

  Status = gDS->SetMemorySpaceAttributes (Address, SIZE_4KB,
                                          Descriptor.Attributes | EFI_MEMORY_RUNTIME);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: SetMemorySpaceAttributes failed\n", __FUNCTION__));
    return RETURN_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "%a: done\n", __FUNCTION__));
  return EFI_SUCCESS;
}
