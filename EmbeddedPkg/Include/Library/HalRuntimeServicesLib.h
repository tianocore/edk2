/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef __RUNTIME_SERVICES_LIB_H__
#define __RUNTIME_SERVICES_LIB_H__

VOID
LibMtcInitialize (VOID);

VOID
LibMtcVirtualAddressChangeEvent (VOID);

EFI_STATUS
EFIAPI
LibMtcGetNextHighMonotonicCount (
  OUT UINT32  *HighCount
  );

EFI_STATUS
LibMtcGetNextMonotonicCount (
  OUT UINT64  *Count
  );



VOID
LibVariableInitialize (VOID);

VOID
LibVariableVirtualAddressChangeEvent (VOID);

EFI_STATUS
LibGetVariable (
  IN CHAR16        *VariableName,
  IN EFI_GUID      *VendorGuid,
  OUT UINT32       *Attributes OPTIONAL,
  IN OUT UINTN     *DataSize,
  OUT VOID         *Data
  );

EFI_STATUS
LibGetNextVariableName (
  IN OUT UINTN     *VariableNameSize,
  IN OUT CHAR16    *VariableName,
  IN OUT EFI_GUID  *VendorGuid
  );

EFI_STATUS
LibSetVariable (
  IN CHAR16        *VariableName,
  IN EFI_GUID      *VendorGuid,
  IN UINT32        Attributes,
  IN UINTN         DataSize,
  IN VOID          *Data
  );

EFI_STATUS
LibQueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize
  );



VOID
LibResetInitializeReset (VOID);

VOID
LibResetVirtualAddressChangeEvent (VOID);

VOID
LibResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN CHAR16           *ResetData OPTIONAL
  );


VOID
LibCapsuleInitialize (VOID);

VOID
LibCapsuleVirtualAddressChangeEvent (VOID);

EFI_STATUS
LibUpdateCapsule (
  IN UEFI_CAPSULE_HEADER     **CapsuleHeaderArray,
  IN UINTN                   CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS    ScatterGatherList OPTIONAL
  );

EFI_STATUS
QueryCapsuleCapabilities (
  IN  UEFI_CAPSULE_HEADER  **CapsuleHeaderArray,
  IN  UINTN                CapsuleCount,
  OUT UINT64               *MaxiumCapsuleSize,
  OUT EFI_RESET_TYPE       *ResetType
  );



VOID
LibRtcInitialize (VOID);

VOID
LibRtcVirtualAddressChangeEvent (VOID);

EFI_STATUS
LibGetTime (
  OUT EFI_TIME                *Time,
  OUT  EFI_TIME_CAPABILITIES  *Capabilities
  );

EFI_STATUS
LibSetTime (
  IN EFI_TIME                *Time
  );

EFI_STATUS
LibGetWakeupTime (
  OUT BOOLEAN     *Enabled,
  OUT BOOLEAN     *Pending,
  OUT EFI_TIME    *Time
  );

EFI_STATUS
LibSetWakeupTime (
  IN BOOLEAN      Enabled,
  OUT EFI_TIME    *Time
  );


VOID
LibReportStatusCodeInitialize (VOID);

VOID
LibReportStatusCodeVirtualAddressChangeEvent (VOID);

EFI_STATUS
LibReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  );


#endif

