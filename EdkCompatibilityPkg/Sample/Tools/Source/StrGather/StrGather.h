/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  StrGather.h

Abstract:

  Common defines and prototypes for StrGather.
  
--*/

#ifndef _STR_GATHER_H_
#define _STR_GATHER_H_

#define MALLOC(size)  malloc (size)
#define FREE(ptr)     free (ptr)

typedef CHAR16  WCHAR;

#define UNICODE_TO_ASCII(w)   (INT8) ((w) & 0xFF)
#define ASCII_TO_UNICODE(a)   (WCHAR) ((UINT8) (a))

#define UNICODE_HASH          L'#'
#define UNICODE_BACKSLASH     L'\\'
#define UNICODE_SLASH         L'/'
#define UNICODE_EQUAL_SIGN    L'='
#define UNICODE_PLUS_SIGN     L'+'

#define UNICODE_FILE_START    0xFEFF
#define UNICODE_CR            0x000D
#define UNICODE_LF            0x000A
#define UNICODE_NULL          0x0000
#define UNICODE_SPACE         L' '
#define UNICODE_SLASH         L'/'
#define UNICODE_DOUBLE_QUOTE  L'"'
#define UNICODE_Z             L'Z'
#define UNICODE_z             L'z'
#define UNICODE_A             L'A'
#define UNICODE_a             L'a'
#define UNICODE_F             L'F'
#define UNICODE_f             L'f'
#define UNICODE_UNDERSCORE    L'_'
#define UNICODE_0             L'0'
#define UNICODE_9             L'9'
#define UNICODE_TAB           L'\t'
#define UNICODE_NBR_STRING    L"\\nbr"
#define UNICODE_BR_STRING     L"\\br"
#define UNICODE_WIDE_STRING   L"\\wide"
#define UNICODE_NARROW_STRING L"\\narrow"

//
// This is the length of a valid string identifier
//
#define LANGUAGE_IDENTIFIER_NAME_LEN  3

typedef struct _TEXT_STRING_LIST {
  struct _TEXT_STRING_LIST  *Next;
  UINT8                     *Str;
} TEXT_STRING_LIST;

typedef struct _WCHAR_STRING_LIST {
  struct _WCHAR_STRING_LIST *Next;
  WCHAR                     *Str;
} WCHAR_STRING_LIST;

typedef struct _WCHAR_MATCHING_STRING_LIST {
  struct _WCHAR_MATCHING_STRING_LIST  *Next;
  WCHAR                               *Str1;
  WCHAR                               *Str2;
} WCHAR_MATCHING_STRING_LIST;

#endif // #ifndef _STR_GATHER_H_
