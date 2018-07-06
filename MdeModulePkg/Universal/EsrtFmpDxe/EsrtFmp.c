/** @file
  Publishes ESRT table from Firmware Management Protocol instances

  Copyright (c) 2016, Microsoft Corporation
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  All rights reserved.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

/**
 Print ESRT to debug console.

 @param[in]  Table   Pointer to the ESRT table.

**/
VOID
EFIAPI
PrintTable (
  IN EFI_SYSTEM_RESOURCE_TABLE  *Table
  );

//
// Number of ESRT entries to grow by each time we run out of room
//
#define GROWTH_STEP  10

//
// Module globals.
//
EFI_EVENT                  mEsrtReadyToBootEvent;
EFI_SYSTEM_RESOURCE_TABLE  *mTable = NULL;
BOOLEAN                    mEsrtInstalled = FALSE;
EFI_EVENT                  mFmpInstallEvent;
VOID                       *mFmpInstallEventRegistration = NULL;

/**
  Install EFI System Resource Table into the UEFI Configuration Table

  @return  Status code.

**/
EFI_STATUS
InstallEfiSystemResourceTableInUefiConfigurationTable (
   VOID
  )
{
  EFI_STATUS Status;

  Status = EFI_SUCCESS;
  if (!mEsrtInstalled) {
    if (mTable == NULL) {
      DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Can't install ESRT table because it is NULL. \n"));
      Status = EFI_OUT_OF_RESOURCES;
    } else if (mTable->FwResourceCount == 0) {
      DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Can't install ESRT table because it has zero Entries. \n"));
      Status = EFI_UNSUPPORTED;
    } else {
      //
      // Install the pointer into config table
      //
      Status = gBS->InstallConfigurationTable (&gEfiSystemResourceTableGuid, mTable);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Can't install ESRT table.  Status: %r. \n", Status));
      } else {
        DEBUG ((DEBUG_INFO, "EsrtFmpDxe: Installed ESRT table. \n"));
        mEsrtInstalled = TRUE;
      }
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
  Count = PcdGetSize (PcdSystemFmpCapsuleImageTypeIdGuid) / sizeof(GUID);

  for (Index = 0; Index < Count; Index++, Guid++) {
    if (CompareGuid (&FmpImageInfo->ImageTypeId, Guid)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Function to create a single ESRT Entry and add it to the ESRT
  given a FMP descriptor.  If the guid is already in the ESRT it
  will be ignored.  The ESRT will grow if it does not have enough room.

  @param[in]  FmpImageInfoBuf    Pointer to the EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[in]  FmpVersion         FMP Version.

  @return  Status code.

**/
EFI_STATUS
EFIAPI
CreateEsrtEntry (
  IN EFI_FIRMWARE_IMAGE_DESCRIPTOR  *FmpImageInfoBuf,
  IN UINT32                         FmpVersion
  )
{
  UINTN                      Index;
  EFI_SYSTEM_RESOURCE_ENTRY  *Entry;
  UINTN                      NewSize;
  EFI_SYSTEM_RESOURCE_TABLE  *NewTable;

  Index = 0;
  Entry = NULL;

  //
  // Get our ESRT table.  This should never be null at this point
  //
  if (mTable == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Entry = (EFI_SYSTEM_RESOURCE_ENTRY *)(mTable + 1);
  //
  // Make sure Guid isn't already in the list
  //
  for (Index = 0; Index < mTable->FwResourceCount; Index++) {
    if (CompareGuid (&Entry->FwClass, &FmpImageInfoBuf->ImageTypeId)) {
      DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: ESRT Entry already exists for FMP Instance with GUID %g\n", &Entry->FwClass));
      return EFI_INVALID_PARAMETER;
    }
    Entry++;
  }

  //
  // Grow table if needed
  //
  if (mTable->FwResourceCount >= mTable->FwResourceCountMax) {
    //
    // Can't grow table after installed.
    // Only because didn't add support for this.
    // Would need to re-install ESRT in system table if wanted to support
    //
    if (mEsrtInstalled) {
      DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failed to install entry because ESRT table needed to grow after table already installed. \n"));
      return EFI_OUT_OF_RESOURCES;
    }

    NewSize  = ((mTable->FwResourceCountMax + GROWTH_STEP) * sizeof (EFI_SYSTEM_RESOURCE_ENTRY)) + sizeof (EFI_SYSTEM_RESOURCE_TABLE);
    NewTable = AllocateZeroPool (NewSize);
    if (NewTable == NULL) {
      DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failed to allocate memory larger table for ESRT. \n"));
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Copy the whole old table into new table buffer
    //
    CopyMem (
      NewTable,
      mTable,
      ((mTable->FwResourceCountMax) * sizeof (EFI_SYSTEM_RESOURCE_ENTRY)) + sizeof (EFI_SYSTEM_RESOURCE_TABLE)
      );
    //
    // Update max
    //
    NewTable->FwResourceCountMax = NewTable->FwResourceCountMax + GROWTH_STEP;
    //
    // Free old table
    //
    FreePool (mTable);
    //
    // Reassign pointer to new table.
    //
    mTable = NewTable;
  }

  //
  // ESRT table has enough room for the new entry so add new entry
  //
  Entry = (EFI_SYSTEM_RESOURCE_ENTRY *)(((UINT8 *)mTable) + sizeof (EFI_SYSTEM_RESOURCE_TABLE));
  //
  // Move to the location of new entry
  //
  Entry = Entry + mTable->FwResourceCount;
  //
  // Increment resource count
  //
  mTable->FwResourceCount++;

  CopyGuid (&Entry->FwClass, &FmpImageInfoBuf->ImageTypeId);

  if (IsSystemFmp (FmpImageInfoBuf)) {
    DEBUG ((DEBUG_INFO, "EsrtFmpDxe: Found an ESRT entry for a System Device.\n"));
    Entry->FwType = (UINT32)(ESRT_FW_TYPE_SYSTEMFIRMWARE);
  } else {
    Entry->FwType = (UINT32)(ESRT_FW_TYPE_DEVICEFIRMWARE);
  }

  Entry->FwVersion = FmpImageInfoBuf->Version;
  Entry->LowestSupportedFwVersion = 0;
  Entry->CapsuleFlags = 0;
  Entry->LastAttemptVersion = 0;
  Entry->LastAttemptStatus = 0;

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
    Entry->LastAttemptStatus = FmpImageInfoBuf->LastAttemptStatus;
  }

  return EFI_SUCCESS;
}

/**
  Notify function for every Firmware Management Protocol being installed.
  Get the descriptors from FMP Instance and create ESRT entries (ESRE)

  @param[in]  Event    The Event that is being processed.
  @param[in]  Context  The Event Context.

**/
VOID
EFIAPI
FmpInstallProtocolNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        Handle;
  UINTN                             BufferSize;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *Fmp;
  UINTN                             DescriptorSize;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR     *FmpImageInfoBuf;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR     *FmpImageInfoBufOrg;
  UINT8                             FmpImageInfoCount;
  UINT32                            FmpImageInfoDescriptorVer;
  UINTN                             ImageInfoSize;
  UINT32                            PackageVersion;
  CHAR16                            *PackageVersionName;

  Status             = EFI_SUCCESS;
  Handle             = 0;
  BufferSize         = 0;
  PackageVersionName = NULL;
  FmpImageInfoBuf    = NULL;
  FmpImageInfoBufOrg = NULL;
  Fmp                = NULL;

  DEBUG ((DEBUG_INFO, "FMP Installed Notify\n"));
  while (TRUE) {
    BufferSize = sizeof (EFI_HANDLE);
    Status = gBS->LocateHandle (ByRegisterNotify, NULL, mFmpInstallEventRegistration, &BufferSize, &Handle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "EsrtFmpDxe: Failed to Locate handle from notify value. Status: %r\n", Status));
      return;
    }

    Status = gBS->HandleProtocol (Handle, &gEfiFirmwareManagementProtocolGuid, (VOID **)&Fmp);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failed to get FMP for a handle 0x%x\n", Handle));
      continue;
    }
    ImageInfoSize = 0;

    Status = Fmp->GetImageInfo (
                    Fmp,                         // FMP Pointer
                    &ImageInfoSize,              // Buffer Size (in this case 0)
                    NULL,                        // NULL so we can get size
                    &FmpImageInfoDescriptorVer,  // DescriptorVersion
                    &FmpImageInfoCount,          // DescriptorCount
                    &DescriptorSize,             // DescriptorSize
                    &PackageVersion,             // PackageVersion
                    &PackageVersionName          // PackageVersionName
                    );

    if (Status != EFI_BUFFER_TOO_SMALL) {
      DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Unexpected Failure in GetImageInfo.  Status = %r\n", Status));
      continue;
    }

    FmpImageInfoBuf = NULL;
    FmpImageInfoBuf = AllocateZeroPool (ImageInfoSize);
    if (FmpImageInfoBuf == NULL) {
      DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failed to get memory for descriptors.\n"));
      continue;
    }

    FmpImageInfoBufOrg = FmpImageInfoBuf;
    PackageVersionName = NULL;
    Status = Fmp->GetImageInfo (
                    Fmp,
                    &ImageInfoSize,              // ImageInfoSize
                    FmpImageInfoBuf,             // ImageInfo
                    &FmpImageInfoDescriptorVer,  // DescriptorVersion
                    &FmpImageInfoCount,          // DescriptorCount
                    &DescriptorSize,             // DescriptorSize
                    &PackageVersion,             // PackageVersion
                    &PackageVersionName          // PackageVersionName
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failure in GetImageInfo.  Status = %r\n", Status));
      goto CleanUp;
    }

    //
    // Check each descriptor and read from the one specified
    //
    while (FmpImageInfoCount > 0) {
      //
      // If the descriptor has the IN USE bit set, create ESRT entry otherwise ignore.
      //
      if ((FmpImageInfoBuf->AttributesSetting & FmpImageInfoBuf->AttributesSupported & IMAGE_ATTRIBUTE_IN_USE) == IMAGE_ATTRIBUTE_IN_USE) {
        //
        // Create ESRT entry
        //
        CreateEsrtEntry (FmpImageInfoBuf, FmpImageInfoDescriptorVer);
      }
      FmpImageInfoCount--;
      //
      // Increment the buffer pointer ahead by the size of the descriptor
      //
      FmpImageInfoBuf = (EFI_FIRMWARE_IMAGE_DESCRIPTOR *)(((UINT8 *)FmpImageInfoBuf) + DescriptorSize);
    }

    if (PackageVersionName != NULL) {
      FreePool (PackageVersionName);
      PackageVersionName = NULL;
    }
    if (FmpImageInfoBufOrg != NULL) {
      FreePool (FmpImageInfoBufOrg);
      FmpImageInfoBufOrg = NULL;
    }
  }

CleanUp:
  if (FmpImageInfoBufOrg != NULL) {
    FreePool (FmpImageInfoBufOrg);
  }
  return;
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
  InstallEfiSystemResourceTableInUefiConfigurationTable ();

  //
  // Print table on debug builds
  //
  DEBUG_CODE_BEGIN ();
  PrintTable (mTable);
  DEBUG_CODE_END ();
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

  //
  // Allocate Memory for table
  //
  mTable = AllocateZeroPool (
             (GROWTH_STEP * sizeof (EFI_SYSTEM_RESOURCE_ENTRY)) + sizeof (EFI_SYSTEM_RESOURCE_TABLE)
             );
  ASSERT (mTable != NULL);
  if (mTable == NULL) {
    DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failed to allocate memory for ESRT.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  mTable->FwResourceCount    = 0;
  mTable->FwResourceCountMax = GROWTH_STEP;
  mTable->FwResourceVersion  = EFI_SYSTEM_RESOURCE_TABLE_FIRMWARE_RESOURCE_VERSION;

  //
  // Register notify function for all FMP installed
  //
  mFmpInstallEvent = EfiCreateProtocolNotifyEvent (
                       &gEfiFirmwareManagementProtocolGuid,
                       TPL_CALLBACK,
                       FmpInstallProtocolNotify,
                       NULL,
                       &mFmpInstallEventRegistration
                       );

  ASSERT (mFmpInstallEvent != NULL);

  if (mFmpInstallEvent == NULL) {
    DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failed to Create Protocol Notify Event for FMP.\n"));
  }

  //
  // Register notify function to install ESRT on ReadyToBoot Event.
  //
  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             EsrtReadyToBootEventNotify,
             NULL,
             &mEsrtReadyToBootEvent
             );

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EsrtFmpDxe: Failed to register for ready to boot\n"));
  }

  return Status;
}
