/** @file

  The boot services environment configuration library for the Hardware Access Sample PRM module.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/PrmConfig.h>
#include <Samples/PrmSampleHardwareAccessModule/Hpet.h>

STATIC EFI_HANDLE  mPrmConfigProtocolHandle;

// {0ef93ed7-14ae-425b-928f-b85a6213b57e}
STATIC CONST EFI_GUID  mPrmModuleGuid = {
  0x0ef93ed7, 0x14ae, 0x425b, { 0x92, 0x8f, 0xb8, 0x5a, 0x62, 0x13, 0xb5, 0x7e }
};

/**
  Constructor of the PRM configuration library.

  @param[in] ImageHandle        The image handle of the driver.
  @param[in] SystemTable        The EFI System Table pointer.

  @retval EFI_SUCCESS           The shell command handlers were installed successfully.
  @retval EFI_UNSUPPORTED       The shell level required was not found.
**/
EFI_STATUS
EFIAPI
HardwareAccessModuleConfigLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS               Status;
  PRM_RUNTIME_MMIO_RANGES  *RuntimeMmioRanges;
  PRM_CONFIG_PROTOCOL      *PrmConfigProtocol;

  RuntimeMmioRanges = NULL;
  PrmConfigProtocol = NULL;

  /*
    In this sample PRM module, the protocol describing this sample module's resources is simply
    installed in the constructor.

    However, if some data is not available until later, this constructor could register a callback
    on the dependency for the data to be available (e.g. ability to communicate with some device)
    and then install the protocol. The requirement is that the protocol is installed before end of DXE.
  */

  // Runtime MMIO Ranges structure

  // Since this sample module only uses 1 runtime MMIO range, it can use the PRM_RUNTIME_MMIO_RANGES
  // type directly without extending the size of the data buffer for additional MMIO ranges.
  RuntimeMmioRanges = AllocateRuntimeZeroPool (sizeof (*RuntimeMmioRanges));
  ASSERT (RuntimeMmioRanges != NULL);
  if (RuntimeMmioRanges == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  // Allocate the PRM Configuration protocol structure for this PRM module
  PrmConfigProtocol = AllocateZeroPool (sizeof (*PrmConfigProtocol));
  ASSERT (PrmConfigProtocol != NULL);
  if (PrmConfigProtocol == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  CopyGuid (&PrmConfigProtocol->ModuleContextBuffers.ModuleGuid, &mPrmModuleGuid);

  // Populate the Runtime MMIO Ranges structure
  RuntimeMmioRanges->Count                        = 1;
  RuntimeMmioRanges->Range[0].PhysicalBaseAddress = HPET_BASE_ADDRESS;
  RuntimeMmioRanges->Range[0].Length              = HPET_RANGE_LENGTH;

  PrmConfigProtocol->ModuleContextBuffers.RuntimeMmioRanges = RuntimeMmioRanges;

  //
  // Install the PRM Configuration Protocol for this module. This indicates the configuration
  // library has completed resource initialization for the PRM module.
  //
  Status = gBS->InstallProtocolInterface (
                  &mPrmConfigProtocolHandle,
                  &gPrmConfigProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)PrmConfigProtocol
                  );

Done:
  if (EFI_ERROR (Status)) {
    if (RuntimeMmioRanges != NULL) {
      FreePool (RuntimeMmioRanges);
    }

    if (PrmConfigProtocol != NULL) {
      FreePool (PrmConfigProtocol);
    }
  }

  return Status;
}
