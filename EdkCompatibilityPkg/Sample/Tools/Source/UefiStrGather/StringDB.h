/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StringDB.h

Abstract:

  Common defines and prototypes for string database management
  
--*/

#ifndef _STRING_DB_H_
#define _STRING_DB_H_

#define LANGUAGE_NAME_STRING_NAME           L"$LANGUAGE_NAME"
#define PRINTABLE_LANGUAGE_NAME_STRING_NAME L"$PRINTABLE_LANGUAGE_NAME"

typedef CHAR16  WCHAR;

#define NARROW_CHAR         0xFFF0
#define WIDE_CHAR           0xFFF1
#define NON_BREAKING_CHAR   0xFFF2
#define GLYPH_WIDTH         8
#define GLYPH_HEIGHT        19

#define STRING_DB_KEY (('S' << 24) | ('D' << 16) | ('B' << 8) | 'K')
//
// Version supported by this tool
//
#define STRING_DB_VERSION             0x00010000

#define STRING_DB_MAJOR_VERSION_MASK  0xFFFF0000
#define STRING_DB_MINOR_VERSION_MASK  0x0000FFFF

#define DEFINE_STR                    L"// #define"

#define EFI_STRING_ID_BEGIN           0x01

//
// This is the header that gets written to the top of the
// output binary database file.
//
typedef struct {
  UINT32  Key;
  UINT32  HeaderSize;
  UINT32  Version;
  UINT32  NumStringIdenfiers;
  UINT32  StringIdentifiersSize;
  UINT32  NumLanguages;
} STRING_DB_HEADER;

//
// When we write out data to the database, we have a UINT16 identifier, which
// indicates what follows, followed by the data. Here's the structure.
//
typedef struct {
  UINT16  DataType;
  UINT16  Reserved;
} DB_DATA_ITEM_HEADER;

#define DB_DATA_TYPE_INVALID              0x0000
#define DB_DATA_TYPE_STRING_IDENTIFIER    0x0001
#define DB_DATA_TYPE_LANGUAGE_DEFINITION  0x0002
#define DB_DATA_TYPE_STRING_DEFINITION    0x0003
#define DB_DATA_TYPE_LAST                 DB_DATA_TYPE_STRING_DEFINITION

//
// We have to keep track of a list of languages, each of which has its own
// list of strings. Define a structure to keep track of all languages and
// their list of strings.
//
typedef struct _STRING_LIST {
  struct _STRING_LIST *Next;
  UINT32              Size;         // number of bytes in string, including null terminator
  WCHAR               *LanguageName;
  WCHAR               *StringName;  // for example STR_ID_TEXT1
  WCHAR               *Scope;       //
  WCHAR               *Str;         // the actual string
  UINT16              Flags;        // properties of this string (used, undefined)
} STRING_LIST;

typedef struct _LANGUAGE_LIST {
  struct _LANGUAGE_LIST *Next;
  WCHAR                 *LanguageName;
  WCHAR                 *PrintableLanguageName;
  WCHAR                 *SecondaryLanguageList;
  STRING_LIST           *String;
  STRING_LIST           *LastString;
} LANGUAGE_LIST;

//
// We also keep track of all the string identifier names, which we assign unique
// values to. Create a structure to keep track of them all.
//
typedef struct _STRING_IDENTIFIER {
  struct _STRING_IDENTIFIER *Next;
  UINT32                    Index;  // only need 16 bits, but makes it easier with UINT32
  WCHAR                     *StringName;
  UINT16                    Flags;  // if someone referenced it via STRING_TOKEN()
} STRING_IDENTIFIER;
//
// Keep our globals in this structure to be as modular as possible.
//
typedef struct {
  FILE              *StringDBFptr;
  LANGUAGE_LIST     *LanguageList;
  LANGUAGE_LIST     *LastLanguageList;
  LANGUAGE_LIST     *CurrentLanguage;         // keep track of the last language they used
  STRING_IDENTIFIER *StringIdentifier;
  STRING_IDENTIFIER *LastStringIdentifier;
  UINT8             *StringDBFileName;
  UINT32            NumStringIdentifiers;
  UINT32            NumStringIdentifiersReferenced;
  STRING_IDENTIFIER *CurrentStringIdentifier; // keep track of the last string identifier they added
  WCHAR             *CurrentScope;
} STRING_DB_DATA;

typedef struct _SPkgBlkBuffer {
  UINT32                mBlkSize;
  VOID                  *mBlkBuffer;
  struct _SPkgBlkBuffer *mNext;
} SPkgBlkBuffer;

void
StringDBConstructor (
  void
  )
;
void
StringDBDestructor (
  void
  )
;

STATUS
StringDBAddString (
  WCHAR   *LanguageName,
  WCHAR   *StringIdentifier,
  WCHAR   *Scope,
  WCHAR   *String,
  BOOLEAN Format,
  UINT16  Flags
  )
;

STATUS
StringDBSetScope (
  WCHAR   *Scope
  )
;

#define STRING_FLAGS_REFERENCED           0x0001  // if referenced somewhere
#define STRING_FLAGS_UNDEFINED            0x0002  // if we added it for padding purposes
#define STRING_FLAGS_INDEX_ASSIGNED       0x0004  // so don't change the index value
#define STRING_ID_INVALID                 0xFFFF
#define STRING_ID_LANGUAGE_NAME           0x0000
#define STRING_ID_PRINTABLE_LANGUAGE_NAME 0x0001

STATUS
StringDBAddStringIdentifier (
  WCHAR     *StringIdentifier,
  UINT16    *NewId,
  UINT16    Flags
  )
;

STATUS
StringDBReadDatabase (
  INT8    *DBFileName,
  BOOLEAN IgnoreIfNotExist,
  BOOLEAN Verbose
  )
;

STATUS
StringDBWriteDatabase (
  INT8    *DBFileName,
  BOOLEAN Verbose
  )
;

STATUS
StringDBDumpDatabase (
  INT8                *DBFileName,
  INT8                *OutputFileName,
  BOOLEAN             Verbose
  )
;

STATUS
StringDBAddLanguage (
  WCHAR *LanguageName,
  WCHAR *PrintableLanguageName,
  WCHAR *SecondaryLanguageList
  )
;

STATUS
StringDBAddSecondaryLanguage (
  WCHAR *LanguageName,
  WCHAR *SecondaryLanguageList
  )
;

STATUS
StringDBDumpCStrings (
  INT8                            *BaseName,
  INT8                            *FileName,
  WCHAR_STRING_LIST               *LanguagesOfInterests
  )
;

STATUS
StringDBCreateHiiExportPack (
  INT8                            *OutputFileName,
  WCHAR_STRING_LIST               *LanguagesOfInterests
  )
;

STATUS
StringDBDumpStringDefines (
  INT8                *FileName,
  INT8                *BaseName
  )
;

STATUS
StringDBSetCurrentLanguage (
  WCHAR *LanguageName
  )
;

STATUS
StringDBSetStringReferenced (
  INT8      *StringIdentifierName,
  BOOLEAN   IgnoreNotFound
  )
;

void
StringDBFormatString (
  WCHAR   *String
  )
;

LANGUAGE_LIST *
StringDBFindLanguageList (
  WCHAR *LanguageName
  )
;

#endif // #ifndef _STRING_DB_H_
