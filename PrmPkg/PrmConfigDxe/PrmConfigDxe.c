/** @file

  This file contains the implementation for a Platform Runtime Mechanism (PRM) configuration driver.

  Copyright (c) Microsoft Corporation
  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>

#include <PiDxe.h>
#include <PrmContextBuffer.h>
#include <PrmDataBuffer.h>
#include <PrmMmio.h>
#include <Protocol/PrmConfig.h>

#define _DBGMSGID_  "[PRMCONFIG]"

STATIC  UINTN  mMaxRuntimeMmioRangeCount;

GLOBAL_REMOVE_IF_UNREFERENCED STATIC  PRM_RUNTIME_MMIO_RANGES  **mRuntimeMmioRanges;

/**
  Converts the runtime memory range physical addresses to virtual addresses.

  @param[in]  RuntimeMmioRanges   A pointer to a PRM_RUNTIME_MMIO_RANGES buffer.

**/
VOID
ConvertRuntimeMemoryRangeAddresses (
  IN  PRM_RUNTIME_MMIO_RANGES  *RuntimeMmioRanges
  )
{
  UINTN  Index;

  if ((RuntimeMmioRanges == NULL) || (RuntimeMmioRanges->Count == 0)) {
    return;
  }

  for (Index = 0; Index < (UINTN)RuntimeMmioRanges->Count; Index++) {
    RuntimeMmioRanges->Range[Index].VirtualBaseAddress = RuntimeMmioRanges->Range[Index].PhysicalBaseAddress;
    gRT->ConvertPointer (0x0, (VOID **)&(RuntimeMmioRanges->Range[Index].VirtualBaseAddress));
  }
}

/**
  Sets the runtime memory range attributes.

  The EFI_MEMORY_RUNTIME attribute is set for each PRM_RUNTIME_MMIO_RANGE present
  in the buffer provided.

  @param[in]  RuntimeMmioRanges     A pointer to a PRM_RUNTIME_MMIO_RANGES buffer.

**/
VOID
SetRuntimeMemoryRangeAttributes (
  IN  PRM_RUNTIME_MMIO_RANGES  *RuntimeMmioRanges
  )
{
  EFI_STATUS                       Status;
  EFI_STATUS                       Status2;
  UINTN                            Index;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  if ((RuntimeMmioRanges == NULL) || (RuntimeMmioRanges->Count == 0)) {
    return;
  }

  for (Index = 0; Index < (UINTN)RuntimeMmioRanges->Count; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "      %a %a: Runtime MMIO Range [%d].\n",
      _DBGMSGID_,
      __FUNCTION__,
      Index
      ));
    DEBUG ((
      DEBUG_INFO,
      "      %a %a: Physical address = 0x%016x. Length = 0x%x.\n",
      _DBGMSGID_,
      __FUNCTION__,
      RuntimeMmioRanges->Range[Index].PhysicalBaseAddress,
      RuntimeMmioRanges->Range[Index].Length
      ));

    // Runtime memory ranges should cover ranges on a page boundary
    ASSERT ((RuntimeMmioRanges->Range[Index].PhysicalBaseAddress & EFI_PAGE_MASK) == 0);
    ASSERT ((RuntimeMmioRanges->Range[Index].Length & EFI_PAGE_MASK) == 0);

    Status2 = EFI_NOT_FOUND;
    Status  = gDS->GetMemorySpaceDescriptor (RuntimeMmioRanges->Range[Index].PhysicalBaseAddress, &Descriptor);
    if (!EFI_ERROR (Status) &&
        (
         ((Descriptor.GcdMemoryType != EfiGcdMemoryTypeMemoryMappedIo) && (Descriptor.GcdMemoryType != EfiGcdMemoryTypeReserved)) ||
         ((Descriptor.Length & EFI_PAGE_MASK) != 0)
        )
        )
    {
      Status2 =  gDS->RemoveMemorySpace (
                        RuntimeMmioRanges->Range[Index].PhysicalBaseAddress,
                        Descriptor.Length
                        );
    }

    if ((Status == EFI_NOT_FOUND) || !EFI_ERROR (Status2)) {
      Status = gDS->AddMemorySpace (
                      EfiGcdMemoryTypeMemoryMappedIo,
                      RuntimeMmioRanges->Range[Index].PhysicalBaseAddress,
                      (UINT64)RuntimeMmioRanges->Range[Index].Length,
                      EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                      );
      ASSERT_EFI_ERROR (Status);

      Status = gDS->AllocateMemorySpace (
                      EfiGcdAllocateAddress,
                      EfiGcdMemoryTypeMemoryMappedIo,
                      0,
                      (UINT64)RuntimeMmioRanges->Range[Index].Length,
                      &RuntimeMmioRanges->Range[Index].PhysicalBaseAddress,
                      gImageHandle,
                      NULL
                      );
      ASSERT_EFI_ERROR (Status);
    }

    Status = gDS->GetMemorySpaceDescriptor (RuntimeMmioRanges->Range[Index].PhysicalBaseAddress, &Descriptor);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "      %a %a: Error [%r] finding descriptor for runtime memory range 0x%016x.\n",
        _DBGMSGID_,
        __FUNCTION__,
        Status,
        RuntimeMmioRanges->Range[Index].PhysicalBaseAddress
        ));
      continue;
    }

    if ((Descriptor.Attributes & EFI_MEMORY_RUNTIME) != 0) {
      continue;
    }

    Status = gDS->SetMemorySpaceAttributes (
                    RuntimeMmioRanges->Range[Index].PhysicalBaseAddress,
                    (UINT64)RuntimeMmioRanges->Range[Index].Length,
                    Descriptor.Attributes | EFI_MEMORY_RUNTIME
                    );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "      %a %a: Error [%r] setting descriptor for runtime memory range 0x%016x.\n",
        _DBGMSGID_,
        __FUNCTION__,
        Status,
        RuntimeMmioRanges->Range[Index].PhysicalBaseAddress
        ));
    } else {
      DEBUG ((DEBUG_INFO, "      %a %a: Successfully set runtime attribute for the MMIO range.\n", _DBGMSGID_, __FUNCTION__));
    }
  }
}

/**
  Stores pointers or pointer to resources that should be converted in the virtual address change event.

**/
VOID
StoreVirtualMemoryAddressChangePointers (
  VOID
  )
{
  EFI_STATUS           Status;
  UINTN                HandleCount;
  UINTN                HandleIndex;
  UINTN                RangeIndex;
  EFI_HANDLE           *HandleBuffer;
  PRM_CONFIG_PROTOCOL  *PrmConfigProtocol;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  RangeIndex = 0;

  mRuntimeMmioRanges = AllocateRuntimeZeroPool (sizeof (*mRuntimeMmioRanges) * mMaxRuntimeMmioRangeCount);
  if ((mRuntimeMmioRanges == NULL) && (mMaxRuntimeMmioRangeCount > 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "  %a %a: Memory allocation for runtime MMIO pointer array failed.\n",
      _DBGMSGID_,
      __FUNCTION__
      ));
    ASSERT (FALSE);
    return;
  }

  HandleBuffer = NULL;
  Status       = gBS->LocateHandleBuffer (
                        ByProtocol,
                        &gPrmConfigProtocolGuid,
                        NULL,
                        &HandleCount,
                        &HandleBuffer
                        );
  if (!EFI_ERROR (Status)) {
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[HandleIndex],
                      &gPrmConfigProtocolGuid,
                      (VOID **)&PrmConfigProtocol
                      );
      ASSERT_EFI_ERROR (Status);
      if (EFI_ERROR (Status) || (PrmConfigProtocol == NULL)) {
        continue;
      }

      if (PrmConfigProtocol->ModuleContextBuffers.RuntimeMmioRanges != NULL) {
        if (RangeIndex >= mMaxRuntimeMmioRangeCount) {
          Status = EFI_BUFFER_TOO_SMALL;
          DEBUG ((
            DEBUG_ERROR,
            "  %a %a: Index out of bounds - Actual count (%d) of runtime MMIO ranges exceeds maximum count (%d).\n",
            _DBGMSGID_,
            __FUNCTION__,
            RangeIndex + 1,
            mMaxRuntimeMmioRangeCount
            ));
          ASSERT_EFI_ERROR (Status);
          return;
        }

        mRuntimeMmioRanges[RangeIndex++] = PrmConfigProtocol->ModuleContextBuffers.RuntimeMmioRanges;
      }
    }

    DEBUG ((
      DEBUG_INFO,
      "  %a %a: %d MMIO ranges buffers saved for future virtual memory conversion.\n",
      _DBGMSGID_,
      __FUNCTION__,
      RangeIndex
      ));
  }
}

/**
  Validates a data buffer for a PRM module.

  Verifies the buffer header signature is valid and the length meets the minimum size.

  @param[in]  PrmDataBuffer         A pointer to the data buffer for this PRM module.

  @retval EFI_SUCCESS               The data buffer was validated successfully.
  @retval EFI_INVALID_PARAMETER     The pointer given for PrmDataBuffer is NULL.
  @retval EFI_NOT_FOUND             The data buffer signature is not valid.
  @retval EFI_BUFFER_TOO_SMALL      The buffer size is too small.

**/
EFI_STATUS
ValidatePrmDataBuffer (
  IN  CONST PRM_DATA_BUFFER  *PrmDataBuffer
  )
{
  if (PrmDataBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (PrmDataBuffer->Header.Signature != PRM_DATA_BUFFER_HEADER_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "  %a %a: The PRM data buffer signature is invalid. PRM module.\n", _DBGMSGID_, __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  if (PrmDataBuffer->Header.Length < sizeof (PRM_DATA_BUFFER_HEADER)) {
    DEBUG ((DEBUG_ERROR, "  %a %a: The PRM data buffer length is invalid.\n", _DBGMSGID_, __FUNCTION__));
    return EFI_BUFFER_TOO_SMALL;
  }

  return EFI_SUCCESS;
}

/**
  Validates a PRM context buffer.

  Verifies the buffer header signature is valid and the GUID is set to a non-zero value.

  @param[in]  PrmContextBuffer      A pointer to the context buffer for this PRM handler.

  @retval EFI_SUCCESS               The context buffer was validated successfully.
  @retval EFI_INVALID_PARAMETER     The pointer given for ContextBuffer is NULL.
  @retval EFI_NOT_FOUND             The proper value for a field was not found.

**/
EFI_STATUS
ValidatePrmContextBuffer (
  IN  CONST PRM_CONTEXT_BUFFER  *PrmContextBuffer
  )
{
  if (PrmContextBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (PrmContextBuffer->Signature != PRM_CONTEXT_BUFFER_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "  %a %a: The PRM context buffer signature is invalid.\n", _DBGMSGID_, __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  if (IsZeroGuid (&PrmContextBuffer->HandlerGuid)) {
    DEBUG ((DEBUG_ERROR, "  %a %a: The PRM context buffer GUID is zero.\n", _DBGMSGID_, __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  if ((PrmContextBuffer->StaticDataBuffer != NULL) && EFI_ERROR (ValidatePrmDataBuffer (PrmContextBuffer->StaticDataBuffer))) {
    DEBUG ((
      DEBUG_ERROR,
      "    %a %a: Error in static buffer for PRM handler %g.\n",
      _DBGMSGID_,
      __FUNCTION__,
      &PrmContextBuffer->HandlerGuid
      ));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is notification function converts any registered PRM_RUNTIME_MMIO_RANGE
  addresses to a virtual address.

  @param[in]  Event        Event whose notification function is being invoked.
  @param[in]  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
PrmConfigVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN  Index;

  //
  // Convert runtime MMIO ranges
  //
  for (Index = 0; Index < mMaxRuntimeMmioRangeCount; Index++) {
    ConvertRuntimeMemoryRangeAddresses (mRuntimeMmioRanges[Index]);
  }
}

/**
  The PRM Config END_OF_DXE protocol notification event handler.

  Finds all of the PRM_CONFIG_PROTOCOL instances installed at end of DXE and
  marks all PRM_RUNTIME_MMIO_RANGE entries as EFI_MEMORY_RUNTIME.

  @param[in]  Event           Event whose notification function is being invoked.
  @param[in]  Context         The pointer to the notification function's context,
                              which is implementation-dependent.

**/
VOID
EFIAPI
PrmConfigEndOfDxeNotification (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS           Status;
  UINTN                HandleCount;
  UINTN                BufferIndex;
  UINTN                HandleIndex;
  EFI_HANDLE           *HandleBuffer;
  PRM_CONTEXT_BUFFER   *CurrentContextBuffer;
  PRM_CONFIG_PROTOCOL  *PrmConfigProtocol;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  HandleBuffer = NULL;
  Status       = gBS->LocateHandleBuffer (
                        ByProtocol,
                        &gPrmConfigProtocolGuid,
                        NULL,
                        &HandleCount,
                        &HandleBuffer
                        );
  if (!EFI_ERROR (Status)) {
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[HandleIndex],
                      &gPrmConfigProtocolGuid,
                      (VOID **)&PrmConfigProtocol
                      );
      ASSERT_EFI_ERROR (Status);
      if (EFI_ERROR (Status) || (PrmConfigProtocol == NULL)) {
        continue;
      }

      DEBUG ((
        DEBUG_INFO,
        "  %a %a: Found PRM configuration protocol for PRM module %g.\n",
        _DBGMSGID_,
        __FUNCTION__,
        &PrmConfigProtocol->ModuleContextBuffers.ModuleGuid
        ));

      DEBUG ((DEBUG_INFO, "      %a %a: Validating module context buffers...\n", _DBGMSGID_, __FUNCTION__));
      for (BufferIndex = 0; BufferIndex < PrmConfigProtocol->ModuleContextBuffers.BufferCount; BufferIndex++) {
        CurrentContextBuffer = &(PrmConfigProtocol->ModuleContextBuffers.Buffer[BufferIndex]);

        Status =  ValidatePrmContextBuffer (CurrentContextBuffer);
        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_ERROR,
            "        %a %a: Context buffer validation failed for PRM handler %g.\n",
            _DBGMSGID_,
            __FUNCTION__,
            CurrentContextBuffer->HandlerGuid
            ));
        }
      }

      DEBUG ((DEBUG_INFO, "      %a %a: Module context buffer validation complete.\n", _DBGMSGID_, __FUNCTION__));

      if (PrmConfigProtocol->ModuleContextBuffers.RuntimeMmioRanges != NULL) {
        DEBUG ((
          DEBUG_INFO,
          "    %a %a: Found %d PRM runtime MMIO ranges.\n",
          _DBGMSGID_,
          __FUNCTION__,
          PrmConfigProtocol->ModuleContextBuffers.RuntimeMmioRanges->Count
          ));
        SetRuntimeMemoryRangeAttributes (PrmConfigProtocol->ModuleContextBuffers.RuntimeMmioRanges);
        mMaxRuntimeMmioRangeCount++;
      }
    }

    StoreVirtualMemoryAddressChangePointers ();
  }

  if (HandleBuffer != NULL) {
    gBS->FreePool (HandleBuffer);
  }

  gBS->CloseEvent (Event);
}

/**
  The entry point for this module.

  @param[in]  ImageHandle     The firmware allocated handle for the EFI image.
  @param[in]  SystemTable     A pointer to the EFI System Table.

  @retval EFI_SUCCESS         The entry point is executed successfully.
  @retval Others              An error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
PrmConfigEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;

  DEBUG ((DEBUG_INFO, "%a %a - Entry.\n", _DBGMSGID_, __FUNCTION__));

  //
  // Register a notification function to change memory attributes at end of DXE
  //
  Event  = NULL;
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  PrmConfigEndOfDxeNotification,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register a notification function for virtual address change
  //
  Event  = NULL;
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  PrmConfigVirtualAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
