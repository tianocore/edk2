/**@file

  This file contains global defines and prototype definitions
  for the HII database.
  
Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HIIDATABASE_H
#define _HIIDATABASE_H


#include <FrameworkDxe.h>

#include <Guid/GlobalVariable.h>
#include <Protocol/FrameworkFormCallback.h>
#include <Protocol/FrameworkHii.h>

//
// UEFI HII Protocols
//
#include <Protocol/HiiFont.h>
#include <Protocol/HiiImage.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>


#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/FrameworkIfrSupportLib.h>
#include <Library/HiiLib.h>
#include "Utility.h"

//
// Macros
//


//
// Typedef
//

typedef struct {
  UINT32                  BinaryLength;
  EFI_HII_PACKAGE_HEADER  PackageHeader;
} TIANO_AUTOGEN_PACKAGES_HEADER;

#define EFI_HII_THUNK_PRIVATE_DATA_FROM_THIS(Record)   CR(Record, EFI_HII_THUNK_PRIVATE_DATA, Hii, EFI_HII_THUNK_DRIVER_DATA_SIGNATURE)
#define EFI_HII_THUNK_DRIVER_DATA_SIGNATURE            EFI_SIGNATURE_32 ('H', 'i', 'I', 'T')
typedef struct {
  UINTN                    Signature;
  EFI_HANDLE               Handle;
  EFI_HII_PROTOCOL         Hii;
  FRAMEWORK_EFI_HII_HANDLE StaticHiiHandle;

  //
  // This LIST_ENTRY is the list head which has HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY type 
  // as list entry.
  //
  LIST_ENTRY               HiiThunkHandleMappingDBListHead;
  
} EFI_HII_THUNK_PRIVATE_DATA;


#define HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_FROM_LISTENTRY(Record) CR(Record, HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY, List, HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_SIGNATURE)
#define HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_SIGNATURE            EFI_SIGNATURE_32 ('H', 'T', 'H', 'M')
typedef struct {
  LIST_ENTRY                List;
  UINT32                    Signature;
  FRAMEWORK_EFI_HII_HANDLE  FrameworkHiiHandle;
  EFI_HII_HANDLE            UefiHiiHandle;

  BOOLEAN                   IsPackageListWithOnlyStringPackages;
  //
  // The field below is only valid if IsPackageListWithOnlyStringPack is TRUE.
  // The HII 0.92 version of HII data implementation in EDK 1.03 and 1.04 make an the following assumption
  // in both HII Database implementation and all modules that registering packages:
  // If a Package List has only IFR package and no String Package, the String Package containing the strings 
  // referenced by this IFR package is in another Package List
  // registered with the HII database with the same EFI_HII_PACKAGES.GuidId.
  //
  //
  // Only valid if IsPackageListWithSingleStringPack is TRUE.
  // UEFI Package List Head Pointer, pointing to a allocated buffer containing the package
  //
  EFI_HII_PACKAGE_LIST_HEADER *UefiStringPackageListHeader; //Only valid if IsStringPack is TRUE.
                                                            //This UEFI Package list only consists of a list of string packages.

  EFI_GUID                   TagGuid;
  //
  // TRUE if the package list identified by UefiHiiHandle imports String Packages from 
  // other package list with IsPackageListWithOnlyStringPackages is TRUE.
  //
  BOOLEAN                    DoesPackageListImportStringPackages;
  EFI_HII_PACKAGE_LIST_HEADER *ImportedUefiStringPackageListHeader; //Only valid if DoesPackageListImportStringPackages is true.

} HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY;

//
// Extern Variables
//
extern EFI_HII_DATABASE_PROTOCOL *mUefiHiiDatabaseProtocol;
extern EFI_HII_FONT_PROTOCOL     *mUefiHiiFontProtocol;
extern EFI_HII_IMAGE_PROTOCOL    *mUefiHiiImageProtocol;
extern EFI_HII_STRING_PROTOCOL   *mUefiStringProtocol;

//
// Prototypes
//

//
// Public Interface Prototypes
//
EFI_STATUS
EFIAPI
InitializeHiiDatabase (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
;

EFI_STATUS
EFIAPI
HiiNewPack (
  IN  EFI_HII_PROTOCOL      *This,
  IN  EFI_HII_PACKAGES      *PackageList,
  OUT FRAMEWORK_EFI_HII_HANDLE         *Handle
  )
;

EFI_STATUS
EFIAPI
HiiRemovePack (
  IN EFI_HII_PROTOCOL    *This,
  IN FRAMEWORK_EFI_HII_HANDLE       Handle
  )
;

EFI_STATUS
EFIAPI
HiiFindHandles (
  IN     EFI_HII_PROTOCOL    *This,
  IN OUT UINT16              *HandleBufferLength,
  OUT    FRAMEWORK_EFI_HII_HANDLE       *Handle
  )
;

EFI_STATUS
EFIAPI
HiiExportDatabase (
  IN     EFI_HII_PROTOCOL *This,
  IN     FRAMEWORK_EFI_HII_HANDLE    Handle,
  IN OUT UINTN            *BufferSize,
  OUT    VOID             *Buffer
  )
;

EFI_STATUS
EFIAPI
HiiGetGlyph (
  IN     EFI_HII_PROTOCOL    *This,
  IN     CHAR16              *Source,
  IN OUT UINT16              *Index,
  OUT    UINT8               **GlyphBuffer,
  OUT    UINT16              *BitWidth,
  IN OUT UINT32              *InternalStatus
  )
;

EFI_STATUS
EFIAPI
HiiGlyphToBlt (
  IN     EFI_HII_PROTOCOL              *This,
  IN     UINT8                         *GlyphBuffer,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background,
  IN     UINTN                         Count,
  IN     UINTN                         Width,
  IN     UINTN                         Height,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer
  )
;

EFI_STATUS
EFIAPI
HiiNewString (
  IN     EFI_HII_PROTOCOL        *This,
  IN     CHAR16                  *Language,
  IN     FRAMEWORK_EFI_HII_HANDLE Handle,
  IN OUT STRING_REF              *Reference,
  IN     CHAR16                  *NewString
  )
;

EFI_STATUS
EFIAPI
HiiGetString (
  IN     EFI_HII_PROTOCOL    *This,
  IN     FRAMEWORK_EFI_HII_HANDLE       Handle,
  IN     STRING_REF          Token,
  IN     BOOLEAN             Raw,
  IN     CHAR16              *LanguageString,
  IN OUT UINTN               *BufferLength,
  OUT    EFI_STRING          StringBuffer
  )
;

EFI_STATUS
EFIAPI
HiiResetStrings (
  IN     EFI_HII_PROTOCOL    *This,
  IN     FRAMEWORK_EFI_HII_HANDLE       Handle
  )
;

EFI_STATUS
EFIAPI
HiiTestString (
  IN     EFI_HII_PROTOCOL    *This,
  IN     CHAR16              *StringToTest,
  IN OUT UINT32              *FirstMissing,
  OUT    UINT32              *GlyphBufferSize
  )
;

EFI_STATUS
EFIAPI
HiiGetPrimaryLanguages (
  IN  EFI_HII_PROTOCOL      *This,
  IN  FRAMEWORK_EFI_HII_HANDLE         Handle,
  OUT EFI_STRING            *LanguageString
  )
;

EFI_STATUS
EFIAPI
HiiGetSecondaryLanguages (
  IN  EFI_HII_PROTOCOL      *This,
  IN  FRAMEWORK_EFI_HII_HANDLE         Handle,
  IN  CHAR16                *PrimaryLanguage,
  OUT EFI_STRING            *LanguageString
  )
;

EFI_STATUS
EFIAPI
HiiGetLine (
  IN     EFI_HII_PROTOCOL    *This,
  IN     FRAMEWORK_EFI_HII_HANDLE       Handle,
  IN     STRING_REF          Token,
  IN OUT UINT16              *Index,
  IN     UINT16              LineWidth,
  IN     CHAR16              *LanguageString,
  IN OUT UINT16              *BufferLength,
  OUT    EFI_STRING          StringBuffer
  )
;

EFI_STATUS
EFIAPI
HiiGetForms (
  IN     EFI_HII_PROTOCOL    *This,
  IN     FRAMEWORK_EFI_HII_HANDLE       Handle,
  IN     EFI_FORM_ID         FormId,
  IN OUT UINTN               *BufferLength,
  OUT    UINT8               *Buffer
  )
;

EFI_STATUS
EFIAPI
HiiGetDefaultImage (
  IN     EFI_HII_PROTOCOL           *This,
  IN     FRAMEWORK_EFI_HII_HANDLE   Handle,
  IN     UINTN                      DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST **VariablePackList
  )
;

EFI_STATUS
EFIAPI
HiiUpdateForm (
  IN EFI_HII_PROTOCOL       *This,
  IN FRAMEWORK_EFI_HII_HANDLE          Handle,
  IN EFI_FORM_LABEL         Label,
  IN BOOLEAN                AddData,
  IN EFI_HII_UPDATE_DATA    *Data
  )
;

EFI_STATUS
EFIAPI
HiiGetKeyboardLayout (
  IN     EFI_HII_PROTOCOL    *This,
  OUT    UINT16              *DescriptorCount,
  OUT    FRAMEWORK_EFI_KEY_DESCRIPTOR  *Descriptor
  )
;

EFI_STATUS
HiiCompareLanguage (
  IN  CHAR16                *LanguageStringLocation,
  IN  CHAR16                *Language
  )
;

#endif
