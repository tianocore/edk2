/**@file
  Copyright (c) 2007 - 2008, Intel Corporation

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "HiiDatabase.h"


/**
  Compare whether two names of languages are identical.

  @param  Language1              Name of language 1
  @param  Language2              Name of language 2

  @retval TRUE                   same
  @retval FALSE                  not same

**/
BOOLEAN
R8_EfiLibCompareLanguage (
  IN  CHAR8  *Language1,
  IN  CHAR8  *Language2
  )
{
  //
  // Porting Guide:
  // This library interface is simply obsolete.
  // Include the source code to user code.
  //
  UINTN Index;

  for (Index = 0; (Language1[Index] != 0) && (Language2[Index] != 0); Index++) {
    if (Language1[Index] != Language2[Index]) {
      return FALSE;
    }
  }

  if (((Language1[Index] == 0) && (Language2[Index] == 0))   || 
  	  ((Language1[Index] == 0) && (Language2[Index] != ';')) ||
  	  ((Language1[Index] == ';') && (Language2[Index] != 0)) ||
  	  ((Language1[Index] == ';') && (Language2[Index] != ';'))) {
    return TRUE;
  }

  return FALSE;
}



