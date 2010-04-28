/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Hii.h

Abstract:

  This file defines the Human Interface Infrastructure protocol which will 
  be used by resources which want to publish IFR/Font/String data and have it 
  collected by the Configuration engine.

--*/

#ifndef _HII_H_
#define _HII_H_

#include "EfiInternalFormRepresentation.h"
#include EFI_PROTOCOL_DEFINITION (UgaDraw)
#include EFI_PROTOCOL_DEFINITION (GraphicsOutput)

//
// EFI_HII_PROTOCOL_GUID has been changed from the one defined in Framework HII 0.92 as the
// Framework HII protocol produced by HII Thunk Layer support the UEF IFR and UEFI String Package
// format.
//
#define EFI_HII_PROTOCOL_GUID \
  { \
    0x5542cce1, 0xdf5c, 0x4d1b, { 0xab, 0xca, 0x36, 0x4f, 0x77, 0xd3, 0x99, 0xfb } \
  }

/*
#define EFI_HII_PROTOCOL_GUID \
  { \
    0xd7ad636e, 0xb997, 0x459b, {0xbf, 0x3f, 0x88, 0x46, 0x89, 0x79, 0x80, 0xe1} \
  }
*/

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_HII_PROTOCOL  EFI_HII_PROTOCOL;

//
// Global definition
//
#define NARROW_CHAR         0xFFF0
#define WIDE_CHAR           0xFFF1
#define NON_BREAKING_CHAR   0xFFF2
#define GLYPH_WIDTH         8
#define GLYPH_HEIGHT        19

#define EFI_HII_FONT        1
#define EFI_HII_STRING      2
#define EFI_HII_IFR         3
#define EFI_HII_KEYBOARD    4
#define EFI_HII_HANDLES     5
#define EFI_HII_VARIABLE    6
#define EFI_HII_DEVICE_PATH 7

#define HANG(foo) { \
    volatile INT32  __iii; \
    __iii = foo; \
    while (__iii) \
      ; \
  }
//
// #define HANG(foo)
//
// References to string tokens must use this macro to enable scanning for
// token usages.
//
#define STRING_TOKEN(t) t

//
// The following types are currently defined:
//
typedef UINT16  EFI_FORM_ID;
typedef UINT16  EFI_FORM_LABEL;
typedef UINT16  EFI_HII_HANDLE;

#pragma pack(1)

typedef struct {
  UINT32  Length;
  UINT16  Type;
} EFI_HII_PACK_HEADER;

//
// A form list consists of a large variety of structure
// possibilities so to represent the binary blob of data
// associated with a package of forms, we will assume a
// pointer to a self-describing data buffer.
//
typedef struct {
  EFI_HII_PACK_HEADER Header;
} EFI_HII_IFR_PACK;

typedef struct {
  EFI_HII_PACK_HEADER Header;           // Must be filled in
  EFI_HANDLE          ImageHandle;      // Must be filled in
  EFI_HANDLE          DeviceHandle;     // Optional
  EFI_HANDLE          ControllerHandle; // Optional
  EFI_HANDLE          CallbackHandle;   // Optional
  EFI_HANDLE          COBExportHandle;  // Optional
} EFI_HII_HANDLE_PACK;

//
// ********************************************************
// EFI_VARIABLE_CONTENTS
// ********************************************************
//
typedef struct {
  EFI_HII_PACK_HEADER Header;
  EFI_GUID            VariableGuid;
  UINT32              VariableNameLength;
  UINT16              VariableId;
  //
  //  CHAR16                VariableName[]; //Null-terminated
  //
} EFI_HII_VARIABLE_PACK;

//
// ********************************************************
// EFI_DEVICE_PATH_PACK
// ********************************************************
//
typedef struct {
  EFI_HII_PACK_HEADER Header;
  //
  //  EFI_DEVICE_PATH       DevicePath[];
  //
} EFI_HII_DEVICE_PATH_PACK;

//
// ********************************************************
// EFI_HII_DATA_TABLE
// ********************************************************
//
typedef struct {
  EFI_HII_HANDLE  HiiHandle;
  EFI_GUID        PackageGuid;
  UINT32          DataTableSize;
  UINT32          IfrDataOffset;
  UINT32          StringDataOffset;
  UINT32          VariableDataOffset;
  UINT32          DevicePathOffset;
  UINT32          NumberOfVariableData;
  UINT32          NumberOfLanguages;
  //
  // EFI_HII_DEVICE_PATH_PACK DevicePath[];
  // EFI_HII_VARIABLE_PACK VariableData[];
  // EFI_HII_IFR_PACK IfrData;
  // EFI_HII_STRING_PACK StringData[];
  //
} EFI_HII_DATA_TABLE;

//
// ********************************************************
// EFI_HII_EXPORT_TABLE
// ********************************************************
//
typedef struct {
  UINT16    NumberOfHiiDataTables;
  EFI_GUID  Revision;
  //
  // EFI_HII_DATA_TABLE HiiDataTable[];
  //
} EFI_HII_EXPORT_TABLE;

typedef struct {
  BOOLEAN               FormSetUpdate;      // If TRUE, next variable is significant
  EFI_PHYSICAL_ADDRESS  FormCallbackHandle; // If not 0, will update Formset with this info
  BOOLEAN               FormUpdate;         // If TRUE, next variable is significant
  UINT16                FormValue;          // specify which form is to be updated if FormUpdate value is TRUE.
  STRING_REF            FormTitle;          // If not 0, will update Form with this info
  UINT16                DataCount;          // The number of Data entries in this structure
  UINT8                 *Data;              // An array of 1+ op-codes, specified by DataCount
} EFI_HII_UPDATE_DATA;

//
// String attributes
//
#define LANG_RIGHT_TO_LEFT  0x00000001

//
// A string package is used to localize strings to a particular
// language.  The package is associated with a particular driver
// or set of drivers.  Tools are used to associate tokens with
// string references in forms and in programs.  These tokens are
// language agnostic.  When paired with a language pack (directly
// or indirectly), the string token resolves into an actual
// UNICODE string.  The NumStringPointers determines how many
// StringPointers (offset values) there are as well as the total
// number of Strings that are defined.
//
typedef struct {
  EFI_HII_PACK_HEADER Header;
  RELOFST             LanguageNameString;
  RELOFST             PrintableLanguageName;
  UINT32              NumStringPointers;
  UINT32              Attributes;
  //
  //  RELOFST               StringPointers[];
  //  EFI_STRING            Strings[];
  //
} EFI_HII_STRING_PACK;

//
// We use this one to get the real size of the header
//
typedef struct {
  EFI_HII_PACK_HEADER Header;
  RELOFST             LanguageNameString;
  RELOFST             PrintableLanguageName;
  UINT32              NumStringPointers;
  UINT32              Attributes;
} EFI_HII_STRING_PACK_HEADER;

//
// Glyph Attributes
//
#define GLYPH_NON_SPACING   1
#define GLYPH_NON_BREAKING  2

typedef struct {
  CHAR16  UnicodeWeight;
  UINT8   Attributes;
  UINT8   GlyphCol1[GLYPH_HEIGHT];
} EFI_NARROW_GLYPH;

typedef struct {
  CHAR16  UnicodeWeight;
  UINT8   Attributes;
  UINT8   GlyphCol1[GLYPH_HEIGHT];
  UINT8   GlyphCol2[GLYPH_HEIGHT];
  UINT8   Pad[3];
} EFI_WIDE_GLYPH;

//
// A font list consists of a font header followed by a series
// of glyph structures.  Note that fonts are not language specific.
//
typedef struct {
  EFI_HII_PACK_HEADER Header;
  UINT16              NumberOfNarrowGlyphs;
  UINT16              NumberOfWideGlyphs;
} EFI_HII_FONT_PACK;

//
// The IfrData in the EFI_HII_IFR_PACK structure definition
// is variable length, and not really part of the header. To
// simplify from code the size of the header, define an
// identical structure that does not include the IfrData field.
// Then use sizeof() this new structure to determine the
// actual size of the header.
//
typedef struct {
  EFI_HII_PACK_HEADER Header;
} EFI_HII_IFR_PACK_HEADER;

//
// pedef EFI_HII_PACK_HEADER EFI_HII_IFR_PACK_HEADER;
//
typedef enum {
  EfiKeyLCtrl,
  EfiKeyA0,
  EfiKeyLAlt,
  EfiKeySpaceBar,
  EfiKeyA2,
  EfiKeyA3,
  EfiKeyA4,
  EfiKeyRCtrl,
  EfiKeyLeftArrow,
  EfiKeyDownArrow,
  EfiKeyRightArrow,
  EfiKeyZero,
  EfiKeyPeriod,
  EfiKeyEnter,
  EfiKeyLShift,
  EfiKeyB0,
  EfiKeyB1,
  EfiKeyB2,
  EfiKeyB3,
  EfiKeyB4,
  EfiKeyB5,
  EfiKeyB6,
  EfiKeyB7,
  EfiKeyB8,
  EfiKeyB9,
  EfiKeyB10,
  EfiKeyRshift,
  EfiKeyUpArrow,
  EfiKeyOne,
  EfiKeyTwo,
  EfiKeyThree,
  EfiKeyCapsLock,
  EfiKeyC1,
  EfiKeyC2,
  EfiKeyC3,
  EfiKeyC4,
  EfiKeyC5,
  EfiKeyC6,
  EfiKeyC7,
  EfiKeyC8,
  EfiKeyC9,
  EfiKeyC10,
  EfiKeyC11,
  EfiKeyC12,
  EfiKeyFour,
  EfiKeyFive,
  EfiKeySix,
  EfiKeyPlus,
  EfiKeyTab,
  EfiKeyD1,
  EfiKeyD2,
  EfiKeyD3,
  EfiKeyD4,
  EfiKeyD5,
  EfiKeyD6,
  EfiKeyD7,
  EfiKeyD8,
  EfiKeyD9,
  EfiKeyD10,
  EfiKeyD11,
  EfiKeyD12,
  EfiKeyD13,
  EfiKeyDel,
  EfiKeyEnd,
  EfiKeyPgDn,
  EfiKeySeven,
  EfiKeyEight,
  EfiKeyNine,
  EfiKeyE0,
  EfiKeyE1,
  EfiKeyE2,
  EfiKeyE3,
  EfiKeyE4,
  EfiKeyE5,
  EfiKeyE6,
  EfiKeyE7,
  EfiKeyE8,
  EfiKeyE9,
  EfiKeyE10,
  EfiKeyE11,
  EfiKeyE12,
  EfiKeyBackSpace,
  EfiKeyIns,
  EfiKeyHome,
  EfiKeyPgUp,
  EfiKeyNLck,
  EfiKeySlash,
  EfiKeyAsterisk,
  EfiKeyMinus,
  EfiKeyEsc,
  EfiKeyF1,
  EfiKeyF2,
  EfiKeyF3,
  EfiKeyF4,
  EfiKeyF5,
  EfiKeyF6,
  EfiKeyF7,
  EfiKeyF8,
  EfiKeyF9,
  EfiKeyF10,
  EfiKeyF11,
  EfiKeyF12,
  EfiKeyPrint,
  EfiKeySLck,
  EfiKeyPause
} EFI_KEY;

typedef struct {
  EFI_KEY Key;
  CHAR16  Unicode;
  CHAR16  ShiftedUnicode;
  CHAR16  AltGrUnicode;
  CHAR16  ShiftedAltGrUnicode;
  UINT16  Modifier;
} EFI_KEY_DESCRIPTOR;

//
// This structure allows a sparse set of keys to be redefined
// or a complete redefinition of the keyboard layout.  Most
// keyboards have a lot of commonality in their layouts, therefore
// only defining those keys that need to change from the default
// minimizes the passed in information.
//
// Additionally, when an update occurs, the active keyboard layout
// will be switched to the newly updated keyboard layout.  This
// allows for situations that when a keyboard layout driver is
// loaded as part of system initialization, the system will default
// the keyboard behavior to the new layout.
//
// Each call to update the keyboard mapping should contain the
// complete set of key descriptors to be updated, since every
// call to the HII which contains an EFI_HII_KEYBOARD_PACK will
// wipe the previous set of overrides.  A call to
//
typedef struct {
  EFI_HII_PACK_HEADER Header;
  EFI_KEY_DESCRIPTOR  *Descriptor;
  UINT8               DescriptorCount;
} EFI_HII_KEYBOARD_PACK;

//
// The EFI_HII_PACKAGES can contain different types of packages just
// after the structure as inline data.
//
typedef struct {
  UINTN     NumberOfPackages;
  EFI_GUID  *GuidId;
  //
  // EFI_HII_HANDLE_PACK    *HandlePack;        // Only one pack.
  // EFI_HII_IFR_PACK       *IfrPack;           // Only one pack.
  // EFI_HII_FONT_PACK      *FontPack[];        // Multiple packs ok
  // EFI_HII_STRING_PACK    *StringPack[];      // Multiple packs ok
  // EFI_HII_KEYBOARD_PACK  *KeyboardPack[];    // Multiple packs ok
  //
} EFI_HII_PACKAGES;

typedef struct _EFI_HII_VARIABLE_PACK_LIST {
  struct _EFI_HII_VARIABLE_PACK_LIST   *NextVariablePack;
  EFI_HII_VARIABLE_PACK                *VariablePack;
} EFI_HII_VARIABLE_PACK_LIST;

#pragma pack()

typedef
EFI_STATUS
(EFIAPI *EFI_HII_NEW_PACK) (
  IN  EFI_HII_PROTOCOL    * This,
  IN  EFI_HII_PACKAGES    * Packages,
  OUT EFI_HII_HANDLE      * Handle
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_REMOVE_PACK) (
  IN EFI_HII_PROTOCOL    *This,
  IN EFI_HII_HANDLE      Handle
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_FIND_HANDLES) (
  IN     EFI_HII_PROTOCOL *This,
  IN OUT UINT16           *HandleBufferLength,
  OUT    EFI_HII_HANDLE   *Handle
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_EXPORT) (
  IN     EFI_HII_PROTOCOL *This,
  IN     EFI_HII_HANDLE   Handle,
  IN OUT UINTN            *BufferSize,
  OUT    VOID             *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_RESET_STRINGS) (
  IN     EFI_HII_PROTOCOL   *This,
  IN     EFI_HII_HANDLE     Handle
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_TEST_STRING) (
  IN     EFI_HII_PROTOCOL  *This,
  IN     CHAR16            *StringToTest,
  IN OUT UINT32            *FirstMissing,
  OUT    UINT32            *GlyphBufferSize
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_GLYPH) (
  IN     EFI_HII_PROTOCOL  *This,
  IN     CHAR16            *Source,
  IN OUT UINT16            *Index,
  OUT    UINT8             **GlyphBuffer,
  OUT    UINT16            *BitWidth,
  IN OUT UINT32            *InternalStatus
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GLYPH_TO_BLT) (
  IN     EFI_HII_PROTOCOL               *This,
  IN     UINT8                          *GlyphBuffer,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Foreground,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Background,
  IN     UINTN                          Count,
  IN     UINTN                          Width,
  IN     UINTN                          Height,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BltBuffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_NEW_STRING) (
  IN     EFI_HII_PROTOCOL      *This,
  IN     CHAR16                *Language,
  IN     EFI_HII_HANDLE        Handle,
  IN OUT STRING_REF            *Reference,
  IN     CHAR16                *NewString
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_PRI_LANGUAGES) (
  IN  EFI_HII_PROTOCOL    *This,
  IN  EFI_HII_HANDLE      Handle,
  OUT EFI_STRING          *LanguageString
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_SEC_LANGUAGES) (
  IN  EFI_HII_PROTOCOL    *This,
  IN  EFI_HII_HANDLE      Handle,
  IN  CHAR16              *PrimaryLanguage,
  OUT EFI_STRING          *LanguageString
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_STRING) (
  IN     EFI_HII_PROTOCOL  *This,
  IN     EFI_HII_HANDLE    Handle,
  IN     STRING_REF        Token,
  IN     BOOLEAN           Raw,
  IN     CHAR16            *LanguageString,
  IN OUT UINTN             *BufferLength,
  OUT    EFI_STRING        StringBuffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_LINE) (
  IN     EFI_HII_PROTOCOL  *This,
  IN     EFI_HII_HANDLE    Handle,
  IN     STRING_REF        Token,
  IN OUT UINT16            *Index,
  IN     UINT16            LineWidth,
  IN     CHAR16            *LanguageString,
  IN OUT UINT16            *BufferLength,
  OUT    EFI_STRING        StringBuffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_FORMS) (
  IN     EFI_HII_PROTOCOL  *This,
  IN     EFI_HII_HANDLE    Handle,
  IN     EFI_FORM_ID       FormId,
  IN OUT UINTN             *BufferLength,
  OUT    UINT8             *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_DEFAULT_IMAGE) (
  IN     EFI_HII_PROTOCOL           *This,
  IN     EFI_HII_HANDLE             Handle,
  IN     UINTN                      DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST **VariablePackList
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_UPDATE_FORM) (
  IN EFI_HII_PROTOCOL     *This,
  IN EFI_HII_HANDLE       Handle,
  IN EFI_FORM_LABEL       Label,
  IN BOOLEAN              AddData,
  IN EFI_HII_UPDATE_DATA  *Data
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_KEYBOARD_LAYOUT) (
  IN     EFI_HII_PROTOCOL    * This,
  OUT    UINT16              *DescriptorCount,
  OUT    EFI_KEY_DESCRIPTOR  * Descriptor
  );

struct _EFI_HII_PROTOCOL {
  EFI_HII_NEW_PACK            NewPack;
  EFI_HII_REMOVE_PACK         RemovePack;
  EFI_HII_FIND_HANDLES        FindHandles;
  EFI_HII_EXPORT              ExportDatabase;

  EFI_HII_TEST_STRING         TestString;
  EFI_HII_GET_GLYPH           GetGlyph;
  EFI_HII_GLYPH_TO_BLT        GlyphToBlt;

  EFI_HII_NEW_STRING          NewString;
  EFI_HII_GET_PRI_LANGUAGES   GetPrimaryLanguages;
  EFI_HII_GET_SEC_LANGUAGES   GetSecondaryLanguages;
  EFI_HII_GET_STRING          GetString;
  EFI_HII_RESET_STRINGS       ResetStrings;
  EFI_HII_GET_LINE            GetLine;
  EFI_HII_GET_FORMS           GetForms;
  EFI_HII_GET_DEFAULT_IMAGE   GetDefaultImage;
  EFI_HII_UPDATE_FORM         UpdateForm;

  EFI_HII_GET_KEYBOARD_LAYOUT GetKeyboardLayout;
};

extern EFI_GUID gEfiHiiProtocolGuid;

#endif
