/** @file
  
  Module to rewrite stdlib references within Oniguruma

  (C) Copyright 2014-2015 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
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
