/** @file
  Lib fucntions for SMBIOS. Used to get system serial number and GUID

  Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "../UefiShellDebug1CommandsLib.h"
#include <Guid/Smbios.h>
#include "LibSmbios.h"

/**
  Return SMBIOS string given the string number.

  @param[in] Smbios         Pointer to SMBIOS structure.
  @param[in] StringNumber   String number to return. -1 is used to skip all strings and
                            point to the next SMBIOS structure.

  @return Pointer to string, or pointer to next SMBIOS strcuture if StringNumber == -1
**/
CHAR8*
LibGetSmbiosString (
  IN  SMBIOS_STRUCTURE_POINTER    *Smbios,
  IN  UINT16                      StringNumber
  )
{
  UINT16  Index;
  CHAR8   *String;

  ASSERT (Smbios != NULL);

  //
  // Skip over formatted section
  //
  String = (CHAR8 *) (Smbios->Raw + Smbios->Hdr->Length);

  //
  // Look through unformated section
  //
  for (Index = 1; Index <= StringNumber; Index++) {
    if (StringNumber == Index) {
      return String;
    }
    //
    // Skip string
    //
    for (; *String != 0; String++);
    String++;

    if (*String == 0) {
      //
      // If double NULL then we are done.
      //  Retrun pointer to next structure in Smbios.
      //  if you pass in a -1 you will always get here
      //
      Smbios->Raw = (UINT8 *)++String;
      return NULL;
    }
  }

  return NULL;
}
