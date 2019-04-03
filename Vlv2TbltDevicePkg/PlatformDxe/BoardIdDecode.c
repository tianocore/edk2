/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:


  BoardIdDecode.c

Abstract:

--*/

#include "PchRegs.h"
#include "PlatformDxe.h"
#include "Platform.h"


//
// Define macros to build data structure signatures from characters.
//
#define EFI_SIGNATURE_16(A, B)        ((A) | (B << 8))
#define EFI_SIGNATURE_32(A, B, C, D)  (EFI_SIGNATURE_16 (A, B) | (EFI_SIGNATURE_16 (C, D) << 16))
#define EFI_SIGNATURE_64(A, B, C, D, E, F, G, H) \
    (EFI_SIGNATURE_32 (A, B, C, D) | ((UINT64) (EFI_SIGNATURE_32 (E, F, G, H)) << 32))

BOARD_ID_DECODE mBoardIdDecodeTable[] = {
  //
  // Board ID, Board Features bitmap, Subsystem Device ID
  // This is a dummy entry that has to exist. Do not delete, just make a generic entry that fit for product.
  //
  {
  	MW_ITX_MPCIE_LVDS_LOEM_AA,
    MW_ITX_MPCIE_LVDS_LOEM_ID,
    B_BOARD_FEATURES_FORM_FACTOR_ATX |
    B_BOARD_FEATURES_SIO_COM2 |
    B_BOARD_FEATURES_2_C0_MEMORY_SLOT |
    V_BOARD_FEATURES_SLEEP_S3 |
    B_BOARD_FEATURES_PS2WAKEFROMS5 |
    B_BOARD_FEATURES_LVDS |
    B_BOARD_FEATURES_VERB_TABLE1,
    V_DEFAULT_SUBSYSTEM_DEVICE_ID,
    0xD625,
    EFI_SIGNATURE_64('M','W','P','N','T','1','0','N')
  },

  {
  	 MW_ITX_MPCIE_LVDS_CHANNEL_AA,
    MW_ITX_MPCIE_LVDS_CHANNEL_ID,
    B_BOARD_FEATURES_FORM_FACTOR_ATX |
    B_BOARD_FEATURES_SIO_COM2 |
    B_BOARD_FEATURES_2_C0_MEMORY_SLOT |
    V_BOARD_FEATURES_SLEEP_S3 |
    B_BOARD_FEATURES_PS2WAKEFROMS5 |
    B_BOARD_FEATURES_LVDS |
    B_BOARD_FEATURES_VERB_TABLE1,
    V_DEFAULT_SUBSYSTEM_DEVICE_ID,
    0xD625,
    EFI_SIGNATURE_64('M','W','P','N','T','1','0','N')
  },

  {
  	MW_ITX_MPCIE_CHANNEL_AA,
    MW_ITX_MPCIE_CHANNEL_ID,
    B_BOARD_FEATURES_FORM_FACTOR_ATX |
    B_BOARD_FEATURES_SIO_COM2 |
    B_BOARD_FEATURES_2_C0_MEMORY_SLOT |
    V_BOARD_FEATURES_SLEEP_S3 |
    B_BOARD_FEATURES_PS2WAKEFROMS5 |
    B_BOARD_FEATURES_VERB_TABLE1,
    V_DEFAULT_SUBSYSTEM_DEVICE_ID,
    0xD625,
    EFI_SIGNATURE_64('M','W','P','N','T','1','0','N')
  },

  {
  	KT_ITX_MPCIE_LVDS_LOEM_AA,
    KT_ITX_MPCIE_LVDS_LOEM_ID,
    B_BOARD_FEATURES_FORM_FACTOR_ATX |
    B_BOARD_FEATURES_SIO_COM2 |
    B_BOARD_FEATURES_2_C0_MEMORY_SLOT |
    V_BOARD_FEATURES_SLEEP_S3 |
    B_BOARD_FEATURES_PS2WAKEFROMS5 |
    B_BOARD_FEATURES_LVDS |
    B_BOARD_FEATURES_VERB_TABLE2,
    V_DEFAULT_SUBSYSTEM_DEVICE_ID_KT,
    0xD626,
    EFI_SIGNATURE_64('K','T','P','N','T','1','0','N')
  },

  {
  	KT_ITX_CHANNEL_AA,
    KT_ITX_CHANNEL_ID,
    B_BOARD_FEATURES_FORM_FACTOR_ATX |
    B_BOARD_FEATURES_SIO_COM2 |
    B_BOARD_FEATURES_2_C0_MEMORY_SLOT |
    V_BOARD_FEATURES_SLEEP_S3 |
    B_BOARD_FEATURES_PS2WAKEFROMS5 |
    B_BOARD_FEATURES_NO_MINIPCIE |
    B_BOARD_FEATURES_VERB_TABLE2,
    V_DEFAULT_SUBSYSTEM_DEVICE_ID_KT,
    0xD626,
    EFI_SIGNATURE_64('K','T','P','N','T','1','0','N')
  },

  {
  	KT_ITX_LOEM_AA,
    KT_ITX_LOEM_ID,
    B_BOARD_FEATURES_FORM_FACTOR_ATX |
    B_BOARD_FEATURES_SIO_COM2 |
    B_BOARD_FEATURES_2_C0_MEMORY_SLOT |
    V_BOARD_FEATURES_SLEEP_S3 |
    B_BOARD_FEATURES_PS2WAKEFROMS5 |
    B_BOARD_FEATURES_NO_MINIPCIE |
    B_BOARD_FEATURES_VERB_TABLE2,
    V_DEFAULT_SUBSYSTEM_DEVICE_ID_KT,
    0xD626,
    EFI_SIGNATURE_64('K','T','P','N','T','1','0','N')
  }
};

UINTN mBoardIdDecodeTableSize = sizeof (mBoardIdDecodeTable) /
                                sizeof (mBoardIdDecodeTable[0]);


