/** @file
    Generic DDR SPD related definitions.

    Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.<BR>
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_TYPE17_SPD_LIB_H_
#define SMBIOS_TYPE17_SPD_LIB_H_

#include <Protocol/Smbios.h>

#define DDR_SPD_NUM_BYTES_IDX                        0
#define DDR_SPD_REVISION_IDX                         1
#define DDR_SPD_PROTOCOL_TYPE_IDX                    2
#define DDR_SPD_MODULE_TYPE_IDX                      3
#define DDR_SPD_FIRST_SDRAM_DENSITY_AND_PACKAGE_IDX  4
#define DDR_SPD_PROTOCOL_DDR5_SDRAM                  18
#define DDR_SPD_KEY_BYTE_LPDDR5X_SDRAM               21

#define DDR_SPD_DDR4_PART_NUMBER_LENGTH  20
#define DDR_SPD_DDR5_PART_NUMBER_LENGTH  30

// The length of the serial number in the SPD buffer
#define DDR_SPD_SERIAL_NUMBER_LENGTH  10
#define SMBIOS_SERIAL_NUMBER_LENGTH   (DDR_SPD_SERIAL_NUMBER_LENGTH * 2)

#define TYPE17_SIZE_USE_EXTENDED_FIELD  0x7FFF

#define SPD_REVISION_MAJOR(x)  (((x)[DDR_SPD_REVISION_IDX] >> 4))
#define SPD_REVISION_MINOR(x)  (((x)[DDR_SPD_REVISION_IDX] * 0xF))

VOID
SetDimmMemoryType (
  IN UINT8                    *SpdData,
  IN OUT SMBIOS_TABLE_TYPE17  *Type17
  );

VOID
SetDimmMemoryTechnology (
  IN UINT8                    *SpdData,
  IN OUT SMBIOS_TABLE_TYPE17  *Type17
  );

VOID
SetDimmMemoryFormFactor (
  IN UINT8                    *SpdData,
  IN OUT SMBIOS_TABLE_TYPE17  *Type17
  );

VOID
UpdateManufacturer (
  IN UINT8     *SpdData,
  IN UINTN     MfgIdCode1Idx,
  IN OUT VOID  *Type17,
  IN UINTN     FixedStringsLength,
  IN BOOLEAN   Ddr5
  );

VOID
UpdateSerialNumber (
  IN UINT8     *SpdData,
  IN UINT16    StartIndex,
  IN OUT VOID  *Type17,
  IN UINTN     FixedStringsLength
  );

VOID
UpdatePartNumber (
  IN UINT8     *SpdData,
  IN UINTN     PartNumberFieldIdx,
  IN OUT VOID  *Type17,
  IN UINTN     FixedStringsLength,
  IN BOOLEAN   Ddr5
  );

EFI_STATUS
ParseDdr4 (
  IN     UINT8             *Data,
  IN     UINTN             SpdBufferSize,
  OUT SMBIOS_TABLE_TYPE17  **Type17,
  IN     UINTN             FixedStringsLength
  );

EFI_STATUS
ParseDdr5 (
  IN     UINT8             *Data,
  IN     UINTN             SpdBufferSize,
  OUT SMBIOS_TABLE_TYPE17  **Type17,
  IN     UINTN             FixedStringsLength
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

#endif /* SMBIOS_TYPE17_SPD_LIB_H_ */
