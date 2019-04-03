/** @file
  This file defines the Human Interface Infrastructure protocol, which is
  used by resources that want to publish IFR/Font/String data and have it
  collected by the Configuration engine.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This protocol is defined in Framework for EFI Human Interface Infrastructure
  Specification Version 0.92.

**/

#ifndef _FRAMEWORK_HII_H_
#define _FRAMEWORK_HII_H_

//
// EFI_GRAPHICS_OUTPUT_BLT_PIXEL is defined in MdePkg/Protocol/GraphicsOutput.h
//
#include <Protocol/GraphicsOutput.h>
///
/// In both EDK and EDK II, there is an incompatbile change in the Framework HII protocol.
/// This change should cause a change of GUID in both of code and HII specification. But we
/// updated the GUID in code in EDK and EDK II. The 0.92 specification is not updated. This
/// is a known issue.
///
///
/// Note that EFI_HII_PROTOCOL_GUID is different from that defined in the Framework HII
/// 0.92 specification because the specification changed part of HII interfaces but did not update the protocol
/// GUID.
///
#define EFI_HII_PROTOCOL_GUID \
  { \
    0xd7ad636e, 0xb997, 0x459b, {0xbf, 0x3f, 0x88, 0x46, 0x89, 0x79, 0x80, 0xe1} \
  }

#define EFI_HII_COMPATIBILITY_PROTOCOL_GUID \
  { \
    0x5542cce1, 0xdf5c, 0x4d1b, { 0xab, 0xca, 0x36, 0x4f, 0x77, 0xd3, 0x99, 0xfb } \
  }

typedef UINT32                    RELOFST;

typedef struct _EFI_HII_PROTOCOL  EFI_HII_PROTOCOL;

///
/// Note: Name difference between code and the Framework HII 0.92 specificaiton.
///       Add FRAMEWORK_ prefix to avoid a name confict with EFI_HII_HANDLE, defined in the
///       UEFI 2.1d specification.
///
typedef UINT16                    FRAMEWORK_EFI_HII_HANDLE;

///
/// HII package type values
///
#define EFI_HII_FONT        1
#define EFI_HII_STRING      2
#define EFI_HII_IFR         3
#define EFI_HII_KEYBOARD    4
#define EFI_HII_HANDLES     5
#define EFI_HII_VARIABLE    6
#define EFI_HII_DEVICE_PATH 7

//
// References to string tokens must use this macro to enable scanning for
// token usages.
//
#define STRING_TOKEN(t) t

//
// The following types are currently defined:
// EFI_FORM_ID has been defined in UEFI spec.
//
typedef UINT16  EFI_FORM_LABEL;

#pragma pack(1)

///
/// The header found at the start of each package.
///
typedef struct {
  UINT32  Length;  ///< The size of the package in bytes.
  UINT16  Type;    ///< The type of the package.
} EFI_HII_PACK_HEADER;

///
/// The IFR package structure.
/// Immediately following the EFI_HII_IFR_PACK structure will be a series of IFR opcodes.
///
typedef struct {
  EFI_HII_PACK_HEADER Header; ///< Header of the IFR package.
} EFI_HII_IFR_PACK;

///
/// HII Handle package structure.
///
typedef struct {
  ///
  /// Header of the package.
  ///
  EFI_HII_PACK_HEADER Header;           ///< Must be filled in.
  ///
  /// The image handle of the driver to which the package is referring.
  ///
  EFI_HANDLE          ImageHandle;      ///< Must be filled in.
  ///
  /// The handle of the device that is being described by this package.
  ///
  EFI_HANDLE          DeviceHandle;     ///< Optional.
  ///
  /// The handle of the parent of the device that is being described by this package.
  ///
  EFI_HANDLE          ControllerHandle; ///< Optional.
  ///
  /// The handle that was registered to receive EFI_FORM_CALLBACK_PROTOCOL calls from other drivers.
  ///
  EFI_HANDLE          CallbackHandle;   ///< Optional.
  ///
  /// Note this field is not defined in the Framework HII 0.92 specificaiton.
  /// Unused. Reserved for source code compatibility.
  ///
  EFI_HANDLE          COBExportHandle;  ///< Optional.
} EFI_HII_HANDLE_PACK;

///
/// The variable package structure.
///
typedef struct {
  ///
  /// The header of the package.
  ///
  EFI_HII_PACK_HEADER Header;
  ///
  /// The GUID of the EFI variable.
  ///
  EFI_GUID            VariableGuid;
  ///
  /// The length in bytes of the EFI variable.
  ///
  UINT32              VariableNameLength;
  ///
  /// The unique value for this variable.
  ///
  UINT16              VariableId;
  //
  //  CHAR16                VariableName[]; //Null-terminated
  //
} EFI_HII_VARIABLE_PACK;

///
/// The device path package structure.
///
typedef struct {
  ///
  /// The header of the package.
  ///
  EFI_HII_PACK_HEADER Header;
  //
  //  EFI_DEVICE_PATH       DevicePath[];
  //
} EFI_HII_DEVICE_PATH_PACK;

typedef struct {
  ///
  /// A unique value that correlates to the original HII handle.
  ///
  FRAMEWORK_EFI_HII_HANDLE  HiiHandle;
  ///
  /// If an IFR pack exists in a data table that does not contain strings,
  /// then the strings for that IFR pack are located in another data table
  /// that contains a string pack and has a matching HiiDataTable.PackageGuid.
  ///
  EFI_GUID                  PackageGuid;
  ///
  /// The size of the EFI_HII_DATA_TABLE in bytes.
  ///
  UINT32                    DataTableSize;
  ///
  /// The byte offset from the start of this structure to the IFR data.
  /// If the offset value is 0, then no IFR data is enclosed.
  ///
  UINT32                    IfrDataOffset;
  ///
  /// The byte offset from the start of this structure to the string data.
  /// If the offset value is 0, then no string data is enclosed.
  ///
  UINT32                    StringDataOffset;
  ///
  /// The byte offset from the start of this structure to the variable data.
  /// If the offset value is 0, then no variable data is enclosed.
  ///
  UINT32                    VariableDataOffset;
  ///
  /// The byte offset from the start of this structure to the device path data.
  /// If the offset value is 0, then no DevicePath data is enclosed.
  ///
  UINT32                    DevicePathOffset;
  ///
  /// The number of VariableData[] elements in the array.
  ///
  UINT32                    NumberOfVariableData;
  ///
  /// The number of language string packages.
  ///
  UINT32                    NumberOfLanguages;
  //
  // EFI_HII_DEVICE_PATH_PACK DevicePath[];
  // EFI_HII_VARIABLE_PACK VariableData[];
  // EFI_HII_IFR_PACK IfrData;
  // EFI_HII_STRING_PACK StringData[];
  //
} EFI_HII_DATA_TABLE;

///
/// The structure defining the format for exporting data from the HII Database.
///
typedef struct {
  ///
  /// Number of EFI_HII_DATA_TABLE entries.
  ///
  UINT32    NumberOfHiiDataTables;
  ///
  /// Defines the revision of the EFI_HII_DATA_TABLE structure.
  ///
  EFI_GUID  Revision;
  //
  // EFI_HII_DATA_TABLE HiiDataTable[];
  //
} EFI_HII_EXPORT_TABLE;

///
/// The structure used to pass data to update a form or form package
/// that has previously been registered with the EFI HII database.
///
typedef struct {
  ///
  /// If TRUE, indicates that the FormCallbackHandle value will
  /// be used to update the contents of the CallBackHandle entry in the form set.
  ///
  BOOLEAN               FormSetUpdate;
  ///
  /// This parameter is valid only when FormSetUpdate is TRUE.
  /// The value in this parameter will be used to update the contents
  /// of the CallbackHandle entry in the form set.
  ///
  EFI_PHYSICAL_ADDRESS  FormCallbackHandle;
  ///
  /// If TRUE, indicates that the FormTitle contents will be
  /// used to update the FormValue's title.
  ///
  BOOLEAN               FormUpdate;
  ///
  /// Specifies which form is to be updated if the FormUpdate value is TRUE.
  ///
  UINT16                FormValue;
  ///
  /// This parameter is valid only when the FormUpdate parameter is TRUE.
  /// The value in this parameter will be used to update the contents of the form title.
  ///
  STRING_REF            FormTitle;
  ///
  /// The number of Data entries in this structure.
  UINT16                DataCount;
  ///
  /// An array of 1+ opcodes, specified by DataCount.
  ///
  UINT8                 *Data;
} EFI_HII_UPDATE_DATA;

//
// String attributes
//
#define LANG_RIGHT_TO_LEFT  0x00000001

///
/// A string package is used to localize strings to a particular
/// language.  The package is associated with a particular driver
/// or set of drivers.  Tools are used to associate tokens with
/// string references in forms and in programs.  These tokens are
/// language agnostic.  When paired with a language pack (directly
/// or indirectly), the string token resolves into an actual
/// UNICODE string.  NumStringPointers determines how many
/// StringPointers (offset values) there are, as well as the total
/// number of Strings that are defined.
///
typedef struct {
  ///
  /// The header of the package.
  ///
  EFI_HII_PACK_HEADER Header;
  ///
  /// The string containing one or more ISO 639-2 three-character designator(s)
  /// of the language or languages whose translations are contained in this language pack.
  /// The first designator indicates the primary language, while the others are secondary languages.
  ///
  RELOFST             LanguageNameString;
  ///
  /// Contains the offset into this structure of a printable name of the language
  /// for use when prompting the user. The language printed is to be the primary language.
  ///
  RELOFST             PrintableLanguageName;
  ///
  /// The number of Strings and StringPointers contained within the string package.
  ///
  UINT32              NumStringPointers;
  ///
  /// Indicates the direction the language is to be printed.
  ///
  UINT32              Attributes;
  //
  //  RELOFST               StringPointers[];
  //  EFI_STRING            Strings[];
  //
} EFI_HII_STRING_PACK;


///
/// A font list consists of a font header followed by a series
/// of glyph structures.  Note that fonts are not language specific.
///
typedef struct {
  ///
  /// The header of the package.
  ///
  EFI_HII_PACK_HEADER Header;
  ///
  /// The number of NarrowGlyphs that are included in the font package.
  ///
  UINT16              NumberOfNarrowGlyphs;
  ///
  /// The number of WideGlyphs that are included in the font package.
  ///
  UINT16              NumberOfWideGlyphs;
  //EFI_NARROW_GLYPH  NarrowGlyphs[];
  //EFI_WIDE_GLYPH    WideGlyphs[];
} EFI_HII_FONT_PACK;

///
/// The definition of a specific physical key
///
/// Note: The name difference between code and the Framework HII 0.92 specification.
///       Add FRAMEWORK_ prefix to avoid name confict with EFI_KEY_DESCRIPTOR defined in the
///       UEFI 2.1d specification.
///
typedef struct {
  ///
  /// Used to describe a physical key on a keyboard.
  ///
  EFI_KEY Key;
  ///
  /// The Unicode value for the Key.
  CHAR16  Unicode;
  ///
  /// The Unicode value for the key with the shift key being held down.
  ///
  CHAR16  ShiftedUnicode;
  ///
  /// The Unicode value for the key with the Alt-GR being held down.
  ///
  CHAR16  AltGrUnicode;
  ///
  /// The Unicode value for the key with the Alt-GR and shift keys being held down.
  ///
  CHAR16  ShiftedAltGrUnicode;
  ///
  /// Modifier keys are defined to allow for special functionality that
  /// is not necessarily accomplished by a printable character.
  ///
  UINT16  Modifier;
} FRAMEWORK_EFI_KEY_DESCRIPTOR;

///
/// This structure allows a sparse set of keys to be redefined
/// or a complete redefinition of the keyboard layout.  Most
/// keyboards have a lot of commonality in their layouts, therefore
/// only defining those keys that need to change from the default
/// minimizes the passed in information.
///
/// Additionally, when an update occurs, the active keyboard layout
/// will be switched to the newly updated keyboard layout.  This
/// allows for situations that when a keyboard layout driver is
/// loaded as part of system initialization, the system will default
/// the keyboard behavior to the new layout.
///
typedef struct {
  ///
  /// The header of the package.
  EFI_HII_PACK_HEADER           Header;
  ///
  /// A pointer to a buffer containing an array of EFI_KEY_DESCRIPTOR entries.
  /// Each entry will reflect the definition of a specific physical key.
  ///
  FRAMEWORK_EFI_KEY_DESCRIPTOR  *Descriptor;
  ///
  /// The number of Descriptor entries being described.
  ///
  UINT8                         DescriptorCount;
} EFI_HII_KEYBOARD_PACK;

///
/// The packages structure that will be used to pass contents into the HII database.
///
/// The EFI_HII_PACKAGES can contain various number of packages of different types just
/// after the structure as inline data.
///
typedef struct {
  ///
  /// The number of packages being defined in this structure.
  ///
  UINTN     NumberOfPackages;
  ///
  /// The GUID to be used to identify this set of packages that are being exported
  /// to the HII database.
  ///
  EFI_GUID  *GuidId;
  //
  // EFI_HII_HANDLE_PACK    *HandlePack;        // Only one pack.
  // EFI_HII_IFR_PACK       *IfrPack;           // Only one pack.
  // EFI_HII_FONT_PACK      *FontPack[];        // Multiple packs ok
  // EFI_HII_STRING_PACK    *StringPack[];      // Multiple packs ok
  // EFI_HII_KEYBOARD_PACK  *KeyboardPack[];    // Multiple packs ok
  //
} EFI_HII_PACKAGES;

///
/// The packed link list that contains all the discernable defaults of variables
/// for the opcodes that are defined in this Handle's domain of data.
///
typedef struct _EFI_HII_VARIABLE_PACK_LIST {
  ///
  /// A pointer points to the next data structure of type
  /// EFI_HII_VARIABLE_PACK_LIST in the packed link list.
  ///
  struct _EFI_HII_VARIABLE_PACK_LIST   *NextVariablePack;
  ///
  /// A pointer points to the content of the variable entry defined by GUID/name/data.
  ///
  EFI_HII_VARIABLE_PACK                *VariablePack;
  //EFI_HII_VARIABLE_PACK              Content
} EFI_HII_VARIABLE_PACK_LIST;


#pragma pack()

/**
  Registers the various packs that are passed in via the Packages parameter.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Packages              A pointer to an EFI_HII_PACKAGES package instance.
  @param  Handle                A pointer to the FRAMEWORK_EFI_HII_HANDLE instance.

  @retval EFI_SUCCESS           Data was extracted from Packages, the database
                                was updated with the data, and Handle returned successfully.
  @retval EFI_INVALID_PARAMETER The content of Packages was invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_NEW_PACK)(
  IN  EFI_HII_PROTOCOL          *This,
  IN  EFI_HII_PACKAGES          *Packages,
  OUT FRAMEWORK_EFI_HII_HANDLE  *Handle
  );

/**
  Removes a package from the HII database.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The handle that was registered to the data that
                                is requested for removal.

  @retval EFI_SUCCESS           The data associated with the Handle was removed
                                from the HII database.
  @retval EFI_INVALID_PARAMETER The Handle was not valid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_REMOVE_PACK)(
  IN EFI_HII_PROTOCOL          *This,
  IN FRAMEWORK_EFI_HII_HANDLE  Handle
  );

/**
  Determines the handles that are currently active in the database.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  HandleBufferLength    On input, a pointer to the length of the handle
                                buffer. On output, the length of the handle buffer that is required
                                for the handles found.
  @param  Handle                An array of FRAMEWORK_EFI_HII_HANDLE  instances returned.

  @retval EFI_SUCCESS           Handle was updated successfully.
  @retval EFI_BUFFER_TOO_SMALL  The HandleBufferLength parameter indicates
                                that Handle is too small to support the number of handles.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_FIND_HANDLES)(
  IN     EFI_HII_PROTOCOL          *This,
  IN OUT UINT16                    *HandleBufferLength,
  OUT    FRAMEWORK_EFI_HII_HANDLE  *Handle
  );

/**
  Exports the contents of the database into a buffer.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                A FRAMEWORK_EFI_HII_HANDLE that corresponds to the desired
                                handle to export. If the value is 0, the entire database will be exported.
                                The data is exported in a format described by the
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
(EFIAPI *EFI_HII_EXPORT)(
  IN     EFI_HII_PROTOCOL          *This,
  IN     FRAMEWORK_EFI_HII_HANDLE  Handle,
  IN OUT UINTN                     *BufferSize,
  OUT    VOID                      *Buffer
  );

/**
  Remove any new strings that were added after the initial string export
  for this handle.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The handle on which the string resides.

  @retval EFI_SUCCESS           Successfully removed strings from the handle.
  @retval EFI_INVALID_PARAMETER The Handle was unknown.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_RESET_STRINGS)(
  IN EFI_HII_PROTOCOL          *This,
  IN FRAMEWORK_EFI_HII_HANDLE  Handle
  );

/**
  Tests if all of the characters in a string have corresponding font characters.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  StringToTest          A pointer to a Unicode string.
  @param  FirstMissing          A pointer to an index into the string. On input,
                                the index of the first character in the StringToTest
                                to examine. On exit, the index of the first character
                                encountered for which a glyph is unavailable.
                                If all glyphs in the string are available, the
                                index is the index of the terminator of the string.
  @param  GlyphBufferSize       A pointer to a value. On output, if the function
                                returns EFI_SUCCESS, it contains the amount of
                                memory that is required to store the string's
                                glyph equivalent.

  @retval EFI_SUCCESS           All glyphs are available. Note that an empty string
                                always returns this value.
  @retval EFI_NOT_FOUND         A glyph was not found for a character.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_TEST_STRING)(
  IN     EFI_HII_PROTOCOL  *This,
  IN     CHAR16            *StringToTest,
  IN OUT UINT32            *FirstMissing,
  OUT    UINT32            *GlyphBufferSize
  );

/**
  Translates a Unicode character into the corresponding font glyph.

  Note that this function prototype name is different from that in the Framework HII 0.92 specification
  to avoid name confict with EFI_HII_GET_GLYPH defined in the UEFI 2.1d specification.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Source                A pointer to a Unicode string.
  @param  Index                 On input, the offset into the string from which to
                                fetch the character. On successful completion, the
                                index is updated to the first character past the
                                character(s) making up the just extracted glyph.
  @param  GlyphBuffer           Pointer to an array where the glyphs corresponding
                                to the characters in the source may be stored.
                                GlyphBuffer is assumed to be wide enough to accept
                                a wide glyph character.
  @param  BitWidth              If EFI_SUCCESS was returned, the UINT16 pointed to by
                                this value is filled with the length of the glyph in
                                pixels. It is unchanged if the call was unsuccessful.
  @param  InternalStatus        The cell pointed to by this parameter must be
                                initialized to zero prior to invoking the call the
                                first time for any string.

  @retval EFI_SUCCESS           Found the corresponding font glyph for a Unicode
                                character.
  @retval EFI_NOT_FOUND         A glyph for a character was not found.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_HII_GET_GLYPH)(
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

  @retval EFI_SUCCESS           Successfully translated a glyph into the required
                                format for input to UGA BLT routines.
  @retval EFI_NOT_FOUND         A glyph for a character was not found.
  @note  Inconsistent with specification here:
         In Framework Spec, HII specification 0.92. The type of 3rd, 4th and 8th parameter is EFI_UGA_PIXEL.
         Here the definition uses the EFI_GRAPHICS_OUTPUT_BLT_PIXEL, which is defined in UEFI 2.1 specification.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GLYPH_TO_BLT)(
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

  Note that this function prototype name is different from that in the Framework HII 0.92 specification
  to avoid name confict with EFI_HII_NEW_STRING defined in the UEFI 2.1d specification.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Pointer               to a NULL-terminated string containing a single
                                ISO 639-2 language identifier, indicating the language
                                in which the string is translated.
  @param  Handle                The handle of the language pack to which the string
                                is to be added.
  @param  Reference             The identifier of the string to be added. If the
                                reference value is zero, then the string will be
                                assigned a new identifier on that handle for
                                the language specified. Otherwise, the string will
                                be updated with the NewString Value.
  @param  NewString             The string to be added.

  @retval EFI_SUCCESS           The string was effectively registered.
  @retval EFI_INVALID_PARAMETER The Handle was unknown.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_HII_NEW_STRING)(
  IN     EFI_HII_PROTOCOL          *This,
  IN     CHAR16                    *Language,
  IN     FRAMEWORK_EFI_HII_HANDLE  Handle,
  IN OUT STRING_REF                *Reference,
  IN     CHAR16                    *NewString
  );

/**
  Allows a program to determine the primary languages that are supported
  on a given handle.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The handle on which the strings reside.
  @param  LanguageString        A string allocated by GetPrimaryLanguages() that
                                contains a list of all primary languages registered
                                on the handle.

  @retval EFI_SUCCESS           LanguageString was correctly returned.
  @retval EFI_INVALID_PARAMETER The Handle was unknown.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_PRI_LANGUAGES)(
  IN  EFI_HII_PROTOCOL          *This,
  IN  FRAMEWORK_EFI_HII_HANDLE  Handle,
  OUT EFI_STRING                *LanguageString
  );

/**
  Allows a program to determine which secondary languages are supported
  on a given handle for a given primary language.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The handle on which the strings reside.
  @param  PrimaryLanguage       Pointer to a NULL-terminated string containing a
                                single ISO 639-2 language identifier, indicating
                                the primary language.
  @param  LanguageString        A string allocated by GetSecondaryLanguages()
                                containing a list of all secondary languages
                                registered on the handle.

  @retval EFI_SUCCESS           LanguageString was correctly returned.
  @retval EFI_INVALID_PARAMETER The Handle was unknown.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_SEC_LANGUAGES)(
  IN  EFI_HII_PROTOCOL          *This,
  IN  FRAMEWORK_EFI_HII_HANDLE  Handle,
  IN  CHAR16                    *PrimaryLanguage,
  OUT EFI_STRING                *LanguageString
  );

/**
  Extracts a string from a package already registered with the EFI HII database.

  Note that this function prototype name is different from that in the Framework HII 0.92 specification
  to avoid name confict with EFI_HII_GET_STRING defined in the UEFI 2.1d specification.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The handle on which the string resides.
  @param  Token                 The string token assigned to the string.
  @param  Raw                   If TRUE, the string is returned unedited in the
                                internal storage format. If false, the string
                                returned is edited by replacing <cr> with <space>
                                and by removing special characters such as the
                                <wide> prefix.
  @param  LanguageString        Pointer to a NULL-terminated string containing a
                                single ISO 639-2 language identifier, indicating
                                the language to print. If the LanguageString is
                                empty (starts with a NULL), the default system
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
(EFIAPI *FRAMEWORK_EFI_HII_GET_STRING)(
  IN     EFI_HII_PROTOCOL          *This,
  IN     FRAMEWORK_EFI_HII_HANDLE  Handle,
  IN     STRING_REF                Token,
  IN     BOOLEAN                   Raw,
  IN     CHAR16                    *LanguageString,
  IN OUT UINTN                     *BufferLength,
  OUT    EFI_STRING                StringBuffer
  );

/**
  Allows a program to extract a part of a string of not more than a given width.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The handle on which the string resides.
  @param  Token                 The string token assigned to the string.
  @param  Index                 On input, the offset into the string where the
                                line is to start. On output, the index is updated
                                to point beyond the last character returned in
                                the call.
  @param  LineWidth             The maximum width of the line in units of narrow glyphs.
  @param  LanguageString        The pointer to a NULL-terminated string containing a
                                single ISO 639-2 language identifier, indicating
                                the language to print.
  @param  BufferLength          The pointer to the length of the StringBuffer.
  @param  StringBuffer          The buffer designed to receive the characters in
                                the string.

  @retval EFI_SUCCESS           StringBuffer filled with characters that will fit
                                on the line.
  @retval EFI_NOT_FOUND         The font glyph for at least one of the characters in
                                the string is not in the font database.
  @retval EFI_BUFFER_TOO_SMALL  The buffer provided was not large enough
                                to allow the entire string to be stored.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_LINE)(
  IN     EFI_HII_PROTOCOL          *This,
  IN     FRAMEWORK_EFI_HII_HANDLE  Handle,
  IN     STRING_REF                Token,
  IN OUT UINT16                    *Index,
  IN     UINT16                    LineWidth,
  IN     CHAR16                    *LanguageString,
  IN OUT UINT16                    *BufferLength,
  OUT    EFI_STRING                StringBuffer
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
(EFIAPI *EFI_HII_GET_FORMS)(
  IN     EFI_HII_PROTOCOL          *This,
  IN     FRAMEWORK_EFI_HII_HANDLE  Handle,
  IN     EFI_FORM_ID               FormId,
  IN OUT UINTN                     *BufferLength,
  OUT    UINT8                     *Buffer
  );

/**
  Extracts the defaults that are associated with a given handle in the HII database.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  Handle                The HII handle from which will have default data retrieved.
  @param  DefaultMask           The mask used to specify some type of default
                                override when extracting the default image data.
  @param  VariablePackList      An indirect pointer to the first entry of a link
                                list with type EFI_HII_VARIABLE_PACK_LIST.

  @retval EFI_SUCCESS           The VariablePackList was populated with the appropriate
                                default setting data.
  @retval EFI_NOT_FOUND         The IFR does not have any explicit or default map(s).
  @retval EFI_INVALID_PARAMETER The HII database entry associated with Handle
                                contains invalid data.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_GET_DEFAULT_IMAGE)(
  IN     EFI_HII_PROTOCOL           *This,
  IN     FRAMEWORK_EFI_HII_HANDLE   Handle,
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
(EFIAPI *EFI_HII_UPDATE_FORM)(
  IN EFI_HII_PROTOCOL          *This,
  IN FRAMEWORK_EFI_HII_HANDLE  Handle,
  IN EFI_FORM_LABEL            Label,
  IN BOOLEAN                   AddData,
  IN EFI_HII_UPDATE_DATA       *Data
  );

/**
  Retrieves the current keyboard layout.

  Note that this function prototype name is different from that in the Framework HII 0.92 specification
  to avoid name confict with EFI_HII_GET_KEYBOARD_LAYOUT defined in the UEFI 2.1d specification.

  @param  This                  A pointer to the EFI_HII_PROTOCOL instance.
  @param  DescriptorCount       A pointer to the number of Descriptor entries being
                                described in the keyboard layout being retrieved.
  @param  Descriptor            A pointer to a buffer containing an array of
                                FRAMEWORK_EFI_KEY_DESCRIPTOR entries. Each entry
                                will reflect the definition of a specific physical key.

  @retval EFI_SUCCESS           The keyboard layout was retrieved successfully.

**/
typedef
EFI_STATUS
(EFIAPI *FRAMEWORK_EFI_HII_GET_KEYBOARD_LAYOUT)(
  IN     EFI_HII_PROTOCOL              *This,
  OUT    UINT16                        *DescriptorCount,
  OUT    FRAMEWORK_EFI_KEY_DESCRIPTOR  *Descriptor
  );

///
///  The HII Protocol manages the HII database, which is a repository for data
///  having to do with fonts, strings, forms, keyboards, and other future human
///  interface items.
///
struct _EFI_HII_PROTOCOL {
  ///
  /// Extracts the various packs from a package list.
  ///
  EFI_HII_NEW_PACK                      NewPack;

  ///
  /// Removes a package from the HII database.
  ///
  EFI_HII_REMOVE_PACK                   RemovePack;

  ///
  /// Determines the handles that are currently active in the database.
  ///
  EFI_HII_FIND_HANDLES                  FindHandles;

  ///
  /// Exports the entire contents of the database to a buffer.
  ///
  EFI_HII_EXPORT                        ExportDatabase;

  ///
  /// Tests if all of the characters in a string have corresponding font characters.
  ///
  EFI_HII_TEST_STRING                   TestString;

  ///
  /// Translates a Unicode character into the corresponding font glyph.
  ///
  FRAMEWORK_EFI_HII_GET_GLYPH           GetGlyph;

  ///
  /// Converts a glyph value into a format that is ready for a UGA BLT command.
  ///
  EFI_HII_GLYPH_TO_BLT                  GlyphToBlt;

  ///
  /// Allows a new string to be added to an already existing string package.
  ///
  FRAMEWORK_EFI_HII_NEW_STRING          NewString;

  ///
  /// Allows a program to determine the primary languages that are supported
  /// on a given handle.
  ///
  EFI_HII_GET_PRI_LANGUAGES             GetPrimaryLanguages;

  ///
  /// Allows a program to determine which secondary languages are supported
  /// on a given handle for a given primary language.
  ///
  EFI_HII_GET_SEC_LANGUAGES             GetSecondaryLanguages;

  ///
  /// Extracts a string from a package that is already registered with the
  /// EFI HII database.
  ///
  FRAMEWORK_EFI_HII_GET_STRING          GetString;

  ///
  /// Removes any new strings that were added after the initial string export
  /// for this handle.
  ///
  /// Note this function is not defined in the Framework HII 0.92 specification.
  ///
  EFI_HII_RESET_STRINGS                 ResetStrings;

  ///
  /// Allows a program to extract a part of a string of not more than a given width.
  ///
  EFI_HII_GET_LINE                      GetLine;

  ///
  /// Allows a program to extract a form or form package that has been previously registered.
  ///
  EFI_HII_GET_FORMS                     GetForms;

  ///
  /// Allows a program to extract the nonvolatile image that represents the default storage image.
  ///
  EFI_HII_GET_DEFAULT_IMAGE             GetDefaultImage;

  ///
  /// Allows a program to update a previously registered form.
  ///
  EFI_HII_UPDATE_FORM                   UpdateForm;

  ///
  /// Allows a program to extract the current keyboard layout.
  ///
  FRAMEWORK_EFI_HII_GET_KEYBOARD_LAYOUT GetKeyboardLayout;
};

extern EFI_GUID gEfiHiiProtocolGuid;
extern EFI_GUID gEfiHiiCompatibilityProtocolGuid;

#endif

