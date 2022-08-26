/** @file

  Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_TYPE17_H_
#define SMBIOS_TYPE17_H_

#include <IndustryStandard/SmBios.h>

EFI_STATUS
EFIAPI
GetSmbiosType17FromSpdData (
  IN     UINT8                *SpdData,
  IN     UINTN                SpdDataSize,
  OUT    SMBIOS_TABLE_TYPE17  **Type17,
  IN     UINTN                FixedStringsLength
  );

/**
   CRC16 algorithm from JEDEC 4.1.2.L-6 R30 v14

   @param Data  Data bytes.
   @param Count Number of bytes to calculate the CRC16 over.

   @return Calculated CRC16 value.
**/
UINT16
Crc16 (
  UINT8  *Data,
  INT32  Count
  );

#endif /* SMBIOS_TYPE17_H_ */
