/** @file
    Functions for parsing SPD buffers for DDR4 and DDR5 DIMMs.

    Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.<BR>
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/JedecJep106Lib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/SmbiosType17SpdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Smbios.h>
#include <IndustryStandard/SdramSpd.h>
//#include <IndustryStandard/SdramSpdDdr4.h>
//#include <IndustryStandard/SpdDdr5.h>

#include "SmbiosType17SpdLibInternal.h"

/** Parses the DIMM memory type from the SPD buffer

  @param SpdData SPD data buffer.
  @param Type17  SMBIOS Type17 table.

**/
VOID
SetDimmMemoryType (
  IN UINT8                 *SpdData,
  OUT SMBIOS_TABLE_TYPE17  *Type17
  )
{
  switch (SpdData[DDR_SPD_PROTOCOL_TYPE_IDX]) {
    case SPD_VAL_SDR_TYPE:
      Type17->MemoryType             = MemoryTypeSdram;
      Type17->TypeDetail.Synchronous = 1;
      break;
    case SPD_VAL_DDR_TYPE:
      Type17->MemoryType = MemoryTypeSdram;
      break;
    case SPD_VAL_DDR2_TYPE:
      Type17->MemoryType             = MemoryTypeDdr2;
      Type17->TypeDetail.Synchronous = 1;
      break;
    case SPD_VAL_DDR3_TYPE:
      Type17->MemoryType             = MemoryTypeDdr3;
      Type17->TypeDetail.Synchronous = 1;
      break;
    case SPD_VAL_DDR4_TYPE:
      Type17->MemoryType             = MemoryTypeDdr4;
      Type17->TypeDetail.Synchronous = 1;
      break;
    case SPD_VAL_LPDDR3_TYPE:
      Type17->MemoryType             = MemoryTypeLpddr3;
      Type17->TypeDetail.Synchronous = 1;
      break;
    case SPD_VAL_LPDDR4_TYPE:
      Type17->MemoryType             = MemoryTypeLpddr4;
      Type17->TypeDetail.Synchronous = 1;
      break;
    case SPD_VAL_DDR5_TYPE:
      Type17->MemoryType             = MemoryTypeDdr5;
      Type17->TypeDetail.Synchronous = 1;
      break;
    case SPD_VAL_LPDDR5_TYPE:
      Type17->MemoryType             = MemoryTypeLpddr5;
      Type17->TypeDetail.Synchronous = 1;
      break;
    case SPD_VAL_DDR5_NVDIMMP_TYPE:
      Type17->MemoryType             = MemoryTypeDdr5;
      Type17->TypeDetail.Nonvolatile = 1;
      break;
    case SPD_VAL_LPDDR5X_TYPE:
      Type17->MemoryType             = MemoryTypeLpddr5;
      Type17->TypeDetail.Synchronous = 1;
      break;
    default:
      Type17->MemoryType = MemoryTypeOther;
      break;
  }
}

/** Parses the memory technology from the SPD buffer.

  @param SpdData SPD data buffer.
  @param Type17  SMBIOS Type17 table.

**/
VOID
SetDimmMemoryTechnology (
  IN UINT8                    *SpdData,
  IN OUT SMBIOS_TABLE_TYPE17  *Type17
  )
{
  switch (SpdData[DDR_SPD_MODULE_TYPE_IDX] & 0xF0) {
    case 0:
      // Module only contains DRAM
      Type17->MemoryTechnology                                  = MemoryTechnologyDram;
      Type17->MemoryOperatingModeCapability.Bits.VolatileMemory = 1;
      break;
    case 0x90:
      Type17->TypeDetail.Nonvolatile                                            = 1;
      Type17->MemoryOperatingModeCapability.Bits.ByteAccessiblePersistentMemory = 1;
      Type17->MemoryTechnology                                                  = MemoryTechnologyNvdimmN; // (or MemoryTechnologyNvdimmF)
      break;
    case 0xA0:
      Type17->TypeDetail.Nonvolatile                                            = 1;
      Type17->MemoryOperatingModeCapability.Bits.ByteAccessiblePersistentMemory = 1;
      Type17->MemoryTechnology                                                  = MemoryTechnologyNvdimmP;
      break;
    case 0xB0:
      Type17->TypeDetail.Nonvolatile                                            = 1;
      Type17->MemoryOperatingModeCapability.Bits.ByteAccessiblePersistentMemory = 1;
      Type17->MemoryTechnology                                                  = MemoryTechnologyOther; // MemoryTechnologyNvdimmH
      break;
    default:
      DEBUG ((DEBUG_ERROR, "Invalid Key Byte / Module Type: %02X\n", SpdData[DDR_SPD_MODULE_TYPE_IDX]));
  }
}

/** Parses the DIMM form factor from the SPD buffer.

  @param SpdData SPD data buffer.
  @param Type17  SMBIOS Type17 table.

**/
VOID
SetDimmMemoryFormFactor (
  IN UINT8                    *SpdData,
  IN OUT SMBIOS_TABLE_TYPE17  *Type17
  )
{
  UINT8  ModuleType;

  ModuleType = SpdData[DDR_SPD_MODULE_TYPE_IDX] & 0x0F;

  // Registered DIMMs
  if ((ModuleType == SPD_VAL_RDIMM_MODULE) ||
      (ModuleType == SPD_VAL_MINI_RDIMM_MODULE) ||
      (ModuleType == SPD_VAL_72B_SORDIMM_MODULE))
  {
    Type17->TypeDetail.Registered = 1;
  }

  // LRDIMMs
  if (ModuleType == SPD_VAL_LRDIMM_MODULE) {
    Type17->TypeDetail.LrDimm = 1;
  }

  // Unbuffered DIMMs
  if ((ModuleType == SPD_VAL_UDIMM_MODULE) ||
      (ModuleType == SPD_VAL_MINI_UDIMM_MODULE) ||
      (ModuleType == SPD_VAL_SODIMM_MODULE) ||
      (ModuleType == SPD_VAL_16B_SODIMM_MODULE) ||
      (ModuleType == SPD_VAL_32B_SODIMM_MODULE) ||
      (ModuleType == SPD_VAL_72B_SOUDIMM_MODULE))
  {
    Type17->TypeDetail.Unbuffered = 1;
  }

  // SODIMMs
  if ((ModuleType == SPD_VAL_SODIMM_MODULE) ||
      (ModuleType == SPD_VAL_16B_SODIMM_MODULE) ||
      (ModuleType == SPD_VAL_32B_SODIMM_MODULE) ||
      (ModuleType == SPD_VAL_72B_SOUDIMM_MODULE) ||
      (ModuleType == SPD_VAL_72B_SORDIMM_MODULE))
  {
    Type17->FormFactor = MemoryFormFactorSodimm;
  }

  // DIMMs
  if ((ModuleType == SPD_VAL_UDIMM_MODULE) ||
      (ModuleType == SPD_VAL_MINI_UDIMM_MODULE) ||
      (ModuleType == SPD_VAL_RDIMM_MODULE) ||
      (ModuleType == SPD_VAL_MINI_RDIMM_MODULE) ||
      (ModuleType == SPD_VAL_LRDIMM_MODULE))
  {
    Type17->FormFactor = MemoryFormFactorDimm;
  }
}

/** Parses the manufacturer from the SPD buffer.

  @param SpdData            SPD data buffer.
  @param MfgIdCode1Idx      The index of the first byte of the manufacturer ID code.
  @param Type17             SMBIOS Type17 table.
  @param FixedStringsLength The length of the fixed strings in the Type 17 table.
  @param Ddr5               Whether the SPD data buffer is for a DDR5 DIMM.

**/
VOID
UpdateManufacturer (
  IN UINT8     *SpdData,
  IN UINTN     MfgIdCode1Idx,
  IN OUT VOID  *Type17,
  IN UINTN     FixedStringsLength,
  IN BOOLEAN   Ddr5
  )
{
  UINTN        Offset;
  UINTN        PartNumberLength;
  UINT8        ContinuationBytes;
  CHAR8        *MfgOffset;
  CONST CHAR8  *ManufacturerName;

  ContinuationBytes = SpdData[MfgIdCode1Idx] & 0x7F;

  if (Ddr5) {
    PartNumberLength = DDR_SPD_DDR5_PART_NUMBER_LENGTH + 1;
  } else {
    PartNumberLength = DDR_SPD_DDR4_PART_NUMBER_LENGTH + 1;
  }

  Offset = sizeof (SMBIOS_TABLE_TYPE17) +
           FixedStringsLength +
           (SMBIOS_SERIAL_NUMBER_LENGTH + 1) +
           PartNumberLength;

  MfgOffset = (CHAR8 *)Type17 + Offset;

  ManufacturerName = Jep106GetManufacturerName (
                       SpdData[MfgIdCode1Idx + 1],
                       ContinuationBytes
                       );
  if (ManufacturerName != NULL) {
    AsciiStrCpyS (MfgOffset, 256, ManufacturerName);
  }
}

/** Parses the serial number from the SPD buffer.

  @param SpdData            SPD data buffer.
  @param SpdSerialNumberIdx The index of the first byte of the serial number.
  @param Type17             SMBIOS Type17 table.
  @param FixedStringsLength The length of the fixed strings in the Type 17 table.

**/
VOID
UpdateSerialNumber (
  IN UINT8     *SpdData,
  IN UINT16    SpdSerialNumberIdx,
  IN OUT VOID  *Type17,
  IN UINTN     FixedStringsLength
  )
{
  UINTN  FieldIndex;
  UINTN  CharIndex;
  UINTN  Offset;
  CHAR8  *SerialNumber;

  Offset = sizeof (SMBIOS_TABLE_TYPE17) +
           FixedStringsLength;

  SerialNumber = (CHAR8 *)Type17 + Offset;

  /*
    Calculate a serial number as suggested in JESD400-5:

    One method of achieving this is by assigning a byte in the field from
    517~520 as a tester ID byte and using the remaining bytes as a sequential
    serial number. Bytes 512~520 will then result in a nine-byte unique module
    identifier. Note that part number is not included in this identifier: the
    supplier may not give the same value for Bytes 517~520 to more than one
    DIMM even if the DIMMs have different part numbers.
  */

  CharIndex = 0;
  for (FieldIndex = 0; FieldIndex < DDR_SPD_SERIAL_NUMBER_LENGTH; FieldIndex++) {
    UINT8  Temp;
    UINT8  Value;

    Value = SpdData[SpdSerialNumberIdx + FieldIndex];

    Temp = Value >> 4;
    if (Temp < 10) {
      SerialNumber[CharIndex] = '0' + Temp;
    } else {
      SerialNumber[CharIndex] = 'A' + (Temp - 10);
    }

    CharIndex++;
    Temp = Value & 0xF;
    if (Temp < 10) {
      SerialNumber[CharIndex] = '0' + Temp;
    } else {
      SerialNumber[CharIndex] = 'A' + (Temp - 10);
    }

    CharIndex++;
  }

  SerialNumber[CharIndex] = '\0';
}

/** Parses the part number from the SPD buffer.

  @param SpdData             SPD data buffer.
  @param PartNumberFieldIdx  Index of the part number field in the SPD data buffer.
  @param Type17              SMBIOS Type17 table.
  @param FixedStringsLength  Length of the fixed strings in the SMBIOS structure
  @param Ddr5                Whether the SPD data buffer is for a DDR5 DIMM.

**/
VOID
UpdatePartNumber (
  IN UINT8     *SpdData,
  IN UINTN     PartNumberFieldIdx,
  IN OUT VOID  *Type17,
  IN UINTN     FixedStringsLength,
  IN BOOLEAN   Ddr5
  )
{
  UINTN  Offset;
  UINTN  PartNumberLength;
  CHAR8  *PartNumber;

  if (Ddr5) {
    PartNumberLength = DDR_SPD_DDR5_PART_NUMBER_LENGTH + 1;
  } else {
    PartNumberLength = DDR_SPD_DDR4_PART_NUMBER_LENGTH + 1;
  }

  Offset = sizeof (SMBIOS_TABLE_TYPE17) +
           FixedStringsLength +
           (SMBIOS_SERIAL_NUMBER_LENGTH + 1);

  PartNumber = (CHAR8 *)Type17 + Offset;

  // The part number is stored as ASCII, and so can just be copied.
  CopyMem (PartNumber, SpdData + PartNumberFieldIdx, PartNumberLength - 1);

  PartNumber[PartNumberLength] = '\0';
}

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
  )
{
  UINT16  Crc;
  UINT32  Index;

  Crc = 0;
  while (--Count >= 0) {
    Crc = Crc ^ (UINT16)*Data++ << 8;
    for (Index = 0; Index < 8; ++Index) {
      if (Crc & 0x8000) {
        Crc = Crc << 1 ^ 0x1021;
      } else {
        Crc = Crc << 1;
      }
    }
  }

  return Crc;
}

/**
  Given an SPD data buffer from a DDR4 or DDR5 DIMM, returns a
  pointer to a new SMBIOS_TABLE_TYPE17 structure containing data
  parsed from the buffer.

  @param SpdData             SPD data buffer.
  @param SpdDataSize         Size of the SPD data buffer.
  @param Type17              SMBIOS Type17 table.
  @param FixedStringsLength  Length of the fixed strings in the SMBIOS structure.

  @return EFI_SUCCESS on success, or an error code.

**/
EFI_STATUS
EFIAPI
GetSmbiosType17FromSpdData (
  IN     UINT8                *SpdData,
  IN     UINTN                SpdDataSize,
  OUT    SMBIOS_TABLE_TYPE17  **Type17,
  IN UINTN                    FixedStringsLength
  )
{
  EFI_STATUS  Status;

  if (SpdDataSize < (DDR_SPD_PROTOCOL_TYPE_IDX + 1)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  if ((SpdData[DDR_SPD_PROTOCOL_TYPE_IDX] >= SPD_VAL_DDR5_TYPE) &&
      (SpdData[DDR_SPD_PROTOCOL_TYPE_IDX] <= SPD_VAL_LPDDR5X_TYPE))
  {
    Status = ParseDdr5 (SpdData, SpdDataSize, Type17, FixedStringsLength);
  } else {
    Status = ParseDdr4 (SpdData, SpdDataSize, Type17, FixedStringsLength);
  }

  return Status;
}
