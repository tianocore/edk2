
/*++

Copyright (c)  2013  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  PchPeiInit.h

Abstract:


--*/

#ifndef _PCH_PEI_INIT_H_
#define _PCH_PEI_INIT_H_

//
// Define the PCH PEI Init PPI GUID
//
#define PCH_PEI_INIT_PPI_GUID \
  { \
    0xACB93B08, 0x5CDC, 0x4A8F, 0x93, 0xD4, 0x6, 0xE3, 0x42, 0xDF, 0x18, 0x2E \
  }

//
// Extern the GUID for PPI users.
//
extern EFI_GUID     gPchPeiInitPpiGuid;

#endif
