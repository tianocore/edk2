/** @file
  Implementation of MD5 algorithm

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Md5.c

Abstract:

  Implementation of MD5 algorithm

**/

#include "Md5.h"

CONST UINT32  MD5_K[][2] = {
  { 0, 1 },
  { 1, 5 },
  { 5, 3 },
  { 0, 7 }
};

CONST UINT32  MD5_S[][4] = {
  { 7, 22, 17, 12 },
  { 5, 20, 14, 9 },
  { 4, 23, 16 ,11 },
  { 6, 21, 15, 10 },
};

CONST UINT32  MD5_T[] = {
  0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
  0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
  0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
  0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
  0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
  0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
  0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
  0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
  0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
  0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
  0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
  0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
  0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
  0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
  0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
  0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
};

CONST UINT8 Md5HashPadding[] =
{
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

//
// ROTATE_LEFT rotates x left n bits.
//
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define SA            S[j & 3]
#define SB            S[(j + 1) & 3]
#define SC            S[(j + 2) & 3]
#define SD            S[(j + 3) & 3]

//
// TF1, TF2, TF3, TF4 are basic MD5 transform functions
//
UINT32 TF1 (UINT32 A, UINT32 B, UINT32 C)
{
  return (A & B) | (~A & C);
}

UINT32 TF2 (UINT32 A, UINT32 B, UINT32 C)
{
  return (A & C) | (B & ~C);
}

UINT32 TF3 (UINT32 A, UINT32 B, UINT32 C)
{
  return A ^ B ^ C;
}

UINT32 TF4 (UINT32 A, UINT32 B, UINT32 C)
{
  return B ^ (A | ~C);
}

typedef
UINT32
(*MD5_TRANSFORM_FUNC) (
  IN UINT32  A,
  IN UINT32  B,
  IN UINT32  C
  );

CONST MD5_TRANSFORM_FUNC MD5_F[] = {
  TF1,
  TF2,
  TF3,
  TF4
};

/**
  Perform the MD5 transform on 64 bytes data segment

  @param  Md5Ctx[in]  it includes the data segment for Md5 transform

  @retval NONE.

**/
VOID
MD5Transform (
  IN MD5_CTX  *Md5Ctx
  )
{
  UINT32  i;
  UINT32  j;
  UINT32  S[MD5_HASHSIZE >> 2];
  UINT32  *X;
  UINT32  k;
  UINT32  t;

  X = (UINT32 *) Md5Ctx->M;

  //
  // Copy MD5 states to S
  //
  CopyMem (S, Md5Ctx->States, MD5_HASHSIZE);

  t = 0;
  for (i = 0; i < 4; i++) {
    k = MD5_K[i][0];
    for (j = 16; j > 0; j--) {
      SA += (*MD5_F[i]) (SB, SC, SD) + X[k] + MD5_T[t];
      SA  = ROTATE_LEFT (SA, MD5_S[i][j & 3]);
      SA += SB;

      k += MD5_K[i][1];
      k &= 15;

      t++;
    }
  }

  for (i = 0; i < 4; i++) {
    Md5Ctx->States[i] += S[i];
  }
}

/**
  Copy data segment into the M field of MD5_CTX structure for later transform.
  If the length of data segment is larger than 64 bytes, then does the transform
  immediately and the generated Md5 code is stored in the States field of MD5_CTX
  data struct for later accumulation. 
  All of Md5 code generated for the sequential 64-bytes data segaments are be
  accumulated in MD5Final() function.

  @param  Md5Ctx[in]  the data structure of storing the original data
                      segment and the final result.

  @param  Data[in]    the data wanted to be transformed

  @param  DataLen[in] the length of data

  @retval NONE.
**/
VOID
MD5UpdateBlock (
  IN MD5_CTX      *Md5Ctx,
  IN CONST UINT8  *Data,
  IN       UINTN  DataLen
  )
{
  UINTN Limit;

  for (Limit = 64 - Md5Ctx->Count; DataLen >= 64 - Md5Ctx->Count; Limit = 64) {
    CopyMem (Md5Ctx->M + Md5Ctx->Count, (VOID *)Data, Limit);
    MD5Transform (Md5Ctx);
    
    Md5Ctx->Count = 0;
    Data         += Limit;
    DataLen      -= Limit;
  }

  CopyMem (Md5Ctx->M + Md5Ctx->Count, (VOID *)Data, DataLen);
  Md5Ctx->Count += DataLen;
}

/**
  Initialize four 32-bits chaining variables and use them to do the Md5 transform.

  @param  Md5Ctx[in]  the data structure of Md5

  @retval EFI_SUCCESS initialization is ok

**/
EFI_STATUS
MD5Init (
  IN MD5_CTX  *Md5Ctx
  )
{
  ZeroMem (Md5Ctx, sizeof (*Md5Ctx));

  //
  // Set magic initialization constants.
  //
  Md5Ctx->States[0] = 0x67452301;
  Md5Ctx->States[1] = 0xefcdab89;
  Md5Ctx->States[2] = 0x98badcfe;
  Md5Ctx->States[3] = 0x10325476;  

  return EFI_SUCCESS;
}

/**
  the external interface of Md5 algorithm

  @param  Md5Ctx[in]  the data structure of storing the original data
                      segment and the final result.

  @param  Data[in]    the data wanted to be transformed.

  @param  DataLen[in] the length of data.

  @retval EFI_SUCCESS the transform is ok.

**/
EFI_STATUS
MD5Update (
  IN  MD5_CTX  *Md5Ctx,
  IN  VOID     *Data,
  IN  UINTN    DataLen
  )
{
  if (EFI_ERROR (Md5Ctx->Status)) {
    return Md5Ctx->Status;
  }

  MD5UpdateBlock (Md5Ctx, (CONST UINT8 *) Data, DataLen);
  Md5Ctx->Length += DataLen;
  return EFI_SUCCESS;
}

/**
  accumulate the MD5 value of every data segment and generate the finial
  result according to MD5 algorithm

  @param  Md5Ctx[in]   the data structure of storing the original data
                       segment and the final result.

  @param  HashVal[out] the final 128-bits output.

  @retval EFI_SUCCESS  the transform is ok.

**/
EFI_STATUS
MD5Final (
  IN  MD5_CTX  *Md5Ctx,
  OUT UINT8    *HashVal
  )
{
  UINTN PadLength;

  if (Md5Ctx->Status == EFI_ALREADY_STARTED) {
    //
    // Store Hashed value & Zeroize sensitive context information.
    //
    CopyMem (HashVal, (UINT8 *) Md5Ctx->States, MD5_HASHSIZE);
    ZeroMem ((UINT8 *)Md5Ctx, sizeof (*Md5Ctx));
    
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Md5Ctx->Status)) {
    return Md5Ctx->Status;
  }

  PadLength  = Md5Ctx->Count >= 56 ? 120 : 56;
  PadLength -= Md5Ctx->Count;
  MD5UpdateBlock (Md5Ctx, Md5HashPadding, PadLength);
  Md5Ctx->Length = LShiftU64 (Md5Ctx->Length, 3);
  MD5UpdateBlock (Md5Ctx, (CONST UINT8 *) &Md5Ctx->Length, 8);

  ZeroMem (Md5Ctx->M, sizeof (Md5Ctx->M));
  Md5Ctx->Length  = 0;
  Md5Ctx->Status  = EFI_ALREADY_STARTED;
  return MD5Final (Md5Ctx, HashVal);
}

