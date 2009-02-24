/**@file

  This file contains global defines and prototype definitions
  for the Framework HII to Uefi HII Thunk Module.
  
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
#include <Protocol/UgaDraw.h>


#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/ExtendedHiiLib.h>
#include <Library/UefiLib.h>

#include <Library/IfrSupportLib.h>
#include <Library/ExtendedIfrSupportLib.h>
#include <Library/PcdLib.h>

#include <Guid/MdeModuleHii.h>

#include "UefiIfrParser.h"


//
// VARSTORE ID of 0 for Buffer Storage Type Storage is defined as invalid in UEFI 2.1 HII. VARSTORE ID
// 0 is the default VarStore ID for storage without explicit declaration in Framework HII 0.92. EDK II UEFI VFR compiler
// in compatible mode will assign 0x0001 as UEFI VARSTORE ID to this default storage id in Framework VFR without
// VARSTORE declaration.
// 
// In addition, the Name of Default VarStore is assumed to be L"Setup" for those storage without explicit VARSTORE declaration in the formset
// by Framework HII. EDK II UEFI VFR compiler in compatible mode hard-coded L"Setup" as VARSTORE name.
//
#define FRAMEWORK_RESERVED_VARSTORE_ID 0x0001
#define FRAMEWORK_RESERVED_VARSTORE_NAME L"Setup"


#pragma pack (1)
typedef struct {
  EFI_HII_PACK_HEADER     FrameworkPackageHeader;
  EFI_HII_PACKAGE_HEADER  PackageHeader;
} TIANO_AUTOGEN_PACKAGES_HEADER;
#pragma pack ()

#define HII_THUNK_PRIVATE_DATA_FROM_THIS(Record)  CR(Record, HII_THUNK_PRIVATE_DATA, Hii, HII_THUNK_PRIVATE_DATA_SIGNATURE)
#define HII_THUNK_PRIVATE_DATA_SIGNATURE            SIGNATURE_32 ('H', 'i', 'I', 'T')
typedef struct {
  UINTN                    Signature;
  EFI_HANDLE               Handle;
  EFI_HII_PROTOCOL         Hii;

  //
  // The head of link list for all HII_THUNK_CONTEXT.
  //
  LIST_ENTRY               ThunkContextListHead;

  EFI_HANDLE               RemovePackNotifyHandle;
  EFI_HANDLE               AddPackNotifyHandle;
} HII_THUNK_PRIVATE_DATA;





#define QUESTION_ID_MAP_ENTRY_FROM_LINK(Record) CR(Record, QUESTION_ID_MAP_ENTRY, Link, QUESTION_ID_MAP_ENTRY_SIGNATURE)
#define QUESTION_ID_MAP_ENTRY_SIGNATURE            SIGNATURE_32 ('Q', 'I', 'M', 'E')
typedef struct {
  UINT32            Signature;
  LIST_ENTRY        Link;
  UINT16            FwQId;
  EFI_QUESTION_ID   UefiQid;
} QUESTION_ID_MAP_ENTRY;



#define QUESTION_ID_MAP_FROM_LINK(Record) CR(Record, QUESTION_ID_MAP, Link, QUESTION_ID_MAP_SIGNATURE)
#define QUESTION_ID_MAP_SIGNATURE            SIGNATURE_32 ('Q', 'I', 'M', 'P')
typedef struct {
  UINT32            Signature;
  LIST_ENTRY        Link;
  UINT16            VarStoreId;
  UINTN             VarSize;
  LIST_ENTRY        MapEntryListHead;
} QUESTION_ID_MAP;



#define HII_THUNK_CONTEXT_FROM_LINK(Record) CR(Record, HII_THUNK_CONTEXT, Link, HII_THUNK_CONTEXT_SIGNATURE)
#define HII_THUNK_CONTEXT_SIGNATURE            SIGNATURE_32 ('H', 'T', 'H', 'M')
typedef struct {
  LIST_ENTRY                Link;
  UINT32                    Signature;
  FRAMEWORK_EFI_HII_HANDLE  FwHiiHandle;
  EFI_HII_HANDLE            UefiHiiHandle;
  EFI_HANDLE                UefiHiiDriverHandle;

  UINTN                     IfrPackageCount;
  UINTN                     StringPackageCount;

  BOOLEAN                   ByFrameworkHiiNewPack;

  //
  // HII Thunk will use TagGuid to associate the String Package and Form Package togehter.
  // See description for TagGuid. This field is to record if either one of the following condition 
  // is TRUE:
  // 1) if ((SharingStringPack == TRUE) && (StringPackageCount != 0 && IfrPackageCount == 0)), then this Package List only 
  ///   has String Packages and provides Strings to other IFR package.
  // 2) if ((SharingStringPack == TRUE) && (StringPackageCount == 0 && IfrPackageCount != 1)), then this Form Package
  //    copied String Packages from other Package List.
  // 3) if ((SharingStringPack == FALSE)), this Package does not provide String Package or copy String Packages from other
  //    Package List.
  //
  //
  // When a Hii->NewString() is called for this FwHiiHandle and SharingStringPack is TRUE, then all Package List that sharing
  // the same TagGuid will update or create String in there respective String Packages. If SharingStringPack is FALSE, then
  // only the String from String Packages in this Package List will be updated or created.
  //
  BOOLEAN                   SharingStringPack;

  //
  // The HII 0.92 version of HII data implementation in EDK 1.03 and 1.04 make an the following assumption
  // in both HII Database implementation and all modules that registering packages:
  // If a Package List has only IFR package and no String Package, the IFR package will reference 
  // String in another Package List registered with the HII database with the same EFI_HII_PACKAGES.GuidId.
  // TagGuid is the used to record this GuidId.
  EFI_GUID                   TagGuid;

  UINT8                      *NvMapOverride;

  FORM_BROWSER_FORMSET       *FormSet;

} HII_THUNK_CONTEXT;



#define BUFFER_STORAGE_ENTRY_SIGNATURE              SIGNATURE_32 ('H', 'T', 's', 'k')
#define BUFFER_STORAGE_ENTRY_FROM_LINK(Record) CR(Record, BUFFER_STORAGE_ENTRY, Link, BUFFER_STORAGE_ENTRY_SIGNATURE)
typedef struct {
  LIST_ENTRY Link;
  UINT32     Signature;
  EFI_GUID   Guid;
  CHAR16     *Name;
  UINTN      Size;
  UINT16     VarStoreId;
} BUFFER_STORAGE_ENTRY;



#define CONFIG_ACCESS_PRIVATE_SIGNATURE            SIGNATURE_32 ('H', 'T', 'c', 'a')
#define CONFIG_ACCESS_PRIVATE_FROM_PROTOCOL(Record) CR(Record, CONFIG_ACCESS_PRIVATE, ConfigAccessProtocol, CONFIG_ACCESS_PRIVATE_SIGNATURE)
typedef struct {
  UINT32                         Signature;
  EFI_HII_CONFIG_ACCESS_PROTOCOL ConfigAccessProtocol;
  //
  // Framework's callback
  //
  EFI_FORM_CALLBACK_PROTOCOL     *FormCallbackProtocol;

  HII_THUNK_CONTEXT              *ThunkContext;
} CONFIG_ACCESS_PRIVATE;



#define EFI_FORMBROWSER_THUNK_PRIVATE_DATA_SIGNATURE            SIGNATURE_32 ('F', 'B', 'T', 'd')
#define EFI_FORMBROWSER_THUNK_PRIVATE_DATA_FROM_THIS(Record)   CR(Record, EFI_FORMBROWSER_THUNK_PRIVATE_DATA, FormBrowser, EFI_FORMBROWSER_THUNK_PRIVATE_DATA_SIGNATURE)
typedef struct {
  UINTN                     Signature;
  EFI_HANDLE                Handle;
  HII_THUNK_PRIVATE_DATA    *ThunkPrivate;
  EFI_FORM_BROWSER_PROTOCOL FormBrowser;
} EFI_FORMBROWSER_THUNK_PRIVATE_DATA;


//
// Extern Variables
//
extern CONST EFI_HII_DATABASE_PROTOCOL            *mHiiDatabase;
extern CONST EFI_HII_IMAGE_PROTOCOL               *mHiiImageProtocol;
extern CONST EFI_HII_STRING_PROTOCOL              *mHiiStringProtocol;
extern CONST EFI_HII_FONT_PROTOCOL                *mHiiFontProtocol;
extern CONST EFI_HII_CONFIG_ROUTING_PROTOCOL      *mHiiConfigRoutingProtocol;
extern CONST EFI_FORM_BROWSER2_PROTOCOL           *mFormBrowser2Protocol;

extern HII_THUNK_PRIVATE_DATA                     *mHiiThunkPrivateData;

extern BOOLEAN                                    mInFrameworkUpdatePakcage;


EFI_STATUS
EFIAPI
HiiNewPack (
  IN  EFI_HII_PROTOCOL              *This,
  IN  EFI_HII_PACKAGES              *PackageList,
  OUT FRAMEWORK_EFI_HII_HANDLE      *Handle
  );

EFI_STATUS
EFIAPI
HiiRemovePack (
  IN EFI_HII_PROTOCOL               *This,
  IN FRAMEWORK_EFI_HII_HANDLE       Handle
  );

EFI_STATUS
EFIAPI
HiiFindHandles (
  IN     EFI_HII_PROTOCOL           *This,
  IN OUT UINT16                     *HandleBufferLength,
  OUT    FRAMEWORK_EFI_HII_HANDLE   *Handle
  );

EFI_STATUS
EFIAPI
HiiExportDatabase (
  IN     EFI_HII_PROTOCOL           *This,
  IN     FRAMEWORK_EFI_HII_HANDLE   Handle,
  IN OUT UINTN                      *BufferSize,
  OUT    VOID                       *Buffer
  );

EFI_STATUS
EFIAPI
HiiGetGlyph (
  IN     EFI_HII_PROTOCOL           *This,
  IN     CHAR16                     *Source,
  IN OUT UINT16                     *Index,
  OUT    UINT8                      **GlyphBuffer,
  OUT    UINT16                     *BitWidth,
  IN OUT UINT32                     *InternalStatus
  );

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
  );

EFI_STATUS
EFIAPI
HiiNewString (
  IN     EFI_HII_PROTOCOL        *This,
  IN     CHAR16                  *Language,
  IN     FRAMEWORK_EFI_HII_HANDLE Handle,
  IN OUT STRING_REF              *Reference,
  IN     CHAR16                  *NewString
  );

EFI_STATUS
EFIAPI
HiiGetString (
  IN     EFI_HII_PROTOCOL           *This,
  IN     FRAMEWORK_EFI_HII_HANDLE   Handle,
  IN     STRING_REF                 Token,
  IN     BOOLEAN                    Raw,
  IN     CHAR16                     *LanguageString,
  IN OUT UINTN                      *BufferLength,
  OUT    EFI_STRING                 StringBuffer
  );

EFI_STATUS
EFIAPI
HiiResetStrings (
  IN     EFI_HII_PROTOCOL           *This,
  IN     FRAMEWORK_EFI_HII_HANDLE   Handle
  );

EFI_STATUS
EFIAPI
HiiTestString (
  IN     EFI_HII_PROTOCOL           *This,
  IN     CHAR16                     *StringToTest,
  IN OUT UINT32                     *FirstMissing,
  OUT    UINT32                     *GlyphBufferSize
  );

EFI_STATUS
EFIAPI
HiiGetPrimaryLanguages (
  IN  EFI_HII_PROTOCOL              *This,
  IN  FRAMEWORK_EFI_HII_HANDLE      Handle,
  OUT EFI_STRING                    *LanguageString
  );

EFI_STATUS
EFIAPI
HiiGetSecondaryLanguages (
  IN  EFI_HII_PROTOCOL                *This,
  IN  FRAMEWORK_EFI_HII_HANDLE        Handle,
  IN  CHAR16                          *PrimaryLanguage,
  OUT EFI_STRING                      *LanguageString
  );

EFI_STATUS
EFIAPI
HiiGetLine (
  IN     EFI_HII_PROTOCOL               *This,
  IN     FRAMEWORK_EFI_HII_HANDLE       Handle,
  IN     STRING_REF                     Token,
  IN OUT UINT16                         *Index,
  IN     UINT16                         LineWidth,
  IN     CHAR16                         *LanguageString,
  IN OUT UINT16                         *BufferLength,
  OUT    EFI_STRING                     StringBuffer
  );

EFI_STATUS
EFIAPI
HiiGetForms (
  IN     EFI_HII_PROTOCOL               *This,
  IN     FRAMEWORK_EFI_HII_HANDLE       Handle,
  IN     EFI_FORM_ID                    FormId,
  IN OUT UINTN                          *BufferLength,
  OUT    UINT8                          *Buffer
  );

EFI_STATUS
EFIAPI
HiiGetDefaultImage (
  IN     EFI_HII_PROTOCOL               *This,
  IN     FRAMEWORK_EFI_HII_HANDLE       Handle,
  IN     UINTN                          DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST     **VariablePackList
  );

EFI_STATUS
EFIAPI
HiiUpdateForm (
  IN EFI_HII_PROTOCOL                   *This,
  IN FRAMEWORK_EFI_HII_HANDLE           Handle,
  IN EFI_FORM_LABEL                     Label,
  IN BOOLEAN                            AddData,
  IN FRAMEWORK_EFI_HII_UPDATE_DATA      *Data
  );

EFI_STATUS
EFIAPI
HiiGetKeyboardLayout (
  IN     EFI_HII_PROTOCOL               *This,
  OUT    UINT16                         *DescriptorCount,
  OUT    FRAMEWORK_EFI_KEY_DESCRIPTOR   *Descriptor
  );

EFI_STATUS
EFIAPI 
ThunkSendForm (
  IN  EFI_FORM_BROWSER_PROTOCOL       *This,
  IN  BOOLEAN                         UseDatabase,
  IN  FRAMEWORK_EFI_HII_HANDLE        *Handle,
  IN  UINTN                           HandleCount,
  IN  FRAMEWORK_EFI_IFR_PACKET        *Packet, OPTIONAL
  IN  EFI_HANDLE                      CallbackHandle, OPTIONAL
  IN  UINT8                           *NvMapOverride, OPTIONAL
  IN  FRAMEWORK_EFI_SCREEN_DESCRIPTOR *ScreenDimensions, OPTIONAL
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

EFI_STATUS
EFIAPI
RemovePackNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  );

EFI_STATUS
EFIAPI
NewOrAddPackNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  );

/**
  Create a EFI_HII_UPDATE_DATA structure used to call IfrLibUpdateForm.

  @param ThunkContext   The HII Thunk Context.
  @param FwUpdateData   The Framework Update Data.
  @param UefiUpdateData The UEFI Update Data.

  @retval EFI_SUCCESS       The UEFI Update Data is created successfully.
  @retval EFI_UNSUPPORTED   There is unsupported opcode in FwUpdateData.
  @retval EFI_OUT_OF_RESOURCES There is not enough resource.
**/
EFI_STATUS
FwUpdateDataToUefiUpdateData (
  IN       HII_THUNK_CONTEXT                *ThunkContext,
  IN CONST FRAMEWORK_EFI_HII_UPDATE_DATA    *FwUpdateData,
  OUT      EFI_HII_UPDATE_DATA              **UefiUpdateData
  )
;

/** 

  Initialize string packages in HII database.

**/
VOID
InitSetBrowserStrings (
  VOID
  )
;

#include "Utility.h"
#include "ConfigAccess.h"

#endif
