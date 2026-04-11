/** @file

  The boot services environment configuration library for the ACPI Parameter Buffer Sample PRM module.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/PrmConfig.h>

STATIC EFI_HANDLE  mPrmConfigProtocolHandle;

// {dc2a58a6-5927-4776-b995-d118a27335a2}
STATIC CONST EFI_GUID  mPrmModuleGuid = {
  0xdc2a58a6, 0x5927, 0x4776, { 0xb9, 0x95, 0xd1, 0x18, 0xa2, 0x73, 0x35, 0xa2 }
};

// {2e4f2d13-6240-4ed0-a401-c723fbdc34e8}
STATIC CONST EFI_GUID  mCheckParamBufferPrmHandlerGuid = {
  0x2e4f2d13, 0x6240, 0x4ed0, { 0xa4, 0x01, 0xc7, 0x23, 0xfb, 0xdc, 0x34, 0xe8 }
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
AcpiParameterBufferModuleConfigLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                        Status;
  VOID                              *AcpiParameterBuffer;
  ACPI_PARAMETER_BUFFER_DESCRIPTOR  *AcpiParamBufferDescriptor;
  PRM_CONFIG_PROTOCOL               *PrmConfigProtocol;

  AcpiParameterBuffer       = NULL;
  AcpiParamBufferDescriptor = NULL;
  PrmConfigProtocol         = NULL;

  /*
    In this sample PRM module, the protocol describing this sample module's resources is simply
    installed in the constructor.

    However, if some data is not available until later, this constructor could register a callback
    on the dependency for the data to be available (e.g. ability to communicate with some device)
    and then install the protocol. The requirement is that the protocol is installed before end of DXE.
  */

  // Allocate the ACPI parameter buffer

  // A parameter buffer is arbitrary data that is handler specific. This handler buffer is specified
  // to consist of a UINT32 that represents the test data signature ('T', 'E', 'S', 'T').
  AcpiParameterBuffer = AllocateRuntimeZeroPool (sizeof (UINT32));
  ASSERT (AcpiParameterBuffer != NULL);
  if (AcpiParameterBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  // Allocate the ACPI Parameter Buffer Descriptor structure for a single PRM handler
  AcpiParamBufferDescriptor = AllocateZeroPool (sizeof (*AcpiParamBufferDescriptor));
  ASSERT (AcpiParamBufferDescriptor != NULL);
  if (AcpiParamBufferDescriptor == NULL) {
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

  // Populate the ACPI Parameter Buffer Descriptor structure
  CopyGuid (&AcpiParamBufferDescriptor->HandlerGuid, &mCheckParamBufferPrmHandlerGuid);
  AcpiParamBufferDescriptor->AcpiParameterBufferAddress = (UINT64)(UINTN)AcpiParameterBuffer;

  // Populate the PRM Module Context Buffers structure
  PrmConfigProtocol->ModuleContextBuffers.AcpiParameterBufferDescriptorCount = 1;
  PrmConfigProtocol->ModuleContextBuffers.AcpiParameterBufferDescriptors     = AcpiParamBufferDescriptor;

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
    if (AcpiParameterBuffer != NULL) {
      FreePool (AcpiParameterBuffer);
    }

    if (AcpiParamBufferDescriptor != NULL) {
      FreePool (AcpiParamBufferDescriptor);
    }

    if (PrmConfigProtocol != NULL) {
      FreePool (PrmConfigProtocol);
    }
  }

  return Status;
}
