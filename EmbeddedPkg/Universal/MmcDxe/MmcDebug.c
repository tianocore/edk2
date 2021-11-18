/** @file
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include "Mmc.h"

#if !defined (MDEPKG_NDEBUG)
CONST CHAR8  *mStrUnit[] = {
  "100kbit/s", "1Mbit/s", "10Mbit/s", "100MBit/s",
  "Unknown",   "Unknown", "Unknown",  "Unknown"
};
CONST CHAR8  *mStrValue[] = {
  "1.0",     "1.2",     "1.3",     "1.5", "2.0", "2.5", "3.0", "3.5", "4.0", "4.5", "5.0",
  "Unknown", "Unknown", "Unknown", "Unknown"
};
#endif

VOID
PrintCID (
  IN UINT32  *Cid
  )
{
  DEBUG ((DEBUG_ERROR, "- PrintCID\n"));
  DEBUG ((DEBUG_ERROR, "\t- Manufacturing date: %d/%d\n", (Cid[0] >> 8) & 0xF, (Cid[0] >> 12) & 0xFF));
  DEBUG ((DEBUG_ERROR, "\t- Product serial number: 0x%X%X\n", Cid[1] & 0xFFFFFF, (Cid[0] >> 24) & 0xFF));
  DEBUG ((DEBUG_ERROR, "\t- Product revision: %d\n", Cid[1] >> 24));
  // DEBUG ((DEBUG_ERROR, "\t- Product name: %s\n", (char*)(Cid + 2)));
  DEBUG ((DEBUG_ERROR, "\t- OEM ID: %c%c\n", (Cid[3] >> 8) & 0xFF, (Cid[3] >> 16) & 0xFF));
}

VOID
PrintCSD (
  IN UINT32  *Csd
  )
{
  UINTN  Value;

  if (((Csd[2] >> 30) & 0x3) == 0) {
    DEBUG ((DEBUG_ERROR, "- PrintCSD Version 1.01-1.10/Version 2.00/Standard Capacity\n"));
  } else if (((Csd[2] >> 30) & 0x3) == 1) {
    DEBUG ((DEBUG_ERROR, "- PrintCSD Version 2.00/High Capacity\n"));
  } else {
    DEBUG ((DEBUG_ERROR, "- PrintCSD Version Higher than v3.3\n"));
  }

  DEBUG ((DEBUG_ERROR, "\t- Supported card command class: 0x%X\n", MMC_CSD_GET_CCC (Csd)));
  DEBUG ((DEBUG_ERROR, "\t- Speed: %a %a\n", mStrValue[(MMC_CSD_GET_TRANSPEED (Csd) >> 3) & 0xF], mStrUnit[MMC_CSD_GET_TRANSPEED (Csd) & 7]));
  DEBUG ((DEBUG_ERROR, "\t- Maximum Read Data Block: %d\n", 2 << (MMC_CSD_GET_READBLLEN (Csd)-1)));
  DEBUG ((DEBUG_ERROR, "\t- Maximum Write Data Block: %d\n", 2 << (MMC_CSD_GET_WRITEBLLEN (Csd)-1)));

  if (!MMC_CSD_GET_FILEFORMATGRP (Csd)) {
    Value = MMC_CSD_GET_FILEFORMAT (Csd);
    if (Value == 0) {
      DEBUG ((DEBUG_ERROR, "\t- Format (0): Hard disk-like file system with partition table\n"));
    } else if (Value == 1) {
      DEBUG ((DEBUG_ERROR, "\t- Format (1): DOS FAT (floppy-like) with boot sector only (no partition table)\n"));
    } else if (Value == 2) {
      DEBUG ((DEBUG_ERROR, "\t- Format (2): Universal File Format\n"));
    } else {
      DEBUG ((DEBUG_ERROR, "\t- Format (3): Others/Unknown\n"));
    }
  } else {
    DEBUG ((DEBUG_ERROR, "\t- Format: Reserved\n"));
  }
}

VOID
PrintRCA (
  IN UINT32  Rca
  )
{
  DEBUG ((DEBUG_ERROR, "- PrintRCA: 0x%X\n", Rca));
  DEBUG ((DEBUG_ERROR, "\t- Status: 0x%X\n", Rca & 0xFFFF));
  DEBUG ((DEBUG_ERROR, "\t- RCA: 0x%X\n", (Rca >> 16) & 0xFFFF));
}

VOID
PrintOCR (
  IN UINT32  Ocr
  )
{
  UINTN  MinV;
  UINTN  MaxV;
  UINTN  Volts;
  UINTN  Loop;

  MinV  = 36;  // 3.6
  MaxV  = 20;  // 2.0
  Volts = 20;  // 2.0

  // The MMC register bits [23:8] indicate the working range of the card
  for (Loop = 8; Loop < 24; Loop++) {
    if (Ocr & (1 << Loop)) {
      if (MinV > Volts) {
        MinV = Volts;
      }

      if (MaxV < Volts) {
        MaxV = Volts + 1;
      }
    }

    Volts++;
  }

  DEBUG ((DEBUG_ERROR, "- PrintOCR Ocr (0x%X)\n", Ocr));
  DEBUG ((DEBUG_ERROR, "\t- Card operating voltage: %d.%d to %d.%d\n", MinV/10, MinV % 10, MaxV/10, MaxV % 10));
  if (((Ocr >> 29) & 3) == 0) {
    DEBUG ((DEBUG_ERROR, "\t- AccessMode: Byte Mode\n"));
  } else {
    DEBUG ((DEBUG_ERROR, "\t- AccessMode: Block Mode (0x%X)\n", ((Ocr >> 29) & 3)));
  }

  if (Ocr & MMC_OCR_POWERUP) {
    DEBUG ((DEBUG_ERROR, "\t- PowerUp\n"));
  } else {
    DEBUG ((DEBUG_ERROR, "\t- Voltage Not Supported\n"));
  }
}

VOID
PrintResponseR1 (
  IN  UINT32  Response
  )
{
  DEBUG ((DEBUG_INFO, "Response: 0x%X\n", Response));
  if (Response & MMC_R0_READY_FOR_DATA) {
    DEBUG ((DEBUG_INFO, "\t- READY_FOR_DATA\n"));
  }

  switch ((Response >> 9) & 0xF) {
    case 0:
      DEBUG ((DEBUG_INFO, "\t- State: Idle\n"));
      break;
    case 1:
      DEBUG ((DEBUG_INFO, "\t- State: Ready\n"));
      break;
    case 2:
      DEBUG ((DEBUG_INFO, "\t- State: Ident\n"));
      break;
    case 3:
      DEBUG ((DEBUG_INFO, "\t- State: StandBy\n"));
      break;
    case 4:
      DEBUG ((DEBUG_INFO, "\t- State: Tran\n"));
      break;
    case 5:
      DEBUG ((DEBUG_INFO, "\t- State: Data\n"));
      break;
    case 6:
      DEBUG ((DEBUG_INFO, "\t- State: Rcv\n"));
      break;
    case 7:
      DEBUG ((DEBUG_INFO, "\t- State: Prg\n"));
      break;
    case 8:
      DEBUG ((DEBUG_INFO, "\t- State: Dis\n"));
      break;
    default:
      DEBUG ((DEBUG_INFO, "\t- State: Reserved\n"));
      break;
  }
}
