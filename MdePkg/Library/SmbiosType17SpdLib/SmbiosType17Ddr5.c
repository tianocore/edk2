/** @file
    Functions for parsing SPD buffers for DDR5 DIMMs.

    Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.<BR>
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/JedecJep106Lib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Smbios.h>
#include <IndustryStandard/SdramSpd.h>
#include <IndustryStandard/SpdDdr5.h>

#include "SmbiosType17SpdLibInternal.h"

/**
 Encoding of the value in the SPD Bytes Total field (byte 0, bits 6:4)
**/
STATIC UINTN  SpdBytes[] = {
  0,
  256,
  512,
  1024,
  2048
};

/**
 Encoding of the value in the Die Per Package field (byte 4 and 8, bits 7:5)
**/
STATIC UINTN  DiePerPackage[] = {
  1,
  2,
  2,
  4,
  8,
  16
};

/**
  Encoding of the value in the SDRAM Density Per Die field (byte 4 and 8, bits 4:0)
**/
STATIC UINT32  SdramCapacitiesPerDie[] = {
  0,
  4096,
  8192,
  12288,
  16384,
  24576,
  32768,
  49152,
  65536
};

/** Parses the DIMM module type from the SPD buffer.

  @param SpdData SPD data buffer.
  @param Type17  SMBIOS Type17 table.

**/
STATIC
EFI_STATUS
UpdateModuleType (
  IN UINT8                    *SpdData,
  IN OUT SMBIOS_TABLE_TYPE17  *Type17
  )
{
  Type17->TypeDetail.Unknown                         = 0;
  Type17->MemoryOperatingModeCapability.Bits.Unknown = 0;

  SetDimmMemoryType (SpdData, Type17);
  SetDimmMemoryTechnology (SpdData, Type17);
  SetDimmMemoryFormFactor (SpdData, Type17);

  return EFI_SUCCESS;
}

/** Parses the SDRAM density from the SPD buffer.

  @param Byte    SPD SDRAM density byte.

  @return SDRAM density per die, in Mb. Returns 0 on error.

**/
UINT32
GetSdramDensityPerDie (
  UINT8  Byte
  )
{
  UINT8  Value;

  Value = Byte & 0x1F;

  if ((Value == 0) || (Value >= ARRAY_SIZE (SdramCapacitiesPerDie))) {
    DEBUG ((
      DEBUG_ERROR,
      "Total SDRAM capacity per die invalid/unknown: %01X\n",
      Value
      ));
    return 0;
  }

  return SdramCapacitiesPerDie[Value];
}

/** Parses the DIMM module type from the SPD buffer.

  @param SpdData SPD data buffer.
  @param Type17  SMBIOS Type17 table.

**/
STATIC
EFI_STATUS
UpdateCapacity (
  IN SPD_DDR5                 *Spd,
  IN OUT SMBIOS_TABLE_TYPE17  *Type17
  )
{
  UINT64   TotalCapacity;
  UINT64   CapacityOfEvenRanks;
  UINT64   CapacityOfOddRanks;
  UINT32   FirstSdramDensityPerDieMb;
  UINT32   SecondSdramDensityPerDieMb;
  UINT8    FirstDieCount;
  UINT8    SecondDieCount;
  UINT8    FirstSdramIOWidth;
  UINT8    SecondSdramIOWidth;
  UINT8    NumChannelsPerDimm;
  UINT8    PrimaryBusWidthPerChannel;
  UINT8    NumPackageRanksPerChannel;
  BOOLEAN  SymmetricalAssembly;

  FirstSdramDensityPerDieMb =
    GetSdramDensityPerDie (Spd->Base.FirstSdramDensityAndPackage.Data);
  FirstDieCount             = (UINT8)DiePerPackage[Spd->Base.FirstSdramDensityAndPackage.Bits.Die];
  FirstSdramIOWidth         = 4 << Spd->Base.FirstSdramIoWidth.Bits.IoWidth;
  NumChannelsPerDimm        = Spd->Common.MemoryChannelBusWidth.Bits.SubChannelsPerDimmCount + 1;
  PrimaryBusWidthPerChannel = 8 << (Spd->Common.MemoryChannelBusWidth.Bits.PrimaryBusWidthPerSubChannel);
  NumPackageRanksPerChannel = Spd->Common.ModuleOrganization.Bits.PackageRanksCount + 1;

  if (Spd->Common.ModuleOrganization.Bits.RankMix == 0) {
    SymmetricalAssembly = TRUE;
  } else {
    SymmetricalAssembly = FALSE;
  }

  Type17->DataWidth  = (8 << Spd->Common.MemoryChannelBusWidth.Bits.PrimaryBusWidthPerSubChannel) * (Spd->Common.MemoryChannelBusWidth.Bits.SubChannelsPerDimmCount + 1);
  Type17->TotalWidth = Type17->DataWidth + ((Spd->Common.MemoryChannelBusWidth.Bits.BusWidthExtensionPerSubChannel << 2) * (Spd->Common.MemoryChannelBusWidth.Bits.SubChannelsPerDimmCount + 1));

  /*
    According to JESD400-5, to calculate the total capacity in bytes for a
    symmetric module, the following math applies:

    Capacity in bytes =
      Number of channels per DIMM *
      Primary bus width per channel / SDRAM I/O Width *
      Die per package *
      SDRAM density per die / 8 *
      Package ranks per channel

    To calculate the total capacity in bytes for an asymmetric module, the
    following math applies:

      Capacity in bytes =
      Capacity of even ranks (first SDRAM type) +
      Capacity of odd ranks (second SDRAM type)

      Commonly, parity or ECC are not counted in total module capacity, though
      they can also be included by adding the bus width extension in SPD byte
      235 bits 4~3 to the primary bus width in the previous examples.
  */

  if (!SymmetricalAssembly) {
    SecondDieCount             = (UINT8)DiePerPackage[Spd->Base.SecondSdramDensityAndPackage.Bits.Die];
    SecondSdramDensityPerDieMb =
      GetSdramDensityPerDie (Spd->Base.SecondSdramDensityAndPackage.Data);
    SecondSdramIOWidth = 4 << Spd->Base.SecondSdramIoWidth.Bits.IoWidth;

    CapacityOfEvenRanks = NumChannelsPerDimm *
                          (PrimaryBusWidthPerChannel / FirstSdramIOWidth) *
                          FirstDieCount *
                          (FirstSdramDensityPerDieMb / 8) *
                          NumPackageRanksPerChannel;

    CapacityOfOddRanks = NumChannelsPerDimm *
                         (PrimaryBusWidthPerChannel / SecondSdramIOWidth) *
                         SecondDieCount *
                         (SecondSdramDensityPerDieMb / 8) *
                         NumPackageRanksPerChannel;

    TotalCapacity = CapacityOfEvenRanks + CapacityOfOddRanks;
  } else {
    TotalCapacity = NumChannelsPerDimm *
                    (PrimaryBusWidthPerChannel / FirstSdramIOWidth) *
                    FirstDieCount *
                    (FirstSdramDensityPerDieMb / 8) *
                    NumPackageRanksPerChannel;
  }

  /*
    From the SMBIOS Specification 3.6:

    If the value is 0, no memory device is installed in
    the socket; if the size is unknown, the field value is
    FFFFh. If the size is 32 GB-1 MB or greater, the
    field value is 7FFFh and the actual size is stored in
    the Extended Size field.
    The granularity in which the value is specified
    depends on the setting of the most-significant bit
    (bit 15). If the bit is 0, the value is specified in
    megabyte units; if the bit is 1, the value is specified
    in kilobyte units. For example, the value 8100h
    identifies a 256 KB memory device and 0100h
    identifies a 256 MB memory device.
  */
  if (TotalCapacity < MAX_INT16) {
    Type17->Size = (UINT16)TotalCapacity;
  } else {
    Type17->Size         = TYPE17_SIZE_USE_EXTENDED_FIELD;
    Type17->ExtendedSize = (UINT32)TotalCapacity;
  }

  Type17->VolatileSize = TotalCapacity * SIZE_1MB;
  Type17->LogicalSize  = TotalCapacity * SIZE_1MB;

  return EFI_SUCCESS;
}

/** Main entry point for parsing a DDR5 SPD buffer.

  @param SpdData       SPD data buffer.
  @param SpdBufferSize The size of the SPD data buffer.
  @param Type17        SMBIOS Type17 table. Allocated by this library. Free with FreePool.
  @param FixedStringsLength The length of fixed strings in the Type17 table.

**/
EFI_STATUS
ParseDdr5 (
  IN     UINT8             *SpdData,
  IN     UINTN             SpdBufferSize,
  OUT SMBIOS_TABLE_TYPE17  **Type17,
  IN     UINTN             FixedStringsLength
  )
{
  EFI_STATUS           Status;
  SPD_DDR5             *Spd;
  UINTN                SpdBytesTotal;
  UINTN                BufferSize;
  SMBIOS_TABLE_TYPE17  *Table;
  UINT16               Crc;

  Spd = (SPD_DDR5 *)SpdData;

  if (SpdBytes[Spd->Base.Description.Bits.BytesTotal] >= ARRAY_SIZE (SpdBytes)) {
    DEBUG ((
      DEBUG_ERROR,
      "SPD bytes total unknown/invalid: %02x (%d vs %d)\n",
      Spd->Base.Description.Bits.BytesTotal,
      SpdBytes[Spd->Base.Description.Bits.BytesTotal],
      SpdBufferSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  SpdBytesTotal = SpdBytes[Spd->Base.Description.Bits.BytesTotal];

  if (SpdBufferSize != SpdBytesTotal) {
    DEBUG ((
      DEBUG_ERROR,
      "SPD bytes total (%d) mismatch buffer size (%d)\n",
      SpdBytesTotal,
      SpdBufferSize
      ));

    return EFI_INVALID_PARAMETER;
  }

  // Check that the CRC is valid
  Crc = Crc16 (SpdData, DDR_SPD_CRC_NUM_BYTES);

  if (((Crc & 0xFF) != SpdData[DDR_SPD_CRC_BYTE_1_IDX]) ||
      (Crc >> 8 != SpdData[DDR_SPD_CRC_BYTE_2_IDX]))
  {
    DEBUG ((DEBUG_ERROR, "!!! ERROR !!! SPD CRC Mismatch\n"));
    return EFI_INVALID_PARAMETER;
  }

  BufferSize = sizeof (SMBIOS_TABLE_TYPE17) +
               FixedStringsLength +
               (Jep106GetLongestManufacturerName () + 1) +
               (SMBIOS_SERIAL_NUMBER_LENGTH + 1)  +
               (DDR_SPD_DDR5_PART_NUMBER_LENGTH + 1);

  *Type17 = AllocateZeroPool (BufferSize);
  if (*Type17 == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Table = *Type17;

  Table->Hdr.Type   = EFI_SMBIOS_TYPE_MEMORY_DEVICE;
  Table->Hdr.Handle = SMBIOS_HANDLE_PI_RESERVED;
  Table->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE17);

  Status = UpdateModuleType (SpdData, Table);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UpdateCapacity (Spd, Table);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // DDR5 operates at 1.1V (1100 mV)
  Table->MinimumVoltage    = 1100;
  Table->MaximumVoltage    = 1100;
  Table->ConfiguredVoltage = 1100;

  Table->ModuleManufacturerID = Spd->ManufactureInfo.ModuleManufacturer.Data;
  Table->ModuleProductID      = 0x0000;

  UpdatePartNumber (SpdData, DDR_SPD_MODULE_PART_NUM_IDX, *Type17, FixedStringsLength, TRUE);

  Table->MemorySubsystemControllerManufacturerID = Spd->ManufactureInfo.DramManufacturer.Data;
  Table->MemorySubsystemControllerProductID      = 0x0000;

  UpdateManufacturer (
    SpdData,
    DDR_SPD_MODULE_MFG_ID_CODE_1_IDX,
    *Type17,
    FixedStringsLength,
    TRUE
    );
  UpdateSerialNumber (
    SpdData,
    DDR_SPD_MODULE_MFG_ID_CODE_1_IDX,
    *Type17,
    FixedStringsLength
    );

  return EFI_SUCCESS;
}
