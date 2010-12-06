/*++

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UefiIfrCommon.c

Abstract:

  Common Library Routines to assist handle HII elements.

--*/

#include "UefiIfrLibrary.h"

//
// Hii vendor device path template
//
HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePathTemplate = {
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8) (sizeof (HII_VENDOR_DEVICE_PATH_NODE)),
          (UINT8) ((sizeof (HII_VENDOR_DEVICE_PATH_NODE)) >> 8)
        }
      },
      EFI_IFR_TIANO_GUID,
    },
    0,
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH
    }
  }
};

//
// Hii relative protocols
//
BOOLEAN  mHiiProtocolsInitialized = FALSE;

EFI_HII_DATABASE_PROTOCOL *gIfrLibHiiDatabase;
EFI_HII_STRING_PROTOCOL   *gIfrLibHiiString;

VOID
LocateHiiProtocols (
  VOID
  )
/*++

Routine Description:
  This function locate Hii relative protocols for later usage.

Arguments:
  None.

Returns:
  None.

--*/
{
  EFI_STATUS  Status;

  if (mHiiProtocolsInitialized) {
    return;
  }

  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **) &gIfrLibHiiDatabase);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiHiiStringProtocolGuid, NULL, (VOID **) &gIfrLibHiiString);
  ASSERT_EFI_ERROR (Status);

  mHiiProtocolsInitialized = TRUE;
}

EFI_HII_PACKAGE_LIST_HEADER *
PreparePackageList (
  IN UINTN                    NumberOfPackages,
  IN EFI_GUID                 *GuidId,
  ...
  )
/*++

Routine Description:
  Assemble EFI_HII_PACKAGE_LIST according to the passed in packages.

Arguments:
  NumberOfPackages  -  Number of packages.
  GuidId            -  Package GUID.

Returns:
  Pointer of EFI_HII_PACKAGE_LIST_HEADER.

--*/
{
  VA_LIST                     Marker;
  EFI_HII_PACKAGE_LIST_HEADER *PackageListHeader;
  UINT8                       *PackageListData;
  UINT32                      PackageListLength;
  UINT32                      PackageLength;
  EFI_HII_PACKAGE_HEADER      PackageHeader;
  UINT8                       *PackageArray;
  UINTN                       Index;

  PackageListLength = sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  VA_START (Marker, GuidId);
  for (Index = 0; Index < NumberOfPackages; Index++) {
    EfiCopyMem (&PackageLength, VA_ARG (Marker, VOID *), sizeof (UINT32));
    PackageListLength += (PackageLength - sizeof (UINT32));
  }
  VA_END (Marker);

  //
  // Include the lenght of EFI_HII_PACKAGE_END
  //
  PackageListLength += sizeof (EFI_HII_PACKAGE_HEADER);
  PackageListHeader = EfiLibAllocateZeroPool (PackageListLength);
  ASSERT (PackageListHeader != NULL);
  EfiCopyMem (&PackageListHeader->PackageListGuid, GuidId, sizeof (EFI_GUID));
  PackageListHeader->PackageLength = PackageListLength;

  PackageListData = ((UINT8 *) PackageListHeader) + sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  VA_START (Marker, GuidId);
  for (Index = 0; Index < NumberOfPackages; Index++) {
    PackageArray = (UINT8 *) VA_ARG (Marker, VOID *);
    EfiCopyMem (&PackageLength, PackageArray, sizeof (UINT32));
    PackageLength  -= sizeof (UINT32);
    PackageArray += sizeof (UINT32);
    EfiCopyMem (PackageListData, PackageArray, PackageLength);
    PackageListData += PackageLength;
  }
  VA_END (Marker);

  //
  // Append EFI_HII_PACKAGE_END
  //
  PackageHeader.Type = EFI_HII_PACKAGE_END;
  PackageHeader.Length = sizeof (EFI_HII_PACKAGE_HEADER);
  EfiCopyMem (PackageListData, &PackageHeader, PackageHeader.Length);

  return PackageListHeader;
}

EFI_STATUS
CreateHiiDriverHandle (
  OUT EFI_HANDLE               *DriverHandle
  )
/*++

Routine Description:
  The HII driver handle passed in for HiiDatabase.NewPackageList() requires
  that there should be DevicePath Protocol installed on it.
  This routine create a virtual Driver Handle by installing a vendor device
  path on it, so as to use it to invoke HiiDatabase.NewPackageList().

Arguments:
  DriverHandle         - Handle to be returned

Returns:
  EFI_SUCCESS          - Handle destroy success.
  EFI_OUT_OF_RESOURCES - Not enough memory.

--*/
{
  EFI_STATUS                   Status;
  HII_VENDOR_DEVICE_PATH_NODE  *VendorDevicePath;

  VendorDevicePath = EfiLibAllocateCopyPool (sizeof (HII_VENDOR_DEVICE_PATH), &mHiiVendorDevicePathTemplate);
  if (VendorDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Use memory address as unique ID to distinguish from different device paths
  //
  VendorDevicePath->UniqueId = (UINT64) ((UINTN) VendorDevicePath);

  *DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  VendorDevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DestroyHiiDriverHandle (
  IN EFI_HANDLE                 DriverHandle
  )
/*++

Routine Description:
  Destroy the Driver Handle created by CreateHiiDriverHandle().

Arguments:
  DriverHandle - Handle returned by CreateHiiDriverHandle()

Returns:
  EFI_SUCCESS - Handle destroy success.
  other       - Handle destroy fail.

--*/
{
  EFI_STATUS                   Status;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath;

  Status = gBS->HandleProtocol (
                  DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->UninstallProtocolInterface (
                  DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  DevicePath
                  );
  gBS->FreePool (DevicePath);
  return Status;
}

EFI_HII_HANDLE
DevicePathToHiiHandle (
  IN EFI_HII_DATABASE_PROTOCOL  *HiiDatabase,
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
/*++

Routine Description:
  Find HII Handle associated with given Device Path.

Arguments:
  HiiDatabase - Point to EFI_HII_DATABASE_PROTOCOL instance.
  DevicePath  - Device Path associated with the HII package list handle.

Returns:
  Handle - HII package list Handle associated with the Device Path.
  NULL   - Hii Package list handle is not found.

--*/
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
  BufferSize = EfiDevicePathSize (DevicePath);
  for(Index = 0; Index < HandleCount; Index++) {
    Handle = Handles[Index];
    gBS->HandleProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID **) &TmpDevicePath);

    //
    // Check whether DevicePath match
    //
    Size = EfiDevicePathSize (TmpDevicePath);
    if ((Size == BufferSize) && EfiCompareMem (DevicePath, TmpDevicePath, Size) == 0) {
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
  HiiHandles = EfiLibAllocatePool (BufferSize);
  ASSERT (HiiHandles != NULL);
  Status = HiiDatabase->ListPackageLists (
                          HiiDatabase,
                          EFI_HII_PACKAGE_TYPE_ALL,
                          NULL,
                          &BufferSize,
                          HiiHandles
                          );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    gBS->FreePool (HiiHandles);
    HiiHandles = EfiLibAllocatePool (BufferSize);
    ASSERT (HiiHandles != NULL);

    Status = HiiDatabase->ListPackageLists (
                            HiiDatabase,
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
    Status = HiiDatabase->GetPackageListHandle (
                            HiiDatabase,
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

EFI_STATUS
GetHiiHandles (
  IN OUT UINTN                     *HandleBufferLength,
  OUT    EFI_HII_HANDLE            **HiiHandleBuffer
  )
/*++

Routine Description:
  Determines the handles that are currently active in the database.
  It's the caller's responsibility to free handle buffer.

Arguments:
  HiiDatabase           - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.
  HandleBufferLength    - On input, a pointer to the length of the handle buffer. On output,
                          the length of the handle buffer that is required for the handles found.
  HiiHandleBuffer       - Pointer to an array of Hii Handles returned.

Returns:
  EFI_SUCCESS           - Get an array of Hii Handles successfully.
  EFI_INVALID_PARAMETER - Hii is NULL.
  EFI_NOT_FOUND         - Database not found.

--*/
{
  UINTN       BufferLength;
  EFI_STATUS  Status;

  BufferLength = 0;

  LocateHiiProtocols ();

  //
  // Try to find the actual buffer size for HiiHandle Buffer.
  //
  Status = gIfrLibHiiDatabase->ListPackageLists (
                                 gIfrLibHiiDatabase,
                                 EFI_HII_PACKAGE_TYPE_ALL,
                                 NULL,
                                 &BufferLength,
                                 *HiiHandleBuffer
                                 );

  if (Status == EFI_BUFFER_TOO_SMALL) {
      *HiiHandleBuffer = EfiLibAllocateZeroPool (BufferLength);
      Status = gIfrLibHiiDatabase->ListPackageLists (
                                     gIfrLibHiiDatabase,
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
ExtractGuidFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle,
  OUT     EFI_GUID            *Guid
  )
/*++

Routine Description:
  Extract Hii package list GUID for given HII handle.

Arguments:
  HiiHandle     - Hii handle
  Guid          - Package list GUID

Returns:
  EFI_SUCCESS   - Successfully extract GUID from Hii database.

--*/
{
  EFI_STATUS                   Status;
  UINTN                        BufferSize;
  EFI_HII_DATABASE_PROTOCOL    *HiiDatabase;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;

  //
  // Locate HII Database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get HII PackageList
  //
  BufferSize = 0;
  HiiPackageList = NULL;
  Status = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = EfiLibAllocatePool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Extract GUID
  //
  EfiCopyMem (Guid, &HiiPackageList->PackageListGuid, sizeof (EFI_GUID));

  gBS->FreePool (HiiPackageList);

  return EFI_SUCCESS;
}

EFI_STATUS
ExtractClassFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle,
  OUT     UINT16              *Class,
  OUT     EFI_STRING_ID       *FormSetTitle,
  OUT     EFI_STRING_ID       *FormSetHelp
  )
/*++

Routine Description:
  Extract formset class for given HII handle.

Arguments:
  HiiHandle       - Hii handle
  Class           - Class of the formset
  FormSetTitle    - Formset title string
  FormSetHelp     - Formset help string

Returns:
  EFI_SUCCESS     - Successfully extract Class for specified Hii handle.
  EFI_NOT_FOUND   - Class not found.

--*/
{
  EFI_STATUS                   Status;
  UINTN                        BufferSize;
  EFI_HII_DATABASE_PROTOCOL    *HiiDatabase;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINT8                        *Package;
  UINT8                        *FormSet;
  UINT8                        *OpCodeData;
  UINT32                       Offset;
  UINT32                       Offset2;
  UINT32                       PackageListLength;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  BOOLEAN                      ClassFound;

  *Class = EFI_NON_DEVICE_CLASS;
  *FormSetTitle = 0;
  *FormSetHelp = 0;
  ClassFound = FALSE;

  //
  // Locate HII Database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get HII PackageList
  //
  BufferSize = 0;
  HiiPackageList = NULL;
  Status = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = EfiLibAllocatePool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
  }
  if (HiiPackageList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  if (EFI_ERROR (Status)) {
    gBS->FreePool (HiiPackageList);
    return Status;
  }

  //
  // Get Form package from this HII package List
  //
  Offset = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  Offset2 = 0;
  FormSet = NULL;
  EfiCopyMem (&PackageListLength, &HiiPackageList->PackageLength, sizeof (UINT32));

  while (Offset < PackageListLength) {
    Package = ((UINT8 *) HiiPackageList) + Offset;
    EfiCopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));

    if (PackageHeader.Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Search Class Opcode in this Form Package
      //
      Offset2 = sizeof (EFI_HII_PACKAGE_HEADER);
      while (Offset2 < PackageHeader.Length) {
        OpCodeData = Package + Offset2;

        if (((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode == EFI_IFR_FORM_SET_OP) {
          //
          // Find FormSet OpCode
          //
          EfiCopyMem (FormSetTitle, &((EFI_IFR_FORM_SET *) OpCodeData)->FormSetTitle, sizeof (EFI_STRING_ID));
          EfiCopyMem (FormSetHelp, &((EFI_IFR_FORM_SET *) OpCodeData)->Help, sizeof (EFI_STRING_ID));
        }

        if ((((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode == EFI_IFR_GUID_OP) &&
            (EfiCompareGuid (&mIfrVendorGuid, &((EFI_IFR_GUID *) OpCodeData)->Guid)) &&
            (((EFI_IFR_GUID_CLASS *) OpCodeData)->ExtendOpCode == EFI_IFR_EXTEND_OP_CLASS)
           ) {
          //
          // Find GUIDed Class OpCode
          //
          EfiCopyMem (Class, &((EFI_IFR_GUID_CLASS *) OpCodeData)->Class, sizeof (UINT16));

          //
          // Till now, we ought to have found the formset Opcode
          //
          ClassFound = TRUE;
          break;
        }

        Offset2 += ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
      }

      if (Offset2 < PackageHeader.Length) {
        //
        // Target formset found
        //
        break;
      }
    }

    Offset += PackageHeader.Length;
  }

  gBS->FreePool (HiiPackageList);

  return ClassFound ? EFI_SUCCESS : EFI_NOT_FOUND;
}

EFI_STATUS
ExtractClassGuidFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle,
  OUT     UINT8               *NumberOfClassGuid,
  OUT     EFI_GUID            **ClassGuid,
  OUT     EFI_STRING_ID       *FormSetTitle,
  OUT     EFI_STRING_ID       *FormSetHelp
  )
/*++

Routine Description:
  Extract formset ClassGuid for given HII handle.

Arguments:
  HiiHandle         - Hii handle
  NumberOfClassGuid - Number of ClassGuid
  ClassGuid         - Pointer to callee allocated buffer, an array of ClassGuid
  FormSetTitle      - Formset title string
  FormSetHelp       - Formset help string

Returns:
  EFI_SUCCESS     - Successfully extract Class for specified Hii handle.

--*/
{
  EFI_STATUS                   Status;
  UINTN                        BufferSize;
  EFI_HII_DATABASE_PROTOCOL    *HiiDatabase;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINT8                        *Package;
  UINT8                        *FormSet;
  UINT8                        *OpCodeData;
  UINT32                       Offset;
  UINT32                       Offset2;
  UINT32                       PackageListLength;
  EFI_HII_PACKAGE_HEADER       PackageHeader;

  if (NumberOfClassGuid == NULL || ClassGuid == NULL || FormSetTitle == NULL || FormSetHelp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *NumberOfClassGuid = 0;
  *ClassGuid = NULL;
  *FormSetTitle = 0;
  *FormSetHelp = 0;

  //
  // Locate HII Database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get HII PackageList
  //
  BufferSize = 0;
  HiiPackageList = NULL;
  Status = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = EfiLibAllocatePool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
  }
  if (EFI_ERROR (Status) || (HiiPackageList == NULL)) {
    return Status;
  }

  //
  // Get Form package from this HII package List
  //
  Offset = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  Offset2 = 0;
  FormSet = NULL;
  EfiCopyMem (&PackageListLength, &HiiPackageList->PackageLength, sizeof (UINT32));

  while (Offset < PackageListLength) {
    Package = ((UINT8 *) HiiPackageList) + Offset;
    EfiCopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));

    if (PackageHeader.Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Search Class Opcode in this Form Package
      //
      Offset2 = sizeof (EFI_HII_PACKAGE_HEADER);
      while (Offset2 < PackageHeader.Length) {
        OpCodeData = Package + Offset2;

        if (((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode == EFI_IFR_FORM_SET_OP) {
          //
          // Find FormSet OpCode
          //
          EfiCopyMem (FormSetTitle, &((EFI_IFR_FORM_SET *) OpCodeData)->FormSetTitle, sizeof (EFI_STRING_ID));
          EfiCopyMem (FormSetHelp, &((EFI_IFR_FORM_SET *) OpCodeData)->Help, sizeof (EFI_STRING_ID));
          if (((EFI_IFR_OP_HEADER *) OpCodeData)->Length > ((UINTN) &((EFI_IFR_FORM_SET *) 0)->Flags)) {
            //
            // New version of formset OpCode
            //
            *NumberOfClassGuid = (UINT8) (((EFI_IFR_FORM_SET *) OpCodeData)->Flags & 0x3);
            *ClassGuid = EfiLibAllocateCopyPool (
                           *NumberOfClassGuid * sizeof (EFI_GUID),
                           ((EFI_IFR_FORM_SET *) OpCodeData)->ClassGuid
                           );
          }
          break;
        }

        Offset2 += ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
      }

      if (Offset2 < PackageHeader.Length) {
        //
        // Target formset found
        //
        break;
      }
    }

    Offset += PackageHeader.Length;
  }

  gBS->FreePool (HiiPackageList);

  return EFI_SUCCESS;
}
