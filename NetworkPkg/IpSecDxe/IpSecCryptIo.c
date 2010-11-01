/** @file
  Common operation for Security.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IpSecCryptIo.h"
//
// Alogrithm's informations for the Encrypt/Decrpt Alogrithm.
//
ENCRYPT_ALGORITHM mIpsecEncryptAlgorithmList[IPSEC_ENCRYPT_ALGORITHM_LIST_SIZE] = {
  {EFI_IPSEC_EALG_NULL, 0, 0, 1, NULL, NULL, NULL, NULL},
  {(UINT8)-1,           0, 0, 0, NULL, NULL, NULL, NULL}
};
//
// Alogrithm's informations for the Authentication algorithm
//
AUTH_ALGORITHM mIpsecAuthAlgorithmList[IPSEC_AUTH_ALGORITHM_LIST_SIZE] = {
  {EFI_IPSEC_AALG_NONE, 0, 0, 0, NULL, NULL, NULL, NULL},
  {EFI_IPSEC_AALG_NULL, 0, 0, 0, NULL, NULL, NULL, NULL},
  {(UINT8)-1,           0, 0, 0, NULL, NULL, NULL, NULL}
};


/**
  Get the block size of encrypt alogrithm. The block size is based on the algorithm used.

  @param[in]  AlgorithmId          The encrypt algorithm ID.

  @return The value of block size.

**/
UINTN
IpSecGetEncryptBlockSize (
  IN UINT8   AlgorithmId
  )
{
  UINT8 Index;

  for (Index = 0; Index < IPSEC_ENCRYPT_ALGORITHM_LIST_SIZE; Index++) {
    if (AlgorithmId == mIpsecEncryptAlgorithmList[Index].AlgorithmId) {
      //
      // The BlockSize is same with IvSize.
      //
      return mIpsecEncryptAlgorithmList[Index].BlockSize;
    }
  }

  return (UINTN) -1;
}

/**
  Get the IV size of encrypt alogrithm. The IV size is based on the algorithm used.

  @param[in]  AlgorithmId          The encrypt algorithm ID.

  @return The value of IV size.

**/
UINTN
IpSecGetEncryptIvLength (
  IN UINT8 AlgorithmId
  )
{
  UINT8 Index;

  for (Index = 0; Index < IPSEC_ENCRYPT_ALGORITHM_LIST_SIZE; Index++) {
    if (AlgorithmId == mIpsecEncryptAlgorithmList[Index].AlgorithmId) {
      //
      // The BlockSize is same with IvSize.
      //
      return mIpsecEncryptAlgorithmList[Index].IvLength;
    }
  }

  return (UINTN) -1;
}

/**
  Get the ICV size of Authenticaion alogrithm. The ICV size is based on the algorithm used.

  @param[in]  AuthAlgorithmId          The Authentication algorithm ID.

  @return The value of ICV size.

**/
UINTN
IpSecGetIcvLength (
  IN UINT8  AuthAlgorithmId
  )
{
  UINT8 Index;
  for (Index = 0; Index < IPSEC_AUTH_ALGORITHM_LIST_SIZE; Index++) {
    if (AuthAlgorithmId == mIpsecAuthAlgorithmList[Index].AlgorithmId) {
      return mIpsecAuthAlgorithmList[Index].IcvLength;
    }
  }
  return (UINTN) -1;
}

/**
  Generate a random data for IV. If the IvSize is zero, not needed to create
  IV and return EFI_SUCCESS.

  @param[in]  IvBuffer  The pointer of the IV buffer.
  @param[in]  IvSize    The IV size.

  @retval     EFI_SUCCESS  Create a random data for IV.

**/
EFI_STATUS
IpSecGenerateIv (
  IN UINT8                           *IvBuffer,
  IN UINTN                           IvSize
  )
{
  if (IvSize != 0) {
    //
    //TODO: return CryptGenerateRandom (IvBuffer, IvSize);
    //
    return EFI_SUCCESS;
  }
  return EFI_SUCCESS;
}
