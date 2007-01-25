/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Crc32.c

Abstract:

  CalculateCrc32 Boot Services as defined in DXE CIS.

  This Boot Services is in the Runtime Driver because this service is
  also required by SetVirtualAddressMap() when the EFI System Table and
  EFI Runtime Services Table are converted from physical address to 
  virtual addresses.  This requires that the 32-bit CRC be recomputed.

Revision History:

--*/

#include "Runtime.h"

UINT32  mCrcTable[256];

EFI_STATUS
EFIAPI
RuntimeDriverCalculateCrc32 (
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  OUT UINT32  *CrcOut
  )
/*++

Routine Description:

  Calculate CRC32 for target data

Arguments:

  Data     - The target data.
  DataSize - The target data size.
  CrcOut   - The CRC32 for target data.

Returns:

  EFI_SUCCESS           - The CRC32 for target data is calculated successfully.
  EFI_INVALID_PARAMETER - Some parameter is not valid, so the CRC32 is not 
                          calculated.

--*/
{
  UINT32  Crc;
  UINTN   Index;
  UINT8   *Ptr;

  if (Data == NULL || DataSize == 0 || CrcOut == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Crc = 0xffffffff;
  for (Index = 0, Ptr = Data; Index < DataSize; Index++, Ptr++) {
    Crc = (Crc >> 8) ^ mCrcTable[(UINT8) Crc ^ *Ptr];
  }

  *CrcOut = Crc ^ 0xffffffff;
  return EFI_SUCCESS;
}

STATIC
UINT32
ReverseBits (
  UINT32  Value
  )
/*++

Routine Description:

  Reverse bits for 32bit data.

Arguments:

  Value - the data to be reversed.

Returns:

  UINT32 data reversed.

--*/
{
  UINTN   Index;
  UINT32  NewValue;

  NewValue = 0;
  for (Index = 0; Index < 32; Index++) {
    if (Value & (1 << Index)) {
      NewValue = NewValue | (1 << (31 - Index));
    }
  }

  return NewValue;
}

VOID
RuntimeDriverInitializeCrc32Table (
  VOID
  )
/*++

Routine Description:

  Initialize CRC32 table.

Arguments:

  None.

Returns:

  None.

--*/
{
  UINTN   TableEntry;
  UINTN   Index;
  UINT32  Value;

  for (TableEntry = 0; TableEntry < 256; TableEntry++) {
    Value = ReverseBits ((UINT32) TableEntry);
    for (Index = 0; Index < 8; Index++) {
      if (Value & 0x80000000) {
        Value = (Value << 1) ^ 0x04c11db7;
      } else {
        Value = Value << 1;
      }
    }

    mCrcTable[TableEntry] = ReverseBits (Value);
  }
}
