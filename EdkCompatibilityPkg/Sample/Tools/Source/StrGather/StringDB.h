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
  WCHAR *PrintableLanguageName
  )
;

STATUS
StringDBDumpCStrings (
  INT8                        *FileName,
  INT8                        *BaseName,
  WCHAR_STRING_LIST           *LanguagesOfInterest,
  WCHAR_MATCHING_STRING_LIST  *IndirectionList
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

STATUS
StringDBCreateHiiExportPack (
  INT8                *OutputFileName,
  WCHAR_STRING_LIST   *LanguagesOfInterest
  );


#endif // #ifndef _STRING_DB_H_
