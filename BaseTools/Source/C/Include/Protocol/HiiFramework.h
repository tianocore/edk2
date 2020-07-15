/** @file
  This file defines the Human Interface Infrastructure protocol which will
  be used by resources which want to publish IFR/Font/String data and have it
  collected by the Configuration engine.

  @par Revision Reference:
  This protocol is defined in HII spec 0.92.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FRAMEWORK_HII_H_
#define _FRAMEWORK_HII_H_

//#include <PiDxe.h>

//
// To get EFI_GRAPHICS_OUTPUT_BLT_PIXEL,
// is defined in MdePkg/Protocol/GraphicsOutput.h
//
#include <Protocol/GraphicsOutput.h>

#define EFI_HII_PROTOCOL_GUID \
  { \
    0xd7ad636e, 0xb997, 0x459b, {0xbf, 0x3f, 0x88, 0x46, 0x89, 0x79, 0x80, 0xe1} \
  }

// BugBug:
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// If UGA goes away we need to put this some place. I'm not sure where?
//
//typedef struct {
//  UINT8 Blue;
//  UINT8 Green;
//  UINT8 Red;
//  UINT8 Reserved;
//} EFI_UGA_PIXEL;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
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


// References to string tokens must use this macro to enable scanning for
// token usages.
//
#define STRING_TOKEN(t) t

//
// The following types are currently defined:
// EFI_FROM_ID has been defined in UEFI spec.
//
typedef UINT16  EFI_FORM_LABEL;

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
  UINT32    NumberOfHiiDataTables;
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
// Glyph Attributes
//
#define EFI_GLYPH_NON_SPACING   1
#define EFI_GLYPH_WIDE          2

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

/**
  Registers the various packs that are passed in via the Packages parameter.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Packages              A pointer to an EFI_HII_PACKAGES package instance.
  @param  Handle                A pointer to the EFI_HII_HANDLE instance.

  @retval EFI_SUCCESS           Data was extracted from Packages, the database
                                was updated with the data, and Handle returned successfully.
  @retval EFI_INVALID_PARAMETER The content of Packages was invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_NEW_PACK) (
  IN  EFI_HII_PROTOCOL    *This,
  IN  EFI_HII_PACKAGES    *Packages,
  OUT EFI_HII_HANDLE      *Handle
  );

/**
  Removes a package from the HII database.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The handle that was registered to the data that is requested
                                for removal.

  @retval EFI_SUCCESS           The data associated with the Handle was removed
                                from the HII database.
  @retval EFI_INVALID_PARAMETER The Handle was not valid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_REMOVE_PACK) (
  IN EFI_HII_PROTOCOL    *This,
  IN EFI_HII_HANDLE      Handle
  );

/**
  Determines the handles that are currently active in the database.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  HandleBufferLength    On input, a pointer to the length of the handle
                                buffer. On output, the length of the handle buffer that is required
                                for the handles found.
  @param  Handle                An array of EFI_HII_HANDLE instances returned.

  @retval EFI_SUCCESS           Handle was updated successfully.
  @retval EFI_BUFFER_TOO_SMALL  The HandleBufferLength parameter indicates
                                that Handle is too small to support the number of handles.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_FIND_HANDLES) (
  IN     EFI_HII_PROTOCOL *This,
  IN OUT UINT16           *HandleBufferLength,
  OUT    EFI_HII_HANDLE   *Handle
  );

/**
  Exports the contents of the database into a buffer.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                An EFI_HII_HANDLE that corresponds to the desired
                                handle to export. If the value is 0, the entire database will be exported.
                                In either case, the data will be exported in a format described by the
                                structure definition of EFI_HII_EXPORT_TABLE.
  @param  BufferSize
  On input, a pointer to the length of the buffer. On output, the length
  of the buffer that is required for the export data.
  @param  Buffer                A pointer to a buffer that will contain the results of the export function.

  @retval EFI_SUCCESS           The buffer was successfully filled with BufferSize amount of data.
  @retval EFI_BUFFER_TOO_SMALL  The value in BufferSize was too small to contain the export data.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_EXPORT) (
  IN     EFI_HII_PROTOCOL *This,
  IN     EFI_HII_HANDLE   Handle,
  IN OUT UINTN            *BufferSize,
  OUT    VOID             *Buffer
  );

/**
  Remove any new strings that were added after the initial string export
  for this handle.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The handle on which the string resides.

  @retval EFI_SUCCESS           Remove strings from the handle successfully.
  @retval EFI_INVALID_PARAMETER The Handle was unknown.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_RESET_STRINGS) (
  IN     EFI_HII_PROTOCOL   *This,
  IN     EFI_HII_HANDLE     Handle
  );

/**
  Tests if all of the characters in a string have corresponding font characters.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  StringToTest          A pointer to a Unicode string.
  @param  FirstMissing          A pointer to an index into the string. On input,
                                the index of the first character in the StringToTest to examine. On exit,
                                the index of the first character encountered for which a glyph is unavailable.
                                If all glyphs in the string are available, the index is the index of the
                                terminator of the string.
  @param  GlyphBufferSize       A pointer to a value. On output, if the function
                                returns EFI_SUCCESS, it contains the amount of memory that is required to
                                store the string's glyph equivalent.

  @retval EFI_SUCCESS           All glyphs are available. Note that an empty string
                                always returns this value.
  @retval EFI_NOT_FOUND         A glyph was not found for a character.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_TEST_STRING) (
  IN     EFI_HII_PROTOCOL  *This,
  IN     CHAR16            *StringToTest,
  IN OUT UINT32            *FirstMissing,
  OUT    UINT32            *GlyphBufferSize
  );

/**
  Translates a Unicode character into the corresponding font glyph.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Source                A pointer to a Unicode string.
  @param  Index                 On input, the offset into the string from which to fetch
                                the character.On successful completion, the index is updated to the first
                                character past the character(s) making up the just extracted glyph.
  @param  GlyphBuffer           Pointer to an array where the glyphs corresponding
                                to the characters in the source may be stored. GlyphBuffer is assumed
                                to be wide enough to accept a wide glyph character.
  @param  BitWidth              If EFI_SUCCESS was returned, the UINT16 pointed to by
                                this value is filled with the length of the glyph in pixels. It is unchanged
                                if the call was unsuccessful.
  @param  InternalStatus        The cell pointed to by this parameter must be
                                initialized to zero prior to invoking the call the first time for any string.

  @retval EFI_SUCCESS           It worked.
  @retval EFI_NOT_FOUND         A glyph for a character was not found.

**/
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

/**
  Translates a glyph into the format required for input to the Universal
  Graphics Adapter (UGA) Block Transfer (BLT) routines.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  GlyphBuffer           A pointer to the buffer that contains glyph data.
  @param  Foreground            The foreground setting requested to be used for the
                                generated BltBuffer data.
  @param  Background            The background setting requested to be used for the
                                generated BltBuffer data.
  @param  Count                 The entry in the BltBuffer upon which to act.
  @param  Width                 The width in bits of the glyph being converted.
  @param  Height                The height in bits of the glyph being converted
  @param  BltBuffer             A pointer to the buffer that contains the data that is
                                ready to be used by the UGA BLT routines.

  @retval EFI_SUCCESS           It worked.
  @retval EFI_NOT_FOUND         A glyph for a character was not found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GLYPH_TO_BLT) (
  IN     EFI_HII_PROTOCOL             *This,
  IN     UINT8                        *GlyphBuffer,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background,
  IN     UINTN                         Count,
  IN     UINTN                         Width,
  IN     UINTN                         Height,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer
  );

/**
  Allows a new string to be added to an already existing string package.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Pointer               to a NULL-terminated string containing a single ISO 639-2
                                language identifier, indicating the language in which the string is translated.
  @param  Handle                The handle of the language pack to which the string is to be added.
  @param  Reference             The identifier of the string to be added. If the reference
                                value is zero, then the string will be assigned a new identifier on that
                                handle for the language specified. Otherwise, the string will be updated
                                with the NewString Value.
  @param  NewString             The string to be added.

  @retval EFI_SUCCESS           The string was effectively registered.
  @retval EFI_INVALID_PARAMETER The Handle was unknown.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_NEW_STRING) (
  IN     EFI_HII_PROTOCOL      *This,
  IN     CHAR16                *Language,
  IN     EFI_HII_HANDLE        Handle,
  IN OUT STRING_REF            *Reference,
  IN     CHAR16                *NewString
  );

/**
  Allows a program to determine the primary languages that are supported
  on a given handle.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The handle on which the strings reside.
  @param  LanguageString        A string allocated by GetPrimaryLanguages() that
                                contains a list of all primary languages registered on the handle.

  @retval EFI_SUCCESS           LanguageString was correctly returned.
  @retval EFI_INVALID_PARAMETER The Handle was unknown.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_PRI_LANGUAGES) (
  IN  EFI_HII_PROTOCOL    *This,
  IN  EFI_HII_HANDLE      Handle,
  OUT EFI_STRING          *LanguageString
  );

/**
  Allows a program to determine which secondary languages are supported
  on a given handle for a given primary language.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The handle on which the strings reside.
  @param  PrimaryLanguage       Pointer to a NULL-terminated string containing a single
                                ISO 639-2 language identifier, indicating the primary language.
  @param  LanguageString        A string allocated by GetSecondaryLanguages()
                                containing a list of all secondary languages registered on the handle.

  @retval EFI_SUCCESS           LanguageString was correctly returned.
  @retval EFI_INVALID_PARAMETER The Handle was unknown.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_SEC_LANGUAGES) (
  IN  EFI_HII_PROTOCOL    *This,
  IN  EFI_HII_HANDLE      Handle,
  IN  CHAR16              *PrimaryLanguage,
  OUT EFI_STRING          *LanguageString
  );

/**
  Extracts a string from a package already registered with the EFI HII database.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The handle on which the string resides.
  @param  Token                 The string token assigned to the string.
  @param  Raw                   If TRUE, the string is returned unedited in the internal
                                storage format described above. If false, the string returned is edited
                                by replacing <cr> with <space> and by removing special characters such
                                as the <wide> prefix.
  @param  LanguageString        Pointer to a NULL-terminated string containing a
                                single ISO 639-2 language identifier, indicating the language to print.
                                If the LanguageString is empty (starts with a NULL), the default system
                                language will be used to determine the language.
  @param  BufferLength          Length of the StringBuffer.
  @param  StringBuffer          The buffer designed to receive the characters in the string.

  @retval EFI_SUCCESS           StringBuffer is filled with a NULL-terminated string.
  @retval EFI_INVALID_PARAMETER The handle or string token is unknown.
  @retval EFI_BUFFER_TOO_SMALL  The buffer provided was not large enough to
                                allow the entire string to be stored.

**/
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

/**
  Allows a program to extract a part of a string of not more than a given width.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The handle on which the string resides.
  @param  Token                 The string token assigned to the string.
  @param  Index                 On input, the offset into the string where the line is to start.
                                On output, the index is updated to point to beyond the last character returned
                                in the call.
  @param  LineWidth             The maximum width of the line in units of narrow glyphs.
  @param  LanguageString        Pointer to a NULL-terminated string containing a
                                single ISO 639-2 language identifier, indicating the language to print.
  @param  BufferLength          Pointer to the length of the StringBuffer.
  @param  StringBuffer          The buffer designed to receive the characters in the string.

  @retval EFI_SUCCESS           StringBuffer filled with characters that will fit on the line.
  @retval EFI_NOT_FOUND         The font glyph for at least one of the characters in
                                the string is not in the font database.
  @retval EFI_BUFFER_TOO_SMALL  The buffer provided was not large enough
                                to allow the entire string to be stored.

**/
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

/**
  Allows a program to extract a form or form package that has previously
  been registered with the HII database.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                Handle on which the form resides.
  @param  FormId                The ID of the form to return. If the ID is zero,
                                the entire form package is returned.
  @param  BufferLength          On input, the length of the Buffer. On output,
                                the length of the returned buffer,
  @param  Buffer                The buffer designed to receive the form(s).

  @retval EFI_SUCCESS           Buffer filled with the requested forms. BufferLength
                                was updated.
  @retval EFI_INVALID_PARAMETER The handle is unknown.
  @retval EFI_NOT_FOUND         A form on the requested handle cannot be found with
                                the requested FormId.
  @retval EFI_BUFFER_TOO_SMALL  The buffer provided was not large enough
                                to allow the form to be stored.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_FORMS) (
  IN     EFI_HII_PROTOCOL  *This,
  IN     EFI_HII_HANDLE    Handle,
  IN     EFI_FORM_ID       FormId,
  IN OUT UINTN             *BufferLength,
  OUT    UINT8             *Buffer
  );

/**
  Extracts the defaults that are associated with a given handle in the HII database.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The HII handle from which will have default data retrieved.
  @param  DefaultMask           The mask used to specify some type of default override when extracting
                                the default image data.
  @param  VariablePackList      A indirect pointer to the first entry of a link list with
                                type EFI_HII_VARIABLE_PACK_LIST.

  @retval EFI_SUCCESS           The VariablePackList was populated with the appropriate
                                default setting data.
  @retval EFI_NOT_FOUND         The IFR does not have any explicit or default map(s).
  @retval EFI_INVALID_PARAMETER The HII database entry associated with Handle
                                contain invalid data.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_DEFAULT_IMAGE) (
  IN     EFI_HII_PROTOCOL           *This,
  IN     EFI_HII_HANDLE             Handle,
  IN     UINTN                      DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST **VariablePackList
  );

/**
  Allows the caller to update a form or form package that has previously been
  registered with the EFI HII database.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                Handle of the package where the form to be updated resides.
  @param  Label                 The label inside the form package where the update is to take place.
  @param  AddData               If TRUE, adding data at a given Label; otherwise,
                                if FALSE, removing data at a given Label.
  @param  Data                  The buffer containing the new tags to insert after the Label

  @retval EFI_SUCCESS           The form was updated with the new tags.
  @retval EFI_INVALID_PARAMETER The buffer for the buffer length does not
                                contain an integral number of tags.
  @retval EFI_NOT_FOUND         The Handle, Label, or FormId was not found.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_UPDATE_FORM) (
  IN EFI_HII_PROTOCOL     *This,
  IN EFI_HII_HANDLE       Handle,
  IN EFI_FORM_LABEL       Label,
  IN BOOLEAN              AddData,
  IN EFI_HII_UPDATE_DATA  *Data
  );

/**
  Retrieves the current keyboard layout.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  DescriptorCount       A pointer to the number of Descriptor entries being
                                described in the keyboard layout being retrieved.
  @param  Descriptor            A pointer to a buffer containing an array of EFI_KEY_DESCRIPTOR
                                entries. Each entry will reflect the definition of a specific physical key.

  @retval EFI_SUCCESS           The keyboard layout was retrieved successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_KEYBOARD_LAYOUT) (
  IN     EFI_HII_PROTOCOL    *This,
  OUT    UINT16              *DescriptorCount,
  OUT    EFI_KEY_DESCRIPTOR  *Descriptor
  );

/**
  @par Protocol Description:
  The HII Protocol manages the HII database, which is a repository for data
  having to do with fonts, strings, forms, keyboards, and other future human
  interface items.

  @param NewPack
  Extracts the various packs from a package list.

  @param RemovePack
  Removes a package from the HII database.

  @param FindHandles
  Determines the handles that are currently active in the database.

  @param ExportDatabase
  Export the entire contents of the database to a buffer.

  @param TestString
  Tests if all of the characters in a string have corresponding font characters.

  @param GetGlyph
  Translates a Unicode character into the corresponding font glyph.

  @param GlyphToBlt
  Converts a glyph value into a format that is ready for a UGA BLT command.

  @param NewString
  Allows a new string to be added to an already existing string package.

  @param GetPrimaryLanguages
  Allows a program to determine the primary languages that are supported
  on a given handle.

  @param GetSecondaryLanguages
  Allows a program to determine which secondary languages are supported
  on a given handle for a given primary language.

  @param GetString
  Extracts a string from a package that is already registered with the
  EFI HII database.

  @param ResetString
  Remove any new strings that were added after the initial string export
  for this handle.

  @param GetLine
  Allows a program to extract a part of a string of not more than a given width.

  @param GetForms
  Allows a program to extract a form or form package that has been previously registered.

  @param GetDefaultImage
  Allows a program to extract the nonvolatile image that represents the default storage image.

  @param UpdateForm
  Allows a program to update a previously registered form.

  @param GetKeyboardLayout
  Allows a program to extract the current keyboard layout.

**/
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
