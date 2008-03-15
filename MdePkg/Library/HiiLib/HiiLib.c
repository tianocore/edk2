/** @file
  HII Library implementation that uses DXE protocols and services.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalHiiLib.h"

EFI_HII_DATABASE_PROTOCOL   *mHiiDatabaseProt;
EFI_HII_STRING_PROTOCOL     *mHiiStringProt;

/**
  The constructor function of Hii Library.
  
  The constructor function caches the value of default HII protocol instances.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
UefiHiiLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS Status;
  
  Status = gBS->LocateProtocol (
      &gEfiHiiDatabaseProtocolGuid,
      NULL,
      (VOID **) &mHiiDatabaseProt
    );
  ASSERT_EFI_ERROR (Status);
  ASSERT (mHiiDatabaseProt != NULL);

  Status = gBS->LocateProtocol (
      &gEfiHiiStringProtocolGuid,
      NULL,
      (VOID **) &mHiiStringProt
    );
  ASSERT_EFI_ERROR (Status);
  ASSERT (mHiiStringProt != NULL);

  return EFI_SUCCESS;
}



EFI_HII_PACKAGE_LIST_HEADER *
InternalHiiLibPreparePackages (
  IN UINTN           NumberOfPackages,
  IN CONST EFI_GUID  *GuidId, OPTIONAL
  VA_LIST            Marker
  )
{
  EFI_HII_PACKAGE_LIST_HEADER *PackageListHeader;
  UINT8                       *PackageListData;
  UINT32                      PackageListLength;
  UINT32                      PackageLength;
  EFI_HII_PACKAGE_HEADER      PackageHeader;
  UINT8                       *PackageArray;
  UINTN                       Index;
  VA_LIST                     MarkerBackup;

  PackageListLength = sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  MarkerBackup = Marker;
  
  for (Index = 0; Index < NumberOfPackages; Index++) {
    CopyMem (&PackageLength, VA_ARG (Marker, VOID *), sizeof (UINT32));
    PackageListLength += (PackageLength - sizeof (UINT32));
  }

  //
  // Include the lenght of EFI_HII_PACKAGE_END
  //
  PackageListLength += sizeof (EFI_HII_PACKAGE_HEADER);
  PackageListHeader = AllocateZeroPool (PackageListLength);
  ASSERT (PackageListHeader != NULL);
  CopyMem (&PackageListHeader->PackageListGuid, GuidId, sizeof (EFI_GUID));
  PackageListHeader->PackageLength = PackageListLength;

  PackageListData = ((UINT8 *) PackageListHeader) + sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  Marker = MarkerBackup;
  for (Index = 0; Index < NumberOfPackages; Index++) {
    PackageArray = (UINT8 *) VA_ARG (Marker, VOID *);
    CopyMem (&PackageLength, PackageArray, sizeof (UINT32));
    PackageLength  -= sizeof (UINT32);
    PackageArray += sizeof (UINT32);
    CopyMem (PackageListData, PackageArray, PackageLength);
    PackageListData += PackageLength;
  }

  //
  // Append EFI_HII_PACKAGE_END
  //
  PackageHeader.Type = EFI_HII_PACKAGE_END;
  PackageHeader.Length = sizeof (EFI_HII_PACKAGE_HEADER);
  CopyMem (PackageListData, &PackageHeader, PackageHeader.Length);

  return PackageListHeader;
}

EFI_HII_PACKAGE_LIST_HEADER *
EFIAPI
HiiLibPreparePackageList (
  IN UINTN                    NumberOfPackages,
  IN CONST EFI_GUID           *GuidId,
  ...
  )
{
  EFI_HII_PACKAGE_LIST_HEADER *PackageListHeader;
  VA_LIST                     Marker;

  ASSERT (GuidId != NULL);

  VA_START (Marker, GuidId);
  PackageListHeader = InternalHiiLibPreparePackages (NumberOfPackages, GuidId, Marker);
  VA_END (Marker);

  return PackageListHeader;
}


EFI_STATUS
EFIAPI
HiiLibAddPackages (
  IN       UINTN               NumberOfPackages,
  IN CONST EFI_GUID            *GuidId,
  IN       EFI_HANDLE          DriverHandle, OPTIONAL
  OUT      EFI_HII_HANDLE      *HiiHandle, OPTIONAL
  ...
  )
{
  VA_LIST                   Args;
  EFI_HII_PACKAGE_LIST_HEADER *PackageListHeader;
  EFI_STATUS                Status;

  ASSERT (HiiHandle != NULL);

  VA_START (Args, HiiHandle);
  PackageListHeader = InternalHiiLibPreparePackages (NumberOfPackages, GuidId, Args);

  Status      = mHiiDatabaseProt->NewPackageList (mHiiDatabaseProt, PackageListHeader, DriverHandle, HiiHandle);
  if (HiiHandle != NULL) {
    if (EFI_ERROR (Status)) {
      *HiiHandle = NULL;
    }
  }

  FreePool (PackageListHeader);
  VA_END (Args);
  
  return Status;
}

VOID
EFIAPI
HiiLibRemovePackages (
  IN      EFI_HII_HANDLE      HiiHandle
  )
{
  EFI_STATUS Status;

  ASSERT (HiiHandle != NULL);
  Status = mHiiDatabaseProt->RemovePackageList (mHiiDatabaseProt, HiiHandle);
  ASSERT_EFI_ERROR (Status);
}


EFI_STATUS
EFIAPI
HiiLibGetHiiHandles (
  IN OUT UINTN                     *HandleBufferLength,
  OUT    EFI_HII_HANDLE            **HiiHandleBuffer
  )
{
  UINTN       BufferLength;
  EFI_STATUS  Status;

  ASSERT (HandleBufferLength != NULL);
  ASSERT (HiiHandleBuffer != NULL);

  BufferLength = 0;

  //
  // Try to find the actual buffer size for HiiHandle Buffer.
  //
  Status = mHiiDatabaseProt->ListPackageLists (
                                 mHiiDatabaseProt,
                                 EFI_HII_PACKAGE_TYPE_ALL,
                                 NULL,
                                 &BufferLength,
                                 *HiiHandleBuffer
                                 );

  if (Status == EFI_BUFFER_TOO_SMALL) {
      *HiiHandleBuffer = AllocateZeroPool (BufferLength);
      Status = mHiiDatabaseProt->ListPackageLists (
                                     mHiiDatabaseProt,
                                     EFI_HII_PACKAGE_TYPE_ALL,
                                     NULL,
                                     &BufferLength,
                                     *HiiHandleBuffer
                                     );
      //
      // we should not fail here.
      //
      ASSERT_EFI_ERROR (Status);
  }

  *HandleBufferLength = BufferLength;

  return Status;
}

EFI_STATUS
EFIAPI
HiiLibExtractGuidFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle,
  OUT     EFI_GUID            *Guid
  )
{
  EFI_STATUS                   Status;
  UINTN                        BufferSize;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;

  ASSERT (Guid != NULL);

  //
  // Get HII PackageList
  //
  BufferSize = 0;
  HiiPackageList = NULL;
  Status = mHiiDatabaseProt->ExportPackageLists (mHiiDatabaseProt, Handle, &BufferSize, HiiPackageList);
  ASSERT (Status != EFI_NOT_FOUND);
  
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = AllocatePool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = mHiiDatabaseProt->ExportPackageLists (mHiiDatabaseProt, Handle, &BufferSize, HiiPackageList);
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Extract GUID
  //
  CopyMem (Guid, &HiiPackageList->PackageListGuid, sizeof (EFI_GUID));

  gBS->FreePool (HiiPackageList);

  return EFI_SUCCESS;
}

EFI_HII_HANDLE
EFIAPI
HiiLibDevicePathToHiiHandle (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  EFI_STATUS                  Status;
  EFI_DEVICE_PATH_PROTOCOL    *TmpDevicePath;
  UINTN                       BufferSize;
  UINTN                       HandleCount;
  UINTN                       Index;
  EFI_HANDLE                  *Handles;
  EFI_HANDLE                  Handle;
  UINTN                       Size;
  EFI_HANDLE                  DriverHandle;
  EFI_HII_HANDLE              *HiiHandles;
  EFI_HII_HANDLE              HiiHandle;

  ASSERT (DevicePath != NULL);

  //
  // Locate Device Path Protocol handle buffer
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDevicePathProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Search Driver Handle by Device Path
  //
  DriverHandle = NULL;
  BufferSize = GetDevicePathSize (DevicePath);
  for(Index = 0; Index < HandleCount; Index++) {
    Handle = Handles[Index];
    gBS->HandleProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID **) &TmpDevicePath);

    //
    // Check whether DevicePath match
    //
    Size = GetDevicePathSize (TmpDevicePath);
    if ((Size == BufferSize) && CompareMem (DevicePath, TmpDevicePath, Size) == 0) {
      DriverHandle = Handle;
      break;
    }
  }
  gBS->FreePool (Handles);

  if (DriverHandle == NULL) {
    return NULL;
  }

  //
  // Retrieve all Hii Handles from HII database
  //
  BufferSize = 0x1000;
  HiiHandles = AllocatePool (BufferSize);
  ASSERT (HiiHandles != NULL);
  Status = mHiiDatabaseProt->ListPackageLists (
                          mHiiDatabaseProt,
                          EFI_HII_PACKAGE_TYPE_ALL,
                          NULL,
                          &BufferSize,
                          HiiHandles
                          );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    gBS->FreePool (HiiHandles);
    HiiHandles = AllocatePool (BufferSize);
    ASSERT (HiiHandles != NULL);

    Status = mHiiDatabaseProt->ListPackageLists (
                            mHiiDatabaseProt,
                            EFI_HII_PACKAGE_TYPE_ALL,
                            NULL,
                            &BufferSize,
                            HiiHandles
                            );
  }

  if (EFI_ERROR (Status)) {
    gBS->FreePool (HiiHandles);
    return NULL;
  }

  //
  // Search Hii Handle by Driver Handle
  //
  HiiHandle = NULL;
  HandleCount = BufferSize / sizeof (EFI_HII_HANDLE);
  for (Index = 0; Index < HandleCount; Index++) {
    Status = mHiiDatabaseProt->GetPackageListHandle (
                            mHiiDatabaseProt,
                            HiiHandles[Index],
                            &Handle
                            );
    if (!EFI_ERROR (Status) && (Handle == DriverHandle)) {
      HiiHandle = HiiHandles[Index];
      break;
    }
  }

  gBS->FreePool (HiiHandles);
  return HiiHandle;
}

BOOLEAN
IsHiiHandleRegistered (
  EFI_HII_HANDLE    HiiHandle
  )
{
  EFI_STATUS                   Status;
  UINTN                        BufferSize;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;

  ASSERT (HiiHandle != NULL);

  HiiPackageList = NULL;
  BufferSize = 0;
  Status = mHiiDatabaseProt->ExportPackageLists (
             mHiiDatabaseProt,
             HiiHandle,
             &BufferSize,
             HiiPackageList
             );

  return (BOOLEAN) (Status == EFI_BUFFER_TOO_SMALL);
}

