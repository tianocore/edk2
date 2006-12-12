/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  HiiDatabase.h

Abstract:

  This file contains global defines and prototype definitions 
  for the HII database.

--*/

#ifndef _HIIDATABASE_H
#define _HIIDATABASE_H

//
// HII Database Global data
//
#define EFI_HII_DATA_SIGNATURE            EFI_SIGNATURE_32 ('H', 'i', 'i', 'P')

#define MAX_GLYPH_COUNT                   65535
#define NARROW_GLYPH_ARRAY_SIZE           19
#define WIDE_GLYPH_ARRAY_SIZE             38

#define SETUP_MAP_NAME                              L"Setup"
#define HII_VARIABLE_SUFFIX_USER_DATA               L"UserSavedData"
#define HII_VARIABLE_SUFFIX_DEFAULT_OVERRIDE        L"DefaultOverride"
#define HII_VARIABLE_SUFFIX_MANUFACTURING_OVERRIDE  L"ManufacturingOverride"

typedef struct _EFI_HII_HANDLE_DATABASE {
  VOID                            *Buffer;        // Actual buffer pointer
  EFI_HII_HANDLE                  Handle;         // Monotonically increasing value to signify the value returned to caller
  UINT32                          NumberOfTokens; // The initial number of tokens when first registered
  struct _EFI_HII_HANDLE_DATABASE *NextHandleDatabase;
} EFI_HII_HANDLE_DATABASE;

typedef struct {
  EFI_NARROW_GLYPH    NarrowGlyphs[MAX_GLYPH_COUNT];
  EFI_WIDE_GLYPH      WideGlyphs[MAX_GLYPH_COUNT];
  EFI_KEY_DESCRIPTOR  SystemKeyboardLayout[106];
  EFI_KEY_DESCRIPTOR  OverrideKeyboardLayout[106];
  BOOLEAN             SystemKeyboardUpdate;       // Has the SystemKeyboard been updated?
} EFI_HII_GLOBAL_DATA;

typedef struct {
  UINTN                   Signature;

  EFI_HII_GLOBAL_DATA     *GlobalData;
  EFI_HII_HANDLE_DATABASE *DatabaseHead;          // Head of the Null-terminated singly-linked list of handles.
  EFI_HII_PROTOCOL        Hii;
} EFI_HII_DATA;

typedef struct {
  EFI_HII_HANDLE      Handle;
  EFI_GUID            Guid;
  EFI_HII_HANDLE_PACK HandlePack;
  UINTN               IfrSize;
  UINTN               StringSize;
  EFI_HII_IFR_PACK    *IfrData;                   // All the IFR data stored here
  EFI_HII_STRING_PACK *StringData;                // All the String data stored at &IfrData + IfrSize (StringData is just a label - never referenced)
} EFI_HII_PACKAGE_INSTANCE;

typedef struct {
  EFI_HII_PACK_HEADER   Header;
  EFI_IFR_FORM_SET      FormSet;
  EFI_IFR_END_FORM_SET  EndFormSet;
} EFI_FORM_SET_STUB;

#define EFI_HII_DATA_FROM_THIS(a) CR (a, EFI_HII_DATA, Hii, EFI_HII_DATA_SIGNATURE)

#define NARROW_WIDTH              8
#define WIDE_WIDTH                16

extern UINT8  mUnknownGlyph[38];

//
// Prototypes
//
EFI_STATUS
GetPackSize (
  IN  VOID                *Pack,
  OUT UINTN               *PackSize,
  OUT UINT32              *NumberOfTokens
  )
;

EFI_STATUS
ValidatePack (
  IN   EFI_HII_PROTOCOL          *This,
  IN   EFI_HII_PACKAGE_INSTANCE  *PackageInstance,
  OUT  EFI_HII_PACKAGE_INSTANCE  **StringPackageInstance,
  OUT  UINT32                    *TotalStringCount
  )
;

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
  OUT EFI_HII_HANDLE        *Handle
  )
;

EFI_STATUS
EFIAPI
HiiRemovePack (
  IN EFI_HII_PROTOCOL    *This,
  IN EFI_HII_HANDLE      Handle
  )
;

EFI_STATUS
EFIAPI
HiiFindHandles (
  IN     EFI_HII_PROTOCOL    *This,
  IN OUT UINT16              *HandleBufferLength,
  OUT    EFI_HII_HANDLE      *Handle
  )
;

EFI_STATUS
EFIAPI
HiiExportDatabase (
  IN     EFI_HII_PROTOCOL *This,
  IN     EFI_HII_HANDLE   Handle,
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
  IN     EFI_HII_HANDLE          Handle,
  IN OUT STRING_REF              *Reference,
  IN     CHAR16                  *NewString
  )
;

EFI_STATUS
EFIAPI
HiiGetString (
  IN     EFI_HII_PROTOCOL    *This,
  IN     EFI_HII_HANDLE      Handle,
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
  IN     EFI_HII_HANDLE      Handle
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
  IN  EFI_HII_HANDLE        Handle,
  OUT EFI_STRING            *LanguageString
  )
;

EFI_STATUS
EFIAPI
HiiGetSecondaryLanguages (
  IN  EFI_HII_PROTOCOL      *This,
  IN  EFI_HII_HANDLE        Handle,
  IN  CHAR16                *PrimaryLanguage,
  OUT EFI_STRING            *LanguageString
  )
;

EFI_STATUS
EFIAPI
HiiGetLine (
  IN     EFI_HII_PROTOCOL    *This,
  IN     EFI_HII_HANDLE      Handle,
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
  IN     EFI_HII_HANDLE      Handle,
  IN     EFI_FORM_ID         FormId,
  IN OUT UINTN               *BufferLength,
  OUT    UINT8               *Buffer
  )
;

EFI_STATUS
EFIAPI
HiiGetDefaultImage (
  IN     EFI_HII_PROTOCOL           *This,
  IN     EFI_HII_HANDLE             Handle,
  IN     UINTN                      DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST **VariablePackList
  )
;

EFI_STATUS
EFIAPI
HiiUpdateForm (
  IN EFI_HII_PROTOCOL       *This,
  IN EFI_HII_HANDLE         Handle,
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
  OUT    EFI_KEY_DESCRIPTOR  *Descriptor
  )
;

EFI_STATUS
HiiCompareLanguage (
  IN  CHAR16                *LanguageStringLocation,
  IN  CHAR16                *Language
  )
;

#endif
