/** @file
  This code fills in BDA (0x400) and EBDA (pointed to by 0x4xx)
  information. There is support for doing initializeation before
  Legacy16 is loaded and before a legacy boot is attempted.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LegacyBiosInterface.h"

/**
  Fill in the standard BDA and EBDA stuff before Legacy16 load

  @param  Private     Legacy BIOS Instance data

  @retval EFI_SUCCESS It should always work.

**/
EFI_STATUS
LegacyBiosInitBda (
  IN  LEGACY_BIOS_INSTANCE  *Private
  )
{
  BDA_STRUC  *Bda;
  UINT8      *Ebda;

  Bda  = (BDA_STRUC *)((UINTN)0x400);
  Ebda = (UINT8 *)((UINTN)0x9fc00);

  ACCESS_PAGE0_CODE (
    ZeroMem (Bda, 0x100);
    //
    // 640k-1k for EBDA
    //
    Bda->MemSize       = 0x27f;
    Bda->KeyHead       = 0x1e;
    Bda->KeyTail       = 0x1e;
    Bda->FloppyData    = 0x00;
    Bda->FloppyTimeout = 0xff;

    Bda->KeyStart       = 0x001E;
    Bda->KeyEnd         = 0x003E;
    Bda->KeyboardStatus = 0x10;
    Bda->Ebda           = 0x9fc0;

    //
    // Move LPT time out here and zero out LPT4 since some SCSI OPROMS
    // use this as scratch pad (LPT4 is Reserved)
    //
    Bda->Lpt1_2Timeout = 0x1414;
    Bda->Lpt3_4Timeout = 0x1400;

    );

  ZeroMem (Ebda, 0x400);
  *Ebda = 0x01;

  return EFI_SUCCESS;
}
