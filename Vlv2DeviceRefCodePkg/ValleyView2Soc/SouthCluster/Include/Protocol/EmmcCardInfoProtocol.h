/*++

Copyright (c)  2013  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent


--*/


/*++
Module Name:

  EmmcCardInfoProtocol.h

Abstract:

  Interface definition for EFI_EMMC_CARD_INFO_PROTOCOL

--*/


#ifndef _EMMC_CARD_INFO_H_
#define _EMMC_CARD_INFO_H_

#define EFI_EMMC_CARD_INFO_PROTOCOL_GUID \
  { \
    0x1ebe5ab9, 0x2129, 0x49e7, {0x84, 0xd7, 0xee, 0xb9, 0xfc, 0xe5, 0xde, 0xdd } \
  }

typedef struct _EFI_EMMC_CARD_INFO_PROTOCOL  EFI_EMMC_CARD_INFO_PROTOCOL;


//
// EMMC Card info Structures
//
struct _EFI_EMMC_CARD_INFO_PROTOCOL {
  CARD_DATA *CardData;
};

extern EFI_GUID gEfiEmmcCardInfoProtocolGuid;
#endif
