/** @file
  Definitions for Authenticated Code Module (ACM) firmware.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ACM_H_
#define _ACM_H_

#include <Base.h>

#define ACM_HEADER_VERSION_3                      (3 << 16)
#define ACM_PKCS_1_5_RSA_SIGNATURE_SHA256_SIZE    256
#define ACM_PKCS_1_5_RSA_SIGNATURE_SHA384_SIZE    384

#pragma pack(push, 1)

//
// ACM definition
//
#define ACM_MODULE_TYPE_CHIPSET_ACM                     2
#define ACM_MODULE_SUBTYPE_CAPABLE_OF_EXECUTE_AT_RESET  0x1
#define ACM_MODULE_SUBTYPE_ANC_MODULE                   0x2
#define ACM_HEADER_FLAG_DEBUG_SIGNED                    BIT15
#define ACM_NPW_SVN                                     0x2

typedef struct {
  UINT16     ModuleType;
  UINT16     ModuleSubType;
  UINT32     HeaderLen;
  UINT32     HeaderVersion;
  UINT16     ChipsetId;
  UINT16     Flags;
  UINT32     ModuleVendor;
  UINT32     Date;
  UINT32     Size;
  UINT16     AcmSvn;
  UINT16     SeAcmSvn;
  UINT32     CodeControl;
  UINT32     ErrorEntryPoint;
  UINT32     GdtLimit;
  UINT32     GdtBasePtr;
  UINT32     SegSel;
  UINT32     EntryPoint;
  UINT8      Rsvd2[64];
  UINT32     KeySize;            // 64 DWORDS in the Key
  UINT32     ScratchSize;
  UINT8      Rsa2048PubKey[256];
  UINT32     RsaPubExp;
  UINT8      Rsa2048Sig[256];
  UINT8      Scratch[572];       // 143 DWORDS = 572 BYTES Scratch Size
} ACM_HEADER;

typedef struct {
  UINT16     ModuleType;
  UINT16     ModuleSubType;
  UINT32     HeaderLen;
  UINT32     HeaderVersion;
  UINT16     ChipsetId;
  UINT16     Flags;
  UINT32     ModuleVendor;
  UINT32     Date;
  UINT32     Size;
  UINT16     AcmSvn;
  UINT16     SeAcmSvn;
  UINT32     CodeControl;
  UINT32     ErrorEntryPoint;
  UINT32     GdtLimit;
  UINT32     GdtBasePtr;
  UINT32     SegSel;
  UINT32     EntryPoint;
  UINT8      Rsvd2[64];
  UINT32     KeySize;            // 96 DWORDS in the Key
  UINT32     ScratchSize;
  UINT8      Rsa3072PubKey[384];
  UINT8      Rsa3072Sig[384];
  UINT8      Scratch[832];       // 208 DWORDS = 832 BYTES Scratch Size
} ACM_HEADER_3;

#pragma pack(pop)

#endif