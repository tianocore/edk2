/** @file MockUefiRuntimeLib.h
  Google Test mocks for UefiRuntimeLib

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_UEFI_RUNTIME_LIB_H_
#define MOCK_UEFI_RUNTIME_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <PiPei.h>
  #include <PiDxe.h>
  #include <PiSmm.h>
  #include <PiMm.h>
  #include <Uefi.h>
  #include <Library/UefiRuntimeLib.h>
}

struct MockUefiRuntimeLib {
  MOCK_INTERFACE_DECLARATION (MockUefiRuntimeLib);

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EfiAtRuntime,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EfiGoneVirtual,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiGetTime,
    (OUT EFI_TIME               *Time,
     OUT EFI_TIME_CAPABILITIES  *Capabilities OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiSetTime,
    (IN EFI_TIME  *Time)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiGetWakeupTime,
    (OUT BOOLEAN   *Enabled,
     OUT BOOLEAN   *Pending,
     OUT EFI_TIME  *Time)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiSetWakeupTime,
    (IN BOOLEAN   Enable,
     IN EFI_TIME  *Time OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiGetVariable,
    (IN     CHAR16    *VariableName,
     IN     EFI_GUID  *VendorGuid,
     OUT    UINT32    *Attributes OPTIONAL,
     IN OUT UINTN     *DataSize,
     OUT    VOID      *Data)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiGetNextVariableName,
    (IN OUT UINTN     *VariableNameSize,
     IN OUT CHAR16    *VariableName,
     IN OUT EFI_GUID  *VendorGuid)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiSetVariable,
    (IN CHAR16    *VariableName,
     IN EFI_GUID  *VendorGuid,
     IN UINT32    Attributes,
     IN UINTN     DataSize,
     IN VOID      *Data)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiGetNextHighMonotonicCount,
    (OUT UINT32  *HighCount)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    EfiResetSystem,
    (IN EFI_RESET_TYPE  ResetType,
     IN EFI_STATUS      ResetStatus,
     IN UINTN           DataSize,
     IN VOID            *ResetData OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiConvertPointer,
    (IN     UINTN  DebugDisposition,
     IN OUT VOID   **Address)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiConvertFunctionPointer,
    (IN     UINTN  DebugDisposition,
     IN OUT VOID   **Address)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiSetVirtualAddressMap,
    (IN       UINTN                  MemoryMapSize,
     IN       UINTN                  DescriptorSize,
     IN       UINT32                 DescriptorVersion,
     IN CONST EFI_MEMORY_DESCRIPTOR  *VirtualMap)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiConvertList,
    (IN     UINTN       DebugDisposition,
     IN OUT LIST_ENTRY  *ListHead)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiUpdateCapsule,
    (IN EFI_CAPSULE_HEADER    **CapsuleHeaderArray,
     IN UINTN                 CapsuleCount,
     IN EFI_PHYSICAL_ADDRESS  ScatterGatherList OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiQueryCapsuleCapabilities,
    (IN  EFI_CAPSULE_HEADER  **CapsuleHeaderArray,
     IN  UINTN               CapsuleCount,
     OUT UINT64              *MaximumCapsuleSize,
     OUT EFI_RESET_TYPE      *ResetType)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiQueryVariableInfo,
    (IN  UINT32  Attributes,
     OUT UINT64  *MaximumVariableStorageSize,
     OUT UINT64  *RemainingVariableStorageSize,
     OUT UINT64  *MaximumVariableSize)
    );
};

#endif // MOCK_UEFI_RUNTIME_LIB_H_
