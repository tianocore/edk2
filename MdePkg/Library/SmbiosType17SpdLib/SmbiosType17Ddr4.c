/** @file
    Functions for parsing SPD buffers for DDR4 DIMMs.

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
#include <IndustryStandard/SdramSpdDdr4.h>

#include "SmbiosType17SpdLibInternal.h"

#define PRIMARY_DIE_COUNT(x)               (1 + (((x)[DDR_SPD_PRIMARY_SDRAM_PACKAGE_TYPE_IDX] >> 4) & 0x7))
#define SECONDARY_DIE_COUNT(x)             (1 + (((x)[DDR_SPD_SECONDARY_SDRAM_PACKAGE_TYPE_IDX] >> 4) & 0x7))
#define PACKAGE_RANKS_PER_DIMM(x)          (1 + (((x)[DDR_SPD_MODULE_ORGANIZATION_IDX] >> 3) & 0x7))
#define DRAM_SENSITY_RATIO(x)              (((x)[DDR_SPD_SECONDARY_SDRAM_PACKAGE_TYPE_IDX] >> 2) & 0x3)
#define PRIMARY_SIGNAL_LOADING_VALUE(x)    ((x)[DDR_SPD_PRIMARY_SDRAM_PACKAGE_TYPE_IDX] & 0x3)
#define SECONDARY_SIGNAL_LOADING_VALUE(x)  ((x)[DDR_SPD_SECONDARY_SDRAM_PACKAGE_TYPE_IDX] & 0x3)
#define SDRAM_DEVICE_WIDTH(x)              (4 << ((x)[DDR_SPD_MODULE_ORGANIZATION_IDX] & 0x7))
#define PRIMARY_BUS_WIDTH(x)               (8 << (((x))[DDR_SPD_MODULE_MEMORY_BUS_WIDTH_IDX] & 0x7))
#define BUS_WIDTH_EXTENSION(x)             ((x)[DDR_SPD_MODULE_MEMORY_BUS_WIDTH_IDX] >> 3)

#define SPD_BYTES_TOTAL(x)  (((x)[DDR_SPD_NUM_BYTES_IDX] & 0xF0) >> 4)
#define SPD_BYTES_USED(x)   ((x)[DDR_SPD_NUM_BYTES_IDX] & 0xF)

/**
 Encoding of the value in the SPD Bytes Total field (byte 0, bits 6:4)
**/
STATIC UINTN  SpdBytes[] = {
  0,
  256,
  512
};

/**
  Encoding of the value in the Total SDRAM capacity per die, in megabits field (byte 4, bits 3:0)
**/
STATIC UINT32  SdramCapacitiesPerDie[] = {
  256,
  512,
  1024,
  2048,
  4096,
  8192,
  16384,
  32768,
  12288,
  24576
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

  @param Byte  SPD SDRAM density byte.

  @return SDRAM density per die, in Mb. Returns 0 on error.

**/
STATIC
UINT32
GetSdramDensityPerDie (
  UINT8  Byte
  )
{
  UINT8  Nibble;

  Nibble = Byte & 0xF;

  if (Nibble > ARRAY_SIZE (SdramCapacitiesPerDie)) {
    DEBUG ((
      DEBUG_ERROR,
      "Total SDRAM capacity per die invalid/unknown: %01X\n",
      Nibble
      ));
    return 0;
  }

  return SdramCapacitiesPerDie[Nibble];
}

/** Parses the DIMM capacity from the SPD buffer.

  @param SpdData SPD data buffer.
  @param Type17  SMBIOS Type17 table.

**/
STATIC
EFI_STATUS
UpdateCapacity (
  IN UINT8                    *SpdData,
  IN OUT SMBIOS_TABLE_TYPE17  *Type17
  )
{
  UINT64                  TotalCapacity;
  UINT64                  EvenRanksCapacity;
  UINT64                  OddRanksCapacity;
  UINT32                  PrimarySdramDensityPerDieMb;
  UINT32                  SecondarySdramDensityPerDieMb;
  UINT8                   PrimaryDieCount;
  UINT8                   SecondaryDieCount;
  BOOLEAN                 SymmetricalAssembly;
  UINT8                   PackageRanksPerDimm;
  UINT8                   SdramDeviceWidth;
  UINT64                  PrimaryLogicalRanksPerDimm;
  UINT64                  SecondaryLogicalRanksPerDimm;
  UINT8                   PrimaryBusWidth;
  UINT8                   BusWidthExtension;
  UINT8                   PrimarySignalLoadingValue;
  UINT8                   SecondarySignalLoadingValue;

  PrimarySdramDensityPerDieMb =
    GetSdramDensityPerDie (SpdData[DDR_SPD_FIRST_SDRAM_DENSITY_AND_PACKAGE_IDX]);

  PrimaryDieCount = PRIMARY_DIE_COUNT (SpdData);

  SymmetricalAssembly = (SpdData[DDR_SPD_SECONDARY_SDRAM_PACKAGE_TYPE_IDX] == 0);

  PackageRanksPerDimm = PACKAGE_RANKS_PER_DIMM (SpdData);
  SdramDeviceWidth    = SDRAM_DEVICE_WIDTH (SpdData);
  PackageRanksPerDimm = PACKAGE_RANKS_PER_DIMM (SpdData);

  if (!SymmetricalAssembly) {
    UINT8  DramDensityRatio;

    SecondaryDieCount = SECONDARY_DIE_COUNT (SpdData);

    DramDensityRatio = DRAM_SENSITY_RATIO (SpdData);

    if (DramDensityRatio == 0) {
      SecondarySdramDensityPerDieMb = PrimarySdramDensityPerDieMb;
    } else if (DramDensityRatio == 1) {
      SecondarySdramDensityPerDieMb =
        GetSdramDensityPerDie (SpdData[DDR_SPD_FIRST_SDRAM_DENSITY_AND_PACKAGE_IDX] - 1);
    } else if (DramDensityRatio == 2) {
      SecondarySdramDensityPerDieMb =
        GetSdramDensityPerDie (SpdData[DDR_SPD_FIRST_SDRAM_DENSITY_AND_PACKAGE_IDX] - 2);
    }

    SecondarySignalLoadingValue = SECONDARY_SIGNAL_LOADING_VALUE (SpdData);

    if (SecondarySignalLoadingValue == VAL_SIGNAL_LOADING_3DS) {
      SecondaryLogicalRanksPerDimm = PackageRanksPerDimm * SecondaryDieCount;
    } else {
      SecondaryLogicalRanksPerDimm = PackageRanksPerDimm;
    }
  }

  PrimarySignalLoadingValue = PRIMARY_SIGNAL_LOADING_VALUE (SpdData);

  BusWidthExtension = 0;

  if (BUS_WIDTH_EXTENSION (SpdData) == 0) {
    BusWidthExtension = 0;
  } else if (BUS_WIDTH_EXTENSION (SpdData) == 1) {
    BusWidthExtension = 8;
  } else {
    DEBUG ((DEBUG_ERROR, "Invalid bus width extension: %d\n", BUS_WIDTH_EXTENSION (SpdData)));
  }

  PrimaryBusWidth = PRIMARY_BUS_WIDTH (SpdData);

  Type17->DataWidth  = PrimaryBusWidth;
  Type17->TotalWidth = PrimaryBusWidth + BusWidthExtension;

  if (PrimarySignalLoadingValue == VAL_SIGNAL_LOADING_3DS) {
    PrimaryLogicalRanksPerDimm = PackageRanksPerDimm * PrimaryDieCount;
  } else {
    PrimaryLogicalRanksPerDimm = PackageRanksPerDimm;
  }

  if (SymmetricalAssembly) {
    TotalCapacity = (PrimarySdramDensityPerDieMb / 8) *
                    (PrimaryBusWidth / SdramDeviceWidth) *
                    PrimaryLogicalRanksPerDimm;
  } else {
    EvenRanksCapacity = (PrimarySdramDensityPerDieMb / 8) *
                        (PrimaryBusWidth / SdramDeviceWidth) *
                        (PrimaryLogicalRanksPerDimm / 2);

    OddRanksCapacity = (SecondarySdramDensityPerDieMb / 8) *
                       (PrimaryBusWidth / SdramDeviceWidth) *
                       (SecondaryLogicalRanksPerDimm / 2);

    TotalCapacity = EvenRanksCapacity + OddRanksCapacity;
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
    Type17->Size = TYPE17_SIZE_USE_EXTENDED_FIELD;
    // Bits 30:0 represent the size of the memory device in megabytes.
    Type17->ExtendedSize = (UINT32)TotalCapacity;
  }

  Type17->VolatileSize = TotalCapacity * SIZE_1MB;
  Type17->LogicalSize  = TotalCapacity * SIZE_1MB;

  return EFI_SUCCESS;
}

/** Main entry point for parsing a DDR4 SPD buffer.

  @param SpdData            SPD data buffer.
  @param SpdBufferSize      The size of the SPD data buffer.
  @param Type17             SMBIOS Type17 table. Allocated by this library.
                            Free with FreePool.
  @param FixedStringsLength The length of fixed strings in the Type17 table.

**/
EFI_STATUS
ParseDdr4 (
  IN     UINT8             *SpdData,
  IN     UINTN             SpdBufferSize,
  OUT SMBIOS_TABLE_TYPE17  **Type17,
  IN     UINTN             FixedStringsLength
  )
{
  EFI_STATUS           Status;
  UINTN                SpdBytesTotal;
  UINTN                BufferSize;
  UINT16               Crc;
  SMBIOS_TABLE_TYPE17  *Table;
  SPD_DDR4             *Spd;

  Spd = (SPD_DDR4 *)SpdData;

  if (Spd->Base.Description.Bits.BytesTotal >= ARRAY_SIZE (SpdBytes)) {
    DEBUG ((
      DEBUG_ERROR,
      "SPD bytes total unknown/invalid: %02x (%d vs %d)\n",
      SPD_BYTES_TOTAL (SpdData),
      SpdBytes[SPD_BYTES_TOTAL (SpdData)],
      SpdBufferSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  SpdBytesTotal = SpdBytes[SPD_BYTES_TOTAL (SpdData)];

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
               (DDR_SPD_DDR4_PART_NUMBER_LENGTH + 1);

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

  Status = UpdateCapacity (SpdData, Table);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // DDR4 operates at 1.2V
  Table->MinimumVoltage    = 1200;
  Table->MaximumVoltage    = 1200;
  Table->ConfiguredVoltage = 1200;

  Table->ModuleManufacturerID = Spd->ManufactureInfo.ModuleId.IdCode.Data;

  UpdatePartNumber (
    SpdData,
    DDR_SPD_MODULE_PART_NUM_IDX,
    *Type17,
    FixedStringsLength,
    FALSE
    );

  Table->MemorySubsystemControllerManufacturerID = Spd->ManufactureInfo.DramIdCode.Data;
  Table->MemorySubsystemControllerProductID      = 0x0000;

  UpdateManufacturer (
    SpdData,
    DDR_SPD_DRAM_MFG_ID_CODE_1_IDX,
    *Type17,
    FixedStringsLength,
    FALSE
    );
  UpdateSerialNumber (
    SpdData,
    DDR_SPD_MODULE_MFG_ID_CODE_1_IDX,
    *Type17,
    FixedStringsLength
    );

  return EFI_SUCCESS;
}
