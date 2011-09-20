/** @file

  This file contains global defines and prototype definitions
  for the Framework HII to Uefi HII Thunk Module.
  
Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HIIDATABASE_H_
#define _HIIDATABASE_H_


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
#include <Guid/HiiFormMapMethodGuid.h>
#include <Guid/FrameworkBdsFrontPageFormSet.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/LanguageLib.h>
#include <Library/PrintLib.h>

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

///
/// The size of a 3 character ISO639 language code.
///
#define ISO_639_2_ENTRY_SIZE            3

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

#pragma pack(1)
///
/// HII specific Vendor Device Path Node definition.
///
typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  UINT32                         Reserved;
  UINT64                         UniqueId;
} HII_VENDOR_DEVICE_PATH_NODE;

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  HII_VENDOR_DEVICE_PATH_NODE    Node;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;
#pragma pack()

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


/**

  Registers the various packages that are passed in a Package List.

  @param This      Pointer of Frameowk HII protocol instance.
  @param Packages  Pointer of HII packages.
  @param Handle    Handle value to be returned.

  @retval EFI_SUCCESS           Pacakges has added to HII database successfully.
  @retval EFI_INVALID_PARAMETER If Handle or Packages is NULL.

**/
EFI_STATUS
EFIAPI
HiiNewPack (
  IN  EFI_HII_PROTOCOL               *This,
  IN  EFI_HII_PACKAGES               *Packages,
  OUT FRAMEWORK_EFI_HII_HANDLE       *Handle
  );

/**

  Remove a package from the HII database.

  @param This      Pointer of Frameowk HII protocol instance.
  @param Handle    Handle value to be removed.

  @retval EFI_SUCCESS           Pacakges has added to HII database successfully.
  @retval EFI_INVALID_PARAMETER If Handle or Packages is NULL.

**/
EFI_STATUS
EFIAPI
HiiRemovePack (
  IN EFI_HII_PROTOCOL               *This,
  IN FRAMEWORK_EFI_HII_HANDLE       Handle
  );

/**
  Determines the handles that are currently active in the database.

  This function determines the handles that are currently active in the database. 
  For example, a program wishing to create a Setup-like configuration utility would use this call 
  to determine the handles that are available. It would then use calls defined in the forms section 
  below to extract forms and then interpret them.

  @param This                 A pointer to the EFI_HII_PROTOCOL instance.
  @param HandleBufferLength   On input, a pointer to the length of the handle buffer. 
                              On output, the length of the handle buffer that is required for the handles found.
  @param Handle               Pointer to an array of EFI_HII_HANDLE instances returned. 
                              Type EFI_HII_HANDLE is defined in EFI_HII_PROTOCOL.NewPack() in the Packages section.

  @retval EFI_SUCCESS         Handle was updated successfully.
 
  @retval EFI_BUFFER_TOO_SMALL The HandleBufferLength parameter indicates that Handle is too small 
                               to support the number of handles. HandleBufferLength is updated with a value that 
                               will enable the data to fit.
**/
EFI_STATUS
EFIAPI
HiiFindHandles (
  IN     EFI_HII_PROTOCOL *This,
  IN OUT UINT16           *HandleBufferLength,
  OUT    FRAMEWORK_EFI_HII_HANDLE    *Handle
  );

/**

  This thunk module only handles UEFI HII packages. The caller of this function 
  won't be able to parse the content. Therefore, it is not supported.
  
  This function will ASSERT and return EFI_UNSUPPORTED.

  @param This            N.A.
  @param Handle          N.A.
  @param BufferSize      N.A.
  @param Buffer          N.A.

  @retval EFI_UNSUPPORTED

**/
EFI_STATUS
EFIAPI
HiiExportDatabase (
  IN     EFI_HII_PROTOCOL *This,
  IN     FRAMEWORK_EFI_HII_HANDLE    Handle,
  IN OUT UINTN            *BufferSize,
  OUT    VOID             *Buffer
  );

/**
  Translates a Unicode character into the corresponding font glyph.
  
  Notes:
    This function is only called by Graphics Console module and GraphicsLib. 
    Wrap the Framework HII GetGlyph function to UEFI Font Protocol.
    
    EDK II provides a UEFI Graphics Console module. ECP provides a GraphicsLib 
    complying to UEFI HII.
  
  @param This            A pointer to the EFI_HII_PROTOCOL instance.
  @param Source          A pointer to a Unicode string.
  @param Index           On input, the offset into the string from which to fetch the character. On successful completion, the 
                         index is updated to the first character past the character(s) making up the just extracted glyph. 
  @param GlyphBuffer     Pointer to an array where the glyphs corresponding to the characters in the source may be stored. 
                         GlyphBuffer is assumed to be wide enough to accept a wide glyph character.
  @param BitWidth        If EFI_SUCCESS was returned, the UINT16 pointed to by this value is filled with the length of the glyph in pixels. 
                         It is unchanged if the call was unsuccessful.
  @param InternalStatus  To save the time required to read the string from the beginning on each glyph extraction 
                         (for example, to ensure that the narrow versus wide glyph mode is correct), this value is 
                         updated each time the function is called with the status that is local to the call. The cell pointed 
                         to by this parameter must be initialized to zero prior to invoking the call the first time for any string.

  @retval EFI_SUCCESS     It worked.
  @retval EFI_NOT_FOUND   A glyph for a character was not found.
 

**/
EFI_STATUS
EFIAPI
HiiGetGlyph (
  IN     EFI_HII_PROTOCOL   *This,
  IN     CHAR16             *Source,
  IN OUT UINT16             *Index,
  OUT    UINT8              **GlyphBuffer,
  OUT    UINT16             *BitWidth,
  IN OUT UINT32             *InternalStatus
  );

/**
  Translates a glyph into the format required for input to the Universal Graphics Adapter (UGA) Block Transfer (BLT) routines.
  
  Notes:
  This function is only called by Graphics Console module and GraphicsLib. 
  EDK II provides a UEFI Graphics Console module. ECP provides a GraphicsLib 
  complying to UEFI HII.
  
  @param This         A pointer to the EFI_HII_PROTOCOL instance.
  @param GlyphBuffer  A pointer to the buffer that contains glyph data. 
  @param Foreground   The foreground setting requested to be used for the generated BltBuffer data. Type EFI_UGA_PIXEL is defined in "Related Definitions" below.
  @param Background   The background setting requested to be used for the generated BltBuffer data. 
  @param Count        The entry in the BltBuffer upon which to act.
  @param Width        The width in bits of the glyph being converted.
  @param Height       The height in bits of the glyph being converted
  @param BltBuffer    A pointer to the buffer that contains the data that is ready to be used by the UGA BLT routines. 

  @retval EFI_SUCCESS  It worked.
  @retval EFI_NOT_FOUND A glyph for a character was not found.
 

**/
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

/**
  Create or update a String Token in a String Package.

  If *Reference == 0, a new String Token is created.

  @param This            A pointer to the EFI_HII_PROTOCOL instance.
  @param Language        Pointer to a NULL-terminated string containing a single ISO 639-2 language
                         identifier, indicating the language to print. A string consisting of
                         all spaces indicates that the string is applicable to all languages.
  @param Handle          The handle of the language pack to which the string is to be added.
  @param Reference       The string token assigned to the string.
  @param NewString       The string to be added.


  @retval EFI_SUCCESS             The string was effectively registered.
  @retval EFI_INVALID_PARAMETER   The Handle was unknown. The string is not created or updated in the
                                  the string package.
**/
EFI_STATUS
EFIAPI
HiiNewString (
  IN     EFI_HII_PROTOCOL           *This,
  IN     CHAR16                     *Language,
  IN     FRAMEWORK_EFI_HII_HANDLE   Handle,
  IN OUT STRING_REF                 *Reference,
  IN     CHAR16                     *NewString
  );

/**
 This function extracts a string from a package already registered with the EFI HII database.

  @param This            A pointer to the EFI_HII_PROTOCOL instance.
  @param Handle          The HII handle on which the string resides.
  @param Token           The string token assigned to the string.
  @param Raw             If TRUE, the string is returned unedited in the internal storage format described
                         above. If false, the string returned is edited by replacing <cr> with <space>
                         and by removing special characters such as the <wide> prefix.
  @param LanguageString  Pointer to a NULL-terminated string containing a single ISO 639-2 language
                         identifier, indicating the language to print. If the LanguageString is empty (starts
                         with a NULL), the default system language will be used to determine the language.
  @param BufferLength    Length of the StringBuffer. If the status reports that the buffer width is too
                         small, this parameter is filled with the length of the buffer needed.
  @param StringBuffer    The buffer designed to receive the characters in the string. Type EFI_STRING is
                         defined in String.

  @retval EFI_INVALID_PARAMETER If input parameter is invalid.
  @retval EFI_BUFFER_TOO_SMALL  If the *BufferLength is too small.
  @retval EFI_SUCCESS           Operation is successful.

**/
EFI_STATUS
EFIAPI
HiiThunkGetString (
  IN     EFI_HII_PROTOCOL           *This,
  IN     FRAMEWORK_EFI_HII_HANDLE   Handle,
  IN     STRING_REF                 Token,
  IN     BOOLEAN                    Raw,
  IN     CHAR16                     *LanguageString,
  IN OUT UINTN                      *BufferLength,
  OUT    EFI_STRING                 StringBuffer
  );

/**
  This function removes any new strings that were added after the initial string export for this handle.
  UEFI HII String Protocol does not have Reset String function. This function perform nothing.

  @param This            A pointer to the EFI_HII_PROTOCOL instance.
  @param Handle          The HII handle on which the string resides.

  @retval EFI_SUCCESS    This function is a NOP and always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
HiiResetStrings (
  IN     EFI_HII_PROTOCOL   *This,
  IN     FRAMEWORK_EFI_HII_HANDLE      Handle
  );

/**
  Test if all of the characters in a string have corresponding font characters.

  This is a deprecated API. No Framework HII module is calling it. This function will ASSERT and
  return EFI_UNSUPPORTED.

  @param This            A pointer to the EFI_HII_PROTOCOL instance.
  @param StringToTest    A pointer to a Unicode string.
  @param FirstMissing    A pointer to an index into the string. On input, the index of 
                         the first character in the StringToTest to examine. On exit, the index 
                         of the first character encountered for which a glyph is unavailable. 
                         If all glyphs in the string are available, the index is the index of the terminator 
                         of the string. 
  @param GlyphBufferSize A pointer to a value. On output, if the function returns EFI_SUCCESS, 
                         it contains the amount of memory that is required to store the string? glyph equivalent.

  @retval EFI_UNSUPPORTED  The function performs nothing and return EFI_UNSUPPORTED.
**/
EFI_STATUS
EFIAPI
HiiTestString (
  IN     EFI_HII_PROTOCOL   *This,
  IN     CHAR16             *StringToTest,
  IN OUT UINT32             *FirstMissing,
  OUT    UINT32             *GlyphBufferSize
  );

/**
  Allows a program to determine the primary languages that are supported on a given handle.

  This routine is intended to be used by drivers to query the interface database for supported languages. 
  This routine returns a string of concatenated 3-byte language identifiers, one per string package associated with the handle.

  @param This           A pointer to the EFI_HII_PROTOCOL instance.
  @param Handle         The handle on which the strings reside. Type EFI_HII_HANDLE is defined in EFI_HII_PROTOCOL.NewPack() 
                        in the Packages section.
  @param LanguageString A string allocated by GetPrimaryLanguages() that contains a list of all primary languages 
                        registered on the handle. The routine will not return the three-spaces language identifier used in 
                        other functions to indicate non-language-specific strings.

  @retval EFI_SUCCESS            LanguageString was correctly returned.
 
  @retval EFI_INVALID_PARAMETER  The Handle was unknown.
**/
EFI_STATUS
EFIAPI
HiiGetPrimaryLanguages (
  IN  EFI_HII_PROTOCOL            *This,
  IN  FRAMEWORK_EFI_HII_HANDLE    Handle,
  OUT EFI_STRING                  *LanguageString
  );

/**
  Allows a program to determine which secondary languages are supported on a given handle for a given primary language

  This routine is intended to be used by drivers to query the interface database for supported languages. 
  This routine returns a string of concatenated 3-byte language identifiers, one per string package associated with the handle.

  @param This           A pointer to the EFI_HII_PROTOCOL instance.
  @param Handle         The handle on which the strings reside. Type EFI_HII_HANDLE is defined in EFI_HII_PROTOCOL.NewPack() 
                        in the Packages section.
  @param PrimaryLanguage Pointer to a NULL-terminated string containing a single ISO 639-2 language identifier, indicating 
                         the primary language.
  @param LanguageString  A string allocated by GetSecondaryLanguages() containing a list of all secondary languages registered 
                         on the handle. The routine will not return the three-spaces language identifier used in other functions 
                         to indicate non-language-specific strings, nor will it return the primary language. This function succeeds 
                         but returns a NULL LanguageString if there are no secondary languages associated with the input Handle and 
                         PrimaryLanguage pair. Type EFI_STRING is defined in String.
  
  @retval EFI_SUCCESS            LanguageString was correctly returned.
  @retval EFI_INVALID_PARAMETER  The Handle was unknown.
**/
EFI_STATUS
EFIAPI
HiiGetSecondaryLanguages (
  IN  EFI_HII_PROTOCOL              *This,
  IN  FRAMEWORK_EFI_HII_HANDLE      Handle,
  IN  CHAR16                        *PrimaryLanguage,
  OUT EFI_STRING                    *LanguageString
  );

/**

  This function allows a program to extract a part of a string of not more than a given width.
  With repeated calls, this allows a calling program to extract "lines" of text that fit inside
  columns.  The effort of measuring the fit of strings inside columns is localized to this call.

  This is a deprecated API. No Framework HII module is calling it. This function will ASSERT and
  return EFI_UNSUPPORTED.

  @param This            A pointer to the EFI_HII_PROTOCOL instance.
  @param Handle          The HII handle on which the string resides.
  @param Token           The string token assigned to the string.
  @param Index           On input, the offset into the string where the line is to start.
                         On output, the index is updated to point to beyond the last character returned
                         in the call.
  @param LineWidth       The maximum width of the line in units of narrow glyphs.
  @param LanguageString  Pointer to a NULL-terminated string containing a single ISO 639-2 language
                         identifier, indicating the language to print. If the LanguageString is empty (starts
                         with a NULL), the default system language will be used to determine the language.
  @param BufferLength    Length of the StringBuffer. If the status reports that the buffer width is too
                         small, this parameter is filled with the length of the buffer needed.
  @param StringBuffer    The buffer designed to receive the characters in the string. Type EFI_STRING is
                         defined in String.

  @retval EFI_UNSUPPORTED.
**/
EFI_STATUS
EFIAPI
HiiGetLine (
  IN     EFI_HII_PROTOCOL           *This,
  IN     FRAMEWORK_EFI_HII_HANDLE   Handle,
  IN     STRING_REF                 Token,
  IN OUT UINT16                     *Index,
  IN     UINT16                     LineWidth,
  IN     CHAR16                     *LanguageString,
  IN OUT UINT16                     *BufferLength,
  OUT    EFI_STRING                 StringBuffer
  );

/**
  This function allows a program to extract a form or form package that has
  previously been registered with the EFI HII database.

  In this thunk module, this function will create a IFR Package with only 
  one Formset. Effectively, only the GUID of the Formset is updated and return
  in this IFR package to caller. This is enable the Framework modules which call 
  a API named GetStringFromToken. GetStringFromToken retieves a String based on
  a String Token from a Package List known only by the Formset GUID.
  


  @param This             A pointer to the EFI_HII_PROTOCOL instance.
  @param Handle           Handle on which the form resides. Type FRAMEWORK_EFI_HII_HANDLE  is defined in
                          EFI_HII_PROTOCOL.NewPack() in the Packages section.
  @param FormId           Ignored by this implementation.
  @param BufferLengthTemp On input, the size of input buffer. On output, it
                          is the size of FW_HII_FORMSET_TEMPLATE.
  @param Buffer           The buffer designed to receive the form(s).

  @retval  EFI_SUCCESS            Buffer filled with the requested forms. BufferLength
                                  was updated.
  @retval  EFI_INVALID_PARAMETER  The handle is unknown.
  @retval  EFI_NOT_FOUND          A form on the requested handle cannot be found with the
                                  requested FormId.
  @retval  EFI_BUFFER_TOO_SMALL   The buffer provided was not large enough to allow the form to be stored.

**/
EFI_STATUS
EFIAPI
HiiGetForms (
  IN     EFI_HII_PROTOCOL             *This,
  IN     FRAMEWORK_EFI_HII_HANDLE     Handle,
  IN     EFI_FORM_ID                  FormId,
  IN OUT UINTN                        *BufferLengthTemp,
  OUT    UINT8                        *Buffer
  );

/**

  This function allows a program to extract the NV Image
  that represents the default storage image


  @param This            A pointer to the EFI_HII_PROTOCOL instance.
  @param Handle          The HII handle from which will have default data retrieved.
                         UINTN            - Mask used to retrieve the default image.
  @param DefaultMask     EDES_TODO: Add parameter description
  @param VariablePackList Callee allocated, tightly-packed, link list data
                         structure that contain all default varaible packs
                         from the Hii Database.

  @retval  EFI_NOT_FOUND          If Hii database does not contain any default images.
  @retval  EFI_INVALID_PARAMETER  Invalid input parameter.
  @retval  EFI_SUCCESS            Operation successful.

**/
EFI_STATUS
EFIAPI
HiiGetDefaultImage (
  IN     EFI_HII_PROTOCOL            *This,
  IN     FRAMEWORK_EFI_HII_HANDLE    Handle,
  IN     UINTN                       DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST  **VariablePackList
  );

/**
  This function allows the caller to update a form that has
  previously been registered with the EFI HII database.


  @param This            EDES_TODO: Add parameter description
  @param Handle          Hii Handle associated with the Formset to modify
  @param Label           Update information starting immediately after this label in the IFR
  @param AddData         If TRUE, add data.  If FALSE, remove data
  @param Data            If adding data, this is the pointer to the data to add

  @retval  EFI_SUCCESS  Update success.
  @retval  Other        Update fail.

**/
EFI_STATUS
EFIAPI
HiiThunkUpdateForm (
  IN EFI_HII_PROTOCOL                  *This,
  IN FRAMEWORK_EFI_HII_HANDLE          Handle,
  IN EFI_FORM_LABEL                    Label,
  IN BOOLEAN                           AddData,
  IN EFI_HII_UPDATE_DATA               *Data
  );

/**
  Retrieves the current keyboard layout. 
  This function is not implemented by HII Thunk Module.

  @param This             A pointer to the EFI_HII_PROTOCOL instance. 
  @param DescriptorCount  A pointer to the number of Descriptor entries being described in the keyboard layout being retrieved.
  @param Descriptor       A pointer to a buffer containing an array of EFI_KEY_DESCRIPTOR entries. Each entry will reflect the 
                          definition of a specific physical key. Type EFI_KEY_DESCRIPTOR is defined in "Related Definitions" below.

  @retval  EFI_SUCCESS   The keyboard layout was retrieved successfully.
 
**/
EFI_STATUS
EFIAPI
HiiGetKeyboardLayout (
  IN     EFI_HII_PROTOCOL   *This,
  OUT    UINT16             *DescriptorCount,
  OUT    FRAMEWORK_EFI_KEY_DESCRIPTOR *Descriptor
  );

/**
  This is the Framework Setup Browser interface which displays a FormSet.

  @param This           The EFI_FORM_BROWSER_PROTOCOL context.
  @param UseDatabase    TRUE if the FormSet is from HII database. The Thunk implementation
                        only support UseDatabase is TRUE.
  @param Handle         The Handle buffer.
  @param HandleCount    The number of Handle in the Handle Buffer. It must be 1 for this implementation.
  @param Packet         The pointer to data buffer containing IFR and String package. Not supported.
  @param CallbackHandle Not supported.
  @param NvMapOverride  The buffer is used only when there is no NV variable to define the 
                        current settings and the caller needs to provide to the browser the
                        current settings for the the "fake" NV variable. If used, no saving of
                        an NV variable is possbile. This parameter is also ignored if Handle is NULL.
  @param ScreenDimensions 
                        Allows the browser to be called so that it occupies a portion of the physical 
                        screen instead of dynamically determining the screen dimensions.
  @param ResetRequired  This BOOLEAN value denotes whether a reset is required based on the data that 
                        might have been changed. The ResetRequired parameter is primarily applicable 
                        for configuration applications, and is an optional parameter.

  @retval EFI_SUCCESS             If the Formset is displayed correctly.
  @retval EFI_UNSUPPORTED         If UseDatabase is FALSE or HandleCount is not 1.
  @retval EFI_INVALID_PARAMETER   If the *Handle passed in is not found in the database.
**/
EFI_STATUS
EFIAPI 
ThunkSendForm (
  IN  EFI_FORM_BROWSER_PROTOCOL         *This,
  IN  BOOLEAN                           UseDatabase,
  IN  FRAMEWORK_EFI_HII_HANDLE          *Handle,
  IN  UINTN                             HandleCount,
  IN  EFI_IFR_PACKET                    *Packet, OPTIONAL
  IN  EFI_HANDLE                        CallbackHandle, OPTIONAL
  IN  UINT8                             *NvMapOverride, OPTIONAL
  IN  FRAMEWORK_EFI_SCREEN_DESCRIPTOR   *ScreenDimensions, OPTIONAL
  OUT BOOLEAN                           *ResetRequired OPTIONAL
  );

/** 

  Rountine used to display a generic dialog interface and return 
  the Key or Input from user input.

  @param LinesNumber   The number of lines for the dialog box.
  @param HotKey        Defines if a single character is parsed (TRUE) and returned in KeyValue
                       or if a string is returned in StringBuffer.
  @param MaximumStringSize The maximum size in bytes of a typed-in string.
  @param StringBuffer  On return contains the typed-in string if HotKey is FALSE.
  @param Key           The EFI_INPUT_KEY value returned if HotKey is TRUE.
  @param FirstString   The pointer to the first string in the list of strings
                       that comprise the dialog box.
  @param ...           A series of NumberOfLines text strings that will be used
                       to construct the dialog box.
  @retval EFI_SUCCESS  The dialog is created successfully and user interaction was received.
  @retval EFI_DEVICE_ERROR The user typed in an ESC.
  @retval EFI_INVALID_PARAMETER One of the parameters was invalid.(StringBuffer == NULL && HotKey == FALSE).
**/
EFI_STATUS
EFIAPI 
ThunkCreatePopUp (
  IN  UINTN                           LinesNumber,
  IN  BOOLEAN                         HotKey,
  IN  UINTN                           MaximumStringSize,
  OUT CHAR16                          *StringBuffer,
  OUT EFI_INPUT_KEY                   *Key,
  IN  CHAR16                          *FirstString,
  ...
  );

/**
  This notification function will be called when a Package List is removed
  using UEFI HII interface. The Package List removed need to be removed from
  Framework Thunk module too.

  If the Package List registered is not Sting Package, 
  then ASSERT. If the NotifyType is not REMOVE_PACK, then ASSERT.
  Both cases means UEFI HII Database itself is buggy. 

  @param PackageType The Package Type.
  @param PackageGuid The Package GUID.
  @param Package     The Package Header.
  @param Handle      The HII Handle of this Package List.
  @param NotifyType  The reason of the notification. 

  @retval EFI_SUCCESS The notification function is successful.
  
**/
EFI_STATUS
EFIAPI
RemovePackNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  );

/**
  This notification function will be called when a Package List is registered
  using UEFI HII interface. The Package List registered need to be recorded in
  Framework Thunk module as Thunk Module may need to look for String Package in
  the package registered.

  If the Package List registered is not either Sting Package or IFR package, 
  then ASSERT. If the NotifyType is not ADD_PACK or NEW_PACK, then ASSERT.
  Both cases means UEFI HII Database itself is buggy. 

  @param PackageType The Package Type.
  @param PackageGuid The Package GUID.
  @param Package     The Package Header.
  @param Handle      The HII Handle of this Package List.
  @param NotifyType  The reason of the notification. 

  @retval EFI_SUCCESS The notification function is successful.
  
**/
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
  Create a Hii Update data Handle used to call IfrLibUpdateForm.

  @param ThunkContext         The HII Thunk Context.
  @param FwUpdateData         The Framework Update Data.
  @param UefiOpCodeHandle     The UEFI Update Data.

  @retval EFI_SUCCESS       The UEFI Update Data is created successfully.
  @retval EFI_UNSUPPORTED   There is unsupported opcode in FwUpdateData.
  @retval EFI_OUT_OF_RESOURCES There is not enough resource.
**/
EFI_STATUS
FwUpdateDataToUefiUpdateData (
  IN       HII_THUNK_CONTEXT    *ThunkContext,
  IN CONST EFI_HII_UPDATE_DATA  *FwUpdateData,
  IN       VOID                 *UefiOpCodeHandle
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

/**
  Convert language code from RFC4646 to ISO639-2.

  LanguageRfc4646 contain a single RFC 4646 code such as
  "en-US" or "fr-FR".

  The LanguageRfc4646 must be a buffer large enough
  for ISO_639_2_ENTRY_SIZE characters.

  If LanguageRfc4646 is NULL, then ASSERT.
  If LanguageIso639 is NULL, then ASSERT.

  @param  LanguageRfc4646        RFC4646 language code.
  @param  LanguageIso639         ISO639-2 language code.

  @retval EFI_SUCCESS            Language code converted.
  @retval EFI_NOT_FOUND          Language code not found.

**/
EFI_STATUS
EFIAPI
ConvertRfc4646LanguageToIso639Language (
  IN  CHAR8   *LanguageRfc4646,
  OUT CHAR8   *LanguageIso639
  )
;

/**
  Convert language code from ISO639-2 to RFC4646 and return the converted language.
  Caller is responsible for freeing the allocated buffer.

  LanguageIso639 contain a single ISO639-2 code such as
  "eng" or "fra".

  If LanguageIso639 is NULL, then ASSERT.
  If LanguageRfc4646 is NULL, then ASSERT.

  @param  LanguageIso639         ISO639-2 language code.

  @return the allocated buffer or NULL, if the language is not found.

**/
CHAR8*
EFIAPI
ConvertIso639LanguageToRfc4646Language (
  IN  CONST CHAR8   *LanguageIso639
  )
;

/**
  Get next language from language code list (with separator ';').

  If LangCode is NULL, then ASSERT.
  If Lang is NULL, then ASSERT.

  @param  LangCode    On input: point to first language in the list. On
                                 output: point to next language in the list, or
                                 NULL if no more language in the list.
  @param  Lang           The first language in the list.

**/
VOID
EFIAPI
GetNextLanguage (
  IN OUT CHAR8      **LangCode,
  OUT CHAR8         *Lang
  )
;

#include "Utility.h"
#include "ConfigAccess.h"

#endif
