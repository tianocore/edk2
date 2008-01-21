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


#include <PiDxe.h>

#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>
#include <Protocol/DevicePath.h>

#include <Guid/GlobalVariable.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HiiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>

#include <MdeModuleHii.h>

#include "InternalHiiLib.h"


EFI_HII_DATABASE_PROTOCOL   *mHiiDatabaseProt;
EFI_HII_STRING_PROTOCOL     *mHiiStringProt;

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
      EFI_IFR_TIANO_GUID
    },
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

EFI_STATUS
HiiLibGetCurrentLanguage (
  OUT     CHAR8               *Lang
  )
/*++

Routine Description:
  Determine what is the current language setting

Arguments:
  Lang      - Pointer of system language

Returns:
  Status code

--*/
{
  EFI_STATUS  Status;
  UINTN       Size;

  //
  // Get current language setting
  //
  Size = RFC_3066_ENTRY_SIZE;
  Status = gRT->GetVariable (
                  L"PlatformLang",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &Size,
                  Lang
                  );

  if (EFI_ERROR (Status)) {
    AsciiStrCpy (Lang, (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLang));
  }

  return Status;
}

VOID
HiiLibGetNextLanguage (
  IN OUT CHAR8      **LangCode,
  OUT CHAR8         *Lang
  )
/*++

Routine Description:
  Get next language from language code list (with separator ';').

Arguments:
  LangCode - On input: point to first language in the list. On output: point to
             next language in the list, or NULL if no more language in the list.
  Lang     - The first language in the list.

Returns:
  None.

--*/
{
  UINTN  Index;
  CHAR8  *StringPtr;

  if (LangCode == NULL || *LangCode == NULL) {
    *Lang = 0;
    return;
  }

  Index = 0;
  StringPtr = *LangCode;
  while (StringPtr[Index] != 0 && StringPtr[Index] != ';') {
    Index++;
  }

  CopyMem (Lang, StringPtr, Index);
  Lang[Index] = 0;

  if (StringPtr[Index] == ';') {
    Index++;
  }
  *LangCode = StringPtr + Index;
}

CHAR8 *
HiiLibGetSupportedLanguages (
  IN EFI_HII_HANDLE           HiiHandle
  )
/*++

Routine Description:
  This function returns the list of supported languages, in the format specified
  in UEFI specification Appendix M.

Arguments:
  HiiHandle  - The HII package list handle.

Returns:
  The supported languages.

--*/
{
  EFI_STATUS  Status;
  UINTN       BufferSize;
  CHAR8       *LanguageString;

  //
  // Collect current supported Languages for given HII handle
  //
  BufferSize = 0x1000;
  LanguageString = AllocatePool (BufferSize);
  Status = mHiiStringProt->GetLanguages (mHiiStringProt, HiiHandle, LanguageString, &BufferSize);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    gBS->FreePool (LanguageString);
    LanguageString = AllocatePool (BufferSize);
    Status = mHiiStringProt->GetLanguages (mHiiStringProt, HiiHandle, LanguageString, &BufferSize);
  }

  if (EFI_ERROR (Status)) {
    LanguageString = NULL;
  }

  return LanguageString;
}

UINT16
HiiLibGetSupportedLanguageNumber (
  IN EFI_HII_HANDLE           HiiHandle
  )
/*++

Routine Description:
  This function returns the number of supported languages

Arguments:
  HiiHandle  - The HII package list handle.

Returns:
  The  number of supported languages.

--*/
{
  CHAR8   *Languages;
  CHAR8   *LanguageString;
  UINT16  LangNumber;
  CHAR8   Lang[RFC_3066_ENTRY_SIZE];

  Languages = HiiLibGetSupportedLanguages (HiiHandle);
  if (Languages == NULL) {
    return 0;
  }

  LangNumber = 0;
  LanguageString = Languages;
  while (*LanguageString != 0) {
    HiiLibGetNextLanguage (&LanguageString, Lang);
    LangNumber++;
  }
  gBS->FreePool (Languages);

  return LangNumber;
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
  EFI_HII_PACKAGE_LIST_HEADER *PackageListHeader;
  VA_LIST                     Marker;

  VA_START (Marker, GuidId);
  PackageListHeader = InternalHiiLibPreparePackages (NumberOfPackages, GuidId, Marker);
  VA_END (Marker);

  return PackageListHeader;
}


/**
  This function allocates pool for an EFI_HII_PACKAGE_LIST structure
  with additional space that is big enough to host all packages described by the variable 
  argument list of package pointers.  The allocated structure is initialized using NumberOfPackages, 
  GuidId,  and the variable length argument list of package pointers.

  Then, EFI_HII_PACKAGE_LIST will be register to the default System HII Database. The
  Handle to the newly registered Package List is returned throught HiiHandle.

  @param  NumberOfPackages  The number of HII packages to register.
  @param  GuidId                    Package List GUID ID.
  @param  HiiHandle                The ID used to retrieve the Package List later.
  @param  ...                          The variable argument list describing all HII Package.

  @return
  The allocated and initialized packages.

**/

EFI_STATUS
EFIAPI
HiiLibAddPackagesToHiiDatabase (
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

EFI_STATUS
EFIAPI
HiiLibAddFontPackageToHiiDatabase (
  IN       UINTN               FontSize,
  IN CONST UINT8               *FontBinary,
  IN CONST EFI_GUID            *GuidId,
  OUT      EFI_HII_HANDLE      *HiiHandle OPTIONAL
  )
{
  EFI_STATUS                           Status;
  UINT8                                *Location;
  EFI_HII_SIMPLE_FONT_PACKAGE_HDR      *SimplifiedFont;
  UINTN                                PackageLength;
  EFI_HII_PACKAGE_LIST_HEADER          *PackageList;
  UINT8                                *Package;

  //
  // Add 4 bytes to the header for entire length for PreparePackageList use only.
  // Looks ugly. Might be updated when font tool is ready.
  //
  PackageLength   = sizeof (EFI_HII_SIMPLE_FONT_PACKAGE_HDR) + FontSize + 4;
  Package = AllocateZeroPool (PackageLength);
  if (Package == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (Package, &PackageLength, 4);
  SimplifiedFont = (EFI_HII_SIMPLE_FONT_PACKAGE_HDR*) (Package + 4);
  SimplifiedFont->Header.Length        = (UINT32) (PackageLength - 4);
  SimplifiedFont->Header.Type          = EFI_HII_PACKAGE_SIMPLE_FONTS;
  SimplifiedFont->NumberOfNarrowGlyphs = (UINT16) (FontSize / sizeof (EFI_NARROW_GLYPH));
  
  Location = (UINT8 *) (&SimplifiedFont->NumberOfWideGlyphs + 1);
  CopyMem (Location, FontBinary, FontSize);
  
  //
  // Add this simplified font package to a package list then install it.
  //
  PackageList = HiiLibPreparePackageList (1, GuidId, Package);
  Status = mHiiDatabaseProt->NewPackageList (mHiiDatabaseProt, PackageList, NULL, HiiHandle);
  ASSERT_EFI_ERROR (Status);
  SafeFreePool (PackageList);
  SafeFreePool (Package);    

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiLibRemovePackagesFromHiiDatabase (
  IN      EFI_HII_HANDLE      HiiHandle
  )
{
  return mHiiDatabaseProt->RemovePackageList (mHiiDatabaseProt, HiiHandle);
}

EFI_STATUS
EFIAPI
HiiLibCreateString (
  IN  EFI_HII_HANDLE                  PackageList,
  OUT EFI_STRING_ID                   *StringId,
  IN  CONST EFI_STRING                String
  )
{
  EFI_STATUS  Status;
  CHAR8       *Languages;
  CHAR8       *LangStrings;
  CHAR8       Lang[RFC_3066_ENTRY_SIZE];

  Status = EFI_SUCCESS;

  Languages = HiiLibGetSupportedLanguages (PackageList);

  LangStrings = Languages;
  while (*LangStrings != 0) {
    HiiLibGetNextLanguage (&LangStrings, Lang);

    Status = mHiiStringProt->NewString (
                                 mHiiStringProt,
                                 PackageList,
                                 StringId,
                                 Lang,
                                 NULL,
                                 String,
                                 NULL
                                 );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  FreePool (Languages);

  return Status;
  
}

EFI_STATUS
EFIAPI
HiiLibUpdateString (
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  IN  CONST EFI_STRING                String
  )
{
  EFI_STATUS  Status;
  CHAR8       *Languages;
  CHAR8       *LangStrings;
  CHAR8       Lang[RFC_3066_ENTRY_SIZE];

  Status = EFI_SUCCESS;

  Languages = HiiLibGetSupportedLanguages (PackageList);

  LangStrings = Languages;
  while (*LangStrings != 0) {
    HiiLibGetNextLanguage (&LangStrings, Lang);

    Status = mHiiStringProt->SetString (
                                 mHiiStringProt,
                                 PackageList,
                                 StringId,
                                 Lang,
                                 String,
                                 NULL
                                 );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  FreePool (Languages);

  return Status;
}

//                                                                                      //
// //////////////////////////////////////////////////
//                                                                                    //

//
// This function is Implementation Specifc. HII_VENDOR_DEVICE_PATH
// This should be moved to MdeModulepkg.
//
EFI_STATUS
EFIAPI
HiiLibCreateHiiDriverHandle (
  OUT EFI_HANDLE               *DriverHandle
  )
{
  EFI_STATUS                   Status;
  HII_VENDOR_DEVICE_PATH_NODE  *VendorDevicePath;
  UINT64                       MonotonicCount;

  VendorDevicePath = AllocateCopyPool (sizeof (HII_VENDOR_DEVICE_PATH), &mHiiVendorDevicePathTemplate);
  if (VendorDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  gBS->GetNextMonotonicCount (&MonotonicCount);
  VendorDevicePath->MonotonicCount = (UINT32) MonotonicCount;

  *DriverHandle = NULL;
  Status = gBS->InstallProtocolInterface (
                  DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  VendorDevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
HiiLibDestroyHiiDriverHandle (
  IN EFI_HANDLE               DriverHandle
  )
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

  return Status;
}

EFI_STATUS
HiiLibExtractDefault(
  IN VOID                         *Buffer,
  IN UINTN                        *BufferSize,
  UINTN                           Number,
  ...
  )
/*++

  Routine Description:

    Configure the buffer accrording to ConfigBody strings.

  Arguments:
    DefaultId             - the ID of default.
    Buffer                - the start address of buffer.
    BufferSize            - the size of buffer.
    Number                - the number of the strings.

  Returns:
    EFI_BUFFER_TOO_SMALL  - the BufferSize is too small to operate.
    EFI_INVALID_PARAMETER - Buffer is NULL or BufferSize is 0.
    EFI_SUCCESS           - Operation successful.

--*/
{
  VA_LIST                         Args;
  UINTN                           Index;
  UINT32                          TotalLen;
  UINT8                           *BufCfgArray;
  UINT8                           *BufferPos;
  UINT16                          Offset;
  UINT16                          Width;
  UINT8                           *Value;

  if ((Buffer == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Offset = 0;
  Width  = 0;
  Value  = NULL;

  VA_START (Args, Number);
  for (Index = 0; Index < Number; Index++) {
    BufCfgArray = (UINT8 *) VA_ARG (Args, VOID *);
    CopyMem (&TotalLen, BufCfgArray, sizeof (UINT32));
    BufferPos = BufCfgArray + sizeof (UINT32);

    while ((UINT32)(BufferPos - BufCfgArray) < TotalLen) {
      CopyMem (&Offset, BufferPos, sizeof (UINT16));
      BufferPos += sizeof (UINT16);
      CopyMem (&Width, BufferPos, sizeof (UINT16));
      BufferPos += sizeof (UINT16);
      Value = BufferPos;
      BufferPos += Width;

      if ((UINTN)(Offset + Width) > *BufferSize) {
        return EFI_BUFFER_TOO_SMALL;
      }

      CopyMem ((UINT8 *)Buffer + Offset, Value, Width);
    }
  }
  VA_END (Args);

  *BufferSize = (UINTN)Offset;

  return EFI_SUCCESS;
}


STATIC EFI_GUID mIfrVendorGuid = EFI_IFR_TIANO_GUID;

EFI_STATUS
HiiLibExtractClassFromHiiHandle (
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

--*/
{
  EFI_STATUS                   Status;
  UINTN                        BufferSize;
  EFI_HII_DATABASE_PROTOCOL    *HiiDatabase;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  UINT32                       Offset;
  UINT32                       Offset2;
  UINT32                       PackageListLength;
  EFI_HII_PACKAGE_HEADER       PackageHeader;

  *Class = EFI_NON_DEVICE_CLASS;
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
    HiiPackageList = AllocatePool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get Form package from this HII package List
  //
  Offset = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  Offset2 = 0;
  CopyMem (&PackageListLength, &HiiPackageList->PackageLength, sizeof (UINT32));

  while (Offset < PackageListLength) {
    Package = ((UINT8 *) HiiPackageList) + Offset;
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));

    if (PackageHeader.Type == EFI_HII_PACKAGE_FORM) {
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
          CopyMem (FormSetTitle, &((EFI_IFR_FORM_SET *) OpCodeData)->FormSetTitle, sizeof (EFI_STRING_ID));
          CopyMem (FormSetHelp, &((EFI_IFR_FORM_SET *) OpCodeData)->Help, sizeof (EFI_STRING_ID));
        }

        if ((((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode == EFI_IFR_GUID_OP) &&
             CompareGuid (&mIfrVendorGuid, (EFI_GUID *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER))) &&
            (((EFI_IFR_GUID_CLASS *) OpCodeData)->ExtendOpCode == EFI_IFR_EXTEND_OP_CLASS)
           ) {
          //
          // Find GUIDed Class OpCode
          //
          CopyMem (Class, &((EFI_IFR_GUID_CLASS *) OpCodeData)->Class, sizeof (UINT16));

          //
          // Till now, we ought to have found the formset Opcode
          //
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

