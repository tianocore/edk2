/** @file

  Copyright (c) 2016-2018, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/MmCommunication.h>

#include <IndustryStandard/ArmStdSmc.h>

#include "MmCommunicate.h"

//
// Address, Length of the pre-allocated buffer for communication with the secure
// world.
//
STATIC ARM_MEMORY_REGION_DESCRIPTOR  mNsCommBuffMemRegion;

// Notification event when virtual address map is set.
STATIC EFI_EVENT  mSetVirtualAddressMapEvent;

//
// Handle to install the MM Communication Protocol
//
STATIC EFI_HANDLE  mMmCommunicateHandle;

/**
  Communicates with a registered handler.

  This function provides an interface to send and receive messages to the
  Standalone MM environment on behalf of UEFI services.  This function is part
  of the MM Communication Protocol that may be called in physical mode prior to
  SetVirtualAddressMap() and in virtual mode after SetVirtualAddressMap().

  @param[in]      This                The EFI_MM_COMMUNICATION_PROTOCOL
                                      instance.
  @param[in, out] CommBuffer          A pointer to the buffer to convey
                                      into MMRAM.
  @param[in, out] CommSize            The size of the data buffer being
                                      passed in. This is optional.

  @retval EFI_SUCCESS                 The message was successfully posted.
  @retval EFI_INVALID_PARAMETER       The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE         The buffer size is incorrect for the MM
                                      implementation. If this error is
                                      returned, the MessageLength field in
                                      the CommBuffer header or the integer
                                      pointed by CommSize are updated to reflect
                                      the maximum payload size the
                                      implementation can accommodate.
  @retval EFI_ACCESS_DENIED           The CommunicateBuffer parameter
                                      or CommSize parameter, if not omitted,
                                      are in address range that cannot be
                                      accessed by the MM environment
**/
STATIC
EFI_STATUS
EFIAPI
MmCommunicationCommunicate (
  IN CONST EFI_MM_COMMUNICATION_PROTOCOL  *This,
  IN OUT VOID                             *CommBuffer,
  IN OUT UINTN                            *CommSize OPTIONAL
  )
{
  EFI_MM_COMMUNICATE_HEADER   *CommunicateHeader;
  ARM_SMC_ARGS                CommunicateSmcArgs;
  EFI_STATUS                  Status;
  UINTN                       BufferSize;

  Status = EFI_ACCESS_DENIED;
  BufferSize = 0;

  ZeroMem (&CommunicateSmcArgs, sizeof (ARM_SMC_ARGS));

  //
  // Check parameters
  //
  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CommunicateHeader = CommBuffer;
  // CommBuffer is a mandatory parameter. Hence, Rely on
  // MessageLength + Header to ascertain the
  // total size of the communication payload rather than
  // rely on optional CommSize parameter
  BufferSize = CommunicateHeader->MessageLength +
               sizeof (CommunicateHeader->HeaderGuid) +
               sizeof (CommunicateHeader->MessageLength);

  // If the length of the CommBuffer is 0 then return the expected length.
  if (CommSize) {
    // This case can be used by the consumer of this driver to find out the
    // max size that can be used for allocating CommBuffer.
    if ((*CommSize == 0) ||
        (*CommSize > mNsCommBuffMemRegion.Length)) {
      *CommSize = mNsCommBuffMemRegion.Length;
      return EFI_BAD_BUFFER_SIZE;
    }
    //
    // CommSize must match MessageLength + sizeof (EFI_MM_COMMUNICATE_HEADER);
    //
    if (*CommSize != BufferSize) {
        return EFI_INVALID_PARAMETER;
    }
  }

  //
  // If the buffer size is 0 or greater than what can be tolerated by the MM
  // environment then return the expected size.
  //
  if ((BufferSize == 0) ||
      (BufferSize > mNsCommBuffMemRegion.Length)) {
    CommunicateHeader->MessageLength = mNsCommBuffMemRegion.Length -
                                       sizeof (CommunicateHeader->HeaderGuid) -
                                       sizeof (CommunicateHeader->MessageLength);
    return EFI_BAD_BUFFER_SIZE;
  }

  // SMC Function ID
  CommunicateSmcArgs.Arg0 = ARM_SMC_ID_MM_COMMUNICATE_AARCH64;

  // Cookie
  CommunicateSmcArgs.Arg1 = 0;

  // Copy Communication Payload
  CopyMem ((VOID *)mNsCommBuffMemRegion.VirtualBase, CommBuffer, BufferSize);

  // comm_buffer_address (64-bit physical address)
  CommunicateSmcArgs.Arg2 = (UINTN)mNsCommBuffMemRegion.PhysicalBase;

  // comm_size_address (not used, indicated by setting to zero)
  CommunicateSmcArgs.Arg3 = 0;

  // Call the Standalone MM environment.
  ArmCallSmc (&CommunicateSmcArgs);

  switch (CommunicateSmcArgs.Arg0) {
  case ARM_SMC_MM_RET_SUCCESS:
    ZeroMem (CommBuffer, BufferSize);
    // On successful return, the size of data being returned is inferred from
    // MessageLength + Header.
    CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)mNsCommBuffMemRegion.VirtualBase;
    BufferSize = CommunicateHeader->MessageLength +
                 sizeof (CommunicateHeader->HeaderGuid) +
                 sizeof (CommunicateHeader->MessageLength);

    CopyMem (
      CommBuffer,
      (VOID *)mNsCommBuffMemRegion.VirtualBase,
      BufferSize
      );
    Status = EFI_SUCCESS;
    break;

  case ARM_SMC_MM_RET_INVALID_PARAMS:
    Status = EFI_INVALID_PARAMETER;
    break;

  case ARM_SMC_MM_RET_DENIED:
    Status = EFI_ACCESS_DENIED;
    break;

  case ARM_SMC_MM_RET_NO_MEMORY:
    // Unexpected error since the CommSize was checked for zero length
    // prior to issuing the SMC
    Status = EFI_OUT_OF_RESOURCES;
    ASSERT (0);
    break;

  default:
    Status = EFI_ACCESS_DENIED;
    ASSERT (0);
  }

  return Status;
}

//
// MM Communication Protocol instance
//
EFI_MM_COMMUNICATION_PROTOCOL  mMmCommunication = {
  MmCommunicationCommunicate
};

/**
  Notification callback on SetVirtualAddressMap event.

  This function notifies the MM communication protocol interface on
  SetVirtualAddressMap event and converts pointers used in this driver
  from physical to virtual address.

  @param  Event          SetVirtualAddressMap event.
  @param  Context        A context when the SetVirtualAddressMap triggered.

  @retval EFI_SUCCESS    The function executed successfully.
  @retval Other          Some error occurred when executing this function.

**/
STATIC
VOID
EFIAPI
NotifySetVirtualAddressMap (
  IN EFI_EVENT  Event,
  IN VOID      *Context
  )
{
  EFI_STATUS  Status;

  Status = gRT->ConvertPointer (
                  EFI_OPTIONAL_PTR,
                  (VOID **)&mNsCommBuffMemRegion.VirtualBase
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "NotifySetVirtualAddressMap():"
            " Unable to convert MM runtime pointer. Status:0x%r\n", Status));
  }

}

STATIC
EFI_STATUS
GetMmCompatibility ()
{
  EFI_STATUS   Status;
  UINT32       MmVersion;
  ARM_SMC_ARGS MmVersionArgs;

  // MM_VERSION uses SMC32 calling conventions
  MmVersionArgs.Arg0 = ARM_SMC_ID_MM_VERSION_AARCH32;

  ArmCallSmc (&MmVersionArgs);

  MmVersion = MmVersionArgs.Arg0;

  if ((MM_MAJOR_VER(MmVersion) == MM_CALLER_MAJOR_VER) &&
      (MM_MINOR_VER(MmVersion) >= MM_CALLER_MINOR_VER)) {
    DEBUG ((DEBUG_INFO, "MM Version: Major=0x%x, Minor=0x%x\n",
            MM_MAJOR_VER(MmVersion), MM_MINOR_VER(MmVersion)));
    Status = EFI_SUCCESS;
  } else {
    DEBUG ((DEBUG_ERROR, "Incompatible MM Versions.\n Current Version: Major=0x%x, Minor=0x%x.\n Expected: Major=0x%x, Minor>=0x%x.\n",
            MM_MAJOR_VER(MmVersion), MM_MINOR_VER(MmVersion), MM_CALLER_MAJOR_VER, MM_CALLER_MINOR_VER));
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/**
  The Entry Point for MM Communication

  This function installs the MM communication protocol interface and finds out
  what type of buffer management will be required prior to invoking the
  communication SMC.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
MmCommunicationInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;

  // Check if we can make the MM call
  Status = GetMmCompatibility ();
  if (EFI_ERROR(Status)) {
    goto ReturnErrorStatus;
  }

  mNsCommBuffMemRegion.PhysicalBase = PcdGet64 (PcdMmBufferBase);
  // During boot , Virtual and Physical are same
  mNsCommBuffMemRegion.VirtualBase = mNsCommBuffMemRegion.PhysicalBase;
  mNsCommBuffMemRegion.Length = PcdGet64 (PcdMmBufferSize);

  ASSERT (mNsCommBuffMemRegion.PhysicalBase != 0);

  ASSERT (mNsCommBuffMemRegion.Length != 0);

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeReserved,
                  mNsCommBuffMemRegion.PhysicalBase,
                  mNsCommBuffMemRegion.Length,
                  EFI_MEMORY_WB |
                  EFI_MEMORY_XP |
                  EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MmCommunicateInitialize: "
            "Failed to add MM-NS Buffer Memory Space\n"));
    goto ReturnErrorStatus;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  mNsCommBuffMemRegion.PhysicalBase,
                  mNsCommBuffMemRegion.Length,
                  EFI_MEMORY_WB | EFI_MEMORY_XP | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MmCommunicateInitialize: "
            "Failed to set MM-NS Buffer Memory attributes\n"));
    goto CleanAddedMemorySpace;
  }

  // Install the communication protocol
  Status = gBS->InstallProtocolInterface (
                  &mMmCommunicateHandle,
                  &gEfiMmCommunicationProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMmCommunication
                  );
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "MmCommunicationInitialize: "
            "Failed to install MM communication protocol\n"));
    goto CleanAddedMemorySpace;
  }

  // Register notification callback when virtual address is associated
  // with the physical address.
  // Create a Set Virtual Address Map event.
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  TPL_NOTIFY,
                  NotifySetVirtualAddressMap,
                  NULL,
                  &mSetVirtualAddressMapEvent
                  );
  if (Status == EFI_SUCCESS) {
    return Status;
  }

  gBS->UninstallProtocolInterface (
         mMmCommunicateHandle,
         &gEfiMmCommunicationProtocolGuid,
         &mMmCommunication
         );

CleanAddedMemorySpace:
  gDS->RemoveMemorySpace (
         mNsCommBuffMemRegion.PhysicalBase,
         mNsCommBuffMemRegion.Length
         );

ReturnErrorStatus:
  return EFI_INVALID_PARAMETER;
}
