/** @file

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Crc32.h

Abstract:

  Header file for CalcuateCrc32 routine
  
**/

#ifndef _CRC32_H
#define _CRC32_H

#include <Common/UefiBaseTypes.h>

EFI_STATUS
CalculateCrc32 (
  IN  UINT8                             *Data,
  IN  UINTN                             DataSize,
  IN OUT UINT32                         *CrcOut
  )
/*++

Routine Description:

  The CalculateCrc32 routine.

Arguments:

  Data        - The buffer contaning the data to be processed
  DataSize    - The size of data to be processed
  CrcOut      - A pointer to the caller allocated UINT32 that on
                contains the CRC32 checksum of Data

Returns:

  EFI_SUCCESS               - Calculation is successful.
  EFI_INVALID_PARAMETER     - Data / CrcOut = NULL, or DataSize = 0

--*/
;

#endif
