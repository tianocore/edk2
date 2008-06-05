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
#include <Protocol/FrameworkFormBrowser.h>

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
//#include <Library/FrameworkIfrSupportLib.h>
#include <Library/HiiLib.h>
#include <Library/ExtendedHiiLib.h>

#include <Library/IfrSupportLib.h>
#include <Library/ExtendedIfrSupportLib.h>

#include <MdeModuleHii.h>

//
// Macros
//


//
// Typedef
//

#pragma pack (push, 1)
typedef struct {
  UINT32                  BinaryLength;
  EFI_HII_PACKAGE_HEADER  PackageHeader;
} TIANO_AUTOGEN_PACKAGES_HEADER;
#pragma pack (pop)

#define EFI_HII_THUNK_PRIVATE_DATA_FROM_THIS(Record)   CR(Record, EFI_HII_THUNK_PRIVATE_DATA, Hii, EFI_HII_THUNK_DRIVER_DATA_SIGNATURE)
#define EFI_HII_THUNK_DRIVER_DATA_SIGNATURE            EFI_SIGNATURE_32 ('H', 'i', 'I', 'T')
typedef struct {
  UINTN                    Signature;
  EFI_HANDLE               Handle;
  EFI_HII_PROTOCOL         Hii;
  FRAMEWORK_EFI_HII_HANDLE StaticHiiHandle;
  FRAMEWORK_EFI_HII_HANDLE StaticPureUefiHiiHandle;

  //
  // This LIST_ENTRY is the list head which has HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY type 
  // as list entry.
  //
  LIST_ENTRY               HiiThunkHandleMappingDBListHead;

  EFI_HANDLE               NewPackNotifyHandle;
  EFI_HANDLE               RemovePackNotifyHandle;
  EFI_HANDLE               AddPackNotifyHandle;
} EFI_HII_THUNK_PRIVATE_DATA;


#define HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_FROM_LISTENTRY(Record) CR(Record, HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY, List, HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_SIGNATURE)
#define HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_SIGNATURE            EFI_SIGNATURE_32 ('H', 'T', 'H', 'M')
typedef struct {
  LIST_ENTRY                List;
  UINT32                    Signature;
  FRAMEWORK_EFI_HII_HANDLE  FrameworkHiiHandle;
  EFI_HII_HANDLE            UefiHiiHandle;
  EFI_HANDLE                UefiHiiDriverHandle;

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
  
} HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY;

#define HII_TRHUNK_BUFFER_STORAGE_KEY_SIGNATURE              EFI_SIGNATURE_32 ('H', 'T', 's', 'k')
#define HII_TRHUNK_BUFFER_STORAGE_KEY_FROM_LIST_ENTRY(Record) CR(Record, HII_TRHUNK_BUFFER_STORAGE_KEY, List, HII_TRHUNK_BUFFER_STORAGE_KEY_SIGNATURE)
typedef struct {
  LIST_ENTRY List;
  UINT32     Signature;
  EFI_GUID   Guid;
  CHAR16     *Name;
  UINTN      Size;
  UINT16     VarStoreId;
} HII_TRHUNK_BUFFER_STORAGE_KEY;

#define HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE_SIGNATURE            EFI_SIGNATURE_32 ('H', 'T', 'c', 'a')
#define HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE_FROM_PROTOCOL(Record) CR(Record, HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE, ConfigAccessProtocol, HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE_SIGNATURE)
typedef struct {
  UINT32                         Signature;
  EFI_HII_CONFIG_ACCESS_PROTOCOL ConfigAccessProtocol;
  EFI_FORM_CALLBACK_PROTOCOL     *FrameworkFormCallbackProtocol;
  LIST_ENTRY                     ConfigAccessBufferStorageListHead;
} HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE;

#define EFI_FORMBROWSER_THUNK_PRIVATE_DATA_SIGNATURE            EFI_SIGNATURE_32 ('F', 'B', 'T', 'd')
typedef struct {
  UINTN                     Signature;
  EFI_HANDLE                Handle;
  EFI_FORM_BROWSER_PROTOCOL FormBrowser;
} EFI_FORMBROWSER_THUNK_PRIVATE_DATA;


//
// Extern Variables
//
extern CONST EFI_HII_DATABASE_PROTOCOL            *mHiiDatabase;
extern CONST EFI_HII_FONT_PROTOCOL                *mHiiFontProtocol;
extern CONST EFI_HII_IMAGE_PROTOCOL               *mHiiImageProtocol;
extern CONST EFI_HII_STRING_PROTOCOL              *mHiiStringProtocol;
extern CONST EFI_HII_CONFIG_ROUTING_PROTOCOL      *mHiiConfigRoutingProtocol;

extern BOOLEAN                                    mInFrameworkHiiNewPack;
extern BOOLEAN                                    mInFrameworkHiiRemovePack;


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
  IN FRAMEWORK_EFI_HII_UPDATE_DATA    *Data
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



EFI_STATUS
EFIAPI 
ThunkSendForm (
  IN  EFI_FORM_BROWSER_PROTOCOL       *This,
  IN  BOOLEAN                         UseDatabase,
  IN  FRAMEWORK_EFI_HII_HANDLE        *Handle,
  IN  UINTN                           HandleCount,
  IN  FRAMEWORK_EFI_IFR_PACKET                  *Packet, OPTIONAL
  IN  EFI_HANDLE                      CallbackHandle, OPTIONAL
  IN  UINT8                           *NvMapOverride, OPTIONAL
  IN  FRAMEWORK_EFI_SCREEN_DESCRIPTOR            *ScreenDimensions, OPTIONAL
  OUT BOOLEAN                         *ResetRequired OPTIONAL
  );

EFI_STATUS
EFIAPI 
ThunkCreatePopUp (
  IN  UINTN                           NumberOfLines,
  IN  BOOLEAN                         HotKey,
  IN  UINTN                           MaximumStringSize,
  OUT CHAR16                          *StringBuffer,
  OUT EFI_INPUT_KEY                   *KeyValue,
  IN  CHAR16                          *String,
  ...
  );

#include "Utility.h"
#include "ConfigAccess.h"

#endif
