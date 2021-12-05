/** @file
  This code fills in standard CMOS values and updates the standard CMOS
  checksum. The Legacy16 code or LegacyBiosPlatform.c is responsible for
  non-standard CMOS locations and non-standard checksums.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LegacyBiosInterface.h"

/**
  Read CMOS register through index/data port.

  @param[in]  Index   The index of the CMOS register to read.

  @return  The data value from the CMOS register specified by Index.

**/
UINT8
LegacyReadStandardCmos (
  IN UINT8  Index
  )
{
  IoWrite8 (PORT_70, Index);
  return IoRead8 (PORT_71);
}

/**
  Write CMOS register through index/data port.

  @param[in]  Index  The index of the CMOS register to write.
  @param[in]  Value  The value of CMOS register to write.

  @return  The value written to the CMOS register specified by Index.

**/
UINT8
LegacyWriteStandardCmos (
  IN UINT8  Index,
  IN UINT8  Value
  )
{
  IoWrite8 (PORT_70, Index);
  return IoWrite8 (PORT_71, Value);
}

/**
  Calculate the new standard CMOS checksum and write it.

  @param  Private      Legacy BIOS Instance data

  @retval EFI_SUCCESS  Calculate 16-bit checksum successfully

**/
EFI_STATUS
LegacyCalculateWriteStandardCmosChecksum (
  VOID
  )
{
  UINT8   Register;
  UINT16  Checksum;

  for (Checksum = 0, Register = 0x10; Register < 0x2e; Register++) {
    Checksum = (UINT16)(Checksum + LegacyReadStandardCmos (Register));
  }

  LegacyWriteStandardCmos (CMOS_2E, (UINT8)(Checksum >> 8));
  LegacyWriteStandardCmos (CMOS_2F, (UINT8)(Checksum & 0xff));
  return EFI_SUCCESS;
}

/**
  Fill in the standard CMOS stuff before Legacy16 load

  @param  Private      Legacy BIOS Instance data

  @retval EFI_SUCCESS  It should always work.

**/
EFI_STATUS
LegacyBiosInitCmos (
  IN  LEGACY_BIOS_INSTANCE  *Private
  )
{
  UINT32  Size;

  //
  //  Clear all errors except RTC lost power
  //
  LegacyWriteStandardCmos (CMOS_0E, (UINT8)(LegacyReadStandardCmos (CMOS_0E) & BIT7));

  //
  // Update CMOS locations 15,16,17,18,30,31 and 32
  // CMOS 16,15 = 640Kb = 0x280
  // CMOS 18,17 = 31,30 = 15Mb max in 1Kb increments =0x3C00 max
  // CMOS 32 = 0x20
  //
  LegacyWriteStandardCmos (CMOS_15, 0x80);
  LegacyWriteStandardCmos (CMOS_16, 0x02);

  Size = 15 * SIZE_1MB;
  if (Private->IntThunk->EfiToLegacy16InitTable.OsMemoryAbove1Mb < (15 * SIZE_1MB)) {
    Size = Private->IntThunk->EfiToLegacy16InitTable.OsMemoryAbove1Mb >> 10;
  }

  LegacyWriteStandardCmos (CMOS_17, (UINT8)(Size & 0xFF));
  LegacyWriteStandardCmos (CMOS_30, (UINT8)(Size & 0xFF));
  LegacyWriteStandardCmos (CMOS_18, (UINT8)(Size >> 8));
  LegacyWriteStandardCmos (CMOS_31, (UINT8)(Size >> 8));

  LegacyCalculateWriteStandardCmosChecksum ();

  return EFI_SUCCESS;
}
