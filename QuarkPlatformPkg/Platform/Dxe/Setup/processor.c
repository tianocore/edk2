/** @file
Platform CPU Data

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "CommonHeader.h"

#include "SetupPlatform.h"


#define NUMBER_OF_PACKAGES 1

CHAR16 *SocketNames[NUMBER_OF_PACKAGES];
CHAR16 *AssetTags[NUMBER_OF_PACKAGES];

CHAR16 EmptyString[] = L" ";
CHAR16 SocketString[] = L"LGA775";

VOID
ProducePlatformCpuData (
  VOID
  )
{
  UINTN                                      Index;

  for (Index = 0; Index < NUMBER_OF_PACKAGES; Index++) {

    //
    // The String Package of a module is registered together with all IFR packages.
    // So we just arbitrarily pick a package GUID that is always installed to get the string.
    //
    AssetTags[Index] = EmptyString;
    SocketNames[Index] = SocketString;
  }
}
