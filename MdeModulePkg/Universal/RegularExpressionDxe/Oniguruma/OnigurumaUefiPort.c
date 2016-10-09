/** @file
  
  Module to rewrite stdlib references within Oniguruma

  (C) Copyright 2014-2015 Hewlett Packard Enterprise Development LP<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License that accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include "OnigurumaUefiPort.h"

int EFIAPI sprintf_s(char *str, size_t sizeOfBuffer, char const *fmt, ...)
{
  VA_LIST Marker;
  int   NumberOfPrinted;

  VA_START (Marker, fmt);
  NumberOfPrinted = (int)AsciiVSPrint (str, sizeOfBuffer, fmt, Marker);
  VA_END (Marker);

  return NumberOfPrinted;
}

int OnigStrCmp (char* Str1, char* Str2)
{
  return (int)AsciiStrCmp (Str1, Str2);
}
