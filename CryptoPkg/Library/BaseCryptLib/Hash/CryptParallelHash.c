/** @file
  ParallelHash Implementation.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <Library/MmServicesTableLib.h>
#include <Library/SynchronizationLib.h>

#define PARALLELHASH_CUSTOMIZATION  "ParallelHash"

UINTN      mBlockNum;
UINTN      mBlockSize;
UINTN      mLastBlockSize;
UINT8      *mInput;
UINTN      mBlockResultSize;
UINT8      *mBlockHashResult;
BOOLEAN    *mBlockIsCompleted;
SPIN_LOCK  *mSpinLockList;

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
  );

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
  );

/**
  Computes the CSHAKE-256 message digest of a input data buffer.

  This function performs the CSHAKE-256 message digest of a given data buffer, and places
  the digest value into the specified memory.

  @param[in]   Data               Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize           Size of Data buffer in bytes.
  @param[in]   OutputLen          Size of output in bytes.
  @param[in]   Name               Pointer to the function name string.
  @param[in]   NameLen            Size of the function name in bytes.
  @param[in]   Customization      Pointer to the customization string.
  @param[in]   CustomizationLen   Size of the customization string in bytes.
  @param[out]  HashValue          Pointer to a buffer that receives the CSHAKE-256 digest
                                  value.

  @retval TRUE   CSHAKE-256 digest computation succeeded.
  @retval FALSE  CSHAKE-256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CShake256HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  IN   UINTN       OutputLen,
  IN   CONST VOID  *Name,
  IN   UINTN       NameLen,
  IN   CONST VOID  *Customization,
  IN   UINTN       CustomizationLen,
  OUT  UINT8       *HashValue
  );

/**
  Complete computation of digest of each block.

  Each AP perform the function called by BSP.

  @param[in] ProcedureArgument Argument of the procedure.
**/
VOID
EFIAPI
ParallelHashApExecute (
  IN VOID  *ProcedureArgument
  )
{
  UINTN    Index;
  BOOLEAN  Status;

  for (Index = 0; Index < mBlockNum; Index++) {
    if (AcquireSpinLockOrFail (&mSpinLockList[Index])) {
      //
      // Completed, try next one.
      //
      if (mBlockIsCompleted[Index]) {
        ReleaseSpinLock (&mSpinLockList[Index]);
        continue;
      }

      //
      // Calculate CShake256 for this block.
      //
      Status = CShake256HashAll (
                 mInput + Index * mBlockSize,
                 (Index == (mBlockNum - 1)) ? mLastBlockSize : mBlockSize,
                 mBlockResultSize,
                 NULL,
                 0,
                 NULL,
                 0,
                 mBlockHashResult + Index * mBlockResultSize
                 );
      if (!EFI_ERROR (Status)) {
        mBlockIsCompleted[Index] = TRUE;
      }

      ReleaseSpinLock (&mSpinLockList[Index]);
    }
  }
}

/**
  Dispatch the block task to each AP in SMM mode.

**/
VOID
EFIAPI
MmDispatchBlockToAP (
  VOID
  )
{
  UINTN  Index;

  for (Index = 0; Index < gMmst->NumberOfCpus; Index++) {
    if (Index != gMmst->CurrentlyExecutingCpu) {
      gMmst->MmStartupThisAp (ParallelHashApExecute, Index, NULL);
    }
  }

  return;
}

/**
  Parallel hash function ParallelHash256, as defined in NIST's Special Publication 800-185,
  published December 2016.

  @param[in]   Input            Pointer to the input message (X).
  @param[in]   InputByteLen     The number(>0) of input bytes provided for the input data.
  @param[in]   BlockSize        The size of each block (B).
  @param[out]  Output           Pointer to the output buffer.
  @param[in]   OutputByteLen    The desired number of output bytes (L).
  @param[in]   Customization    Pointer to the customization string (S).
  @param[in]   CustomByteLen    The length of the customization string in bytes.

  @retval TRUE   ParallelHash256 digest computation succeeded.
  @retval FALSE  ParallelHash256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
ParallelHash256HashAll (
  IN CONST VOID   *Input,
  IN       UINTN  InputByteLen,
  IN       UINTN  BlockSize,
  OUT      VOID   *Output,
  IN       UINTN  OutputByteLen,
  IN CONST VOID   *Customization,
  IN       UINTN  CustomByteLen
  )
{
  UINT8    EncBufB[sizeof (UINTN)+1];
  UINTN    EncSizeB;
  UINT8    EncBufN[sizeof (UINTN)+1];
  UINTN    EncSizeN;
  UINT8    EncBufL[sizeof (UINTN)+1];
  UINTN    EncSizeL;
  UINTN    Index;
  UINT8    *CombinedInput;
  UINTN    CombinedInputSize;
  BOOLEAN  AllCompleted;
  UINTN    Offset;
  BOOLEAN  ReturnValue;

  if ((InputByteLen == 0) || (OutputByteLen == 0) || (BlockSize == 0)) {
    return FALSE;
  }

  if ((Input == NULL) || (Output == NULL)) {
    return FALSE;
  }

  if ((CustomByteLen != 0) && (Customization == NULL)) {
    return FALSE;
  }

  mBlockSize = BlockSize;

  //
  // Calculate block number n.
  //
  mBlockNum = InputByteLen % mBlockSize == 0 ? InputByteLen / mBlockSize : InputByteLen / mBlockSize + 1;

  //
  // Set hash result size of each block in bytes.
  //
  mBlockResultSize = OutputByteLen;

  //
  // Encode B, n, L to string and record size.
  //
  EncSizeB = LeftEncode (EncBufB, mBlockSize);
  EncSizeN = RightEncode (EncBufN, mBlockNum);
  EncSizeL = RightEncode (EncBufL, OutputByteLen * CHAR_BIT);

  //
  // Allocate buffer for combined input (newX), Block completed flag and SpinLock.
  //
  CombinedInputSize = EncSizeB + EncSizeN + EncSizeL + mBlockNum * mBlockResultSize;
  CombinedInput     = AllocateZeroPool (CombinedInputSize);
  mBlockIsCompleted = AllocateZeroPool (mBlockNum * sizeof (BOOLEAN));
  mSpinLockList     = AllocatePool (mBlockNum * sizeof (SPIN_LOCK));
  if ((CombinedInput == NULL) || (mBlockIsCompleted == NULL) || (mSpinLockList == NULL)) {
    ReturnValue = FALSE;
    goto Exit;
  }

  //
  // Fill LeftEncode(B).
  //
  CopyMem (CombinedInput, EncBufB, EncSizeB);

  //
  // Prepare for parallel hash.
  //
  mBlockHashResult = CombinedInput + EncSizeB;
  mInput           = (UINT8 *)Input;
  mLastBlockSize   = InputByteLen % mBlockSize == 0 ? mBlockSize : InputByteLen % mBlockSize;

  //
  // Initialize SpinLock for each result block.
  //
  for (Index = 0; Index < mBlockNum; Index++) {
    InitializeSpinLock (&mSpinLockList[Index]);
  }

  //
  // Dispatch blocklist to each AP.
  //
  if (gMmst != NULL) {
    MmDispatchBlockToAP ();
  }

  //
  // Wait until all block hash completed.
  //
  do {
    AllCompleted = TRUE;
    for (Index = 0; Index < mBlockNum; Index++) {
      if (AcquireSpinLockOrFail (&mSpinLockList[Index])) {
        if (!mBlockIsCompleted[Index]) {
          AllCompleted = FALSE;
          ReturnValue  = CShake256HashAll (
                           mInput + Index * mBlockSize,
                           (Index == (mBlockNum - 1)) ? mLastBlockSize : mBlockSize,
                           mBlockResultSize,
                           NULL,
                           0,
                           NULL,
                           0,
                           mBlockHashResult + Index * mBlockResultSize
                           );
          if (ReturnValue) {
            mBlockIsCompleted[Index] = TRUE;
          }

          ReleaseSpinLock (&mSpinLockList[Index]);
          break;
        }

        ReleaseSpinLock (&mSpinLockList[Index]);
      } else {
        AllCompleted = FALSE;
        break;
      }
    }
  } while (!AllCompleted);

  //
  // Fill LeftEncode(n).
  //
  Offset = EncSizeB + mBlockNum * mBlockResultSize;
  CopyMem (CombinedInput + Offset, EncBufN, EncSizeN);

  //
  // Fill LeftEncode(L).
  //
  Offset += EncSizeN;
  CopyMem (CombinedInput + Offset, EncBufL, EncSizeL);

  ReturnValue = CShake256HashAll (
                  CombinedInput,
                  CombinedInputSize,
                  OutputByteLen,
                  PARALLELHASH_CUSTOMIZATION,
                  AsciiStrLen (PARALLELHASH_CUSTOMIZATION),
                  Customization,
                  CustomByteLen,
                  Output
                  );

Exit:
  ZeroMem (CombinedInput, CombinedInputSize);

  if (CombinedInput != NULL) {
    FreePool (CombinedInput);
  }

  if (mSpinLockList != NULL) {
    FreePool ((VOID *)mSpinLockList);
  }

  if (mBlockIsCompleted != NULL) {
    FreePool (mBlockIsCompleted);
  }

  return ReturnValue;
}
