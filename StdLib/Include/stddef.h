/** @file
  Common Definitions.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _STDDEF_H
#define _STDDEF_H
#include  <sys/EfiCdefs.h>

/** ptrdiff_t is the signed integer type of the result of subtracting two pointers.
**/
#ifdef _EFI_PTRDIFF_T_
  typedef _EFI_PTRDIFF_T_  ptrdiff_t;
  #undef _EFI_PTRDIFF_T_
#endif

/** size_t is the unsigned integer type of the result of the sizeof operator.
**/
#ifdef _EFI_SIZE_T_
  typedef _EFI_SIZE_T_  size_t;
  #undef _EFI_SIZE_T_
  #undef _BSD_SIZE_T_
#endif

/** wchar_t is an integer type whose range of values can represent distinct
    codes for all members of the largest extended character set specified among
    the supported locales.  The null character shall have the code value zero.
**/
#ifndef __cplusplus
  #ifdef _EFI_WCHAR_T
    typedef _EFI_WCHAR_T wchar_t;
    #undef  _EFI_WCHAR_T
    #undef _BSD_WCHAR_T_
  #endif
#endif

/** NULL expands to an implementation-defined null pointer constant.
    NULL is defined in MdePkg/Include/Base.h which is automatically included
    by the EDK II build tools.
**/

/** offsetof(type, member-designator) expands to an integer constant expression
    that has type size_t, the value of which is the offset in bytes, to the
    structure member (designated by member-designator), from the beginning of
    its structure (designated by type). The type and member designator shall be
    such that given<BR>
    static type t;<BR>
    then the expression &(t.member-designator) evaluates to an address constant.
    (If the specified member is a bit-field, the behavior is undefined.)

    Alliased to OFFSET_OF which is defined in MdePkg/Include/Base.h which is
    automatically included by the EDK II build tools.
**/
#define offsetof(type, member)  OFFSET_OF(type, member)

#endif  /* _STDDEF_H */
