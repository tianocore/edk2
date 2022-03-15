/** @file

  The boot services environment configuration library for the Context Buffer Sample PRM module.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/PrmConfig.h>
#include <Samples/PrmSampleContextBufferModule/Include/StaticData.h>

#include <PrmContextBuffer.h>
#include <PrmDataBuffer.h>

STATIC EFI_HANDLE  mPrmConfigProtocolHandle;

// {5a6cf42b-8bb4-472c-a233-5c4dc4033dc7}
STATIC CONST EFI_GUID  mPrmModuleGuid = {
  0x5a6cf42b, 0x8bb4, 0x472c, { 0xa2, 0x33, 0x5c, 0x4d, 0xc4, 0x03, 0x3d, 0xc7 }
};

// {e1466081-7562-430f-896b-b0e523dc335a}
STATIC CONST EFI_GUID  mCheckStaticDataBufferPrmHandlerGuid = {
  0xe1466081, 0x7562, 0x430f, { 0x89, 0x6b, 0xb0, 0xe5, 0x23, 0xdc, 0x33, 0x5a }
};

/**
  Populates the static data buffer for this PRM module.

  @param[out] StaticDataBuffer  A pointer to the static data buffer.

  @retval EFI_SUCCESS           The static data buffer was populated successfully.
  @retval EFI_INVALID_PARAMETER The StaticDataBuffer pointer argument is NULL.

**/
EFI_STATUS
PopulateStaticDataBuffer (
  OUT STATIC_DATA_SAMPLE_CONTEXT_BUFFER_MODULE  *StaticDataBuffer
  )
{
  if (StaticDataBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Note: In a real-world module these values would likely come from somewhere
  // like a Setup menu option, PCD, binary data, runtime device info, etc. Ideally,
  // this configuration library would be provided an API to get what it needs (the data)
  // and not be concerned with how the data is provided. This makes the PRM module more
  // portable across systems.
  //
  StaticDataBuffer->Policy1Enabled = TRUE;
  StaticDataBuffer->Policy2Enabled = FALSE;
  SetMem (StaticDataBuffer->SomeValueArray, ARRAY_SIZE (StaticDataBuffer->SomeValueArray), 'D');

  return EFI_SUCCESS;
}

/**
  Allocates and populates the static data buffer for this PRM module.

  @param[out] StaticDataBuffer  A pointer to a pointer to the static data buffer.

  @retval EFI_SUCCESS           The static data buffer was allocated and filled successfully.
  @retval EFI_INVALID_PARAMETER The StaticDataBuffer pointer argument is NULL.
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory resources to allocate the static data buffer.

**/
EFI_STATUS
GetStaticDataBuffer (
  OUT PRM_DATA_BUFFER  **StaticDataBuffer
  )
{
  EFI_STATUS       Status;
  PRM_DATA_BUFFER  *DataBuffer;
  UINTN            DataBufferLength;

  if (StaticDataBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *StaticDataBuffer = NULL;

  //
  // Length of the data buffer = Buffer Header Size + Buffer Data Size
  //
  DataBufferLength = sizeof (PRM_DATA_BUFFER_HEADER) + sizeof (STATIC_DATA_SAMPLE_CONTEXT_BUFFER_MODULE);

  DataBuffer = AllocateRuntimeZeroPool (DataBufferLength);
  if (DataBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the data buffer header
  //
  DataBuffer->Header.Signature = PRM_DATA_BUFFER_HEADER_SIGNATURE;
  DataBuffer->Header.Length    = (UINT32)DataBufferLength;

  Status = PopulateStaticDataBuffer ((STATIC_DATA_SAMPLE_CONTEXT_BUFFER_MODULE *)&DataBuffer->Data[0]);
  ASSERT_EFI_ERROR (Status);

  *StaticDataBuffer = DataBuffer;
  return EFI_SUCCESS;
}

/**
  Constructor of the PRM configuration library.

  @param[in] ImageHandle        The image handle of the driver.
  @param[in] SystemTable        The EFI System Table pointer.

  @retval EFI_SUCCESS           The shell command handlers were installed successfully.
  @retval EFI_UNSUPPORTED       The shell level required was not found.
**/
EFI_STATUS
EFIAPI
ContextBufferModuleConfigLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;
  PRM_CONTEXT_BUFFER   *PrmContextBuffer;
  PRM_DATA_BUFFER      *StaticDataBuffer;
  PRM_CONFIG_PROTOCOL  *PrmConfigProtocol;

  PrmContextBuffer  = NULL;
  StaticDataBuffer  = NULL;
  PrmConfigProtocol = NULL;

  /*
    In this sample PRM module, the protocol describing this sample module's resources is simply
    installed in the constructor.

    However, if some data is not available until later, this constructor could register a callback
    on the dependency for the data to be available (e.g. ability to communicate with some device)
    and then install the protocol. The requirement is that the protocol is installed before end of DXE.
  */

  //
  // Allocate and populate the static data buffer
  //
  Status = GetStaticDataBuffer (&StaticDataBuffer);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status) || (StaticDataBuffer == NULL)) {
    goto Done;
  }

  //
  // Allocate and populate the context buffer
  //

  //
  // This context buffer is not actually used by PRM handler at OS runtime. The OS will allocate
  // the actual context buffer passed to the PRM handler.
  //
  // This context buffer is used internally in the firmware to associate a PRM handler with a
  // a static data buffer and a runtime MMIO ranges array so those can be placed into the
  // PRM_HANDLER_INFORMATION_STRUCT and PRM_MODULE_INFORMATION_STRUCT respectively for the PRM handler.
  //
  PrmContextBuffer = AllocateZeroPool (sizeof (*PrmContextBuffer));
  ASSERT (PrmContextBuffer != NULL);
  if (PrmContextBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  CopyGuid (&PrmContextBuffer->HandlerGuid, &mCheckStaticDataBufferPrmHandlerGuid);
  PrmContextBuffer->Signature        = PRM_CONTEXT_BUFFER_SIGNATURE;
  PrmContextBuffer->Version          = PRM_CONTEXT_BUFFER_INTERFACE_VERSION;
  PrmContextBuffer->StaticDataBuffer = StaticDataBuffer;

  PrmConfigProtocol = AllocateZeroPool (sizeof (*PrmConfigProtocol));
  ASSERT (PrmConfigProtocol != NULL);
  if (PrmConfigProtocol == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  CopyGuid (&PrmConfigProtocol->ModuleContextBuffers.ModuleGuid, &mPrmModuleGuid);
  PrmConfigProtocol->ModuleContextBuffers.BufferCount = 1;
  PrmConfigProtocol->ModuleContextBuffers.Buffer      = PrmContextBuffer;

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
    if (StaticDataBuffer != NULL) {
      FreePool (StaticDataBuffer);
    }

    if (PrmContextBuffer != NULL) {
      FreePool (PrmContextBuffer);
    }

    if (PrmConfigProtocol != NULL) {
      FreePool (PrmConfigProtocol);
    }
  }

  return Status;
}
