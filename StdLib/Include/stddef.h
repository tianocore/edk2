/** @file
  Common "Standard" Definitions.

  The files stddef.h and stdlib.h are "catch all" headers for definitions and declarations
  that don't fit well in the other headers.  There are two separate header files because
  the contents of <stddef.h> are valid in both freestanding and hosted environment, while the
  header <stdlib.h> contains elements that are only valid in a hosted environment.

  This means that the elements in this file may not impose dependencies on headers other than
  <float.h>, <iso646.h>, <limits.h>, <stdarg.h>, <stdbool.h>, and (of course) <sys/EfiCdefs.h>.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _STDDEF_H
#define _STDDEF_H
#include  <sys/EfiCdefs.h>

#ifdef _EFI_PTRDIFF_T_
  /** ptrdiff_t is the signed integer type of the result of subtracting two pointers. **/
  typedef _EFI_PTRDIFF_T_  ptrdiff_t;
  #undef _EFI_PTRDIFF_T_
#endif

#ifdef _EFI_SIZE_T_
  /** size_t is the unsigned integer type of the result of the sizeof operator. **/
  typedef _EFI_SIZE_T_  size_t;
  #undef _EFI_SIZE_T_
  #undef _BSD_SIZE_T_
#endif

#ifndef __cplusplus
  #ifdef _EFI_WCHAR_T
    /** wchar_t is an integer type whose range of values can represent distinct
        codes for all members of the largest extended character set specified among
        the supported locales.  The null character shall have the code value zero.
    **/
    typedef _EFI_WCHAR_T wchar_t;
    #undef  _EFI_WCHAR_T
    #undef _BSD_WCHAR_T_
  #endif
#endif

/** @def NULL
    A macro that expands to a null pointer constant.<BR>
    NULL is defined in MdePkg/Include/Base.h which is automatically included
    by the EDK II build tools.
**/

/** The offsetof macro determines the offset of the beginning of a structure
    member from the beginning of the structure.

    The macro expands to an integer constant expression that has type size_t,
    the value of which is the offset in bytes, to the structure member (Member),
    from the beginning of its structure (StrucName).

    Alliased to OFFSET_OF which is defined in MdePkg/Include/Base.h which is
    automatically included by the EDK II build tools.
**/
#define offsetof(StrucName, Member)  OFFSET_OF(StrucName, Member)

#endif  /* _STDDEF_H */
