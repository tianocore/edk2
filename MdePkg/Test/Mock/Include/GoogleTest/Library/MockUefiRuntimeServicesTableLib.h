/** @file MockUefiRuntimeServicesTableLib.h
  Google Test mocks for UefiRuntimeServicesTableLib

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_UEFI_RUNTIME_SERVICES_TABLE_LIB_H_
#define MOCK_UEFI_RUNTIME_SERVICES_TABLE_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/UefiRuntimeServicesTableLib.h>
}

struct MockUefiRuntimeServicesTableLib {
  MOCK_INTERFACE_DECLARATION (MockUefiRuntimeServicesTableLib);

  //
  // Time Services
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_GetTime,
    (OUT  EFI_TIME                    *Time,
     OUT  EFI_TIME_CAPABILITIES       *Capabilities OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_SetTime,
    (IN  EFI_TIME                    *Time)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_GetWakeupTime,
    (OUT  BOOLEAN                     *Enabled,
     OUT  BOOLEAN                     *Pending,
     OUT  EFI_TIME                    *Time)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_SetWakeupTime,
    (IN  BOOLEAN                     Enable,
     IN  EFI_TIME                    *Time OPTIONAL)
    );

  //
  // Virtual Memory Services
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_SetVirtualAddressMap,
    (IN  UINTN                        MemoryMapSize,
     IN  UINTN                        DescriptorSize,
     IN  UINT32                       DescriptorVersion,
     IN  EFI_MEMORY_DESCRIPTOR         *VirtualMap)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_ConvertPointer,
    (IN  UINTN                        DebugDisposition,
     IN OUT VOID                      **Address)
    );

  //
  // Variable Services
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_GetVariable,
    (IN      CHAR16    *VariableName,
     IN      EFI_GUID  *VendorGuid,
     OUT     UINT32    *Attributes OPTIONAL,
     IN OUT  UINTN     *DataSize,
     OUT     VOID      *Data)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_GetNextVariableName,
    (IN OUT  UINTN                    *VariableNameSize,
     IN OUT  CHAR16                   *VariableName,
     IN OUT  EFI_GUID                 *VendorGuid)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_SetVariable,
    (IN CHAR16    *VariableName,
     IN EFI_GUID  *VendorGuid,
     IN UINT32    Attributes,
     IN UINTN     DataSize,
     IN VOID      *Data)
    );

  //
  // Miscellaneous Services
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_GetNextHighMonotonicCount,
    (OUT  UINT32                    *HighCount)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    gRT_ResetSystem,
    (IN EFI_RESET_TYPE           ResetType,
     IN EFI_STATUS               ResetStatus,
     IN UINTN                    DataSize,
     IN VOID                     *ResetData OPTIONAL)
    );

  //
  // UEFI 2.0 Capsule Services
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_UpdateCapsule,
    (IN EFI_CAPSULE_HEADER        **CapsuleHeaderArray,
     IN UINTN                     CapsuleCount,
     IN EFI_PHYSICAL_ADDRESS       ScatterGatherList OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_QueryCapsuleCapabilities,
    (IN  EFI_CAPSULE_HEADER        **CapsuleHeaderArray,
     IN  UINTN                     CapsuleCount,
     OUT UINT64                    *MaximumCapsuleSize,
     OUT EFI_RESET_TYPE            *ResetType)
    );

  //
  // Miscellaneous UEFI 2.0 Service
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gRT_QueryVariableInfo,
    (IN  UINT32                    Attributes,
     OUT UINT64                    *MaximumVariableStorageSize,
     OUT UINT64                    *RemainingVariableStorageSize,
     OUT UINT64                    *MaximumVariableSize)
    );
};

#endif
