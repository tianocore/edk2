/** @file
Private MACRO, structure and function definitions for ITK.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/
#ifndef _ItkSupport_H_
#define _ItkSupport_H_

#include "Setup.h"
#include <Protocol/ITK50PostBoot.h>

/**
  Reset Question to its default value from ITK.

  @param  FormSet                The form set.
  @param  Form                   The form.
  @param  Question               The question.
  @param  DefaultId              The Class of the default.

  @retval EFI_SUCCESS            Question is reset to default value.

**/
EFI_STATUS
GetDefaultValueFromITK (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_STATEMENT           *Question,
  IN UINT16                           DefaultId
  );

/**
  Generate a sub string then output it.

  This is a internal function.

  @param  String                 A constant string which is the prefix of the to be
                                 generated string, e.g. GUID=

  @param  BufferLen              The length of the Buffer in bytes.

  @param  Buffer                 Points to a buffer which will be converted to be the
                                 content of the generated string.

  @param  Flag                   If 1, the buffer contains data for the value of GUID or PATH stored in
                                 UINT8 *; if 2, the buffer contains unicode string for the value of NAME;
                                 if 3, the buffer contains other data.

  @param  SubStr                 Points to the output string. It's caller's
                                 responsibility to free this buffer.


**/
VOID
GenerateSubStr (
  IN CONST EFI_STRING              String,
  IN  UINTN                        BufferLen,
  IN  VOID                         *Buffer,
  IN  UINT8                        Flag,
  OUT EFI_STRING                   *SubStr
  );

/**
  Converts the unicode character of the string from uppercase to lowercase.
  This is a internal function.

  @param ConfigString  String to be converted

**/
VOID
EFIAPI
HiiToLower (
  IN EFI_STRING  ConfigString
  );

#endif
