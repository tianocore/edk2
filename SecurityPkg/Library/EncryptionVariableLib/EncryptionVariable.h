/** @file
  Definitions used by this library implementation.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ENCRYPTION_VARIABLE_H_
#define _ENCRYPTION_VARIABLE_H_

#define ENC_KEY_SEP           L":"
#define ENC_KEY_SEP_SIZE      2
#define ENC_KEY_NAME          L"VAR_ENC_KEY"
#define ENC_KEY_NAME_SIZE     22

#define ENC_KEY_SIZE          (256/8)
#define ENC_BLOCK_SIZE        AES_BLOCK_SIZE
#define ENC_IVEC_SIZE         ENC_BLOCK_SIZE

#define ENC_PADDING_BYTE      0x0F

//
// PKCS#5 padding
//
//#define AES_CIPHER_DATA_SIZE(PlainDataSize) \
//  (AES_BLOCK_SIZE + (PlainDataSize)) & (~(AES_BLOCK_SIZE - 1))
//
#define AES_CIPHER_DATA_SIZE(PlainDataSize) ALIGN_VALUE (PlainDataSize, AES_BLOCK_SIZE)

#define FREE_POOL(Address)      \
    if ((Address) != NULL) {    \
      FreePool (Address);       \
      (Address) = NULL;         \
    }

#pragma pack(1)

typedef struct {
  UINT32     DataType;        // SYM_TYPE_AES
  UINT32     HeaderSize;      // sizeof(VARIABLE_ENCRYPTION_HEADER)
  UINT32     PlainDataSize;   // Plain data size
  UINT32     CipherDataSize;  // Cipher data size
  UINT8      KeyIvec[ENC_IVEC_SIZE];
} VARIABLE_ENCRYPTION_HEADER;

#pragma pack()

#endif  // _ENCRYPTION_VARIABLE_H_
