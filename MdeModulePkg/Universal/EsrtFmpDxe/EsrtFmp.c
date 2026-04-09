/** @file
  Publishes ESRT table from Firmware Management Protocol instances

  Copyright (c) 2016, Microsoft Corporation
  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>

  All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Protocol/FirmwareManagement.h>
#include <Guid/EventGroup.h>
#include <Guid/SystemResourceTable.h>

///
/// Structure for array of unique GUID/HardwareInstance pairs from the
/// current set of EFI_FIRMWARE_IMAGE_DESCRIPTORs from all FMP Protocols.
///
typedef struct {
  ///
  /// A unique GUID identifying the firmware image type.
  ///
  EFI_GUID    ImageTypeGuid;
  ///
  /// An optional number to identify the unique hardware instance within the
  /// system for devices that may have multiple instances whenever possible.
  ///
  UINT64      HardwareInstance;
} GUID_HARDWAREINSTANCE_PAIR;

/**
 Print ESRT to debug console.

 @param[in]  Table   Pointer to the ESRT table.

**/
VOID
EFIAPI
PrintTable (
  IN EFI_SYSTEM_RESOURCE_TABLE  *Table
  );

/**
  Install EFI System Resource Table into the UEFI Configuration Table

  @param[in] Table                  Pointer to the ESRT.

  @return  Status code.

**/
EFI_STATUS
InstallEfiSystemResourceTableInUefiConfigurationTable (
  IN EFI_SYSTEM_RESOURCE_TABLE  *Table
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;
  if (Table->FwResourceCount == 0) {
    DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Can't install ESRT table because it has zero Entries. \n"));
    Status = EFI_UNSUPPORTED;
  } else {
    //
    // Install the pointer into config table
    //
    Status = gBS->InstallConfigurationTable (&gEfiSystemResourceTableGuid, Table);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Can't install ESRT table.  Status: %r. \n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "EsrtFmpDxe: Installed ESRT table. \n"));
    }
  }

  return Status;
}

/**
  Return if this FMP is a system FMP or a device FMP, based upon FmpImageInfo.

  @param[in] FmpImageInfo A pointer to EFI_FIRMWARE_IMAGE_DESCRIPTOR

  @return TRUE  It is a system FMP.
  @return FALSE It is a device FMP.
**/
BOOLEAN
IsSystemFmp (
  IN EFI_FIRMWARE_IMAGE_DESCRIPTOR  *FmpImageInfo
  )
{
  GUID   *Guid;
  UINTN  Count;
  UINTN  Index;

  Guid  = PcdGetPtr (PcdSystemFmpCapsuleImageTypeIdGuid);
  Count = PcdGetSize (PcdSystemFmpCapsuleImageTypeIdGuid) / sizeof (GUID);

  for (Index = 0; Index < Count; Index++, Guid++) {
    if (CompareGuid (&FmpImageInfo->ImageTypeId, Guid)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Function to create a single ESRT Entry and add it to the ESRT with
  a given FMP descriptor.  If the GUID is already in the ESRT, then the ESRT
  entry is updated.

  @param[in,out] Table                Pointer to the ESRT Table.
  @param[in,out] HardwareInstances    Pointer to the GUID_HARDWAREINSTANCE_PAIR.
  @param[in,out] NumberOfDescriptors  The number of EFI_FIRMWARE_IMAGE_DESCRIPTORs.
  @param[in]     FmpImageInfoBuf      Pointer to the EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in]     FmpVersion           FMP Version.

  @retval  EFI_SUCCESS     FmpImageInfoBuf was use to fill in a new ESRT entry
                           in Table.
  @retval  EFI_SUCCESS     The ImageTypeId GUID in FmpImageInfoBuf matches an
                           existing ESRT entry in Table, and the information
                           from FmpImageInfoBuf was merged into the the existing
                           ESRT entry.
  @retval  EFI_UNSPOORTED  The GUID/HardareInstance in FmpImageInfoBuf has is a
                           duplicate.

**/
EFI_STATUS
CreateEsrtEntry (
  IN OUT EFI_SYSTEM_RESOURCE_TABLE      *Table,
  IN OUT GUID_HARDWAREINSTANCE_PAIR     *HardwareInstances,
  IN OUT UINT32                         *NumberOfDescriptors,
  IN     EFI_FIRMWARE_IMAGE_DESCRIPTOR  *FmpImageInfoBuf,
  IN     UINT32                         FmpVersion
  )
{
  UINTN                      Index;
  EFI_SYSTEM_RESOURCE_ENTRY  *Entry;
  UINT64                     FmpHardwareInstance;

  FmpHardwareInstance = 0;
  if (FmpVersion >= 3) {
    FmpHardwareInstance = FmpImageInfoBuf->HardwareInstance;
  }

  //
  // Check to see of FmpImageInfoBuf GUID/HardwareInstance is unique
  // Skip if HardwareInstance is 0 as this is the case if FmpVersion < 3
  // or the device can not create a unique ID per UEFI specification
  //
  if (FmpHardwareInstance != 0) {
    for (Index = 0; Index < *NumberOfDescriptors; Index++) {
      if (CompareGuid (&HardwareInstances[Index].ImageTypeGuid, &FmpImageInfoBuf->ImageTypeId)) {
        if (HardwareInstances[Index].HardwareInstance == FmpHardwareInstance) {
          DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Duplicate firmware image descriptor with GUID %g HardwareInstance:0x%x\n", &FmpImageInfoBuf->ImageTypeId, FmpHardwareInstance));
          ASSERT (
            !CompareGuid (&HardwareInstances[Index].ImageTypeGuid, &FmpImageInfoBuf->ImageTypeId) ||
            HardwareInstances[Index].HardwareInstance != FmpHardwareInstance
            );
          return EFI_UNSUPPORTED;
        }
      }
    }
  }

  //
  // Record new GUID/HardwareInstance pair
  //
  CopyGuid (&HardwareInstances[*NumberOfDescriptors].ImageTypeGuid, &FmpImageInfoBuf->ImageTypeId);
  HardwareInstances[*NumberOfDescriptors].HardwareInstance = FmpHardwareInstance;
  *NumberOfDescriptors                                     = *NumberOfDescriptors + 1;

  DEBUG ((DEBUG_INFO, "EsrtFmpDxe: Add new image descriptor with GUID %g HardwareInstance:0x%x\n", &FmpImageInfoBuf->ImageTypeId, FmpHardwareInstance));

  //
  // Check to see if GUID is already in the ESRT table
  //
  Entry = (EFI_SYSTEM_RESOURCE_ENTRY *)(Table + 1);
  for (Index = 0; Index < Table->FwResourceCount; Index++, Entry++) {
    if (!CompareGuid (&Entry->FwClass, &FmpImageInfoBuf->ImageTypeId)) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "EsrtFmpDxe: ESRT Entry already exists for FMP Instance with GUID %g\n", &Entry->FwClass));

    //
    // Set ESRT FwVersion to the smaller of the two values
    //
    Entry->FwVersion = MIN (FmpImageInfoBuf->Version, Entry->FwVersion);

    //
    // VERSION 2 has Lowest Supported
    //
    if (FmpVersion >= 2) {
      //
      // Set ESRT LowestSupportedFwVersion to the smaller of the two values
      //
      Entry->LowestSupportedFwVersion =
        MIN (
          FmpImageInfoBuf->LowestSupportedImageVersion,
          Entry->LowestSupportedFwVersion
          );
    }

    //
    // VERSION 3 supports last attempt values
    //
    if (FmpVersion >= 3) {
      //
      // Update the ESRT entry with the last attempt status and last attempt
      // version from the first FMP instance whose last attempt status is not
      // SUCCESS.  If all FMP instances are SUCCESS, then set version to the
      // smallest value from all FMP instances.
      //
      if (Entry->LastAttemptStatus == LAST_ATTEMPT_STATUS_SUCCESS) {
        if (FmpImageInfoBuf->LastAttemptStatus != LAST_ATTEMPT_STATUS_SUCCESS) {
          Entry->LastAttemptStatus  = FmpImageInfoBuf->LastAttemptStatus;
          Entry->LastAttemptVersion = FmpImageInfoBuf->LastAttemptVersion;
        } else {
          Entry->LastAttemptVersion =
            MIN (
              FmpImageInfoBuf->LastAttemptVersion,
              Entry->LastAttemptVersion
              );
        }
      }
    }

    return EFI_SUCCESS;
  }

  //
  // Add a new ESRT Table Entry
  //
  Entry = (EFI_SYSTEM_RESOURCE_ENTRY *)(Table + 1) + Table->FwResourceCount;

  CopyGuid (&Entry->FwClass, &FmpImageInfoBuf->ImageTypeId);

  if (IsSystemFmp (FmpImageInfoBuf)) {
    DEBUG ((DEBUG_INFO, "EsrtFmpDxe: Found an ESRT entry for a System Device.\n"));
    Entry->FwType = (UINT32)(ESRT_FW_TYPE_SYSTEMFIRMWARE);
  } else {
    Entry->FwType = (UINT32)(ESRT_FW_TYPE_DEVICEFIRMWARE);
  }

  Entry->FwVersion                = FmpImageInfoBuf->Version;
  Entry->LowestSupportedFwVersion = 0;
  Entry->CapsuleFlags             = 0;
  Entry->LastAttemptVersion       = 0;
  Entry->LastAttemptStatus        = 0;

  //
  // VERSION 2 has Lowest Supported
  //
  if (FmpVersion >= 2) {
    Entry->LowestSupportedFwVersion = FmpImageInfoBuf->LowestSupportedImageVersion;
  }

  //
  // VERSION 3 supports last attempt values
  //
  if (FmpVersion >= 3) {
    Entry->LastAttemptVersion = FmpImageInfoBuf->LastAttemptVersion;
    Entry->LastAttemptStatus  = FmpImageInfoBuf->LastAttemptStatus;
  }

  //
  // Increment the number of active ESRT Table Entries
  //
  Table->FwResourceCount++;

  return EFI_SUCCESS;
}

/**
  Function to retrieve the EFI_FIRMWARE_IMAGE_DESCRIPTOR from an FMP Instance.
  The returned buffer is allocated using AllocatePool() and must be freed by the
  caller using FreePool().

  @param[in]  Fmp                        Pointer to an EFI_FIRMWARE_MANAGEMENT_PROTOCOL.
  @param[out] FmpImageInfoDescriptorVer  Pointer to the version number associated
                                         with the returned EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] FmpImageInfoCount          Pointer to the number of the returned
                                         EFI_FIRMWARE_IMAGE_DESCRIPTORs.
  @param[out] DescriptorSize             Pointer to the size, in bytes, of each
                                         returned EFI_FIRMWARE_IMAGE_DESCRIPTOR.

  @return  Pointer to the retrieved EFI_FIRMWARE_IMAGE_DESCRIPTOR.  If the
           descriptor can not be retrieved, then NULL is returned.

**/
EFI_FIRMWARE_IMAGE_DESCRIPTOR *
FmpGetFirmwareImageDescriptor (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *Fmp,
  OUT UINT32                            *FmpImageInfoDescriptorVer,
  OUT UINT8                             *FmpImageInfoCount,
  OUT UINTN                             *DescriptorSize
  )
{
  EFI_STATUS                     Status;
  UINTN                          ImageInfoSize;
  UINT32                         PackageVersion;
  CHAR16                         *PackageVersionName;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR  *FmpImageInfoBuf;

  ImageInfoSize = 0;
  Status        = Fmp->GetImageInfo (
                         Fmp,                       // FMP Pointer
                         &ImageInfoSize,            // Buffer Size (in this case 0)
                         NULL,                      // NULL so we can get size
                         FmpImageInfoDescriptorVer, // DescriptorVersion
                         FmpImageInfoCount,         // DescriptorCount
                         DescriptorSize,            // DescriptorSize
                         &PackageVersion,           // PackageVersion
                         &PackageVersionName        // PackageVersionName
                         );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Unexpected Failure in GetImageInfo.  Status = %r\n", Status));
    return NULL;
  }

  FmpImageInfoBuf = AllocateZeroPool (ImageInfoSize);
  if (FmpImageInfoBuf == NULL) {
    DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failed to get memory for FMP descriptor.\n"));
    return NULL;
  }

  PackageVersionName = NULL;
  Status             = Fmp->GetImageInfo (
                              Fmp,                       // FMP Pointer
                              &ImageInfoSize,            // ImageInfoSize
                              FmpImageInfoBuf,           // ImageInfo
                              FmpImageInfoDescriptorVer, // DescriptorVersion
                              FmpImageInfoCount,         // DescriptorCount
                              DescriptorSize,            // DescriptorSize
                              &PackageVersion,           // PackageVersion
                              &PackageVersionName        // PackageVersionName
                              );
  if (PackageVersionName != NULL) {
    FreePool (PackageVersionName);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failure in GetImageInfo.  Status = %r\n", Status));
    FreePool (FmpImageInfoBuf);
    return NULL;
  }

  return FmpImageInfoBuf;
}

/**
  Function to create ESRT based on FMP Instances.
  Create ESRT table, get the descriptors from FMP Instance and
  create ESRT entries (ESRE).

  @return Pointer to the ESRT created.

**/
EFI_SYSTEM_RESOURCE_TABLE *
CreateFmpBasedEsrt (
  VOID
  )
{
  EFI_STATUS                     Status;
  UINTN                          NoProtocols;
  VOID                           **Buffer;
  UINTN                          Index;
  UINT32                         FmpImageInfoDescriptorVer;
  UINT8                          FmpImageInfoCount;
  UINTN                          DescriptorSize;
  UINT32                         NumberOfDescriptors;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR  *FmpImageInfoBuf;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR  *OrgFmpImageInfoBuf;
  EFI_SYSTEM_RESOURCE_TABLE      *Table;
  GUID_HARDWAREINSTANCE_PAIR     *HardwareInstances;

  Status             = EFI_SUCCESS;
  NoProtocols        = 0;
  Buffer             = NULL;
  FmpImageInfoBuf    = NULL;
  OrgFmpImageInfoBuf = NULL;
  Table              = NULL;
  HardwareInstances  = NULL;

  Status = EfiLocateProtocolBuffer (
             &gEfiFirmwareManagementProtocolGuid,
             &NoProtocols,
             &Buffer
             );
  if (EFI_ERROR (Status) || (Buffer == NULL)) {
    return NULL;
  }

  //
  // Count the total number of EFI_FIRMWARE_IMAGE_DESCRIPTORs
  //
  for (Index = 0, NumberOfDescriptors = 0; Index < NoProtocols; Index++) {
    FmpImageInfoBuf = FmpGetFirmwareImageDescriptor (
                        (EFI_FIRMWARE_MANAGEMENT_PROTOCOL *)Buffer[Index],
                        &FmpImageInfoDescriptorVer,
                        &FmpImageInfoCount,
                        &DescriptorSize
                        );
    if (FmpImageInfoBuf != NULL) {
      NumberOfDescriptors += FmpImageInfoCount;
      FreePool (FmpImageInfoBuf);
    }
  }

  //
  // Allocate ESRT Table and GUID/HardwareInstance table
  //
  Table = AllocateZeroPool (
            (NumberOfDescriptors * sizeof (EFI_SYSTEM_RESOURCE_ENTRY)) + sizeof (EFI_SYSTEM_RESOURCE_TABLE)
            );
  if (Table == NULL) {
    DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failed to allocate memory for ESRT.\n"));
    FreePool (Buffer);
    return NULL;
  }

  HardwareInstances = AllocateZeroPool (NumberOfDescriptors * sizeof (GUID_HARDWAREINSTANCE_PAIR));
  if (HardwareInstances == NULL) {
    DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failed to allocate memory for HW Instance Table.\n"));
    FreePool (Table);
    FreePool (Buffer);
    return NULL;
  }

  //
  // Initialize ESRT Table
  //
  Table->FwResourceCount    = 0;
  Table->FwResourceCountMax = NumberOfDescriptors;
  Table->FwResourceVersion  = EFI_SYSTEM_RESOURCE_TABLE_FIRMWARE_RESOURCE_VERSION;

  NumberOfDescriptors = 0;
  for (Index = 0; Index < NoProtocols; Index++) {
    FmpImageInfoBuf = FmpGetFirmwareImageDescriptor (
                        (EFI_FIRMWARE_MANAGEMENT_PROTOCOL *)Buffer[Index],
                        &FmpImageInfoDescriptorVer,
                        &FmpImageInfoCount,
                        &DescriptorSize
                        );
    if (FmpImageInfoBuf == NULL) {
      continue;
    }

    //
    // Check each descriptor and read from the one specified
    //
    OrgFmpImageInfoBuf = FmpImageInfoBuf;
    while (FmpImageInfoCount > 0) {
      //
      // If the descriptor has the IN USE bit set, create ESRT entry otherwise ignore.
      //
      if ((FmpImageInfoBuf->AttributesSetting & FmpImageInfoBuf->AttributesSupported & IMAGE_ATTRIBUTE_IN_USE) == IMAGE_ATTRIBUTE_IN_USE) {
        //
        // Create ESRT entry
        //
        CreateEsrtEntry (Table, HardwareInstances, &NumberOfDescriptors, FmpImageInfoBuf, FmpImageInfoDescriptorVer);
      }

      FmpImageInfoCount--;
      //
      // Increment the buffer pointer ahead by the size of the descriptor
      //
      FmpImageInfoBuf = (EFI_FIRMWARE_IMAGE_DESCRIPTOR *)(((UINT8 *)FmpImageInfoBuf) + DescriptorSize);
    }

    FreePool (OrgFmpImageInfoBuf);
    OrgFmpImageInfoBuf = NULL;
  }

  FreePool (Buffer);
  FreePool (HardwareInstances);
  return Table;
}

/**
  Notify function for event group EFI_EVENT_GROUP_READY_TO_BOOT. This is used to
  install the Efi System Resource Table.

  @param[in]  Event    The Event that is being processed.
  @param[in]  Context  The Event Context.

**/
VOID
EFIAPI
EsrtReadyToBootEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                 Status;
  EFI_SYSTEM_RESOURCE_TABLE  *Table;

  Table = CreateFmpBasedEsrt ();
  if (Table != NULL) {
    //
    // Print table on debug builds
    //
    DEBUG_CODE_BEGIN ();
    PrintTable (Table);
    DEBUG_CODE_END ();

    Status = InstallEfiSystemResourceTableInUefiConfigurationTable (Table);
    if (EFI_ERROR (Status)) {
      FreePool (Table);
    }
  } else {
    DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Can't install ESRT table because it is NULL. \n"));
  }

  //
  // Close the event to prevent it be signalled again.
  //
  gBS->CloseEvent (Event);
}

/**
  The module Entry Point of the Efi System Resource Table DXE driver.

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval  EFI_SUCCESS  The entry point is executed successfully.
  @retval  Other        Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
EsrtFmpEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   EsrtReadyToBootEvent;

  //
  // Register notify function to install ESRT on ReadyToBoot Event.
  //
  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             EsrtReadyToBootEventNotify,
             NULL,
             &EsrtReadyToBootEvent
             );

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failed to register for ready to boot\n"));
  }

  return Status;
}
