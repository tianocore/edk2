/** @file
  Implementation specific support for Single-byte character classification and
  case conversion macros and function declarations.

  This file is intended to only be included by <ctype.h>.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _CTYPE_H
#error  This file, <sys/_ctype.h>, may only be included by <ctype.h>.
#endif

__BEGIN_DECLS
extern const UINT16 *_cClass;  ///< Locale independent pointer to Character Classification Table.
extern const UINT8  *_uConvT;  ///< Locale independent pointer to Lowercase to Uppercase Conversion Table.
extern const UINT8  *_lConvT;  ///< Locale independent pointer to Uppercase to Lowercase Conversion Table.

extern  int  __isCClass( int _c, unsigned int mask);   ///< Internal character classification function.
__END_DECLS


/** @{
Character Class bit masks.
**/
#define _CC   0x0001U     ///< Control Characters
#define _CW   0x0002U     ///< White Space
#define _CP   0x0004U     ///< Punctuation
#define _CD   0x0008U     ///< Digits [0-9]
#define _CU   0x0010U     ///< Uppercase Letter [A-Z]
#define _CL   0x0020U     ///< Lowercase Letter [a-z]
#define _CX   0x0040U     ///< Hexadecimal Digits [A-Fa-f]
#define _C0   0x0080U     ///< Path Separator Characters, '/' and '\\'
#define _CS   0x0100U     ///< Space Characters, ' ' in C locale
#define _CG   0x0200U     ///< Graphic Characters
#define _CB   0x0400U     ///< Blank Characters, ' ' and '\t' in C locale
#define _C4   0x0800U
#define _XA   0x1000U     ///< eXtra Alpha characters not in _CU or _CL
#define _C6   0x2000U
#define _C7   0x4000U
#define _C8   0x8000U
/// @}

#ifndef NO_CTYPE_MACROS
  #define __isCClass( _c, mask)   (((_c) < 0 || (_c) > 127) ? 0 : (_cClass[(_c)] & (mask)))
  #define __toLower( _c)          ((__isCClass( ((int)_c), (_CU))) ? _lConvT[(_c)] : (_c))
  #define __toUpper( _c)          ((__isCClass( ((int)_c), (_CL))) ? _uConvT[(_c)] : (_c))
#endif  /* NO_CTYPE_MACROS */

/* Macros used by implementation functions */
#define __isHexLetter(_c)         (__isCClass( (int)c, (_CX)))

#ifdef _CTYPE_PRIVATE
  #define _CTYPE_NUM_CHARS  (256)

  #define _CTYPE_ID   "BSDCTYPE"
  #define _CTYPE_REV    2

  extern const UINT16  _C_CharClassTable[];
  extern const UINT8   _C_ToUpperTable[];
  extern const UINT8   _C_ToLowerTable[];
#endif
