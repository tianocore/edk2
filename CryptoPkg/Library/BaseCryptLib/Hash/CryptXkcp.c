/** @file
  Encode realted functions from Xkcp.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Copyright 2022 The eXtended Keccak Code Package (XKCP)
https://github.com/XKCP/XKCP
Keccak, designed by Guido Bertoni, Joan Daemen, Michael Peeters and Gilles Van Assche.
Implementation by the designers, hereby denoted as "the implementer".
For more information, feedback or questions, please refer to the Keccak Team website:
https://keccak.team/
To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/

**/

#include "CryptParallelHash.h"

/**
  Encode function from XKCP.

  Encodes the input as a byte string in a way that can be unambiguously parsed
  from the beginning of the string by inserting the length of the byte string
  before the byte string representation of input.

  @param[out] EncBuf  Result of left encode.
  @param[in]  Value   Input of left encode.

  @retval EncLen  Size of encode result in bytes.
**/
UINTN
EFIAPI
LeftEncode (
  OUT UINT8  *EncBuf,
  IN  UINTN  Value
  )
{
  UINT32  BlockNum;
  UINT32  EncLen;
  UINT32  Index;
  UINTN   ValueCopy;

  for ( ValueCopy = Value, BlockNum = 0; ValueCopy && (BlockNum < sizeof (UINTN)); ++BlockNum, ValueCopy >>= 8 ) {
    //
    // Empty
    //
  }

  if (BlockNum == 0) {
    BlockNum = 1;
  }

  for (Index = 1; Index <= BlockNum; ++Index) {
    EncBuf[Index] = (UINT8)(Value >> (8 * (BlockNum - Index)));
  }

  EncBuf[0] = (UINT8)BlockNum;
  EncLen    = BlockNum + 1;

  return EncLen;
}

/**
  Encode function from XKCP.

  Encodes the input as a byte string in a way that can be unambiguously parsed
  from the end of the string by inserting the length of the byte string after
  the byte string representation of input.

  @param[out] EncBuf  Result of right encode.
  @param[in]  Value   Input of right encode.

  @retval EncLen  Size of encode result in bytes.
**/
UINTN
EFIAPI
RightEncode (
  OUT UINT8  *EncBuf,
  IN  UINTN  Value
  )
{
  UINT32  BlockNum;
  UINT32  EncLen;
  UINT32  Index;
  UINTN   ValueCopy;

  for (ValueCopy = Value, BlockNum = 0; ValueCopy && (BlockNum < sizeof (UINTN)); ++BlockNum, ValueCopy >>= 8) {
    //
    // Empty
    //
  }

  if (BlockNum == 0) {
    BlockNum = 1;
  }

  for (Index = 1; Index <= BlockNum; ++Index) {
    EncBuf[Index-1] = (UINT8)(Value >> (8 * (BlockNum-Index)));
  }

  EncBuf[BlockNum] = (UINT8)BlockNum;
  EncLen           = BlockNum + 1;

  return EncLen;
}
