/** @file
  HII Library implementation that uses DXE protocols and services.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalHiiLib.h"

#include <Library/SafeIntLib.h>

#define GUID_CONFIG_STRING_TYPE  0x00
#define NAME_CONFIG_STRING_TYPE  0x01
#define PATH_CONFIG_STRING_TYPE  0x02

#define ACTION_SET_DEFAUTL_VALUE  0x01
#define ACTION_VALIDATE_SETTING   0x02

#define HII_LIB_DEFAULT_VARSTORE_SIZE  0x200

typedef struct {
  LIST_ENTRY    Entry;            // Link to Block array
  UINT16        Offset;
  UINT16        Width;
  UINT8         OpCode;
  UINT8         Scope;
} IFR_BLOCK_DATA;

typedef struct {
  EFI_VARSTORE_ID    VarStoreId;
  UINT16             Size;
} IFR_VARSTORAGE_DATA;

//
// <ConfigHdr> Template
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR16  mConfigHdrTemplate[] = L"GUID=00000000000000000000000000000000&NAME=0000&PATH=00";

EFI_FORM_BROWSER2_PROTOCOL  *mUefiFormBrowser2 = NULL;

//
// Template used to mark the end of a list of packages
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST EFI_HII_PACKAGE_HEADER  mEndOfPakageList = {
  sizeof (EFI_HII_PACKAGE_HEADER),
  EFI_HII_PACKAGE_END
};

/**
  Extract Hii package list GUID for given HII handle.

  If HiiHandle could not be found in the HII database, then ASSERT.
  If Guid is NULL, then ASSERT.

  @param  Handle                Hii handle
  @param  Guid                  Package list GUID

  @retval EFI_SUCCESS           Successfully extract GUID from Hii database.
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory resources to perform a necessary memory allocation.

**/
EFI_STATUS
EFIAPI
InternalHiiExtractGuidFromHiiHandle (
  IN      EFI_HII_HANDLE  Handle,
  OUT     EFI_GUID        *Guid
  )
{
  EFI_STATUS                   Status;
  UINTN                        BufferSize;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;

  if ((Handle == NULL) || (Guid == NULL)) {
    ASSERT (Guid != NULL);
    ASSERT (Handle != NULL);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get HII PackageList
  //
  BufferSize     = 0;
  HiiPackageList = NULL;

  Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, Handle, &BufferSize, HiiPackageList);
  ASSERT (Status != EFI_NOT_FOUND);

  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = AllocatePool (BufferSize);
    if (HiiPackageList == NULL) {
      ASSERT (HiiPackageList != NULL);
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, Handle, &BufferSize, HiiPackageList);
  }

  if (EFI_ERROR (Status)) {
    FreePool (HiiPackageList);
    return Status;
  }

  //
  // Extract GUID
  //
  if (HiiPackageList != NULL) {
    CopyGuid (Guid, &HiiPackageList->PackageListGuid);
  }

  FreePool (HiiPackageList);

  return EFI_SUCCESS;
}

/**
  Registers a list of packages in the HII Database and returns the HII Handle
  associated with that registration.  If an HII Handle has already been registered
  with the same PackageListGuid and DeviceHandle, then NULL is returned.  If there
  are not enough resources to perform the registration, then NULL is returned.
  If an empty list of packages is passed in, then NULL is returned.  If the size of
  the list of package is 0, then NULL is returned.

  The variable arguments are pointers which point to package header that defined
  by UEFI VFR compiler and StringGather tool.

  #pragma pack (push, 1)
  typedef struct {
    UINT32                  BinaryLength;
    EFI_HII_PACKAGE_HEADER  PackageHeader;
  } EDKII_AUTOGEN_PACKAGES_HEADER;
  #pragma pack (pop)

  @param[in]  PackageListGuid  The GUID of the package list.
  @param[in]  DeviceHandle     If not NULL, the Device Handle on which
                               an instance of DEVICE_PATH_PROTOCOL is installed.
                               This Device Handle uniquely defines the device that
                               the added packages are associated with.
  @param[in]  ...              The variable argument list that contains pointers
                               to packages terminated by a NULL.

  @retval NULL   A HII Handle has already been registered in the HII Database with
                 the same PackageListGuid and DeviceHandle.
  @retval NULL   The HII Handle could not be created.
  @retval NULL   An empty list of packages was passed in.
  @retval NULL   All packages are empty.
  @retval Other  The HII Handle associated with the newly registered package list.

**/
EFI_HII_HANDLE
EFIAPI
HiiAddPackages (
  IN CONST EFI_GUID    *PackageListGuid,
  IN       EFI_HANDLE  DeviceHandle  OPTIONAL,
  ...
  )
{
  EFI_STATUS                   Status;
  VA_LIST                      Args;
  UINT32                       *Package;
  EFI_HII_PACKAGE_LIST_HEADER  *PackageListHeader;
  EFI_HII_HANDLE               HiiHandle;
  UINT32                       Length;
  UINT8                        *Data;

  ASSERT (PackageListGuid != NULL);

  //
  // Calculate the length of all the packages in the variable argument list
  //
  for (Length = 0, VA_START (Args, DeviceHandle); (Package = VA_ARG (Args, UINT32 *)) != NULL; ) {
    Length += (ReadUnaligned32 (Package) - sizeof (UINT32));
  }

  VA_END (Args);

  //
  // If there are no packages in the variable argument list or all the packages
  // are empty, then return a NULL HII Handle
  //
  if (Length == 0) {
    return NULL;
  }

  //
  // Add the length of the Package List Header and the terminating Package Header
  //
  Length += sizeof (EFI_HII_PACKAGE_LIST_HEADER) + sizeof (EFI_HII_PACKAGE_HEADER);

  //
  // Allocate the storage for the entire Package List
  //
  PackageListHeader = AllocateZeroPool (Length);

  //
  // If the Package List can not be allocated, then return a NULL HII Handle
  //
  if (PackageListHeader == NULL) {
    return NULL;
  }

  //
  // Fill in the GUID and Length of the Package List Header
  //
  CopyGuid (&PackageListHeader->PackageListGuid, PackageListGuid);
  PackageListHeader->PackageLength = Length;

  //
  // Initialize a pointer to the beginning if the Package List data
  //
  Data = (UINT8 *)(PackageListHeader + 1);

  //
  // Copy the data from each package in the variable argument list
  //
  for (VA_START (Args, DeviceHandle); (Package = VA_ARG (Args, UINT32 *)) != NULL; ) {
    Length = ReadUnaligned32 (Package) - sizeof (UINT32);
    CopyMem (Data, Package + 1, Length);
    Data += Length;
  }

  VA_END (Args);

  //
  // Append a package of type EFI_HII_PACKAGE_END to mark the end of the package list
  //
  CopyMem (Data, &mEndOfPakageList, sizeof (mEndOfPakageList));

  //
  // Register the package list with the HII Database
  //
  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageListHeader,
                           DeviceHandle,
                           &HiiHandle
                           );
  if (EFI_ERROR (Status)) {
    HiiHandle = NULL;
  }

  //
  // Free the allocated package list
  //
  FreePool (PackageListHeader);

  //
  // Return the new HII Handle
  //
  return HiiHandle;
}

/**
  Removes a package list from the HII database.

  If HiiHandle is NULL, then ASSERT.
  If HiiHandle is not a valid EFI_HII_HANDLE in the HII database, then ASSERT.

  @param[in]  HiiHandle   The handle that was previously registered in the HII database

**/
VOID
EFIAPI
HiiRemovePackages (
  IN      EFI_HII_HANDLE  HiiHandle
  )
{
  EFI_STATUS  Status;

  ASSERT (HiiHandle != NULL);
  Status = gHiiDatabase->RemovePackageList (gHiiDatabase, HiiHandle);
  ASSERT_EFI_ERROR (Status);
}

/**
  Retrieves the array of all the HII Handles or the HII handles of a specific
  package list GUID in the HII Database.
  This array is terminated with a NULL HII Handle.
  This function allocates the returned array using AllocatePool().
  The caller is responsible for freeing the array with FreePool().

  @param[in]  PackageListGuid  An optional parameter that is used to request
                               HII Handles associated with a specific
                               Package List GUID.  If this parameter is NULL,
                               then all the HII Handles in the HII Database
                               are returned.  If this parameter is not NULL,
                               then zero or more HII Handles associated with
                               PackageListGuid are returned.

  @retval NULL   No HII handles were found in the HII database
  @retval NULL   The array of HII Handles could not be retrieved
  @retval Other  A pointer to the NULL terminated array of HII Handles

**/
EFI_HII_HANDLE *
EFIAPI
HiiGetHiiHandles (
  IN CONST EFI_GUID  *PackageListGuid  OPTIONAL
  )
{
  EFI_STATUS      Status;
  UINTN           HandleBufferLength;
  EFI_HII_HANDLE  TempHiiHandleBuffer;
  EFI_HII_HANDLE  *HiiHandleBuffer;
  EFI_GUID        Guid;
  UINTN           Index1;
  UINTN           Index2;

  //
  // Retrieve the size required for the buffer of all HII handles.
  //
  HandleBufferLength = 0;
  Status             = gHiiDatabase->ListPackageLists (
                                       gHiiDatabase,
                                       EFI_HII_PACKAGE_TYPE_ALL,
                                       NULL,
                                       &HandleBufferLength,
                                       &TempHiiHandleBuffer
                                       );

  //
  // If ListPackageLists() returns EFI_SUCCESS for a zero size,
  // then there are no HII handles in the HII database.  If ListPackageLists()
  // returns an error other than EFI_BUFFER_TOO_SMALL, then there are no HII
  // handles in the HII database.
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    //
    // Return NULL if the size can not be retrieved, or if there are no HII
    // handles in the HII Database
    //
    return NULL;
  }

  //
  // Allocate the array of HII handles to hold all the HII Handles and a NULL terminator
  //
  HiiHandleBuffer = AllocateZeroPool (HandleBufferLength + sizeof (EFI_HII_HANDLE));
  if (HiiHandleBuffer == NULL) {
    //
    // Return NULL if allocation fails.
    //
    return NULL;
  }

  //
  // Retrieve the array of HII Handles in the HII Database
  //
  Status = gHiiDatabase->ListPackageLists (
                           gHiiDatabase,
                           EFI_HII_PACKAGE_TYPE_ALL,
                           NULL,
                           &HandleBufferLength,
                           HiiHandleBuffer
                           );
  if (EFI_ERROR (Status)) {
    //
    // Free the buffer and return NULL if the HII handles can not be retrieved.
    //
    FreePool (HiiHandleBuffer);
    return NULL;
  }

  if (PackageListGuid == NULL) {
    //
    // Return the NULL terminated array of HII handles in the HII Database
    //
    return HiiHandleBuffer;
  } else {
    for (Index1 = 0, Index2 = 0; HiiHandleBuffer[Index1] != NULL; Index1++) {
      Status = InternalHiiExtractGuidFromHiiHandle (HiiHandleBuffer[Index1], &Guid);
      ASSERT_EFI_ERROR (Status);
      if (CompareGuid (&Guid, PackageListGuid)) {
        HiiHandleBuffer[Index2++] = HiiHandleBuffer[Index1];
      }
    }

    if (Index2 > 0) {
      HiiHandleBuffer[Index2] = NULL;
      return HiiHandleBuffer;
    } else {
      FreePool (HiiHandleBuffer);
      return NULL;
    }
  }
}

/**
  This function allows a caller to extract the form set opcode form the Hii Handle.
  The returned buffer is allocated using AllocatePool().The caller is responsible
  for freeing the allocated buffer using FreePool().

  @param Handle            The HII handle.
  @param Buffer            On return, points to a pointer which point to the buffer that contain the formset opcode.
  @param BufferSize        On return, points to the length of the buffer.

  @retval EFI_OUT_OF_RESOURCES   No enough memory resource is allocated.
  @retval EFI_NOT_FOUND          Can't find the package data for the input Handle.
  @retval EFI_INVALID_PARAMETER  The input parameters are not correct.
  @retval EFI_SUCCESS            Get the formset opcode from the hii handle successfully.

**/
EFI_STATUS
EFIAPI
HiiGetFormSetFromHiiHandle (
  IN  EFI_HII_HANDLE    Handle,
  OUT EFI_IFR_FORM_SET  **Buffer,
  OUT UINTN             *BufferSize
  )
{
  EFI_STATUS                   Status;
  UINTN                        PackageListSize;
  UINTN                        TempSize;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  UINT8                        *FormSetBuffer;
  UINT8                        *TempBuffer;
  UINT32                       Offset;
  UINT32                       Offset2;
  UINT32                       PackageListLength;
  EFI_HII_PACKAGE_HEADER       PackageHeader;

  TempSize      = 0;
  FormSetBuffer = NULL;
  TempBuffer    = NULL;

  //
  // Get HII PackageList
  //
  PackageListSize = 0;
  HiiPackageList  = NULL;
  Status          = gHiiDatabase->ExportPackageLists (gHiiDatabase, Handle, &PackageListSize, HiiPackageList);
  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    return Status;
  }

  HiiPackageList = AllocatePool (PackageListSize);
  if (HiiPackageList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, Handle, &PackageListSize, HiiPackageList);
  ASSERT_EFI_ERROR (Status);

  //
  // Get Form package from this HII package List
  //
  Status            = EFI_NOT_FOUND;
  Offset            = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  PackageListLength = ReadUnaligned32 (&HiiPackageList->PackageLength);

  while (Offset < PackageListLength) {
    Package = ((UINT8 *)HiiPackageList) + Offset;
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
    Offset += PackageHeader.Length;

    if (PackageHeader.Type != EFI_HII_PACKAGE_FORMS) {
      continue;
    }

    //
    // Search FormSet Opcode in this Form Package
    //
    Offset2 = sizeof (EFI_HII_PACKAGE_HEADER);
    while (Offset2 < PackageHeader.Length) {
      OpCodeData = Package + Offset2;
      Offset2   += ((EFI_IFR_OP_HEADER *)OpCodeData)->Length;

      if (((EFI_IFR_OP_HEADER *)OpCodeData)->OpCode != EFI_IFR_FORM_SET_OP) {
        continue;
      }

      if (FormSetBuffer != NULL) {
        TempBuffer = ReallocatePool (
                       TempSize,
                       TempSize + ((EFI_IFR_OP_HEADER *)OpCodeData)->Length,
                       FormSetBuffer
                       );
        if (TempBuffer == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }

        CopyMem (TempBuffer + TempSize, OpCodeData, ((EFI_IFR_OP_HEADER *)OpCodeData)->Length);
        FormSetBuffer = NULL;
      } else {
        TempBuffer = AllocatePool (TempSize + ((EFI_IFR_OP_HEADER *)OpCodeData)->Length);
        if (TempBuffer == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }

        CopyMem (TempBuffer, OpCodeData, ((EFI_IFR_OP_HEADER *)OpCodeData)->Length);
      }

      TempSize     += ((EFI_IFR_OP_HEADER *)OpCodeData)->Length;
      FormSetBuffer = TempBuffer;

      Status = EFI_SUCCESS;
      //
      // One form package has one formset, exit current form package to search other form package in the packagelist.
      //
      break;
    }
  }

Done:
  FreePool (HiiPackageList);

  *BufferSize = TempSize;
  *Buffer     = (EFI_IFR_FORM_SET *)FormSetBuffer;

  return Status;
}

/**
  Converts all hex dtring characters in range ['A'..'F'] to ['a'..'f'] for
  hex digits that appear between a '=' and a '&' in a config string.

  If ConfigString is NULL, then ASSERT().

  @param[in] ConfigString  Pointer to a Null-terminated Unicode string.

  @return  Pointer to the Null-terminated Unicode result string.

**/
EFI_STRING
EFIAPI
InternalHiiLowerConfigString (
  IN EFI_STRING  ConfigString
  )
{
  EFI_STRING  String;
  BOOLEAN     Lower;

  ASSERT (ConfigString != NULL);

  //
  // Convert all hex digits in range [A-F] in the configuration header to [a-f]
  //
  for (String = ConfigString, Lower = FALSE; *String != L'\0'; String++) {
    if (*String == L'=') {
      Lower = TRUE;
    } else if (*String == L'&') {
      Lower = FALSE;
    } else if (Lower && (*String >= L'A') && (*String <= L'F')) {
      *String = (CHAR16)(*String - L'A' + L'a');
    }
  }

  return ConfigString;
}

/**
  Uses the BlockToConfig() service of the Config Routing Protocol to
  convert <ConfigRequest> and a buffer to a <ConfigResp>

  If ConfigRequest is NULL, then ASSERT().
  If Block is NULL, then ASSERT().

  @param[in] ConfigRequest  Pointer to a Null-terminated Unicode string.
  @param[in] Block          Pointer to a block of data.
  @param[in] BlockSize      The zie, in bytes, of Block.

  @retval NULL   The <ConfigResp> string could not be generated.
  @retval Other  Pointer to the Null-terminated Unicode <ConfigResp> string.

**/
EFI_STRING
EFIAPI
InternalHiiBlockToConfig (
  IN CONST EFI_STRING  ConfigRequest,
  IN CONST UINT8       *Block,
  IN UINTN             BlockSize
  )
{
  EFI_STATUS  Status;
  EFI_STRING  ConfigResp;
  CHAR16      *Progress;

  ASSERT (ConfigRequest != NULL);
  ASSERT (Block != NULL);

  //
  // Convert <ConfigRequest> to <ConfigResp>
  //
  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                Block,
                                BlockSize,
                                &ConfigResp,
                                &Progress
                                );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return ConfigResp;
}

/**
  Uses the BrowserCallback() service of the Form Browser Protocol to retrieve
  or set uncommitted data.  If sata i being retrieved, then the buffer is
  allocated using AllocatePool().  The caller is then responsible for freeing
  the buffer using FreePool().

  @param[in]  VariableGuid    Pointer to an EFI_GUID structure.  This is an optional
                              parameter that may be NULL.
  @param[in]  VariableName    Pointer to a Null-terminated Unicode string.  This
                              is an optional parameter that may be NULL.
  @param[in]  SetResultsData  If not NULL, then this parameter specified the buffer
                              of uncommited data to set.  If this parameter is NULL,
                              then the caller is requesting to get the uncommited data
                              from the Form Browser.

  @retval NULL   The uncommitted data could not be retrieved.
  @retval Other  A pointer to a buffer containing the uncommitted data.

**/
EFI_STRING
EFIAPI
InternalHiiBrowserCallback (
  IN CONST EFI_GUID    *VariableGuid   OPTIONAL,
  IN CONST CHAR16      *VariableName   OPTIONAL,
  IN CONST EFI_STRING  SetResultsData  OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       ResultsDataSize;
  EFI_STRING  ResultsData;
  CHAR16      TempResultsData;

  //
  // Locate protocols
  //
  if (mUefiFormBrowser2 == NULL) {
    Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **)&mUefiFormBrowser2);
    if (EFI_ERROR (Status) || (mUefiFormBrowser2 == NULL)) {
      return NULL;
    }
  }

  ResultsDataSize = 0;

  if (SetResultsData != NULL) {
    //
    // Request to to set data in the uncommitted browser state information
    //
    ResultsData = SetResultsData;
  } else {
    //
    // Retrieve the length of the buffer required ResultsData from the Browser Callback
    //
    Status = mUefiFormBrowser2->BrowserCallback (
                                  mUefiFormBrowser2,
                                  &ResultsDataSize,
                                  &TempResultsData,
                                  TRUE,
                                  VariableGuid,
                                  VariableName
                                  );

    if (!EFI_ERROR (Status)) {
      //
      // No Resluts Data, only allocate one char for '\0'
      //
      ResultsData = AllocateZeroPool (sizeof (CHAR16));
      return ResultsData;
    }

    if (Status != EFI_BUFFER_TOO_SMALL) {
      return NULL;
    }

    //
    // Allocate the ResultsData buffer
    //
    ResultsData = AllocateZeroPool (ResultsDataSize);
    if (ResultsData == NULL) {
      return NULL;
    }
  }

  //
  // Retrieve or set the ResultsData from the Browser Callback
  //
  Status = mUefiFormBrowser2->BrowserCallback (
                                mUefiFormBrowser2,
                                &ResultsDataSize,
                                ResultsData,
                                (BOOLEAN)(SetResultsData == NULL),
                                VariableGuid,
                                VariableName
                                );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return ResultsData;
}

/**
  Allocates and returns a Null-terminated Unicode <ConfigHdr> string using routing
  information that includes a GUID, an optional Unicode string name, and a device
  path.  The string returned is allocated with AllocatePool().  The caller is
  responsible for freeing the allocated string with FreePool().

  The format of a <ConfigHdr> is as follows:

    GUID=<HexCh>32&NAME=<Char>NameLength&PATH=<HexChar>DevicePathSize<Null>

  @param[in]  Guid          Pointer to an EFI_GUID that is the routing information
                            GUID.  Each of the 16 bytes in Guid is converted to
                            a 2 Unicode character hexadecimal string.  This is
                            an optional parameter that may be NULL.
  @param[in]  Name          Pointer to a Null-terminated Unicode string that is
                            the routing information NAME.  This is an optional
                            parameter that may be NULL.  Each 16-bit Unicode
                            character in Name is converted to a 4 character Unicode
                            hexadecimal string.
  @param[in]  DriverHandle  The driver handle which supports a Device Path Protocol
                            that is the routing information PATH.  Each byte of
                            the Device Path associated with DriverHandle is converted
                            to a 2 Unicode character hexadecimal string.

  @retval NULL   DriverHandle does not support the Device Path Protocol.
  @retval Other  A pointer to the Null-terminate Unicode <ConfigHdr> string

**/
EFI_STRING
EFIAPI
HiiConstructConfigHdr (
  IN CONST EFI_GUID  *Guid   OPTIONAL,
  IN CONST CHAR16    *Name   OPTIONAL,
  IN EFI_HANDLE      DriverHandle
  )
{
  UINTN                     NameLength;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     DevicePathSize;
  CHAR16                    *String;
  CHAR16                    *ReturnString;
  UINTN                     Index;
  UINT8                     *Buffer;
  UINTN                     MaxLen;

  //
  // Compute the length of Name in Unicode characters.
  // If Name is NULL, then the length is 0.
  //
  NameLength = 0;
  if (Name != NULL) {
    NameLength = StrLen (Name);
  }

  DevicePath     = NULL;
  DevicePathSize = 0;
  //
  // Retrieve DevicePath Protocol associated with DriverHandle
  //
  if (DriverHandle != NULL) {
    DevicePath = DevicePathFromHandle (DriverHandle);
    if (DevicePath == NULL) {
      return NULL;
    }

    //
    // Compute the size of the device path in bytes
    //
    DevicePathSize = GetDevicePathSize (DevicePath);
  }

  //
  // GUID=<HexCh>32&NAME=<Char>NameLength&PATH=<HexChar>DevicePathSize <Null>
  // | 5 | sizeof (EFI_GUID) * 2 | 6 | NameStrLen*4 | 6 | DevicePathSize * 2 | 1 |
  //
  MaxLen = 5 + sizeof (EFI_GUID) * 2 + 6 + NameLength * 4 + 6 + DevicePathSize * 2 + 1;
  String = AllocateZeroPool (MaxLen * sizeof (CHAR16));
  if (String == NULL) {
    return NULL;
  }

  //
  // Start with L"GUID="
  //
  StrCpyS (String, MaxLen, L"GUID=");
  ReturnString = String;
  String      += StrLen (String);

  if (Guid != NULL) {
    //
    // Append Guid converted to <HexCh>32
    //
    for (Index = 0, Buffer = (UINT8 *)Guid; Index < sizeof (EFI_GUID); Index++) {
      UnicodeValueToStringS (
        String,
        MaxLen * sizeof (CHAR16) - ((UINTN)String - (UINTN)ReturnString),
        PREFIX_ZERO | RADIX_HEX,
        *(Buffer++),
        2
        );
      String += StrnLenS (String, MaxLen - ((UINTN)String - (UINTN)ReturnString) / sizeof (CHAR16));
    }
  }

  //
  // Append L"&NAME="
  //
  StrCatS (ReturnString, MaxLen, L"&NAME=");
  String += StrLen (String);

  if (Name != NULL) {
    //
    // Append Name converted to <Char>NameLength
    //
    for ( ; *Name != L'\0'; Name++) {
      UnicodeValueToStringS (
        String,
        sizeof (CHAR16) * MaxLen - ((UINTN)String - (UINTN)ReturnString),
        PREFIX_ZERO | RADIX_HEX,
        *Name,
        4
        );
      String += StrnLenS (String, MaxLen - ((UINTN)String - (UINTN)ReturnString) / sizeof (CHAR16));
    }
  }

  //
  // Append L"&PATH="
  //
  StrCatS (ReturnString, MaxLen, L"&PATH=");
  String += StrLen (String);

  //
  // Append the device path associated with DriverHandle converted to <HexChar>DevicePathSize
  //
  for (Index = 0, Buffer = (UINT8 *)DevicePath; Index < DevicePathSize; Index++) {
    UnicodeValueToStringS (
      String,
      sizeof (CHAR16) * MaxLen - ((UINTN)String - (UINTN)ReturnString),
      PREFIX_ZERO | RADIX_HEX,
      *(Buffer++),
      2
      );
    String += StrnLenS (String, MaxLen - ((UINTN)String - (UINTN)ReturnString) / sizeof (CHAR16));
  }

  //
  // Null terminate the Unicode string
  //
  *String = L'\0';

  //
  // Convert all hex digits in range [A-F] in the configuration header to [a-f]
  //
  return InternalHiiLowerConfigString (ReturnString);
}

/**
  Convert the hex UNICODE encoding string of UEFI GUID, NAME or device path
  to binary buffer from <ConfigHdr>.

  This is a internal function.

  @param  String                 UEFI configuration string.
  @param  Flag                   Flag specifies what type buffer will be retrieved.
  @param  Buffer                 Binary of Guid, Name or Device path.

  @retval EFI_INVALID_PARAMETER  Any incoming parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Lake of resources to store neccesary structures.
  @retval EFI_SUCCESS            The buffer data is retrieved and translated to
                                 binary format.

**/
EFI_STATUS
InternalHiiGetBufferFromString (
  IN  EFI_STRING  String,
  IN  UINT8       Flag,
  OUT UINT8       **Buffer
  )
{
  UINTN       Length;
  EFI_STRING  ConfigHdr;
  CHAR16      *StringPtr;
  UINT8       *DataBuffer;
  CHAR16      TemStr[5];
  UINTN       Index;
  UINT8       DigitUint8;

  if ((String == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DataBuffer = NULL;
  StringPtr  = NULL;
  ConfigHdr  = String;
  //
  // The content between 'GUID', 'NAME', 'PATH' of <ConfigHdr> and '&' of next element
  // or '\0' (end of configuration string) is the UNICODE %02x bytes encoding string.
  //
  for (Length = 0; *String != 0 && *String != L'&'; String++, Length++) {
  }

  switch (Flag) {
    case GUID_CONFIG_STRING_TYPE:
    case PATH_CONFIG_STRING_TYPE:
      //
      // The data in <ConfigHdr> is encoded as hex UNICODE %02x bytes in the same order
      // as the device path and Guid resides in RAM memory.
      // Translate the data into binary.
      //
      DataBuffer = (UINT8 *)AllocateZeroPool ((Length + 1) / 2);
      if (DataBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Convert binary byte one by one
      //
      ZeroMem (TemStr, sizeof (TemStr));
      for (Index = 0; Index < Length; Index++) {
        TemStr[0]  = ConfigHdr[Index];
        DigitUint8 = (UINT8)StrHexToUint64 (TemStr);
        if ((Index & 1) == 0) {
          DataBuffer[Index/2] = DigitUint8;
        } else {
          DataBuffer[Index/2] = (UINT8)((DataBuffer[Index/2] << 4) + DigitUint8);
        }
      }

      *Buffer = DataBuffer;
      break;

    case NAME_CONFIG_STRING_TYPE:
      //
      // Convert Config String to Unicode String, e.g. "0041004200430044" => "ABCD"
      //

      //
      // Add the tailling char L'\0'
      //
      DataBuffer = (UINT8 *)AllocateZeroPool ((Length/4 + 1) * sizeof (CHAR16));
      if (DataBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Convert character one by one
      //
      StringPtr = (CHAR16 *)DataBuffer;
      ZeroMem (TemStr, sizeof (TemStr));
      for (Index = 0; Index < Length; Index += 4) {
        StrnCpyS (TemStr, sizeof (TemStr) / sizeof (CHAR16), ConfigHdr + Index, 4);
        StringPtr[Index/4] = (CHAR16)StrHexToUint64 (TemStr);
      }

      //
      // Add tailing L'\0' character
      //
      StringPtr[Index/4] = L'\0';

      *Buffer = DataBuffer;
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  This function checks VarOffset and VarWidth is in the block range.

  @param  BlockArray         The block array is to be checked.
  @param  VarOffset          Offset of var to the structure
  @param  VarWidth           Width of var.

  @retval TRUE   This Var is in the block range.
  @retval FALSE  This Var is not in the block range.
**/
BOOLEAN
BlockArrayCheck (
  IN IFR_BLOCK_DATA  *BlockArray,
  IN UINT16          VarOffset,
  IN UINT16          VarWidth
  )
{
  LIST_ENTRY      *Link;
  IFR_BLOCK_DATA  *BlockData;

  //
  // No Request Block array, all vars are got.
  //
  if (BlockArray == NULL) {
    return TRUE;
  }

  //
  // Check the input var is in the request block range.
  //
  for (Link = BlockArray->Entry.ForwardLink; Link != &BlockArray->Entry; Link = Link->ForwardLink) {
    BlockData = BASE_CR (Link, IFR_BLOCK_DATA, Entry);
    if ((VarOffset >= BlockData->Offset) && ((VarOffset + VarWidth) <= (BlockData->Offset + BlockData->Width))) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Get the value of <Number> in <BlockConfig> format, i.e. the value of OFFSET
  or WIDTH or VALUE.
  <BlockConfig> ::= 'OFFSET='<Number>&'WIDTH='<Number>&'VALUE'=<Number>

  @param  ValueString            String in <BlockConfig> format and points to the
                                 first character of <Number>.
  @param  ValueData              The output value. Caller takes the responsibility
                                 to free memory.
  @param  ValueLength            Length of the <Number>, in characters.

  @retval EFI_OUT_OF_RESOURCES   Insufficient resources to store neccessary
                                 structures.
  @retval EFI_SUCCESS            Value of <Number> is outputted in Number
                                 successfully.

**/
EFI_STATUS
EFIAPI
InternalHiiGetValueOfNumber (
  IN  EFI_STRING  ValueString,
  OUT UINT8       **ValueData,
  OUT UINTN       *ValueLength
  )
{
  EFI_STRING  StringPtr;
  UINTN       Length;
  UINT8       *Buf;
  UINT8       DigitUint8;
  UINTN       Index;
  CHAR16      TemStr[2];

  ASSERT (ValueString != NULL && ValueData != NULL && ValueLength != NULL);
  ASSERT (*ValueString != L'\0');

  //
  // Get the length of value string
  //
  StringPtr = ValueString;
  while (*StringPtr != L'\0' && *StringPtr != L'&') {
    StringPtr++;
  }

  Length = StringPtr - ValueString;

  //
  // Allocate buffer to store the value
  //
  Buf = (UINT8 *)AllocateZeroPool ((Length + 1) / 2);
  if (Buf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Convert character one by one to the value buffer
  //
  ZeroMem (TemStr, sizeof (TemStr));
  for (Index = 0; Index < Length; Index++) {
    TemStr[0]  = ValueString[Length - Index - 1];
    DigitUint8 = (UINT8)StrHexToUint64 (TemStr);
    if ((Index & 1) == 0) {
      Buf[Index/2] = DigitUint8;
    } else {
      Buf[Index/2] = (UINT8)((DigitUint8 << 4) + Buf[Index/2]);
    }
  }

  //
  // Set the converted value and string length.
  //
  *ValueData   = Buf;
  *ValueLength = Length;
  return EFI_SUCCESS;
}

/**
  Get value from config request resp string.

  @param ConfigElement           ConfigResp string contains the current setting.
  @param VarName                 The variable name which need to get value.
  @param VarValue                The return value.

  @retval EFI_SUCCESS            Get the value for the VarName
  @retval EFI_OUT_OF_RESOURCES   The memory is not enough.
**/
EFI_STATUS
GetValueFromRequest (
  IN CHAR16   *ConfigElement,
  IN CHAR16   *VarName,
  OUT UINT64  *VarValue
  )
{
  UINT8       *TmpBuffer;
  CHAR16      *StringPtr;
  UINTN       Length;
  EFI_STATUS  Status;

  //
  // Find VarName related string.
  //
  StringPtr = StrStr (ConfigElement, VarName);
  ASSERT (StringPtr != NULL);

  //
  // Skip the "VarName=" string
  //
  StringPtr += StrLen (VarName) + 1;

  //
  // Get Offset
  //
  Status = InternalHiiGetValueOfNumber (StringPtr, &TmpBuffer, &Length);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *VarValue = 0;
  CopyMem (VarValue, TmpBuffer, (((Length + 1) / 2) < sizeof (UINT64)) ? ((Length + 1) / 2) : sizeof (UINT64));

  FreePool (TmpBuffer);

  return EFI_SUCCESS;
}

/**
  This internal function parses IFR data to validate current setting.

  Base on the NameValueType, if it is TRUE, RequestElement and HiiHandle is valid;
  else the VarBuffer and CurrentBlockArray is valid.

  @param HiiPackageList     Point to Hii package list.
  @param PackageListLength  The length of the pacakge.
  @param VarGuid            Guid of the buffer storage.
  @param VarName            Name of the buffer storage.
  @param VarBuffer          The data buffer for the storage.
  @param CurrentBlockArray  The block array from the config Requst string.
  @param RequestElement     The config string for this storage.
  @param HiiHandle          The HiiHandle for this formset.
  @param NameValueType      Whether current storage is name/value varstore or not.

  @retval EFI_SUCCESS            The current setting is valid.
  @retval EFI_OUT_OF_RESOURCES   The memory is not enough.
  @retval EFI_INVALID_PARAMETER  The config string or the Hii package is invalid.
**/
EFI_STATUS
ValidateQuestionFromVfr (
  IN EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList,
  IN UINTN                        PackageListLength,
  IN EFI_GUID                     *VarGuid,
  IN CHAR16                       *VarName,
  IN UINT8                        *VarBuffer,
  IN IFR_BLOCK_DATA               *CurrentBlockArray,
  IN CHAR16                       *RequestElement,
  IN EFI_HII_HANDLE               HiiHandle,
  IN BOOLEAN                      NameValueType
  )
{
  IFR_BLOCK_DATA               VarBlockData;
  UINT16                       Offset;
  UINT16                       Width;
  UINT64                       VarValue;
  EFI_IFR_TYPE_VALUE           TmpValue;
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  UINT32                       PackageOffset;
  UINT8                        *PackageData;
  UINTN                        IfrOffset;
  EFI_IFR_OP_HEADER            *IfrOpHdr;
  EFI_IFR_VARSTORE             *IfrVarStore;
  EFI_IFR_VARSTORE_NAME_VALUE  *IfrNameValueStore;
  EFI_IFR_VARSTORE_EFI         *IfrEfiVarStore;
  IFR_VARSTORAGE_DATA          VarStoreData;
  EFI_IFR_ONE_OF               *IfrOneOf;
  EFI_IFR_NUMERIC              *IfrNumeric;
  EFI_IFR_ONE_OF_OPTION        *IfrOneOfOption;
  EFI_IFR_CHECKBOX             *IfrCheckBox;
  EFI_IFR_STRING               *IfrString;
  CHAR8                        *VarStoreName;
  UINTN                        Index;
  CHAR16                       *QuestionName;
  CHAR16                       *StringPtr;
  UINT16                       BitOffset;
  UINT16                       BitWidth;
  UINT16                       TotalBits;
  UINTN                        StartBit;
  UINTN                        EndBit;
  BOOLEAN                      QuestionReferBitField;
  UINT32                       BufferValue;

  //
  // Initialize the local variables.
  //
  Index             = 0;
  VarStoreName      = NULL;
  Status            = EFI_SUCCESS;
  VarValue          = 0;
  IfrVarStore       = NULL;
  IfrNameValueStore = NULL;
  IfrEfiVarStore    = NULL;
  ZeroMem (&VarStoreData, sizeof (IFR_VARSTORAGE_DATA));
  ZeroMem (&VarBlockData, sizeof (VarBlockData));
  BitOffset             = 0;
  BitWidth              = 0;
  QuestionReferBitField = FALSE;

  //
  // Check IFR value is in block data, then Validate Value
  //
  PackageOffset = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  while ((UINTN)PackageOffset < PackageListLength) {
    CopyMem (&PackageHeader, (UINT8 *)HiiPackageList + PackageOffset, sizeof (PackageHeader));

    //
    // Parse IFR opcode from the form package.
    //
    if (PackageHeader.Type == EFI_HII_PACKAGE_FORMS) {
      IfrOffset   = sizeof (PackageHeader);
      PackageData = (UINT8 *)HiiPackageList + PackageOffset;
      while (IfrOffset < PackageHeader.Length) {
        IfrOpHdr = (EFI_IFR_OP_HEADER *)(PackageData + IfrOffset);
        //
        // Validate current setting to the value built in IFR opcode
        //
        switch (IfrOpHdr->OpCode) {
          case EFI_IFR_VARSTORE_OP:
            //
            // VarStoreId has been found. No further found.
            //
            if (VarStoreData.VarStoreId != 0) {
              break;
            }

            //
            // Find the matched VarStoreId to the input VarGuid and VarName
            //
            IfrVarStore = (EFI_IFR_VARSTORE *)IfrOpHdr;
            if (CompareGuid ((EFI_GUID *)(VOID *)&IfrVarStore->Guid, VarGuid)) {
              VarStoreName = (CHAR8 *)IfrVarStore->Name;
              for (Index = 0; VarStoreName[Index] != 0; Index++) {
                if ((CHAR16)VarStoreName[Index] != VarName[Index]) {
                  break;
                }
              }

              //
              // The matched VarStore is found.
              //
              if ((VarStoreName[Index] != 0) || (VarName[Index] != 0)) {
                IfrVarStore = NULL;
              }
            } else {
              IfrVarStore = NULL;
            }

            if (IfrVarStore != NULL) {
              VarStoreData.VarStoreId = IfrVarStore->VarStoreId;
              VarStoreData.Size       = IfrVarStore->Size;
            }

            break;
          case EFI_IFR_VARSTORE_NAME_VALUE_OP:
            //
            // VarStoreId has been found. No further found.
            //
            if (VarStoreData.VarStoreId != 0) {
              break;
            }

            //
            // Find the matched VarStoreId to the input VarGuid
            //
            IfrNameValueStore = (EFI_IFR_VARSTORE_NAME_VALUE *)IfrOpHdr;
            if (!CompareGuid ((EFI_GUID *)(VOID *)&IfrNameValueStore->Guid, VarGuid)) {
              IfrNameValueStore = NULL;
            }

            if (IfrNameValueStore != NULL) {
              VarStoreData.VarStoreId = IfrNameValueStore->VarStoreId;
            }

            break;
          case EFI_IFR_VARSTORE_EFI_OP:
            //
            // VarStore is found. Don't need to search any more.
            //
            if (VarStoreData.VarStoreId != 0) {
              break;
            }

            IfrEfiVarStore = (EFI_IFR_VARSTORE_EFI *)IfrOpHdr;

            //
            // If the length is small than the structure, this is from old efi
            // varstore definition. Old efi varstore get config directly from
            // GetVariable function.
            //
            if (IfrOpHdr->Length < sizeof (EFI_IFR_VARSTORE_EFI)) {
              break;
            }

            if (CompareGuid ((EFI_GUID *)(VOID *)&IfrEfiVarStore->Guid, VarGuid)) {
              VarStoreName = (CHAR8 *)IfrEfiVarStore->Name;
              for (Index = 0; VarStoreName[Index] != 0; Index++) {
                if ((CHAR16)VarStoreName[Index] != VarName[Index]) {
                  break;
                }
              }

              //
              // The matched VarStore is found.
              //
              if ((VarStoreName[Index] != 0) || (VarName[Index] != 0)) {
                IfrEfiVarStore = NULL;
              }
            } else {
              IfrEfiVarStore = NULL;
            }

            if (IfrEfiVarStore != NULL) {
              //
              // Find the matched VarStore
              //
              VarStoreData.VarStoreId = IfrEfiVarStore->VarStoreId;
              VarStoreData.Size       = IfrEfiVarStore->Size;
            }

            break;
          case EFI_IFR_FORM_OP:
          case EFI_IFR_FORM_MAP_OP:
            //
            // Check the matched VarStoreId is found.
            //
            if (VarStoreData.VarStoreId == 0) {
              return EFI_SUCCESS;
            }

            break;
          case EFI_IFR_ONE_OF_OP:
            //
            // Check whether current value is the one of option.
            //

            //
            // OneOf question is not in IFR Form. This IFR form is not valid.
            //
            if (VarStoreData.VarStoreId == 0) {
              return EFI_INVALID_PARAMETER;
            }

            //
            // Check whether this question is for the requested varstore.
            //
            IfrOneOf = (EFI_IFR_ONE_OF *)IfrOpHdr;
            if (IfrOneOf->Question.VarStoreId != VarStoreData.VarStoreId) {
              break;
            }

            if (NameValueType) {
              QuestionName = HiiGetString (HiiHandle, IfrOneOf->Question.VarStoreInfo.VarName, NULL);
              if (QuestionName == NULL) {
                ASSERT (QuestionName != NULL);
                return EFI_INVALID_PARAMETER;
              }

              if (StrStr (RequestElement, QuestionName) == NULL) {
                //
                // This question is not in the current configuration string. Skip it.
                //
                break;
              }

              Status = GetValueFromRequest (RequestElement, QuestionName, &VarValue);
              if (EFI_ERROR (Status)) {
                return Status;
              }
            } else {
              //
              // Get Offset by Question header and Width by DataType Flags
              //
              if (QuestionReferBitField) {
                //
                // Get the byte offset/width for bit field.
                //
                BitOffset = IfrOneOf->Question.VarStoreInfo.VarOffset;
                BitWidth  = IfrOneOf->Flags & EDKII_IFR_NUMERIC_SIZE_BIT;
                Offset    = BitOffset / 8;
                TotalBits = BitOffset % 8 + BitWidth;
                Width     = (TotalBits % 8 == 0 ? TotalBits / 8 : TotalBits / 8 + 1);
              } else {
                Offset = IfrOneOf->Question.VarStoreInfo.VarOffset;
                Width  = (UINT16)(1 << (IfrOneOf->Flags & EFI_IFR_NUMERIC_SIZE));
              }

              //
              // Check whether this question is in current block array.
              //
              if (!BlockArrayCheck (CurrentBlockArray, Offset, Width)) {
                //
                // This question is not in the current configuration string. Skip it.
                //
                break;
              }

              //
              // Check this var question is in the var storage
              //
              if ((Offset + Width) > VarStoreData.Size) {
                //
                // This question exceeds the var store size.
                //
                return EFI_INVALID_PARAMETER;
              }

              //
              // Get the current value for oneof opcode
              //
              VarValue = 0;
              if (QuestionReferBitField) {
                //
                // Get the value in bit fields.
                //
                StartBit = BitOffset % 8;
                EndBit   = StartBit + BitWidth - 1;
                CopyMem ((UINT8 *)&BufferValue, VarBuffer + Offset, Width);
                VarValue = BitFieldRead32 (BufferValue, StartBit, EndBit);
              } else {
                CopyMem (&VarValue, VarBuffer +  Offset, Width);
              }
            }

            //
            // Set Block Data, to be checked in the following Oneof option opcode.
            //
            VarBlockData.OpCode = IfrOpHdr->OpCode;
            VarBlockData.Scope  = IfrOpHdr->Scope;
            break;
          case EFI_IFR_NUMERIC_OP:
            //
            // Check the current value is in the numeric range.
            //

            //
            // Numeric question is not in IFR Form. This IFR form is not valid.
            //
            if (VarStoreData.VarStoreId == 0) {
              return EFI_INVALID_PARAMETER;
            }

            //
            // Check whether this question is for the requested varstore.
            //
            IfrNumeric = (EFI_IFR_NUMERIC *)IfrOpHdr;
            if (IfrNumeric->Question.VarStoreId != VarStoreData.VarStoreId) {
              break;
            }

            if (NameValueType) {
              QuestionName = HiiGetString (HiiHandle, IfrNumeric->Question.VarStoreInfo.VarName, NULL);
              if (QuestionName == NULL) {
                ASSERT (QuestionName != NULL);
                return EFI_INVALID_PARAMETER;
              }

              if (StrStr (RequestElement, QuestionName) == NULL) {
                //
                // This question is not in the current configuration string. Skip it.
                //
                break;
              }

              Status = GetValueFromRequest (RequestElement, QuestionName, &VarValue);
              if (EFI_ERROR (Status)) {
                return Status;
              }
            } else {
              //
              // Get Offset by Question header and Width by DataType Flags
              //
              if (QuestionReferBitField) {
                //
                // Get the byte offset/width for bit field.
                //
                BitOffset = IfrNumeric->Question.VarStoreInfo.VarOffset;
                BitWidth  = IfrNumeric->Flags & EDKII_IFR_NUMERIC_SIZE_BIT;
                Offset    = BitOffset / 8;
                TotalBits = BitOffset % 8 + BitWidth;
                Width     = (TotalBits % 8 == 0 ? TotalBits / 8 : TotalBits / 8 + 1);
              } else {
                Offset = IfrNumeric->Question.VarStoreInfo.VarOffset;
                Width  = (UINT16)(1 << (IfrNumeric->Flags & EFI_IFR_NUMERIC_SIZE));
              }

              //
              // Check whether this question is in current block array.
              //
              if (!BlockArrayCheck (CurrentBlockArray, Offset, Width)) {
                //
                // This question is not in the current configuration string. Skip it.
                //
                break;
              }

              //
              // Check this var question is in the var storage
              //
              if ((Offset + Width) > VarStoreData.Size) {
                //
                // This question exceeds the var store size.
                //
                return EFI_INVALID_PARAMETER;
              }

              //
              // Check the current value is in the numeric range.
              //
              VarValue = 0;
              if (QuestionReferBitField) {
                //
                // Get the value in the bit fields.
                //
                StartBit = BitOffset % 8;
                EndBit   = StartBit + BitWidth - 1;
                CopyMem ((UINT8 *)&BufferValue, VarBuffer + Offset, Width);
                VarValue = BitFieldRead32 (BufferValue, StartBit, EndBit);
              } else {
                CopyMem (&VarValue, VarBuffer +  Offset, Width);
              }
            }

            if ( QuestionReferBitField) {
              //
              // Value in bit fields was stored as UINt32 type.
              //
              if ((IfrNumeric->Flags & EDKII_IFR_DISPLAY_BIT) == 0) {
                if (((INT32)VarValue < (INT32)IfrNumeric->data.u32.MinValue) || ((INT32)VarValue > (INT32)IfrNumeric->data.u32.MaxValue)) {
                  //
                  // Not in the valid range.
                  //
                  return EFI_INVALID_PARAMETER;
                }
              } else {
                if ((VarValue < IfrNumeric->data.u32.MinValue) || (VarValue > IfrNumeric->data.u32.MaxValue)) {
                  //
                  // Not in the valid range.
                  //
                  return EFI_INVALID_PARAMETER;
                }
              }
            } else {
              if ((IfrNumeric->Flags & EFI_IFR_DISPLAY) == 0) {
                switch (IfrNumeric->Flags & EFI_IFR_NUMERIC_SIZE) {
                  case EFI_IFR_NUMERIC_SIZE_1:
                    if (((INT8)VarValue < (INT8)IfrNumeric->data.u8.MinValue) || ((INT8)VarValue > (INT8)IfrNumeric->data.u8.MaxValue)) {
                      //
                      // Not in the valid range.
                      //
                      return EFI_INVALID_PARAMETER;
                    }

                    break;
                  case EFI_IFR_NUMERIC_SIZE_2:
                    if (((INT16)VarValue < (INT16)IfrNumeric->data.u16.MinValue) || ((INT16)VarValue > (INT16)IfrNumeric->data.u16.MaxValue)) {
                      //
                      // Not in the valid range.
                      //
                      return EFI_INVALID_PARAMETER;
                    }

                    break;
                  case EFI_IFR_NUMERIC_SIZE_4:
                    if (((INT32)VarValue < (INT32)IfrNumeric->data.u32.MinValue) || ((INT32)VarValue > (INT32)IfrNumeric->data.u32.MaxValue)) {
                      //
                      // Not in the valid range.
                      //
                      return EFI_INVALID_PARAMETER;
                    }

                    break;
                  case EFI_IFR_NUMERIC_SIZE_8:
                    if (((INT64)VarValue < (INT64)IfrNumeric->data.u64.MinValue) || ((INT64)VarValue > (INT64)IfrNumeric->data.u64.MaxValue)) {
                      //
                      // Not in the valid range.
                      //
                      return EFI_INVALID_PARAMETER;
                    }

                    break;
                }
              } else {
                switch (IfrNumeric->Flags & EFI_IFR_NUMERIC_SIZE) {
                  case EFI_IFR_NUMERIC_SIZE_1:
                    if (((UINT8)VarValue < IfrNumeric->data.u8.MinValue) || ((UINT8)VarValue > IfrNumeric->data.u8.MaxValue)) {
                      //
                      // Not in the valid range.
                      //
                      return EFI_INVALID_PARAMETER;
                    }

                    break;
                  case EFI_IFR_NUMERIC_SIZE_2:
                    if (((UINT16)VarValue < IfrNumeric->data.u16.MinValue) || ((UINT16)VarValue > IfrNumeric->data.u16.MaxValue)) {
                      //
                      // Not in the valid range.
                      //
                      return EFI_INVALID_PARAMETER;
                    }

                    break;
                  case EFI_IFR_NUMERIC_SIZE_4:
                    if (((UINT32)VarValue < IfrNumeric->data.u32.MinValue) || ((UINT32)VarValue > IfrNumeric->data.u32.MaxValue)) {
                      //
                      // Not in the valid range.
                      //
                      return EFI_INVALID_PARAMETER;
                    }

                    break;
                  case EFI_IFR_NUMERIC_SIZE_8:
                    if (((UINT64)VarValue < IfrNumeric->data.u64.MinValue) || ((UINT64)VarValue > IfrNumeric->data.u64.MaxValue)) {
                      //
                      // Not in the valid range.
                      //
                      return EFI_INVALID_PARAMETER;
                    }

                    break;
                }
              }
            }

            break;
          case EFI_IFR_CHECKBOX_OP:
            //
            // Check value is BOOLEAN type, only 0 and 1 is valid.
            //

            //
            // CheckBox question is not in IFR Form. This IFR form is not valid.
            //
            if (VarStoreData.VarStoreId == 0) {
              return EFI_INVALID_PARAMETER;
            }

            //
            // Check whether this question is for the requested varstore.
            //
            IfrCheckBox = (EFI_IFR_CHECKBOX *)IfrOpHdr;
            if (IfrCheckBox->Question.VarStoreId != VarStoreData.VarStoreId) {
              break;
            }

            if (NameValueType) {
              QuestionName = HiiGetString (HiiHandle, IfrCheckBox->Question.VarStoreInfo.VarName, NULL);
              if (QuestionName == NULL) {
                ASSERT (QuestionName != NULL);
                return EFI_INVALID_PARAMETER;
              }

              if (StrStr (RequestElement, QuestionName) == NULL) {
                //
                // This question is not in the current configuration string. Skip it.
                //
                break;
              }

              Status = GetValueFromRequest (RequestElement, QuestionName, &VarValue);
              if (EFI_ERROR (Status)) {
                return Status;
              }
            } else {
              //
              // Get Offset by Question header
              //
              if (QuestionReferBitField) {
                //
                // Get the byte offset/width for bit field.
                //
                BitOffset = IfrCheckBox->Question.VarStoreInfo.VarOffset;
                BitWidth  = 1;
                Offset    = BitOffset / 8;
                TotalBits = BitOffset % 8 + BitWidth;
                Width     = (TotalBits % 8 == 0 ? TotalBits / 8 : TotalBits / 8 + 1);
              } else {
                Offset = IfrCheckBox->Question.VarStoreInfo.VarOffset;
                Width  = (UINT16)sizeof (BOOLEAN);
              }

              //
              // Check whether this question is in current block array.
              //
              if (!BlockArrayCheck (CurrentBlockArray, Offset, Width)) {
                //
                // This question is not in the current configuration string. Skip it.
                //
                break;
              }

              //
              // Check this var question is in the var storage
              //
              if ((Offset + Width) > VarStoreData.Size) {
                //
                // This question exceeds the var store size.
                //
                return EFI_INVALID_PARAMETER;
              }

              //
              // Check the current value is in the numeric range.
              //
              VarValue = 0;
              if (QuestionReferBitField) {
                //
                // Get the value in bit fields.
                //
                StartBit = BitOffset % 8;
                EndBit   = StartBit + BitWidth - 1;
                CopyMem ((UINT8 *)&BufferValue, VarBuffer + Offset, Width);
                VarValue = BitFieldRead32 (BufferValue, StartBit, EndBit);
              } else {
                CopyMem (&VarValue, VarBuffer +  Offset, Width);
              }
            }

            //
            // Boolean type, only 1 and 0 is valid.
            //
            if (VarValue > 1) {
              return EFI_INVALID_PARAMETER;
            }

            break;
          case EFI_IFR_STRING_OP:
            //
            // Check current string length is less than maxsize
            //

            //
            // CheckBox question is not in IFR Form. This IFR form is not valid.
            //
            if (VarStoreData.VarStoreId == 0) {
              return EFI_INVALID_PARAMETER;
            }

            //
            // Check whether this question is for the requested varstore.
            //
            IfrString = (EFI_IFR_STRING *)IfrOpHdr;
            if (IfrString->Question.VarStoreId != VarStoreData.VarStoreId) {
              break;
            }

            //
            // Get the Max size of the string.
            //
            Width = (UINT16)(IfrString->MaxSize * sizeof (UINT16));
            if (NameValueType) {
              QuestionName = HiiGetString (HiiHandle, IfrString->Question.VarStoreInfo.VarName, NULL);
              if (QuestionName == NULL) {
                ASSERT (QuestionName != NULL);
                return EFI_INVALID_PARAMETER;
              }

              StringPtr = StrStr (RequestElement, QuestionName);
              if (StringPtr == NULL) {
                //
                // This question is not in the current configuration string. Skip it.
                //
                break;
              }

              //
              // Skip the VarName.
              //
              StringPtr += StrLen (QuestionName);

              //
              // Skip the "=".
              //
              StringPtr += 1;

              //
              // Check current string length is less than maxsize
              // e.g Config String: "0041004200430044", Unicode String: "ABCD". Unicode String length = Config String length / 4.
              // Config string format in UEFI spec.
              // <NvConfig> ::= <Label>'='<String>
              // <String> ::= [<Char>]+
              // <Char> ::= <HexCh>4
              //
              if (StrLen (StringPtr) / 4 > IfrString->MaxSize) {
                return EFI_INVALID_PARAMETER;
              }
            } else {
              //
              // Get Offset/Width by Question header and OneOf Flags
              //
              Offset = IfrString->Question.VarStoreInfo.VarOffset;
              //
              // Check whether this question is in current block array.
              //
              if (!BlockArrayCheck (CurrentBlockArray, Offset, Width)) {
                //
                // This question is not in the current configuration string. Skip it.
                //
                break;
              }

              //
              // Check this var question is in the var storage
              //
              if ((Offset + Width) > VarStoreData.Size) {
                //
                // This question exceeds the var store size.
                //
                return EFI_INVALID_PARAMETER;
              }

              //
              // Check current string length is less than maxsize
              //
              if (StrLen ((CHAR16 *)(VarBuffer + Offset)) > IfrString->MaxSize) {
                return EFI_INVALID_PARAMETER;
              }
            }

            break;
          case EFI_IFR_ONE_OF_OPTION_OP:
            //
            // Opcode Scope is zero. This one of option is not to be checked.
            //
            if (VarBlockData.Scope == 0) {
              break;
            }

            //
            // Only check for OneOf and OrderList opcode
            //
            IfrOneOfOption = (EFI_IFR_ONE_OF_OPTION *)IfrOpHdr;
            if (VarBlockData.OpCode == EFI_IFR_ONE_OF_OP) {
              //
              // Check current value is the value of one of option.
              //
              ASSERT (IfrOneOfOption->Type <= EFI_IFR_TYPE_NUM_SIZE_64);
              ZeroMem (&TmpValue, sizeof (EFI_IFR_TYPE_VALUE));
              CopyMem (&TmpValue, &IfrOneOfOption->Value, IfrOneOfOption->Header.Length - OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value));
              if (VarValue == TmpValue.u64) {
                //
                // The value is one of option value.
                // Set OpCode to Zero, don't need check again.
                //
                VarBlockData.OpCode = 0;
              }
            }

            break;
          case EFI_IFR_END_OP:
            QuestionReferBitField = FALSE;
            //
            // Decrease opcode scope for the validated opcode
            //
            if (VarBlockData.Scope > 0) {
              VarBlockData.Scope--;
            }

            //
            // OneOf value doesn't belong to one of option value.
            //
            if ((VarBlockData.Scope == 0) && (VarBlockData.OpCode == EFI_IFR_ONE_OF_OP)) {
              return EFI_INVALID_PARAMETER;
            }

            break;
          case EFI_IFR_GUID_OP:
            if (CompareGuid ((EFI_GUID *)((UINT8 *)IfrOpHdr + sizeof (EFI_IFR_OP_HEADER)), &gEdkiiIfrBitVarstoreGuid)) {
              QuestionReferBitField = TRUE;
            }

            break;
          default:
            //
            // Increase Scope for the validated opcode
            //
            if (VarBlockData.Scope > 0) {
              VarBlockData.Scope = (UINT8)(VarBlockData.Scope + IfrOpHdr->Scope);
            }

            break;
        }

        //
        // Go to the next opcode
        //
        IfrOffset += IfrOpHdr->Length;
      }

      //
      // Only one form is in a package list.
      //
      break;
    }

    //
    // Go to next package.
    //
    PackageOffset += PackageHeader.Length;
  }

  return EFI_SUCCESS;
}

/**
  This internal function parses IFR data to validate current setting.

  @param ConfigElement         ConfigResp element string contains the current setting.
  @param CurrentBlockArray     Current block array.
  @param VarBuffer             Data buffer for this varstore.

  @retval EFI_SUCCESS            The current setting is valid.
  @retval EFI_OUT_OF_RESOURCES   The memory is not enough.
  @retval EFI_INVALID_PARAMETER  The config string or the Hii package is invalid.
**/
EFI_STATUS
GetBlockDataInfo (
  IN  CHAR16          *ConfigElement,
  OUT IFR_BLOCK_DATA  **CurrentBlockArray,
  OUT UINT8           **VarBuffer
  )
{
  IFR_BLOCK_DATA  *BlockData;
  IFR_BLOCK_DATA  *NewBlockData;
  EFI_STRING      StringPtr;
  UINTN           Length;
  UINT8           *TmpBuffer;
  UINT16          Offset;
  UINT16          Width;
  LIST_ENTRY      *Link;
  UINTN           MaxBufferSize;
  EFI_STATUS      Status;
  IFR_BLOCK_DATA  *BlockArray;
  UINT8           *DataBuffer;
  UINT16          Sum1;
  UINT16          Sum2;

  //
  // Initialize the local variables.
  //
  Status        = EFI_SUCCESS;
  BlockData     = NULL;
  NewBlockData  = NULL;
  TmpBuffer     = NULL;
  BlockArray    = NULL;
  MaxBufferSize = HII_LIB_DEFAULT_VARSTORE_SIZE;
  DataBuffer    = AllocateZeroPool (MaxBufferSize);
  if (DataBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Init BlockArray
  //
  BlockArray = (IFR_BLOCK_DATA *)AllocateZeroPool (sizeof (IFR_BLOCK_DATA));
  if (BlockArray == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  StringPtr = StrStr (ConfigElement, L"&OFFSET=");
  if (StringPtr == NULL) {
    ASSERT (StringPtr != NULL);
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  InitializeListHead (&BlockArray->Entry);

  //
  // Parse each <RequestElement> if exists
  // Only <BlockName> format is supported by this help function.
  // <BlockName> ::= &'OFFSET='<Number>&'WIDTH='<Number>
  //
  while (*StringPtr != 0 && StrnCmp (StringPtr, L"&OFFSET=", StrLen (L"&OFFSET=")) == 0) {
    //
    // Skip the &OFFSET= string
    //
    StringPtr += StrLen (L"&OFFSET=");

    //
    // Get Offset
    //
    Status = InternalHiiGetValueOfNumber (StringPtr, &TmpBuffer, &Length);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Offset = 0;
    CopyMem (
      &Offset,
      TmpBuffer,
      (((Length + 1) / 2) < sizeof (UINT16)) ? ((Length + 1) / 2) : sizeof (UINT16)
      );
    FreePool (TmpBuffer);
    TmpBuffer = NULL;

    StringPtr += Length;
    if (StrnCmp (StringPtr, L"&WIDTH=", StrLen (L"&WIDTH=")) != 0) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    StringPtr += StrLen (L"&WIDTH=");

    //
    // Get Width
    //
    Status = InternalHiiGetValueOfNumber (StringPtr, &TmpBuffer, &Length);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Width = 0;
    CopyMem (
      &Width,
      TmpBuffer,
      (((Length + 1) / 2) < sizeof (UINT16)) ? ((Length + 1) / 2) : sizeof (UINT16)
      );
    FreePool (TmpBuffer);
    TmpBuffer = NULL;

    StringPtr += Length;
    if ((*StringPtr != 0) && (*StringPtr != L'&')) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    if (StrnCmp (StringPtr, L"&VALUE=", StrLen (L"&VALUE=")) != 0) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    StringPtr += StrLen (L"&VALUE=");

    //
    // Get Value
    //
    Status = InternalHiiGetValueOfNumber (StringPtr, &TmpBuffer, &Length);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    StringPtr += Length;
    if ((*StringPtr != 0) && (*StringPtr != L'&')) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    //
    // Check whether VarBuffer is enough
    //
    if ((UINT32)Offset + Width > MaxBufferSize) {
      DataBuffer = ReallocatePool (
                     MaxBufferSize,
                     Offset + Width + HII_LIB_DEFAULT_VARSTORE_SIZE,
                     DataBuffer
                     );
      if (DataBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      MaxBufferSize = Offset + Width + HII_LIB_DEFAULT_VARSTORE_SIZE;
    }

    //
    // Update the Block with configuration info
    //
    CopyMem (DataBuffer + Offset, TmpBuffer, Width);
    FreePool (TmpBuffer);
    TmpBuffer = NULL;

    //
    // Set new Block Data
    //
    NewBlockData = (IFR_BLOCK_DATA *)AllocateZeroPool (sizeof (IFR_BLOCK_DATA));
    if (NewBlockData == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    NewBlockData->Offset = Offset;
    NewBlockData->Width  = Width;

    //
    // Insert the new block data into the block data array.
    //
    for (Link = BlockArray->Entry.ForwardLink; Link != &BlockArray->Entry; Link = Link->ForwardLink) {
      BlockData = BASE_CR (Link, IFR_BLOCK_DATA, Entry);
      if (NewBlockData->Offset == BlockData->Offset) {
        if (NewBlockData->Width > BlockData->Width) {
          BlockData->Width = NewBlockData->Width;
        }

        FreePool (NewBlockData);
        break;
      } else if (NewBlockData->Offset < BlockData->Offset) {
        //
        // Insert new block data as the previous one of this link.
        //
        InsertTailList (Link, &NewBlockData->Entry);
        break;
      }
    }

    //
    // Insert new block data into the array tail.
    //
    if (Link == &BlockArray->Entry) {
      InsertTailList (Link, &NewBlockData->Entry);
    }

    //
    // If '\0', parsing is finished.
    //
    if (*StringPtr == 0) {
      break;
    }

    //
    // Go to next ConfigBlock
    //
  }

  //
  // Merge the aligned block data into the single block data.
  //
  Link = BlockArray->Entry.ForwardLink;
  while ((Link != &BlockArray->Entry) && (Link->ForwardLink != &BlockArray->Entry)) {
    BlockData    = BASE_CR (Link, IFR_BLOCK_DATA, Entry);
    NewBlockData = BASE_CR (Link->ForwardLink, IFR_BLOCK_DATA, Entry);
    if ((!EFI_ERROR (SafeUint16Add (BlockData->Offset, BlockData->Width, &Sum1))) &&
        (!EFI_ERROR (SafeUint16Add (NewBlockData->Offset, NewBlockData->Width, &Sum2))) &&
        (NewBlockData->Offset >= BlockData->Offset) &&
        (NewBlockData->Offset <= Sum1) &&
        (Sum2 > Sum1))
    {
      Sum1 = BlockData->Width;
      if (!EFI_ERROR (SafeUint16Sub (Sum2, BlockData->Offset, &BlockData->Width))) {
        RemoveEntryList (Link->ForwardLink);
        FreePool (NewBlockData);
        continue;
      } else {
        BlockData->Width = Sum1;
      }
    }

    Link = Link->ForwardLink;
  }

  *VarBuffer         = DataBuffer;
  *CurrentBlockArray = BlockArray;
  return EFI_SUCCESS;

Done:
  if (DataBuffer != NULL) {
    FreePool (DataBuffer);
  }

  if (BlockArray != NULL) {
    //
    // Free Link Array CurrentBlockArray
    //
    while (!IsListEmpty (&BlockArray->Entry)) {
      BlockData = BASE_CR (BlockArray->Entry.ForwardLink, IFR_BLOCK_DATA, Entry);
      RemoveEntryList (&BlockData->Entry);
      FreePool (BlockData);
    }

    FreePool (BlockArray);
  }

  return Status;
}

/**
  This internal function parses IFR data to validate current setting.

  @param ConfigResp         ConfigResp string contains the current setting.
  @param HiiPackageList     Point to Hii package list.
  @param PackageListLength  The length of the pacakge.
  @param VarGuid            Guid of the buffer storage.
  @param VarName            Name of the buffer storage.
  @param HiiHandle          The HiiHandle for this package.

  @retval EFI_SUCCESS            The current setting is valid.
  @retval EFI_OUT_OF_RESOURCES   The memory is not enough.
  @retval EFI_INVALID_PARAMETER  The config string or the Hii package is invalid.
**/
EFI_STATUS
EFIAPI
InternalHiiValidateCurrentSetting (
  IN EFI_STRING                   ConfigResp,
  IN EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList,
  IN UINTN                        PackageListLength,
  IN EFI_GUID                     *VarGuid,
  IN CHAR16                       *VarName,
  IN EFI_HII_HANDLE               HiiHandle
  )
{
  CHAR16          *StringPtr;
  EFI_STATUS      Status;
  IFR_BLOCK_DATA  *CurrentBlockArray;
  IFR_BLOCK_DATA  *BlockData;
  UINT8           *VarBuffer;
  BOOLEAN         NameValueType;

  CurrentBlockArray = NULL;
  VarBuffer         = NULL;
  StringPtr         = NULL;
  Status            = EFI_SUCCESS;

  //
  // If StringPtr != NULL, get the request elements.
  //
  if (StrStr (ConfigResp, L"&OFFSET=") != NULL) {
    Status = GetBlockDataInfo (ConfigResp, &CurrentBlockArray, &VarBuffer);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    NameValueType = FALSE;
  } else {
    //
    // Skip header part.
    //
    StringPtr = StrStr (ConfigResp, L"PATH=");
    if (StringPtr == NULL) {
      ASSERT (StringPtr != NULL);
      return EFI_OUT_OF_RESOURCES;
    }

    if (StrStr (StringPtr, L"&") != NULL) {
      NameValueType = TRUE;
    } else {
      //
      // Not found Request element, return success.
      //
      return EFI_SUCCESS;
    }
  }

  Status = ValidateQuestionFromVfr (
             HiiPackageList,
             PackageListLength,
             VarGuid,
             VarName,
             VarBuffer,
             CurrentBlockArray,
             ConfigResp,
             HiiHandle,
             NameValueType
             );

  if (VarBuffer != NULL) {
    FreePool (VarBuffer);
  }

  if (CurrentBlockArray != NULL) {
    //
    // Free Link Array CurrentBlockArray
    //
    while (!IsListEmpty (&CurrentBlockArray->Entry)) {
      BlockData = BASE_CR (CurrentBlockArray->Entry.ForwardLink, IFR_BLOCK_DATA, Entry);
      RemoveEntryList (&BlockData->Entry);
      FreePool (BlockData);
    }

    FreePool (CurrentBlockArray);
  }

  return Status;
}

/**
  Check whether the ConfigRequest string has the request elements.
  For EFI_HII_VARSTORE_BUFFER type, the request has "&OFFSET=****&WIDTH=****..." format.
  For EFI_HII_VARSTORE_NAME_VALUE type, the request has "&NAME1**&NAME2..." format.

  @param  ConfigRequest      The input config request string.

  @retval  TRUE              The input include config request elements.
  @retval  FALSE             The input string not includes.

**/
BOOLEAN
GetElementsFromRequest (
  IN EFI_STRING  ConfigRequest
  )
{
  EFI_STRING  TmpRequest;

  TmpRequest = StrStr (ConfigRequest, L"PATH=");
  if (TmpRequest == NULL) {
    ASSERT (TmpRequest != NULL);
    return FALSE;
  }

  if ((StrStr (TmpRequest, L"&OFFSET=") != NULL) || (StrStr (TmpRequest, L"&") != NULL)) {
    return TRUE;
  }

  return FALSE;
}

/**
  This function parses the input ConfigRequest string and its matched IFR code
  string for setting default value and validating current setting.

  1. For setting default action, Reset the default value specified by DefaultId
  to the driver configuration got by Request string.
  2. For validating current setting, Validate the current configuration
  by parsing HII form IFR opcode.

  NULL request string support depends on the ExportConfig interface of
  HiiConfigRouting protocol in UEFI specification.

  @param Request    A null-terminated Unicode string in
                    <MultiConfigRequest> format. It can be NULL.
                    If it is NULL, all current configuration for the
                    entirety of the current HII database will be validated.
                    If it is NULL, all configuration for the
                    entirety of the current HII database will be reset.
  @param DefaultId  Specifies the type of defaults to retrieve only for setting default action.
  @param ActionType Action supports setting defaults and validate current setting.

  @retval TRUE    Action runs successfully.
  @retval FALSE   Action is not valid or Action can't be executed successfully..
**/
BOOLEAN
EFIAPI
InternalHiiIfrValueAction (
  IN CONST EFI_STRING  Request   OPTIONAL,
  IN UINT16            DefaultId,
  IN UINT8             ActionType
  )
{
  EFI_STRING      ConfigAltResp;
  EFI_STRING      ConfigAltHdr;
  EFI_STRING      ConfigResp;
  EFI_STRING      Progress;
  EFI_STRING      StringPtr;
  EFI_STRING      StringHdr;
  EFI_STATUS      Status;
  EFI_HANDLE      DriverHandle;
  EFI_HANDLE      TempDriverHandle;
  EFI_HII_HANDLE  *HiiHandleBuffer;
  EFI_HII_HANDLE  HiiHandle;
  UINT32          Index;
  EFI_GUID        *VarGuid;
  EFI_STRING      VarName;

  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINTN                        PackageListLength;
  UINTN                        MaxLen;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL     *TempDevicePath;

  ConfigAltResp    = NULL;
  ConfigResp       = NULL;
  VarGuid          = NULL;
  VarName          = NULL;
  DevicePath       = NULL;
  ConfigAltHdr     = NULL;
  HiiHandleBuffer  = NULL;
  Index            = 0;
  TempDriverHandle = NULL;
  HiiHandle        = NULL;
  HiiPackageList   = NULL;

  //
  // Only support set default and validate setting action.
  //
  if ((ActionType != ACTION_SET_DEFAUTL_VALUE) && (ActionType != ACTION_VALIDATE_SETTING)) {
    return FALSE;
  }

  //
  // Get the full requested value and deault value string.
  //
  if (Request != NULL) {
    Status = gHiiConfigRouting->ExtractConfig (
                                  gHiiConfigRouting,
                                  Request,
                                  &Progress,
                                  &ConfigAltResp
                                  );
  } else {
    Status = gHiiConfigRouting->ExportConfig (
                                  gHiiConfigRouting,
                                  &ConfigAltResp
                                  );
  }

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  StringPtr = ConfigAltResp;
  ASSERT (StringPtr != NULL);

  while (*StringPtr != L'\0') {
    //
    // 1. Find <ConfigHdr> GUID=...&NAME=...&PATH=...
    //
    StringHdr = StringPtr;

    //
    // Get Guid value
    //
    if (StrnCmp (StringPtr, L"GUID=", StrLen (L"GUID=")) != 0) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    StringPtr += StrLen (L"GUID=");
    Status     = InternalHiiGetBufferFromString (StringPtr, GUID_CONFIG_STRING_TYPE, (UINT8 **)&VarGuid);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Get Name value VarName
    //
    while (*StringPtr != L'\0' && StrnCmp (StringPtr, L"&NAME=", StrLen (L"&NAME=")) != 0) {
      StringPtr++;
    }

    if (*StringPtr == L'\0') {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    StringPtr += StrLen (L"&NAME=");
    Status     = InternalHiiGetBufferFromString (StringPtr, NAME_CONFIG_STRING_TYPE, (UINT8 **)&VarName);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Get Path value DevicePath
    //
    while (*StringPtr != L'\0' && StrnCmp (StringPtr, L"&PATH=", StrLen (L"&PATH=")) != 0) {
      StringPtr++;
    }

    if (*StringPtr == L'\0') {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    StringPtr += StrLen (L"&PATH=");
    Status     = InternalHiiGetBufferFromString (StringPtr, PATH_CONFIG_STRING_TYPE, (UINT8 **)&DevicePath);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Get the Driver handle by the got device path.
    //
    TempDevicePath = DevicePath;
    Status         = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &TempDevicePath, &DriverHandle);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Find the matched Hii Handle for the found Driver handle
    //
    HiiHandleBuffer = HiiGetHiiHandles (NULL);
    if (HiiHandleBuffer == NULL) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }

    for (Index = 0; HiiHandleBuffer[Index] != NULL; Index++) {
      gHiiDatabase->GetPackageListHandle (gHiiDatabase, HiiHandleBuffer[Index], &TempDriverHandle);
      if (TempDriverHandle == DriverHandle) {
        break;
      }
    }

    HiiHandle = HiiHandleBuffer[Index];
    FreePool (HiiHandleBuffer);

    if (HiiHandle == NULL) {
      //
      // This request string has no its Hii package.
      // Its default value and validating can't execute by parsing IFR data.
      // Directly jump into the next ConfigAltResp string for another pair Guid, Name, and Path.
      //
      Status = EFI_SUCCESS;
      goto NextConfigAltResp;
    }

    //
    // 2. Get HiiPackage by HiiHandle
    //
    PackageListLength = 0;
    HiiPackageList    = NULL;
    Status            = gHiiDatabase->ExportPackageLists (gHiiDatabase, HiiHandle, &PackageListLength, HiiPackageList);

    //
    // The return status should always be EFI_BUFFER_TOO_SMALL as input buffer's size is 0.
    //
    if (Status != EFI_BUFFER_TOO_SMALL) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    HiiPackageList = AllocatePool (PackageListLength);
    if (HiiPackageList == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    //
    // Get PackageList on HiiHandle
    //
    Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, HiiHandle, &PackageListLength, HiiPackageList);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // 3. Call ConfigRouting GetAltCfg(ConfigRoute, <ConfigResponse>, Guid, Name, DevicePath, AltCfgId, AltCfgResp)
    //    Get the default configuration string according to the default ID.
    //
    Status = gHiiConfigRouting->GetAltConfig (
                                  gHiiConfigRouting,
                                  ConfigAltResp,
                                  VarGuid,
                                  VarName,
                                  DevicePath,
                                  (ActionType == ACTION_SET_DEFAUTL_VALUE) ? &DefaultId : NULL,  // it can be NULL to get the current setting.
                                  &ConfigResp
                                  );

    //
    // The required setting can't be found. So, it is not required to be validated and set.
    //
    if (EFI_ERROR (Status)) {
      Status = EFI_SUCCESS;
      goto NextConfigAltResp;
    }

    //
    // Only the ConfigHdr is found. Not any block data is found. No data is required to be validated and set.
    //
    if (!GetElementsFromRequest (ConfigResp)) {
      goto NextConfigAltResp;
    }

    //
    // 4. Set the default configuration information or Validate current setting by parse IFR code.
    //    Current Setting is in ConfigResp, will be set into buffer, then check it again.
    //
    if (ActionType == ACTION_SET_DEFAUTL_VALUE) {
      //
      // Set the default configuration information.
      //
      Status = gHiiConfigRouting->RouteConfig (gHiiConfigRouting, ConfigResp, &Progress);
    } else {
      //
      // Current Setting is in ConfigResp, will be set into buffer, then check it again.
      //
      Status = InternalHiiValidateCurrentSetting (ConfigResp, HiiPackageList, PackageListLength, VarGuid, VarName, HiiHandle);
    }

    if (EFI_ERROR (Status)) {
      goto Done;
    }

NextConfigAltResp:
    //
    // Free the allocated pacakge buffer and the got ConfigResp string.
    //
    if (HiiPackageList != NULL) {
      FreePool (HiiPackageList);
      HiiPackageList = NULL;
    }

    if (ConfigResp != NULL) {
      FreePool (ConfigResp);
      ConfigResp = NULL;
    }

    //
    // Free the allocated buffer.
    //
    FreePool (VarGuid);
    VarGuid = NULL;

    FreePool (VarName);
    VarName = NULL;

    FreePool (DevicePath);
    DevicePath = NULL;

    //
    // 5. Jump to next ConfigAltResp for another Guid, Name, Path.
    //

    //
    // Get and Skip ConfigHdr
    //
    while (*StringPtr != L'\0' && *StringPtr != L'&') {
      StringPtr++;
    }

    if (*StringPtr == L'\0') {
      break;
    }

    //
    // Construct ConfigAltHdr string  "&<ConfigHdr>&ALTCFG=\0"
    //                               | 1 | StrLen (ConfigHdr) | 8 | 1 |
    //
    MaxLen       = 1 + StringPtr - StringHdr + 8 + 1;
    ConfigAltHdr = AllocateZeroPool (MaxLen * sizeof (CHAR16));
    if (ConfigAltHdr == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    StrCpyS (ConfigAltHdr, MaxLen, L"&");
    StrnCatS (ConfigAltHdr, MaxLen, StringHdr, StringPtr - StringHdr);
    StrCatS (ConfigAltHdr, MaxLen, L"&ALTCFG=");

    //
    // Skip all AltResp (AltConfigHdr ConfigBody) for the same ConfigHdr
    //
    while ((StringHdr = StrStr (StringPtr, ConfigAltHdr)) != NULL) {
      StringPtr = StringHdr + StrLen (ConfigAltHdr);
      if (*StringPtr == L'\0') {
        break;
      }
    }

    //
    // Free the allocated ConfigAltHdr string
    //
    FreePool (ConfigAltHdr);
    if (*StringPtr == L'\0') {
      break;
    }

    //
    // Find &GUID as the next ConfigHdr
    //
    StringPtr = StrStr (StringPtr, L"&GUID");
    if (StringPtr == NULL) {
      break;
    }

    //
    // Skip char '&'
    //
    StringPtr++;
  }

Done:
  if (VarGuid != NULL) {
    FreePool (VarGuid);
  }

  if (VarName != NULL) {
    FreePool (VarName);
  }

  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }

  if (ConfigResp != NULL) {
    FreePool (ConfigResp);
  }

  if (ConfigAltResp != NULL) {
    FreePool (ConfigAltResp);
  }

  if (HiiPackageList != NULL) {
    FreePool (HiiPackageList);
  }

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Validate the current configuration by parsing HII form IFR opcode.

  NULL request string support depends on the ExportConfig interface of
  HiiConfigRouting protocol in UEFI specification.

  @param  Request   A null-terminated Unicode string in
                    <MultiConfigRequest> format. It can be NULL.
                    If it is NULL, all current configuration for the
                    entirety of the current HII database will be validated.

  @retval TRUE    Current configuration is valid.
  @retval FALSE   Current configuration is invalid.
**/
BOOLEAN
EFIAPI
HiiValidateSettings (
  IN CONST EFI_STRING  Request  OPTIONAL
  )
{
  return InternalHiiIfrValueAction (Request, 0, ACTION_VALIDATE_SETTING);
}

/**
  Reset the default value specified by DefaultId to the driver
  configuration got by Request string.

  NULL request string support depends on the ExportConfig interface of
  HiiConfigRouting protocol in UEFI specification.

  @param Request    A null-terminated Unicode string in
                    <MultiConfigRequest> format. It can be NULL.
                    If it is NULL, all configuration for the
                    entirety of the current HII database will be reset.
  @param DefaultId  Specifies the type of defaults to retrieve.

  @retval TRUE    The default value is set successfully.
  @retval FALSE   The default value can't be found and set.
**/
BOOLEAN
EFIAPI
HiiSetToDefaults (
  IN CONST EFI_STRING  Request   OPTIONAL,
  IN UINT16            DefaultId
  )
{
  return InternalHiiIfrValueAction (Request, DefaultId, ACTION_SET_DEFAUTL_VALUE);
}

/**
  Determines if two values in config strings match.

  Compares the substring between StartSearchString and StopSearchString in
  FirstString to the substring between StartSearchString and StopSearchString
  in SecondString.  If the two substrings match, then TRUE is returned.  If the
  two substrings do not match, then FALSE is returned.

  If FirstString is NULL, then ASSERT().
  If SecondString is NULL, then ASSERT().
  If StartSearchString is NULL, then ASSERT().
  If StopSearchString is NULL, then ASSERT().

  @param FirstString        Pointer to the first Null-terminated Unicode string.
  @param SecondString       Pointer to the second Null-terminated Unicode string.
  @param StartSearchString  Pointer to the Null-terminated Unicode string that
                            marks the start of the value string to compare.
  @param StopSearchString   Pointer to the Null-terminated Unicode string that
                            marks the end of the value string to compare.

  @retval FALSE             StartSearchString is not present in FirstString.
  @retval FALSE             StartSearchString is not present in SecondString.
  @retval FALSE             StopSearchString is not present in FirstString.
  @retval FALSE             StopSearchString is not present in SecondString.
  @retval FALSE             The length of the substring in FirstString is not the
                            same length as the substring in SecondString.
  @retval FALSE             The value string in FirstString does not matche the
                            value string in SecondString.
  @retval TRUE              The value string in FirstString matches the value
                            string in SecondString.

**/
BOOLEAN
EFIAPI
InternalHiiCompareSubString (
  IN CHAR16  *FirstString,
  IN CHAR16  *SecondString,
  IN CHAR16  *StartSearchString,
  IN CHAR16  *StopSearchString
  )
{
  CHAR16  *EndFirstString;
  CHAR16  *EndSecondString;

  ASSERT (FirstString != NULL);
  ASSERT (SecondString != NULL);
  ASSERT (StartSearchString != NULL);
  ASSERT (StopSearchString != NULL);

  FirstString = StrStr (FirstString, StartSearchString);
  if (FirstString == NULL) {
    return FALSE;
  }

  SecondString = StrStr (SecondString, StartSearchString);
  if (SecondString == NULL) {
    return FALSE;
  }

  EndFirstString = StrStr (FirstString, StopSearchString);
  if (EndFirstString == NULL) {
    return FALSE;
  }

  EndSecondString = StrStr (SecondString, StopSearchString);
  if (EndSecondString == NULL) {
    return FALSE;
  }

  if ((EndFirstString - FirstString) != (EndSecondString - SecondString)) {
    return FALSE;
  }

  return (BOOLEAN)(StrnCmp (FirstString, SecondString, EndFirstString - FirstString) == 0);
}

/**
  Determines if the routing data specified by GUID and NAME match a <ConfigHdr>.

  If ConfigHdr is NULL, then ASSERT().

  @param[in] ConfigHdr  Either <ConfigRequest> or <ConfigResp>.
  @param[in] Guid       GUID of the storage.
  @param[in] Name       NAME of the storage.

  @retval TRUE   Routing information matches <ConfigHdr>.
  @retval FALSE  Routing information does not match <ConfigHdr>.

**/
BOOLEAN
EFIAPI
HiiIsConfigHdrMatch (
  IN CONST EFI_STRING  ConfigHdr,
  IN CONST EFI_GUID    *Guid      OPTIONAL,
  IN CONST CHAR16      *Name      OPTIONAL
  )
{
  EFI_STRING  CompareConfigHdr;
  BOOLEAN     Result;

  ASSERT (ConfigHdr != NULL);

  //
  // Use Guid and Name to generate a <ConfigHdr> string
  //
  CompareConfigHdr = HiiConstructConfigHdr (Guid, Name, NULL);
  if (CompareConfigHdr == NULL) {
    return FALSE;
  }

  Result = TRUE;
  if (Guid != NULL) {
    //
    // Compare GUID value strings
    //
    Result = InternalHiiCompareSubString (ConfigHdr, CompareConfigHdr, L"GUID=", L"&NAME=");
  }

  if (Result && (Name != NULL)) {
    //
    // Compare NAME value strings
    //
    Result = InternalHiiCompareSubString (ConfigHdr, CompareConfigHdr, L"&NAME=", L"&PATH=");
  }

  //
  // Free the <ConfigHdr> string
  //
  FreePool (CompareConfigHdr);

  return Result;
}

/**
  Retrieves uncommitted data from the Form Browser and converts it to a binary
  buffer.

  @param[in]  VariableGuid  Pointer to an EFI_GUID structure.  This is an optional
                            parameter that may be NULL.
  @param[in]  VariableName  Pointer to a Null-terminated Unicode string.  This
                            is an optional parameter that may be NULL.
  @param[in]  BufferSize    Length in bytes of buffer to hold retrieved data.
  @param[out] Buffer        Buffer of data to be updated.

  @retval FALSE  The uncommitted data could not be retrieved.
  @retval TRUE   The uncommitted data was retrieved.

**/
BOOLEAN
EFIAPI
HiiGetBrowserData (
  IN CONST EFI_GUID  *VariableGuid   OPTIONAL,
  IN CONST CHAR16    *VariableName   OPTIONAL,
  IN UINTN           BufferSize,
  OUT UINT8          *Buffer
  )
{
  EFI_STRING  ResultsData;
  UINTN       Size;
  EFI_STRING  ConfigResp;
  EFI_STATUS  Status;
  CHAR16      *Progress;

  //
  // Retrieve the results data from the Browser Callback
  //
  ResultsData = InternalHiiBrowserCallback (VariableGuid, VariableName, NULL);
  if (ResultsData == NULL) {
    return FALSE;
  }

  //
  // Construct <ConfigResp> mConfigHdrTemplate L'&' ResultsData L'\0'
  //
  Size       = (StrLen (mConfigHdrTemplate) + 1) * sizeof (CHAR16);
  Size       = Size + (StrLen (ResultsData) + 1) * sizeof (CHAR16);
  ConfigResp = AllocateZeroPool (Size);
  if (ConfigResp == NULL) {
    FreePool (ResultsData);
    return FALSE;
  }

  UnicodeSPrint (ConfigResp, Size, L"%s&%s", mConfigHdrTemplate, ResultsData);

  //
  // Free the allocated buffer
  //
  FreePool (ResultsData);
  if (ConfigResp == NULL) {
    return FALSE;
  }

  //
  // Convert <ConfigResp> to a buffer
  //
  Status = gHiiConfigRouting->ConfigToBlock (
                                gHiiConfigRouting,
                                ConfigResp,
                                Buffer,
                                &BufferSize,
                                &Progress
                                );
  //
  // Free the allocated buffer
  //
  FreePool (ConfigResp);

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Updates uncommitted data in the Form Browser.

  If Buffer is NULL, then ASSERT().

  @param[in]  VariableGuid    Pointer to an EFI_GUID structure.  This is an optional
                              parameter that may be NULL.
  @param[in]  VariableName    Pointer to a Null-terminated Unicode string.  This
                              is an optional parameter that may be NULL.
  @param[in]  BufferSize      Length, in bytes, of Buffer.
  @param[in]  Buffer          Buffer of data to commit.
  @param[in]  RequestElement  An optional field to specify which part of the
                              buffer data will be send back to Browser. If NULL,
                              the whole buffer of data will be committed to
                              Browser.
                              <RequestElement> ::= &OFFSET=<Number>&WIDTH=<Number>*

  @retval FALSE  The uncommitted data could not be updated.
  @retval TRUE   The uncommitted data was updated.

**/
BOOLEAN
EFIAPI
HiiSetBrowserData (
  IN CONST EFI_GUID  *VariableGuid  OPTIONAL,
  IN CONST CHAR16    *VariableName  OPTIONAL,
  IN UINTN           BufferSize,
  IN CONST UINT8     *Buffer,
  IN CONST CHAR16    *RequestElement  OPTIONAL
  )
{
  UINTN       Size;
  EFI_STRING  ConfigRequest;
  EFI_STRING  ConfigResp;
  EFI_STRING  ResultsData;

  ASSERT (Buffer != NULL);

  //
  // Construct <ConfigRequest>
  //
  if (RequestElement == NULL) {
    //
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    Size          = (StrLen (mConfigHdrTemplate) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    if (ConfigRequest == NULL) {
      return FALSE;
    }

    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", mConfigHdrTemplate, (UINT64)BufferSize);
  } else {
    //
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by <RequestElement> followed by a Null-terminator
    //
    Size          = StrLen (mConfigHdrTemplate) * sizeof (CHAR16);
    Size          = Size + (StrLen (RequestElement) + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    if (ConfigRequest == NULL) {
      return FALSE;
    }

    UnicodeSPrint (ConfigRequest, Size, L"%s%s", mConfigHdrTemplate, RequestElement);
  }

  if (ConfigRequest == NULL) {
    return FALSE;
  }

  //
  // Convert <ConfigRequest> to <ConfigResp>
  //
  ConfigResp = InternalHiiBlockToConfig (ConfigRequest, Buffer, BufferSize);
  FreePool (ConfigRequest);
  if (ConfigResp == NULL) {
    return FALSE;
  }

  //
  // Set data in the uncommitted browser state information
  //
  ResultsData = InternalHiiBrowserCallback (VariableGuid, VariableName, ConfigResp + StrLen (mConfigHdrTemplate) + 1);
  FreePool (ConfigResp);

  return (BOOLEAN)(ResultsData != NULL);
}

/////////////////////////////////////////
/////////////////////////////////////////
/// IFR Functions
/////////////////////////////////////////
/////////////////////////////////////////

#define HII_LIB_OPCODE_ALLOCATION_SIZE  0x200

typedef struct {
  UINT8    *Buffer;
  UINTN    BufferSize;
  UINTN    Position;
} HII_LIB_OPCODE_BUFFER;

///
/// Lookup table that converts EFI_IFR_TYPE_X enum values to a width in bytes
///
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  mHiiDefaultTypeToWidth[] = {
  1, // EFI_IFR_TYPE_NUM_SIZE_8
  2, // EFI_IFR_TYPE_NUM_SIZE_16
  4, // EFI_IFR_TYPE_NUM_SIZE_32
  8, // EFI_IFR_TYPE_NUM_SIZE_64
  1, // EFI_IFR_TYPE_BOOLEAN
  3, // EFI_IFR_TYPE_TIME
  4, // EFI_IFR_TYPE_DATE
  2  // EFI_IFR_TYPE_STRING
};

/**
  Allocates and returns a new OpCode Handle.  OpCode Handles must be freed with
  HiiFreeOpCodeHandle().

  @retval NULL   There are not enough resources to allocate a new OpCode Handle.
  @retval Other  A new OpCode handle.

**/
VOID *
EFIAPI
HiiAllocateOpCodeHandle (
  VOID
  )
{
  HII_LIB_OPCODE_BUFFER  *OpCodeBuffer;

  OpCodeBuffer = (HII_LIB_OPCODE_BUFFER *)AllocatePool (sizeof (HII_LIB_OPCODE_BUFFER));
  if (OpCodeBuffer == NULL) {
    return NULL;
  }

  OpCodeBuffer->Buffer = (UINT8 *)AllocatePool (HII_LIB_OPCODE_ALLOCATION_SIZE);
  if (OpCodeBuffer->Buffer == NULL) {
    FreePool (OpCodeBuffer);
    return NULL;
  }

  OpCodeBuffer->BufferSize = HII_LIB_OPCODE_ALLOCATION_SIZE;
  OpCodeBuffer->Position   = 0;
  return (VOID *)OpCodeBuffer;
}

/**
  Frees an OpCode Handle that was previously allocated with HiiAllocateOpCodeHandle().
  When an OpCode Handle is freed, all of the opcodes associated with the OpCode
  Handle are also freed.

  If OpCodeHandle is NULL, then ASSERT().

  @param[in]  OpCodeHandle   Handle to the buffer of opcodes.

**/
VOID
EFIAPI
HiiFreeOpCodeHandle (
  VOID  *OpCodeHandle
  )
{
  HII_LIB_OPCODE_BUFFER  *OpCodeBuffer;

  ASSERT (OpCodeHandle != NULL);

  OpCodeBuffer = (HII_LIB_OPCODE_BUFFER *)OpCodeHandle;
  if (OpCodeBuffer->Buffer != NULL) {
    FreePool (OpCodeBuffer->Buffer);
  }

  FreePool (OpCodeBuffer);
}

/**
  Internal function gets the current position of opcode buffer.

  @param[in]  OpCodeHandle   Handle to the buffer of opcodes.

  @return Current position of opcode buffer.
**/
UINTN
EFIAPI
InternalHiiOpCodeHandlePosition (
  IN VOID  *OpCodeHandle
  )
{
  return ((HII_LIB_OPCODE_BUFFER  *)OpCodeHandle)->Position;
}

/**
  Internal function gets the start pointer of opcode buffer.

  @param[in]  OpCodeHandle   Handle to the buffer of opcodes.

  @return Pointer to the opcode buffer base.
**/
UINT8 *
EFIAPI
InternalHiiOpCodeHandleBuffer (
  IN VOID  *OpCodeHandle
  )
{
  return ((HII_LIB_OPCODE_BUFFER  *)OpCodeHandle)->Buffer;
}

/**
  Internal function reserves the enough buffer for current opcode.
  When the buffer is not enough, Opcode buffer will be extended.

  @param[in]  OpCodeHandle   Handle to the buffer of opcodes.
  @param[in]  Size           Size of current opcode.

  @return Pointer to the current opcode.
**/
UINT8 *
EFIAPI
InternalHiiGrowOpCodeHandle (
  IN VOID   *OpCodeHandle,
  IN UINTN  Size
  )
{
  HII_LIB_OPCODE_BUFFER  *OpCodeBuffer;
  UINT8                  *Buffer;

  ASSERT (OpCodeHandle != NULL);

  OpCodeBuffer = (HII_LIB_OPCODE_BUFFER *)OpCodeHandle;
  if (OpCodeBuffer->Position + Size > OpCodeBuffer->BufferSize) {
    Buffer = ReallocatePool (
               OpCodeBuffer->BufferSize,
               OpCodeBuffer->BufferSize + (Size + HII_LIB_OPCODE_ALLOCATION_SIZE),
               OpCodeBuffer->Buffer
               );
    ASSERT (Buffer != NULL);
    OpCodeBuffer->Buffer      = Buffer;
    OpCodeBuffer->BufferSize += (Size + HII_LIB_OPCODE_ALLOCATION_SIZE);
  }

  Buffer                  = OpCodeBuffer->Buffer + OpCodeBuffer->Position;
  OpCodeBuffer->Position += Size;
  return Buffer;
}

/**
  Internal function creates opcode based on the template opcode.

  @param[in]  OpCodeHandle    Handle to the buffer of opcodes.
  @param[in]  OpCodeTemplate  Pointer to the template buffer of opcode.
  @param[in]  OpCode          OpCode IFR value.
  @param[in]  OpCodeSize      Size of opcode.
  @param[in]  ExtensionSize   Size of extended opcode.
  @param[in]  Scope           Scope bit of opcode.

  @return Pointer to the current opcode with opcode data.
**/
UINT8 *
EFIAPI
InternalHiiCreateOpCodeExtended (
  IN VOID   *OpCodeHandle,
  IN VOID   *OpCodeTemplate,
  IN UINT8  OpCode,
  IN UINTN  OpCodeSize,
  IN UINTN  ExtensionSize,
  IN UINT8  Scope
  )
{
  EFI_IFR_OP_HEADER  *Header;
  UINT8              *Buffer;

  ASSERT (OpCodeTemplate != NULL);
  ASSERT ((OpCodeSize + ExtensionSize) <= 0x7F);

  Header         = (EFI_IFR_OP_HEADER *)OpCodeTemplate;
  Header->OpCode = OpCode;
  Header->Scope  = Scope;
  Header->Length = (UINT8)(OpCodeSize + ExtensionSize);
  Buffer         = InternalHiiGrowOpCodeHandle (OpCodeHandle, Header->Length);
  return (UINT8 *)CopyMem (Buffer, Header, OpCodeSize);
}

/**
  Internal function creates opcode based on the template opcode for the normal opcode.

  @param[in]  OpCodeHandle    Handle to the buffer of opcodes.
  @param[in]  OpCodeTemplate  Pointer to the template buffer of opcode.
  @param[in]  OpCode          OpCode IFR value.
  @param[in]  OpCodeSize      Size of opcode.

  @return Pointer to the current opcode with opcode data.
**/
UINT8 *
EFIAPI
InternalHiiCreateOpCode (
  IN VOID   *OpCodeHandle,
  IN VOID   *OpCodeTemplate,
  IN UINT8  OpCode,
  IN UINTN  OpCodeSize
  )
{
  return InternalHiiCreateOpCodeExtended (OpCodeHandle, OpCodeTemplate, OpCode, OpCodeSize, 0, 0);
}

/**
  Append raw opcodes to an OpCodeHandle.

  If OpCodeHandle is NULL, then ASSERT().
  If RawBuffer is NULL, then ASSERT();

  @param[in]  OpCodeHandle   Handle to the buffer of opcodes.
  @param[in]  RawBuffer      Buffer of opcodes to append.
  @param[in]  RawBufferSize  The size, in bytes, of Buffer.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the appended opcodes.

**/
UINT8 *
EFIAPI
HiiCreateRawOpCodes (
  IN VOID   *OpCodeHandle,
  IN UINT8  *RawBuffer,
  IN UINTN  RawBufferSize
  )
{
  UINT8  *Buffer;

  ASSERT (RawBuffer != NULL);

  Buffer = InternalHiiGrowOpCodeHandle (OpCodeHandle, RawBufferSize);
  return (UINT8 *)CopyMem (Buffer, RawBuffer, RawBufferSize);
}

/**
  Append opcodes from one OpCode Handle to another OpCode handle.

  If OpCodeHandle is NULL, then ASSERT().
  If RawOpCodeHandle is NULL, then ASSERT();

  @param[in]  OpCodeHandle     Handle to the buffer of opcodes.
  @param[in]  RawOpCodeHandle  Handle to the buffer of opcodes.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the appended opcodes.

**/
UINT8 *
EFIAPI
InternalHiiAppendOpCodes (
  IN VOID  *OpCodeHandle,
  IN VOID  *RawOpCodeHandle
  )
{
  HII_LIB_OPCODE_BUFFER  *RawOpCodeBuffer;

  ASSERT (RawOpCodeHandle != NULL);

  RawOpCodeBuffer = (HII_LIB_OPCODE_BUFFER *)RawOpCodeHandle;
  return HiiCreateRawOpCodes (OpCodeHandle, RawOpCodeBuffer->Buffer, RawOpCodeBuffer->Position);
}

/**
  Create EFI_IFR_END_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateEndOpCode (
  IN VOID  *OpCodeHandle
  )
{
  EFI_IFR_END  OpCode;

  return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_END_OP, sizeof (OpCode));
}

/**
  Create EFI_IFR_ONE_OF_OPTION_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If Type is invalid, then ASSERT().
  If Flags is invalid, then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.
  @param[in]  StringId      StringId for the option
  @param[in]  Flags         Flags for the option
  @param[in]  Type          Type for the option
  @param[in]  Value         Value for the option

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateOneOfOptionOpCode (
  IN VOID    *OpCodeHandle,
  IN UINT16  StringId,
  IN UINT8   Flags,
  IN UINT8   Type,
  IN UINT64  Value
  )
{
  EFI_IFR_ONE_OF_OPTION  OpCode;

  ASSERT (Type < EFI_IFR_TYPE_OTHER);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Option = StringId;
  OpCode.Flags  = (UINT8)(Flags & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG));
  OpCode.Type   = Type;
  CopyMem (&OpCode.Value, &Value, mHiiDefaultTypeToWidth[Type]);

  return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_ONE_OF_OPTION_OP, OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value) + mHiiDefaultTypeToWidth[Type]);
}

/**
  Create EFI_IFR_DEFAULT_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If Type is invalid, then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.
  @param[in]  DefaultId     DefaultId for the default
  @param[in]  Type          Type for the default
  @param[in]  Value         Value for the default

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateDefaultOpCode (
  IN VOID    *OpCodeHandle,
  IN UINT16  DefaultId,
  IN UINT8   Type,
  IN UINT64  Value
  )
{
  EFI_IFR_DEFAULT  OpCode;

  ASSERT (Type < EFI_IFR_TYPE_OTHER);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Type      = Type;
  OpCode.DefaultId = DefaultId;
  CopyMem (&OpCode.Value, &Value, mHiiDefaultTypeToWidth[Type]);

  return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_DEFAULT_OP, OFFSET_OF (EFI_IFR_DEFAULT, Value) + mHiiDefaultTypeToWidth[Type]);
}

/**
  Create EFI_IFR_GUID opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If Guid is NULL, then ASSERT().
  If OpCodeSize < sizeof (EFI_IFR_GUID), then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.
  @param[in]  Guid          Pointer to EFI_GUID of this guided opcode.
  @param[in]  GuidOpCode    Pointer to an EFI_IFR_GUID opcode.  This is an
                            optional parameter that may be NULL.  If this
                            parameter is NULL, then the GUID extension
                            region of the created opcode is filled with zeros.
                            If this parameter is not NULL, then the GUID
                            extension region of GuidData will be copied to
                            the GUID extension region of the created opcode.
  @param[in]  OpCodeSize    The size, in bytes, of created opcode.  This value
                            must be >= sizeof(EFI_IFR_GUID).

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateGuidOpCode (
  IN VOID            *OpCodeHandle,
  IN CONST EFI_GUID  *Guid,
  IN CONST VOID      *GuidOpCode     OPTIONAL,
  IN UINTN           OpCodeSize
  )
{
  EFI_IFR_GUID  OpCode;
  EFI_IFR_GUID  *OpCodePointer;

  ASSERT (Guid != NULL);
  ASSERT (OpCodeSize >= sizeof (OpCode));

  ZeroMem (&OpCode, sizeof (OpCode));
  CopyGuid ((EFI_GUID *)(VOID *)&OpCode.Guid, Guid);

  OpCodePointer = (EFI_IFR_GUID *)InternalHiiCreateOpCodeExtended (
                                    OpCodeHandle,
                                    &OpCode,
                                    EFI_IFR_GUID_OP,
                                    sizeof (OpCode),
                                    OpCodeSize - sizeof (OpCode),
                                    0
                                    );
  if ((OpCodePointer != NULL) && (GuidOpCode != NULL)) {
    CopyMem (OpCodePointer + 1, (EFI_IFR_GUID *)GuidOpCode + 1, OpCodeSize - sizeof (OpCode));
  }

  return (UINT8 *)OpCodePointer;
}

/**
  Create EFI_IFR_ACTION_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.
  @param[in]  QuestionId      Question ID
  @param[in]  Prompt          String ID for Prompt
  @param[in]  Help            String ID for Help
  @param[in]  QuestionFlags   Flags in Question Header
  @param[in]  QuestionConfig  String ID for configuration

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateActionOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN EFI_STRING_ID    QuestionConfig
  )
{
  EFI_IFR_ACTION  OpCode;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.QuestionId    = QuestionId;
  OpCode.Question.Header.Prompt = Prompt;
  OpCode.Question.Header.Help   = Help;
  OpCode.Question.Flags         = QuestionFlags;
  OpCode.QuestionConfig         = QuestionConfig;

  return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_ACTION_OP, sizeof (OpCode));
}

/**
  Create EFI_IFR_SUBTITLE_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in Flags, then ASSERT().
  If Scope > 1, then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.
  @param[in]  Prompt      String ID for Prompt
  @param[in]  Help        String ID for Help
  @param[in]  Flags       Subtitle opcode flags
  @param[in]  Scope       1 if this opcpde is the beginning of a new scope.
                          0 if this opcode is within the current scope.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateSubTitleOpCode (
  IN VOID           *OpCodeHandle,
  IN EFI_STRING_ID  Prompt,
  IN EFI_STRING_ID  Help,
  IN UINT8          Flags,
  IN UINT8          Scope
  )
{
  EFI_IFR_SUBTITLE  OpCode;

  ASSERT (Scope <= 1);
  ASSERT ((Flags & (~(EFI_IFR_FLAGS_HORIZONTAL))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Statement.Prompt = Prompt;
  OpCode.Statement.Help   = Help;
  OpCode.Flags            = Flags;

  return InternalHiiCreateOpCodeExtended (
           OpCodeHandle,
           &OpCode,
           EFI_IFR_SUBTITLE_OP,
           sizeof (OpCode),
           0,
           Scope
           );
}

/**
  Create EFI_IFR_REF_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().

  @param[in]  OpCodeHandle   Handle to the buffer of opcodes.
  @param[in]  FormId         Destination Form ID
  @param[in]  Prompt         String ID for Prompt
  @param[in]  Help           String ID for Help
  @param[in]  QuestionFlags  Flags in Question Header
  @param[in]  QuestionId     Question ID

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateGotoOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_FORM_ID      FormId,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN EFI_QUESTION_ID  QuestionId
  )
{
  EFI_IFR_REF  OpCode;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.Header.Prompt = Prompt;
  OpCode.Question.Header.Help   = Help;
  OpCode.Question.QuestionId    = QuestionId;
  OpCode.Question.Flags         = QuestionFlags;
  OpCode.FormId                 = FormId;

  return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_REF_OP, sizeof (OpCode));
}

/**
  Create EFI_IFR_REF_OP, EFI_IFR_REF2_OP, EFI_IFR_REF3_OP and EFI_IFR_REF4_OP opcode.

  When RefDevicePath is not zero, EFI_IFR_REF4 opcode will be created.
  When RefDevicePath is zero and RefFormSetId is not NULL, EFI_IFR_REF3 opcode will be created.
  When RefDevicePath is zero, RefFormSetId is NULL and RefQuestionId is not zero, EFI_IFR_REF2 opcode will be created.
  When RefDevicePath is zero, RefFormSetId is NULL and RefQuestionId is zero, EFI_IFR_REF opcode will be created.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().

  @param[in]  OpCodeHandle   The handle to the buffer of opcodes.
  @param[in]  RefFormId      The Destination Form ID.
  @param[in]  Prompt         The string ID for Prompt.
  @param[in]  Help           The string ID for Help.
  @param[in]  QuestionFlags  The flags in Question Header
  @param[in]  QuestionId     Question ID.
  @param[in]  RefQuestionId  The question on the form to which this link is referring.
                             If its value is zero, then the link refers to the top of the form.
  @param[in]  RefFormSetId   The form set to which this link is referring. If its value is NULL, and RefDevicePath is
                             zero, then the link is to the current form set.
  @param[in]  RefDevicePath  The string identifier that specifies the string containing the text representation of
                             the device path to which the form set containing the form specified by FormId.
                             If its value is zero, then the link refers to the current page.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateGotoExOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_FORM_ID      RefFormId,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_QUESTION_ID  RefQuestionId,
  IN EFI_GUID         *RefFormSetId     OPTIONAL,
  IN EFI_STRING_ID    RefDevicePath
  )
{
  EFI_IFR_REF4  OpCode;
  UINTN         OpCodeSize;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.Header.Prompt = Prompt;
  OpCode.Question.Header.Help   = Help;
  OpCode.Question.QuestionId    = QuestionId;
  OpCode.Question.Flags         = QuestionFlags;
  OpCode.FormId                 = RefFormId;
  OpCode.QuestionId             = RefQuestionId;
  OpCode.DevicePath             = RefDevicePath;
  if (RefFormSetId != NULL) {
    CopyMem (&OpCode.FormSetId, RefFormSetId, sizeof (OpCode.FormSetId));
  }

  //
  // Cacluate OpCodeSize based on the input Ref value.
  // Try to use the small OpCode to save size.
  //
  OpCodeSize = sizeof (EFI_IFR_REF);
  if (RefDevicePath != 0) {
    OpCodeSize = sizeof (EFI_IFR_REF4);
  } else if (RefFormSetId != NULL) {
    OpCodeSize = sizeof (EFI_IFR_REF3);
  } else if (RefQuestionId != 0) {
    OpCodeSize = sizeof (EFI_IFR_REF2);
  }

  return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_REF_OP, OpCodeSize);
}

/**
  Create EFI_IFR_CHECKBOX_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in CheckBoxFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID
  @param[in]  VarOffset             Offset in Storage or String ID of the name (VarName)
                                    for this name/value pair.
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  CheckBoxFlags         Flags for checkbox opcode
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateCheckBoxOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId,
  IN UINT16           VarOffset,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            CheckBoxFlags,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  )
{
  EFI_IFR_CHECKBOX  OpCode;
  UINTN             Position;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_REST_STYLE))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.QuestionId             = QuestionId;
  OpCode.Question.VarStoreId             = VarStoreId;
  OpCode.Question.VarStoreInfo.VarOffset = VarOffset;
  OpCode.Question.Header.Prompt          = Prompt;
  OpCode.Question.Header.Help            = Help;
  OpCode.Question.Flags                  = QuestionFlags;
  OpCode.Flags                           = CheckBoxFlags;

  if (DefaultsOpCodeHandle == NULL) {
    return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_CHECKBOX_OP, sizeof (OpCode));
  }

  Position = InternalHiiOpCodeHandlePosition (OpCodeHandle);
  InternalHiiCreateOpCodeExtended (OpCodeHandle, &OpCode, EFI_IFR_CHECKBOX_OP, sizeof (OpCode), 0, 1);
  InternalHiiAppendOpCodes (OpCodeHandle, DefaultsOpCodeHandle);
  HiiCreateEndOpCode (OpCodeHandle);
  return InternalHiiOpCodeHandleBuffer (OpCodeHandle) + Position;
}

/**
  Create EFI_IFR_NUMERIC_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in NumericFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID
  @param[in]  VarOffset             Offset in Storage or String ID of the name (VarName)
                                    for this name/value pair.
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  NumericFlags          Flags for numeric opcode
  @param[in]  Minimum               Numeric minimum value
  @param[in]  Maximum               Numeric maximum value
  @param[in]  Step                  Numeric step for edit
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateNumericOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId,
  IN UINT16           VarOffset,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            NumericFlags,
  IN UINT64           Minimum,
  IN UINT64           Maximum,
  IN UINT64           Step,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  )
{
  EFI_IFR_NUMERIC  OpCode;
  UINTN            Position;
  UINTN            Length;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_REST_STYLE))) == 0);

  Length = 0;
  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.QuestionId             = QuestionId;
  OpCode.Question.VarStoreId             = VarStoreId;
  OpCode.Question.VarStoreInfo.VarOffset = VarOffset;
  OpCode.Question.Header.Prompt          = Prompt;
  OpCode.Question.Header.Help            = Help;
  OpCode.Question.Flags                  = QuestionFlags;
  OpCode.Flags                           = NumericFlags;

  switch (NumericFlags & EFI_IFR_NUMERIC_SIZE) {
    case EFI_IFR_NUMERIC_SIZE_1:
      OpCode.data.u8.MinValue = (UINT8)Minimum;
      OpCode.data.u8.MaxValue = (UINT8)Maximum;
      OpCode.data.u8.Step     = (UINT8)Step;
      Length                  = 3;
      break;

    case EFI_IFR_NUMERIC_SIZE_2:
      OpCode.data.u16.MinValue = (UINT16)Minimum;
      OpCode.data.u16.MaxValue = (UINT16)Maximum;
      OpCode.data.u16.Step     = (UINT16)Step;
      Length                   = 6;
      break;

    case EFI_IFR_NUMERIC_SIZE_4:
      OpCode.data.u32.MinValue = (UINT32)Minimum;
      OpCode.data.u32.MaxValue = (UINT32)Maximum;
      OpCode.data.u32.Step     = (UINT32)Step;
      Length                   = 12;
      break;

    case EFI_IFR_NUMERIC_SIZE_8:
      OpCode.data.u64.MinValue = Minimum;
      OpCode.data.u64.MaxValue = Maximum;
      OpCode.data.u64.Step     = Step;
      Length                   = 24;
      break;
  }

  Length += OFFSET_OF (EFI_IFR_NUMERIC, data);

  if (DefaultsOpCodeHandle == NULL) {
    return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_NUMERIC_OP, Length);
  }

  Position = InternalHiiOpCodeHandlePosition (OpCodeHandle);
  InternalHiiCreateOpCodeExtended (OpCodeHandle, &OpCode, EFI_IFR_NUMERIC_OP, Length, 0, 1);
  InternalHiiAppendOpCodes (OpCodeHandle, DefaultsOpCodeHandle);
  HiiCreateEndOpCode (OpCodeHandle);
  return InternalHiiOpCodeHandleBuffer (OpCodeHandle) + Position;
}

/**
  Create EFI_IFR_STRING_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in StringFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID
  @param[in]  VarOffset             Offset in Storage or String ID of the name (VarName)
                                    for this name/value pair.
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  StringFlags           Flags for string opcode
  @param[in]  MinSize               String minimum length
  @param[in]  MaxSize               String maximum length
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateStringOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId,
  IN UINT16           VarOffset,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            StringFlags,
  IN UINT8            MinSize,
  IN UINT8            MaxSize,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  )
{
  EFI_IFR_STRING  OpCode;
  UINTN           Position;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_REST_STYLE))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.Header.Prompt          = Prompt;
  OpCode.Question.Header.Help            = Help;
  OpCode.Question.QuestionId             = QuestionId;
  OpCode.Question.VarStoreId             = VarStoreId;
  OpCode.Question.VarStoreInfo.VarOffset = VarOffset;
  OpCode.Question.Flags                  = QuestionFlags;
  OpCode.MinSize                         = MinSize;
  OpCode.MaxSize                         = MaxSize;
  OpCode.Flags                           = (UINT8)(StringFlags & EFI_IFR_STRING_MULTI_LINE);

  if (DefaultsOpCodeHandle == NULL) {
    return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_STRING_OP, sizeof (OpCode));
  }

  Position = InternalHiiOpCodeHandlePosition (OpCodeHandle);
  InternalHiiCreateOpCodeExtended (OpCodeHandle, &OpCode, EFI_IFR_STRING_OP, sizeof (OpCode), 0, 1);
  InternalHiiAppendOpCodes (OpCodeHandle, DefaultsOpCodeHandle);
  HiiCreateEndOpCode (OpCodeHandle);
  return InternalHiiOpCodeHandleBuffer (OpCodeHandle) + Position;
}

/**
  Create EFI_IFR_ONE_OF_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in OneOfFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID
  @param[in]  VarOffset             Offset in Storage or String ID of the name (VarName)
                                    for this name/value pair.
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  OneOfFlags            Flags for oneof opcode
  @param[in]  OptionsOpCodeHandle   Handle for a buffer of ONE_OF_OPTION opcodes.
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateOneOfOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId,
  IN UINT16           VarOffset,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            OneOfFlags,
  IN VOID             *OptionsOpCodeHandle,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  )
{
  EFI_IFR_ONE_OF  OpCode;
  UINTN           Position;
  UINTN           Length;

  ASSERT (OptionsOpCodeHandle != NULL);
  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_REST_STYLE | EFI_IFR_FLAG_OPTIONS_ONLY))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.Header.Prompt          = Prompt;
  OpCode.Question.Header.Help            = Help;
  OpCode.Question.QuestionId             = QuestionId;
  OpCode.Question.VarStoreId             = VarStoreId;
  OpCode.Question.VarStoreInfo.VarOffset = VarOffset;
  OpCode.Question.Flags                  = QuestionFlags;
  OpCode.Flags                           = OneOfFlags;

  Length  = OFFSET_OF (EFI_IFR_ONE_OF, data);
  Length += (1 << (OneOfFlags & EFI_IFR_NUMERIC_SIZE)) * 3;

  Position = InternalHiiOpCodeHandlePosition (OpCodeHandle);
  InternalHiiCreateOpCodeExtended (OpCodeHandle, &OpCode, EFI_IFR_ONE_OF_OP, Length, 0, 1);
  InternalHiiAppendOpCodes (OpCodeHandle, OptionsOpCodeHandle);
  if (DefaultsOpCodeHandle != NULL) {
    InternalHiiAppendOpCodes (OpCodeHandle, DefaultsOpCodeHandle);
  }

  HiiCreateEndOpCode (OpCodeHandle);
  return InternalHiiOpCodeHandleBuffer (OpCodeHandle) + Position;
}

/**
  Create EFI_IFR_ORDERED_LIST_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in OrderedListFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID
  @param[in]  VarOffset             Offset in Storage or String ID of the name (VarName)
                                    for this name/value pair.
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  OrderedListFlags      Flags for ordered list opcode
  @param[in]  DataType              Type for option value
  @param[in]  MaxContainers         Maximum count for options in this ordered list
  @param[in]  OptionsOpCodeHandle   Handle for a buffer of ONE_OF_OPTION opcodes.
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateOrderedListOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId,
  IN UINT16           VarOffset,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            OrderedListFlags,
  IN UINT8            DataType,
  IN UINT8            MaxContainers,
  IN VOID             *OptionsOpCodeHandle,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  )
{
  EFI_IFR_ORDERED_LIST  OpCode;
  UINTN                 Position;

  ASSERT (OptionsOpCodeHandle != NULL);
  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_REST_STYLE | EFI_IFR_FLAG_OPTIONS_ONLY))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.Header.Prompt          = Prompt;
  OpCode.Question.Header.Help            = Help;
  OpCode.Question.QuestionId             = QuestionId;
  OpCode.Question.VarStoreId             = VarStoreId;
  OpCode.Question.VarStoreInfo.VarOffset = VarOffset;
  OpCode.Question.Flags                  = QuestionFlags;
  OpCode.MaxContainers                   = MaxContainers;
  OpCode.Flags                           = OrderedListFlags;

  Position = InternalHiiOpCodeHandlePosition (OpCodeHandle);
  InternalHiiCreateOpCodeExtended (OpCodeHandle, &OpCode, EFI_IFR_ORDERED_LIST_OP, sizeof (OpCode), 0, 1);
  InternalHiiAppendOpCodes (OpCodeHandle, OptionsOpCodeHandle);
  if (DefaultsOpCodeHandle != NULL) {
    InternalHiiAppendOpCodes (OpCodeHandle, DefaultsOpCodeHandle);
  }

  HiiCreateEndOpCode (OpCodeHandle);
  return InternalHiiOpCodeHandleBuffer (OpCodeHandle) + Position;
}

/**
  Create EFI_IFR_TEXT_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.
  @param[in]  Prompt        String ID for Prompt.
  @param[in]  Help          String ID for Help.
  @param[in]  TextTwo       String ID for TextTwo.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateTextOpCode (
  IN VOID           *OpCodeHandle,
  IN EFI_STRING_ID  Prompt,
  IN EFI_STRING_ID  Help,
  IN EFI_STRING_ID  TextTwo
  )
{
  EFI_IFR_TEXT  OpCode;

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Statement.Prompt = Prompt;
  OpCode.Statement.Help   = Help;
  OpCode.TextTwo          = TextTwo;

  return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_TEXT_OP, sizeof (OpCode));
}

/**
  Create EFI_IFR_DATE_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in DateFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID, optional. If DateFlags is not
                                    QF_DATE_STORAGE_NORMAL, this parameter is ignored.
  @param[in]  VarOffset             Offset in Storage or String ID of the name (VarName)
                                    for this name/value pair, optional. If DateFlags is not
                                    QF_DATE_STORAGE_NORMAL, this parameter is ignored.
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  DateFlags             Flags for date opcode
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateDateOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId    OPTIONAL,
  IN UINT16           VarOffset     OPTIONAL,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            DateFlags,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  )
{
  EFI_IFR_DATE  OpCode;
  UINTN         Position;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_REST_STYLE))) == 0);
  ASSERT ((DateFlags & (~(EFI_QF_DATE_YEAR_SUPPRESS | EFI_QF_DATE_MONTH_SUPPRESS | EFI_QF_DATE_DAY_SUPPRESS | EFI_QF_DATE_STORAGE))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.Header.Prompt          = Prompt;
  OpCode.Question.Header.Help            = Help;
  OpCode.Question.QuestionId             = QuestionId;
  OpCode.Question.VarStoreId             = VarStoreId;
  OpCode.Question.VarStoreInfo.VarOffset = VarOffset;
  OpCode.Question.Flags                  = QuestionFlags;
  OpCode.Flags                           = DateFlags;

  if (DefaultsOpCodeHandle == NULL) {
    return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_DATE_OP, sizeof (OpCode));
  }

  Position = InternalHiiOpCodeHandlePosition (OpCodeHandle);
  InternalHiiCreateOpCodeExtended (OpCodeHandle, &OpCode, EFI_IFR_DATE_OP, sizeof (OpCode), 0, 1);
  InternalHiiAppendOpCodes (OpCodeHandle, DefaultsOpCodeHandle);
  HiiCreateEndOpCode (OpCodeHandle);
  return InternalHiiOpCodeHandleBuffer (OpCodeHandle) + Position;
}

/**
  Create EFI_IFR_TIME_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in TimeFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID, optional. If TimeFlags is not
                                    QF_TIME_STORAGE_NORMAL, this parameter is ignored.
  @param[in]  VarOffset             Offset in Storage or String ID of the name (VarName)
                                    for this name/value pair, optional. If TimeFlags is not
                                    QF_TIME_STORAGE_NORMAL, this parameter is ignored.
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  TimeFlags             Flags for time opcode
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateTimeOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId    OPTIONAL,
  IN UINT16           VarOffset     OPTIONAL,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            TimeFlags,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  )
{
  EFI_IFR_TIME  OpCode;
  UINTN         Position;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_REST_STYLE))) == 0);
  ASSERT ((TimeFlags & (~(QF_TIME_HOUR_SUPPRESS | QF_TIME_MINUTE_SUPPRESS | QF_TIME_SECOND_SUPPRESS | QF_TIME_STORAGE))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.Header.Prompt          = Prompt;
  OpCode.Question.Header.Help            = Help;
  OpCode.Question.QuestionId             = QuestionId;
  OpCode.Question.VarStoreId             = VarStoreId;
  OpCode.Question.VarStoreInfo.VarOffset = VarOffset;
  OpCode.Question.Flags                  = QuestionFlags;
  OpCode.Flags                           = TimeFlags;

  if (DefaultsOpCodeHandle == NULL) {
    return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_TIME_OP, sizeof (OpCode));
  }

  Position = InternalHiiOpCodeHandlePosition (OpCodeHandle);
  InternalHiiCreateOpCodeExtended (OpCodeHandle, &OpCode, EFI_IFR_TIME_OP, sizeof (OpCode), 0, 1);
  InternalHiiAppendOpCodes (OpCodeHandle, DefaultsOpCodeHandle);
  HiiCreateEndOpCode (OpCodeHandle);
  return InternalHiiOpCodeHandleBuffer (OpCodeHandle) + Position;
}

/**
  This is the internal worker function to update the data in
  a form specified by FormSetGuid, FormId and Label.

  @param[in] FormSetGuid       The optional Formset GUID.
  @param[in] FormId            The Form ID.
  @param[in] Package           The package header.
  @param[in] OpCodeBufferStart An OpCode buffer that contains the set of IFR
                               opcodes to be inserted or replaced in the form.
  @param[in] OpCodeBufferEnd   An OpCcode buffer that contains the IFR opcode
                               that marks the end of a replace operation in the form.
  @param[out] TempPackage      The resultant package.

  @retval EFI_SUCCESS    The function completes successfully.
  @retval EFI_NOT_FOUND  The updated opcode or endopcode is not found.

**/
EFI_STATUS
EFIAPI
InternalHiiUpdateFormPackageData (
  IN  EFI_GUID                *FormSetGuid  OPTIONAL,
  IN  EFI_FORM_ID             FormId,
  IN  EFI_HII_PACKAGE_HEADER  *Package,
  IN  HII_LIB_OPCODE_BUFFER   *OpCodeBufferStart,
  IN  HII_LIB_OPCODE_BUFFER   *OpCodeBufferEnd     OPTIONAL,
  OUT EFI_HII_PACKAGE_HEADER  *TempPackage
  )
{
  UINTN                   AddSize;
  UINT8                   *BufferPos;
  EFI_HII_PACKAGE_HEADER  PackageHeader;
  UINTN                   Offset;
  EFI_IFR_OP_HEADER       *IfrOpHdr;
  EFI_IFR_OP_HEADER       *UpdateIfrOpHdr;
  BOOLEAN                 GetFormSet;
  BOOLEAN                 GetForm;
  BOOLEAN                 Updated;
  UINTN                   UpdatePackageLength;

  CopyMem (TempPackage, Package, sizeof (EFI_HII_PACKAGE_HEADER));
  UpdatePackageLength = sizeof (EFI_HII_PACKAGE_HEADER);
  BufferPos           = (UINT8 *)(TempPackage + 1);

  CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
  IfrOpHdr   = (EFI_IFR_OP_HEADER *)((UINT8 *)Package + sizeof (EFI_HII_PACKAGE_HEADER));
  Offset     = sizeof (EFI_HII_PACKAGE_HEADER);
  GetFormSet = (BOOLEAN)((FormSetGuid == NULL) ? TRUE : FALSE);
  GetForm    = FALSE;
  Updated    = FALSE;

  while (Offset < PackageHeader.Length) {
    CopyMem (BufferPos, IfrOpHdr, IfrOpHdr->Length);
    BufferPos           += IfrOpHdr->Length;
    UpdatePackageLength += IfrOpHdr->Length;

    //
    // Find the matched FormSet and Form
    //
    if ((IfrOpHdr->OpCode == EFI_IFR_FORM_SET_OP) && (FormSetGuid != NULL)) {
      if (CompareGuid ((GUID *)(VOID *)&((EFI_IFR_FORM_SET *)IfrOpHdr)->Guid, FormSetGuid)) {
        GetFormSet = TRUE;
      } else {
        GetFormSet = FALSE;
      }
    } else if ((IfrOpHdr->OpCode == EFI_IFR_FORM_OP) || (IfrOpHdr->OpCode == EFI_IFR_FORM_MAP_OP)) {
      if (CompareMem (&((EFI_IFR_FORM *)IfrOpHdr)->FormId, &FormId, sizeof (EFI_FORM_ID)) == 0) {
        GetForm = TRUE;
      } else {
        GetForm = FALSE;
      }
    }

    //
    // The matched Form is found, and Update data in this form
    //
    if (GetFormSet && GetForm) {
      UpdateIfrOpHdr = (EFI_IFR_OP_HEADER *)OpCodeBufferStart->Buffer;
      if ((UpdateIfrOpHdr->Length == IfrOpHdr->Length) && \
          (CompareMem (IfrOpHdr, UpdateIfrOpHdr, UpdateIfrOpHdr->Length) == 0))
      {
        //
        // Remove the original data when End OpCode buffer exist.
        //
        if (OpCodeBufferEnd != NULL) {
          Offset        += IfrOpHdr->Length;
          IfrOpHdr       = (EFI_IFR_OP_HEADER *)((UINT8 *)(IfrOpHdr) + IfrOpHdr->Length);
          UpdateIfrOpHdr = (EFI_IFR_OP_HEADER *)OpCodeBufferEnd->Buffer;
          while (Offset < PackageHeader.Length) {
            //
            // Search the matched end opcode
            //
            if ((UpdateIfrOpHdr->Length == IfrOpHdr->Length) && \
                (CompareMem (IfrOpHdr, UpdateIfrOpHdr, UpdateIfrOpHdr->Length) == 0))
            {
              break;
            }

            //
            // Go to the next Op-Code
            //
            Offset  += IfrOpHdr->Length;
            IfrOpHdr = (EFI_IFR_OP_HEADER *)((UINT8 *)(IfrOpHdr) + IfrOpHdr->Length);
          }

          if (Offset >= PackageHeader.Length) {
            //
            // The end opcode is not found.
            //
            return EFI_NOT_FOUND;
          }
        }

        //
        // Insert the updated data
        //
        AddSize = ((EFI_IFR_OP_HEADER *)OpCodeBufferStart->Buffer)->Length;
        CopyMem (BufferPos, OpCodeBufferStart->Buffer + AddSize, OpCodeBufferStart->Position - AddSize);
        BufferPos           += OpCodeBufferStart->Position - AddSize;
        UpdatePackageLength += OpCodeBufferStart->Position - AddSize;

        if (OpCodeBufferEnd != NULL) {
          //
          // Add the end opcode
          //
          CopyMem (BufferPos, IfrOpHdr, IfrOpHdr->Length);
          BufferPos           += IfrOpHdr->Length;
          UpdatePackageLength += IfrOpHdr->Length;
        }

        //
        // Copy the left package data.
        //
        Offset += IfrOpHdr->Length;
        CopyMem (BufferPos, (UINT8 *)Package + Offset, PackageHeader.Length - Offset);
        UpdatePackageLength += PackageHeader.Length - Offset;

        //
        // Set update flag
        //
        Updated = TRUE;
        break;
      }
    }

    //
    // Go to the next Op-Code
    //
    Offset  += IfrOpHdr->Length;
    IfrOpHdr = (EFI_IFR_OP_HEADER *)((CHAR8 *)(IfrOpHdr) + IfrOpHdr->Length);
  }

  if (!Updated) {
    //
    // The updated opcode buffer is not found.
    //
    return EFI_NOT_FOUND;
  }

  //
  // Update the package length.
  //
  PackageHeader.Length = (UINT32)UpdatePackageLength;
  CopyMem (TempPackage, &PackageHeader, sizeof (EFI_HII_PACKAGE_HEADER));

  return EFI_SUCCESS;
}

/**
  This function updates a form that has previously been registered with the HII
  Database.  This function will perform at most one update operation.

  The form to update is specified by Handle, FormSetGuid, and FormId.  Binary
  comparisons of IFR opcodes are performed from the beginning of the form being
  updated until an IFR opcode is found that exactly matches the first IFR opcode
  specified by StartOpCodeHandle.  The following rules are used to determine if
  an insert, replace, or delete operation is performed.

  1) If no matches are found, then NULL is returned.
  2) If a match is found, and EndOpCodeHandle is NULL, then all of the IFR opcodes
     from StartOpCodeHandle except the first opcode are inserted immediately after
     the matching IFR opcode in the form to be updated.
  3) If a match is found, and EndOpCodeHandle is not NULL, then a search is made
     from the matching IFR opcode until an IFR opcode exactly matches the first
     IFR opcode specified by EndOpCodeHandle.  If no match is found for the first
     IFR opcode specified by EndOpCodeHandle, then NULL is returned.  If a match
     is found, then all of the IFR opcodes between the start match and the end
     match are deleted from the form being updated and all of the IFR opcodes
     from StartOpCodeHandle except the first opcode are inserted immediately after
     the matching start IFR opcode.  If StartOpCcodeHandle only contains one
     IFR instruction, then the result of this operation will delete all of the IFR
     opcodes between the start end matches.

  If HiiHandle is NULL, then ASSERT().
  If StartOpCodeHandle is NULL, then ASSERT().

  @param[in]  HiiHandle          The HII Handle of the form to update.
  @param[in]  FormSetGuid        The Formset GUID of the form to update.  This
                                 is an optional parameter that may be NULL.
                                 If it is NULL, all FormSet will be updated.
  @param[in]  FormId             The ID of the form to update.
  @param[in]  StartOpCodeHandle  An OpCode Handle that contains the set of IFR
                                 opcodes to be inserted or replaced in the form.
                                 The first IFR instruction in StartOpCodeHandle
                                 is used to find matching IFR opcode in the
                                 form.
  @param[in]  EndOpCodeHandle    An OpCcode Handle that contains the IFR opcode
                                 that marks the end of a replace operation in
                                 the form.  This is an optional parameter that
                                 may be NULL.  If it is NULL, then an the IFR
                                 opcodes specified by StartOpCodeHandle are
                                 inserted into the form.

  @retval EFI_OUT_OF_RESOURCES   No enough memory resource is allocated.
  @retval EFI_NOT_FOUND          The following cases will return EFI_NOT_FOUND.
                                 1) The form specified by HiiHandle, FormSetGuid,
                                 and FormId could not be found in the HII Database.
                                 2) No IFR opcodes in the target form match the first
                                 IFR opcode in StartOpCodeHandle.
                                 3) EndOpCOde is not NULL, and no IFR opcodes in the
                                 target form following a matching start opcode match
                                 the first IFR opcode in EndOpCodeHandle.
  @retval EFI_SUCCESS            The matched form is updated by StartOpcode.

**/
EFI_STATUS
EFIAPI
HiiUpdateForm (
  IN EFI_HII_HANDLE  HiiHandle,
  IN EFI_GUID        *FormSetGuid         OPTIONAL,
  IN EFI_FORM_ID     FormId,
  IN VOID            *StartOpCodeHandle,
  IN VOID            *EndOpCodeHandle     OPTIONAL
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINT32                       PackageListLength;
  UINT32                       Offset;
  EFI_HII_PACKAGE_LIST_HEADER  *UpdatePackageList;
  UINTN                        BufferSize;
  UINT8                        *UpdateBufferPos;
  EFI_HII_PACKAGE_HEADER       *Package;
  EFI_HII_PACKAGE_HEADER       *TempPackage;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  BOOLEAN                      Updated;
  HII_LIB_OPCODE_BUFFER        *OpCodeBufferStart;
  HII_LIB_OPCODE_BUFFER        *OpCodeBufferEnd;

  //
  // Input update data can't be NULL.
  //
  ASSERT (HiiHandle != NULL);
  ASSERT (StartOpCodeHandle != NULL);
  UpdatePackageList = NULL;
  TempPackage       = NULL;
  HiiPackageList    = NULL;

  //
  // Retrieve buffer data from Opcode Handle
  //
  OpCodeBufferStart = (HII_LIB_OPCODE_BUFFER *)StartOpCodeHandle;
  OpCodeBufferEnd   = (HII_LIB_OPCODE_BUFFER *)EndOpCodeHandle;

  //
  // Get the original package list
  //
  BufferSize     = 0;
  HiiPackageList = NULL;
  Status         = gHiiDatabase->ExportPackageLists (gHiiDatabase, HiiHandle, &BufferSize, HiiPackageList);
  //
  // The return status should always be EFI_BUFFER_TOO_SMALL as input buffer's size is 0.
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  HiiPackageList = AllocatePool (BufferSize);
  if (HiiPackageList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }

  Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, HiiHandle, &BufferSize, HiiPackageList);
  if (EFI_ERROR (Status)) {
    goto Finish;
  }

  //
  // Calculate and allocate space for retrieval of IFR data
  //
  BufferSize       += OpCodeBufferStart->Position;
  UpdatePackageList = AllocateZeroPool (BufferSize);
  if (UpdatePackageList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }

  //
  // Allocate temp buffer to store the temp updated package buffer
  //
  TempPackage = AllocateZeroPool (BufferSize);
  if (TempPackage == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }

  UpdateBufferPos = (UINT8 *)UpdatePackageList;

  //
  // Copy the package list header
  //
  CopyMem (UpdateBufferPos, HiiPackageList, sizeof (EFI_HII_PACKAGE_LIST_HEADER));
  UpdateBufferPos += sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  //
  // Go through each package to find the matched package and update one by one
  //
  Updated           = FALSE;
  Offset            = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  PackageListLength = ReadUnaligned32 (&HiiPackageList->PackageLength);
  while (Offset < PackageListLength) {
    Package = (EFI_HII_PACKAGE_HEADER *)(((UINT8 *)HiiPackageList) + Offset);
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
    Offset += Package->Length;

    if (Package->Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Check this package is the matched package.
      //
      Status = InternalHiiUpdateFormPackageData (FormSetGuid, FormId, Package, OpCodeBufferStart, OpCodeBufferEnd, TempPackage);
      //
      // The matched package is found. Its package buffer will be updated by the input new data.
      //
      if (!EFI_ERROR (Status)) {
        //
        // Set Update Flag
        //
        Updated = TRUE;
        //
        // Add updated package buffer
        //
        Package = TempPackage;
      }
    }

    //
    // Add pacakge buffer
    //
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
    CopyMem (UpdateBufferPos, Package, PackageHeader.Length);
    UpdateBufferPos += PackageHeader.Length;
  }

  if (Updated) {
    //
    // Update package list length
    //
    BufferSize = UpdateBufferPos - (UINT8 *)UpdatePackageList;
    WriteUnaligned32 (&UpdatePackageList->PackageLength, (UINT32)BufferSize);

    //
    // Update Package to show form
    //
    Status = gHiiDatabase->UpdatePackageList (gHiiDatabase, HiiHandle, UpdatePackageList);
  } else {
    //
    // Not matched form is found and updated.
    //
    Status = EFI_NOT_FOUND;
  }

Finish:
  if (HiiPackageList != NULL) {
    FreePool (HiiPackageList);
  }

  if (UpdatePackageList != NULL) {
    FreePool (UpdatePackageList);
  }

  if (TempPackage != NULL) {
    FreePool (TempPackage);
  }

  return Status;
}
