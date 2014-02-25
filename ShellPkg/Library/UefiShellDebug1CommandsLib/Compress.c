/** @file
  Main file for compression routine.

  Compression routine. The compression algorithm is a mixture of
  LZ77 and Huffman coding. LZ77 transforms the source data into a
  sequence of Original Characters and Pointers to repeated strings.
  This sequence is further divided into Blocks and Huffman codings
  are applied to each Block.

  Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <ShellBase.h>
#include <Uefi.h>

//
// Macro Definitions
//
typedef INT16             NODE;
#define UINT8_MAX         0xff
#define UINT8_BIT         8
#define THRESHOLD         3
#define INIT_CRC          0
#define WNDBIT            13
#define WNDSIZ            (1U << WNDBIT)
#define MAXMATCH          256
#define BLKSIZ            (1U << 14)  // 16 * 1024U
#define PERC_FLAG         0x8000U
#define CODE_BIT          16
#define NIL               0
#define MAX_HASH_VAL      (3 * WNDSIZ + (WNDSIZ / 512 + 1) * UINT8_MAX)
#define HASH(LoopVar7, LoopVar5)        ((LoopVar7) + ((LoopVar5) << (WNDBIT - 9)) + WNDSIZ * 2)
#define CRCPOLY           0xA001
#define UPDATE_CRC(LoopVar5)     mCrc = mCrcTable[(mCrc ^ (LoopVar5)) & 0xFF] ^ (mCrc >> UINT8_BIT)

//
// C: the Char&Len Set; P: the Position Set; T: the exTra Set
//
#define NC                (UINT8_MAX + MAXMATCH + 2 - THRESHOLD)
#define CBIT              9
#define NP                (WNDBIT + 1)
#define PBIT              4
#define NT                (CODE_BIT + 3)
#define TBIT              5
#if NT > NP
  #define                 NPT NT
#else
  #define                 NPT NP
#endif
//
// Function Prototypes
//

/**
  Put a dword to output stream

  @param[in] Data    The dword to put.
**/
VOID
EFIAPI
PutDword(
  IN UINT32 Data
  );

//
//  Global Variables
//
STATIC UINT8  *mSrc;
STATIC UINT8  *mDst;
STATIC UINT8  *mSrcUpperLimit;
STATIC UINT8  *mDstUpperLimit;

STATIC UINT8  *mLevel;
STATIC UINT8  *mText;
STATIC UINT8  *mChildCount;
STATIC UINT8  *mBuf;
STATIC UINT8  mCLen[NC];
STATIC UINT8  mPTLen[NPT];
STATIC UINT8  *mLen;
STATIC INT16  mHeap[NC + 1];
STATIC INT32  mRemainder;
STATIC INT32  mMatchLen;
STATIC INT32  mBitCount;
STATIC INT32  mHeapSize;
STATIC INT32  mTempInt32;
STATIC UINT32 mBufSiz = 0;
STATIC UINT32 mOutputPos;
STATIC UINT32 mOutputMask;
STATIC UINT32 mSubBitBuf;
STATIC UINT32 mCrc;
STATIC UINT32 mCompSize;
STATIC UINT32 mOrigSize;

STATIC UINT16 *mFreq;
STATIC UINT16 *mSortPtr;
STATIC UINT16 mLenCnt[17];
STATIC UINT16 mLeft[2 * NC - 1];
STATIC UINT16 mRight[2 * NC - 1];
STATIC UINT16 mCrcTable[UINT8_MAX + 1];
STATIC UINT16 mCFreq[2 * NC - 1];
STATIC UINT16 mCCode[NC];
STATIC UINT16 mPFreq[2 * NP - 1];
STATIC UINT16 mPTCode[NPT];
STATIC UINT16 mTFreq[2 * NT - 1];

STATIC NODE   mPos;
STATIC NODE   mMatchPos;
STATIC NODE   mAvail;
STATIC NODE   *mPosition;
STATIC NODE   *mParent;
STATIC NODE   *mPrev;
STATIC NODE   *mNext = NULL;
INT32         mHuffmanDepth = 0;

/**
  Make a CRC table.

**/
VOID
EFIAPI
MakeCrcTable (
  VOID
  )
{
  UINT32  LoopVar1;

  UINT32  LoopVar2;

  UINT32  LoopVar4;

  for (LoopVar1 = 0; LoopVar1 <= UINT8_MAX; LoopVar1++) {
    LoopVar4 = LoopVar1;
    for (LoopVar2 = 0; LoopVar2 < UINT8_BIT; LoopVar2++) {
      if ((LoopVar4 & 1) != 0) {
        LoopVar4 = (LoopVar4 >> 1) ^ CRCPOLY;
      } else {
        LoopVar4 >>= 1;
      }
    }

    mCrcTable[LoopVar1] = (UINT16) LoopVar4;
  }
}

/**
  Put a dword to output stream

  @param[in] Data    The dword to put.
**/
VOID
EFIAPI
PutDword (
  IN UINT32 Data
  )
{
  if (mDst < mDstUpperLimit) {
    *mDst++ = (UINT8) (((UINT8) (Data)) & 0xff);
  }

  if (mDst < mDstUpperLimit) {
    *mDst++ = (UINT8) (((UINT8) (Data >> 0x08)) & 0xff);
  }

  if (mDst < mDstUpperLimit) {
    *mDst++ = (UINT8) (((UINT8) (Data >> 0x10)) & 0xff);
  }

  if (mDst < mDstUpperLimit) {
    *mDst++ = (UINT8) (((UINT8) (Data >> 0x18)) & 0xff);
  }
}

/**
  Allocate memory spaces for data structures used in compression process.
  
  @retval EFI_SUCCESS           Memory was allocated successfully.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
AllocateMemory (
  VOID
  )
{
  mText       = AllocateZeroPool (WNDSIZ * 2 + MAXMATCH);
  mLevel      = AllocateZeroPool ((WNDSIZ + UINT8_MAX + 1) * sizeof (*mLevel));
  mChildCount = AllocateZeroPool ((WNDSIZ + UINT8_MAX + 1) * sizeof (*mChildCount));
  mPosition   = AllocateZeroPool ((WNDSIZ + UINT8_MAX + 1) * sizeof (*mPosition));
  mParent     = AllocateZeroPool (WNDSIZ * 2 * sizeof (*mParent));
  mPrev       = AllocateZeroPool (WNDSIZ * 2 * sizeof (*mPrev));
  mNext       = AllocateZeroPool ((MAX_HASH_VAL + 1) * sizeof (*mNext));

  mBufSiz     = BLKSIZ;
  mBuf        = AllocateZeroPool (mBufSiz);
  while (mBuf == NULL) {
    mBufSiz = (mBufSiz / 10U) * 9U;
    if (mBufSiz < 4 * 1024U) {
      return EFI_OUT_OF_RESOURCES;
    }

    mBuf = AllocateZeroPool (mBufSiz);
  }

  mBuf[0] = 0;

  return EFI_SUCCESS;
}

/**
  Called when compression is completed to free memory previously allocated.

**/
VOID
EFIAPI
FreeMemory (
  VOID
  )
{
  SHELL_FREE_NON_NULL (mText);
  SHELL_FREE_NON_NULL (mLevel);
  SHELL_FREE_NON_NULL (mChildCount);
  SHELL_FREE_NON_NULL (mPosition);
  SHELL_FREE_NON_NULL (mParent);
  SHELL_FREE_NON_NULL (mPrev);
  SHELL_FREE_NON_NULL (mNext);
  SHELL_FREE_NON_NULL (mBuf);
}

/**
  Initialize String Info Log data structures.
**/
VOID
EFIAPI
InitSlide (
  VOID
  )
{
  NODE  LoopVar1;

  SetMem (mLevel + WNDSIZ, (UINT8_MAX + 1) * sizeof (UINT8), 1);
  SetMem (mPosition + WNDSIZ, (UINT8_MAX + 1) * sizeof (NODE), 0);

  SetMem (mParent + WNDSIZ, WNDSIZ * sizeof (NODE), 0);

  mAvail = 1;
  for (LoopVar1 = 1; LoopVar1 < WNDSIZ - 1; LoopVar1++) {
    mNext[LoopVar1] = (NODE) (LoopVar1 + 1);
  }

  mNext[WNDSIZ - 1] = NIL;
  SetMem (mNext + WNDSIZ * 2, (MAX_HASH_VAL - WNDSIZ * 2 + 1) * sizeof (NODE), 0);
}

/**
  Find child node given the parent node and the edge character

  @param[in] LoopVar6       The parent node.
  @param[in] LoopVar5       The edge character.

  @return             The child node.
  @retval NIL(Zero)   No child could be found.

**/
NODE
EFIAPI
Child (
  IN NODE   LoopVar6,
  IN UINT8  LoopVar5
  )
{
  NODE  LoopVar4;

  LoopVar4             = mNext[HASH (LoopVar6, LoopVar5)];
  mParent[NIL]  = LoopVar6;  /* sentinel */
  while (mParent[LoopVar4] != LoopVar6) {
    LoopVar4 = mNext[LoopVar4];
  }

  return LoopVar4;
}

/**
  Create a new child for a given parent node.

  @param[in] LoopVar6       The parent node.
  @param[in] LoopVar5       The edge character.
  @param[in] LoopVar4       The child node.
**/
VOID
EFIAPI
MakeChild (
  IN NODE   LoopVar6,
  IN UINT8  LoopVar5,
  IN NODE   LoopVar4
  )
{
  NODE  LoopVar12;

  NODE  LoopVar10;

  LoopVar12          = (NODE) HASH (LoopVar6, LoopVar5);
  LoopVar10          = mNext[LoopVar12];
  mNext[LoopVar12]   = LoopVar4;
  mNext[LoopVar4]    = LoopVar10;
  mPrev[LoopVar10]   = LoopVar4;
  mPrev[LoopVar4]    = LoopVar12;
  mParent[LoopVar4]  = LoopVar6;
  mChildCount[LoopVar6]++;
}

/**
  Split a node.

  @param[in] Old     The node to split.
**/
VOID
EFIAPI
Split (
  IN NODE Old
  )
{
  NODE  New;

  NODE  LoopVar10;

  New               = mAvail;
  mAvail            = mNext[New];
  mChildCount[New]  = 0;
  LoopVar10                 = mPrev[Old];
  mPrev[New]        = LoopVar10;
  mNext[LoopVar10]          = New;
  LoopVar10                 = mNext[Old];
  mNext[New]        = LoopVar10;
  mPrev[LoopVar10]          = New;
  mParent[New]      = mParent[Old];
  mLevel[New]       = (UINT8) mMatchLen;
  mPosition[New]    = mPos;
  MakeChild (New, mText[mMatchPos + mMatchLen], Old);
  MakeChild (New, mText[mPos + mMatchLen], mPos);
}

/**
  Insert string info for current position into the String Info Log.

**/
VOID
EFIAPI
InsertNode (
  VOID
  )
{
  NODE  LoopVar6;

  NODE  LoopVar4;

  NODE  LoopVar2;

  NODE  LoopVar10;
  UINT8 LoopVar5;
  UINT8 *TempString3;
  UINT8 *TempString2;

  if (mMatchLen >= 4) {
    //
    // We have just got a long match, the target tree
    // can be located by MatchPos + 1. Travese the tree
    // from bottom up to get to a proper starting point.
    // The usage of PERC_FLAG ensures proper node deletion
    // in DeleteNode() later.
    //
    mMatchLen--;
    LoopVar4 = (NODE) ((mMatchPos + 1) | WNDSIZ);
    LoopVar6 = mParent[LoopVar4];
    while (LoopVar6 == NIL) {
      LoopVar4 = mNext[LoopVar4];
      LoopVar6 = mParent[LoopVar4];
    }

    while (mLevel[LoopVar6] >= mMatchLen) {
      LoopVar4 = LoopVar6;
      LoopVar6 = mParent[LoopVar6];
    }

    LoopVar10 = LoopVar6;
    while (mPosition[LoopVar10] < 0) {
      mPosition[LoopVar10]  = mPos;
      LoopVar10             = mParent[LoopVar10];
    }

    if (LoopVar10 < WNDSIZ) {
      mPosition[LoopVar10] = (NODE) (mPos | PERC_FLAG);
    }
  } else {
    //
    // Locate the target tree
    //
    LoopVar6 = (NODE) (mText[mPos] + WNDSIZ);
    LoopVar5 = mText[mPos + 1];
    LoopVar4 = Child (LoopVar6, LoopVar5);
    if (LoopVar4 == NIL) {
      MakeChild (LoopVar6, LoopVar5, mPos);
      mMatchLen = 1;
      return ;
    }

    mMatchLen = 2;
  }
  //
  // Traverse down the tree to find a match.
  // Update Position value along the route.
  // Node split or creation is involved.
  //
  for (;;) {
    if (LoopVar4 >= WNDSIZ) {
      LoopVar2         = MAXMATCH;
      mMatchPos = LoopVar4;
    } else {
      LoopVar2         = mLevel[LoopVar4];
      mMatchPos = (NODE) (mPosition[LoopVar4] & ~PERC_FLAG);
    }

    if (mMatchPos >= mPos) {
      mMatchPos -= WNDSIZ;
    }

    TempString3  = &mText[mPos + mMatchLen];
    TempString2  = &mText[mMatchPos + mMatchLen];
    while (mMatchLen < LoopVar2) {
      if (*TempString3 != *TempString2) {
        Split (LoopVar4);
        return ;
      }

      mMatchLen++;
      TempString3++;
      TempString2++;
    }

    if (mMatchLen >= MAXMATCH) {
      break;
    }

    mPosition[LoopVar4]  = mPos;
    LoopVar6             = LoopVar4;
    LoopVar4             = Child (LoopVar6, *TempString3);
    if (LoopVar4 == NIL) {
      MakeChild (LoopVar6, *TempString3, mPos);
      return ;
    }

    mMatchLen++;
  }

  LoopVar10             = mPrev[LoopVar4];
  mPrev[mPos]   = LoopVar10;
  mNext[LoopVar10]      = mPos;
  LoopVar10             = mNext[LoopVar4];
  mNext[mPos]   = LoopVar10;
  mPrev[LoopVar10]      = mPos;
  mParent[mPos] = LoopVar6;
  mParent[LoopVar4]    = NIL;

  //
  // Special usage of 'next'
  //
  mNext[LoopVar4] = mPos;

}

/**
  Delete outdated string info. (The Usage of PERC_FLAG
  ensures a clean deletion).

**/
VOID
EFIAPI
DeleteNode (
  VOID
  )
{
  NODE  LoopVar6;

  NODE  LoopVar4;

  NODE  LoopVar11;

  NODE  LoopVar10;

  NODE  LoopVar9;

  if (mParent[mPos] == NIL) {
    return ;
  }

  LoopVar4             = mPrev[mPos];
  LoopVar11             = mNext[mPos];
  mNext[LoopVar4]      = LoopVar11;
  mPrev[LoopVar11]      = LoopVar4;
  LoopVar4             = mParent[mPos];
  mParent[mPos] = NIL;
  if (LoopVar4 >= WNDSIZ) {
    return ;
  }

  mChildCount[LoopVar4]--;
  if (mChildCount[LoopVar4] > 1) {
    return ;
  }

  LoopVar10 = (NODE) (mPosition[LoopVar4] & ~PERC_FLAG);
  if (LoopVar10 >= mPos) {
    LoopVar10 -= WNDSIZ;
  }

  LoopVar11 = LoopVar10;
  LoopVar6 = mParent[LoopVar4];
  LoopVar9 = mPosition[LoopVar6];
  while ((LoopVar9 & PERC_FLAG) != 0){
    LoopVar9 &= ~PERC_FLAG;
    if (LoopVar9 >= mPos) {
      LoopVar9 -= WNDSIZ;
    }

    if (LoopVar9 > LoopVar11) {
      LoopVar11 = LoopVar9;
    }

    mPosition[LoopVar6]  = (NODE) (LoopVar11 | WNDSIZ);
    LoopVar6             = mParent[LoopVar6];
    LoopVar9             = mPosition[LoopVar6];
  }

  if (LoopVar6 < WNDSIZ) {
    if (LoopVar9 >= mPos) {
      LoopVar9 -= WNDSIZ;
    }

    if (LoopVar9 > LoopVar11) {
      LoopVar11 = LoopVar9;
    }

    mPosition[LoopVar6] = (NODE) (LoopVar11 | WNDSIZ | PERC_FLAG);
  }

  LoopVar11           = Child (LoopVar4, mText[LoopVar10 + mLevel[LoopVar4]]);
  LoopVar10           = mPrev[LoopVar11];
  LoopVar9           = mNext[LoopVar11];
  mNext[LoopVar10]    = LoopVar9;
  mPrev[LoopVar9]    = LoopVar10;
  LoopVar10           = mPrev[LoopVar4];
  mNext[LoopVar10]    = LoopVar11;
  mPrev[LoopVar11]    = LoopVar10;
  LoopVar10           = mNext[LoopVar4];
  mPrev[LoopVar10]    = LoopVar11;
  mNext[LoopVar11]    = LoopVar10;
  mParent[LoopVar11]  = mParent[LoopVar4];
  mParent[LoopVar4]  = NIL;
  mNext[LoopVar4]    = mAvail;
  mAvail      = LoopVar4;
}

/**
  Read in source data

  @param[out] LoopVar7   The buffer to hold the data.
  @param[in] LoopVar8    The number of bytes to read.

  @return The number of bytes actually read.
**/
INT32
EFIAPI
FreadCrc (
  OUT UINT8 *LoopVar7,
  IN  INT32 LoopVar8
  )
{
  INT32 LoopVar1;

  for (LoopVar1 = 0; mSrc < mSrcUpperLimit && LoopVar1 < LoopVar8; LoopVar1++) {
    *LoopVar7++ = *mSrc++;
  }

  LoopVar8 = LoopVar1;

  LoopVar7 -= LoopVar8;
  mOrigSize += LoopVar8;
  LoopVar1--;
  while (LoopVar1 >= 0) {
    UPDATE_CRC (*LoopVar7++);
    LoopVar1--;
  }

  return LoopVar8;
}

/**
  Advance the current position (read in new data if needed).
  Delete outdated string info. Find a match string for current position.

  @retval TRUE      The operation was successful.
  @retval FALSE     The operation failed due to insufficient memory.
**/
BOOLEAN
EFIAPI
GetNextMatch (
  VOID
  )
{
  INT32 LoopVar8;
  VOID  *Temp;

  mRemainder--;
  mPos++;
  if (mPos == WNDSIZ * 2) {
    Temp = AllocateZeroPool (WNDSIZ + MAXMATCH);
    if (Temp == NULL) {
      return (FALSE);
    }
    CopyMem (Temp, &mText[WNDSIZ], WNDSIZ + MAXMATCH);
    CopyMem (&mText[0], Temp, WNDSIZ + MAXMATCH);
    FreePool (Temp);
    LoopVar8 = FreadCrc (&mText[WNDSIZ + MAXMATCH], WNDSIZ);
    mRemainder += LoopVar8;
    mPos = WNDSIZ;
  }

  DeleteNode ();
  InsertNode ();

  return (TRUE);
}

/**
  Send entry LoopVar1 down the queue.

  @param[in] LoopVar1    The index of the item to move.
**/
VOID
EFIAPI
DownHeap (
  IN INT32 i
  )
{
  INT32 LoopVar1;

  INT32 LoopVar2;

  //
  // priority queue: send i-th entry down heap
  //
  LoopVar2 = mHeap[i];
  LoopVar1 = 2 * i;
  while (LoopVar1 <= mHeapSize) {
    if (LoopVar1 < mHeapSize && mFreq[mHeap[LoopVar1]] > mFreq[mHeap[LoopVar1 + 1]]) {
      LoopVar1++;
    }

    if (mFreq[LoopVar2] <= mFreq[mHeap[LoopVar1]]) {
      break;
    }

    mHeap[i]  = mHeap[LoopVar1];
    i         = LoopVar1;
    LoopVar1         = 2 * i;
  }

  mHeap[i] = (INT16) LoopVar2;
}

/**
  Count the number of each code length for a Huffman tree.

  @param[in] LoopVar1      The top node.
**/
VOID
EFIAPI
CountLen (
  IN INT32 LoopVar1
  )
{
  if (LoopVar1 < mTempInt32) {
    mLenCnt[(mHuffmanDepth < 16) ? mHuffmanDepth : 16]++;
  } else {
    mHuffmanDepth++;
    CountLen (mLeft[LoopVar1]);
    CountLen (mRight[LoopVar1]);
    mHuffmanDepth--;
  }
}

/**
  Create code length array for a Huffman tree.

  @param[in] Root   The root of the tree.
**/
VOID
EFIAPI
MakeLen (
  IN INT32 Root
  )
{
  INT32   LoopVar1;

  INT32   LoopVar2;
  UINT32  Cum;

  for (LoopVar1 = 0; LoopVar1 <= 16; LoopVar1++) {
    mLenCnt[LoopVar1] = 0;
  }

  CountLen (Root);

  //
  // Adjust the length count array so that
  // no code will be generated longer than its designated length
  //
  Cum = 0;
  for (LoopVar1 = 16; LoopVar1 > 0; LoopVar1--) {
    Cum += mLenCnt[LoopVar1] << (16 - LoopVar1);
  }

  while (Cum != (1U << 16)) {
    mLenCnt[16]--;
    for (LoopVar1 = 15; LoopVar1 > 0; LoopVar1--) {
      if (mLenCnt[LoopVar1] != 0) {
        mLenCnt[LoopVar1]--;
        mLenCnt[LoopVar1 + 1] += 2;
        break;
      }
    }

    Cum--;
  }

  for (LoopVar1 = 16; LoopVar1 > 0; LoopVar1--) {
    LoopVar2 = mLenCnt[LoopVar1];
    LoopVar2--;
    while (LoopVar2 >= 0) {
      mLen[*mSortPtr++] = (UINT8) LoopVar1;
      LoopVar2--;
    }
  }
}

/**
  Assign code to each symbol based on the code length array.
  
  @param[in] LoopVar8      The number of symbols.
  @param[in] Len    The code length array.
  @param[out] Code  The stores codes for each symbol.
**/
VOID
EFIAPI
MakeCode (
  IN  INT32         LoopVar8,
  IN  UINT8 Len[    ],
  OUT UINT16 Code[  ]
  )
{
  INT32   LoopVar1;
  UINT16  Start[18];

  Start[1] = 0;
  for (LoopVar1 = 1; LoopVar1 <= 16; LoopVar1++) {
    Start[LoopVar1 + 1] = (UINT16) ((Start[LoopVar1] + mLenCnt[LoopVar1]) << 1);
  }

  for (LoopVar1 = 0; LoopVar1 < LoopVar8; LoopVar1++) {
    Code[LoopVar1] = Start[Len[LoopVar1]]++;
  }
}
  
/**
  Generates Huffman codes given a frequency distribution of symbols.

  @param[in] NParm      The number of symbols.
  @param[in] FreqParm   The frequency of each symbol.
  @param[out] LenParm   The code length for each symbol.
  @param[out] CodeParm  The code for each symbol.

  @return The root of the Huffman tree.
**/
INT32
EFIAPI
MakeTree (
  IN  INT32             NParm,
  IN  UINT16  FreqParm[ ],
  OUT UINT8   LenParm[  ],
  OUT UINT16  CodeParm[ ]
  )
{
  INT32 LoopVar1;

  INT32 LoopVar2;

  INT32 LoopVar3;

  INT32 Avail;

  //
  // make tree, calculate len[], return root
  //
  mTempInt32        = NParm;
  mFreq     = FreqParm;
  mLen      = LenParm;
  Avail     = mTempInt32;
  mHeapSize = 0;
  mHeap[1]  = 0;
  for (LoopVar1 = 0; LoopVar1 < mTempInt32; LoopVar1++) {
    mLen[LoopVar1] = 0;
    if ((mFreq[LoopVar1]) != 0) {
      mHeapSize++;
      mHeap[mHeapSize] = (INT16) LoopVar1;
    }
  }

  if (mHeapSize < 2) {
    CodeParm[mHeap[1]] = 0;
    return mHeap[1];
  }

  for (LoopVar1 = mHeapSize / 2; LoopVar1 >= 1; LoopVar1--) {
    //
    // make priority queue
    //
    DownHeap (LoopVar1);
  }

  mSortPtr = CodeParm;
  do {
    LoopVar1 = mHeap[1];
    if (LoopVar1 < mTempInt32) {
      *mSortPtr++ = (UINT16) LoopVar1;
    }

    mHeap[1] = mHeap[mHeapSize--];
    DownHeap (1);
    LoopVar2 = mHeap[1];
    if (LoopVar2 < mTempInt32) {
      *mSortPtr++ = (UINT16) LoopVar2;
    }

    LoopVar3         = Avail++;
    mFreq[LoopVar3]  = (UINT16) (mFreq[LoopVar1] + mFreq[LoopVar2]);
    mHeap[1]  = (INT16) LoopVar3;
    DownHeap (1);
    mLeft[LoopVar3]  = (UINT16) LoopVar1;
    mRight[LoopVar3] = (UINT16) LoopVar2;
  } while (mHeapSize > 1);

  mSortPtr = CodeParm;
  MakeLen (LoopVar3);
  MakeCode (NParm, LenParm, CodeParm);

  //
  // return root
  //
  return LoopVar3;
}

/**
  Outputs rightmost LoopVar8 bits of x

  @param[in] LoopVar8   The rightmost LoopVar8 bits of the data is used.
  @param[in] x   The data.
**/
VOID
EFIAPI
PutBits (
  IN INT32    LoopVar8,
  IN UINT32   x
  )
{
  UINT8 Temp;

  if (LoopVar8 < mBitCount) {
    mSubBitBuf |= x << (mBitCount -= LoopVar8);
  } else {

    Temp = (UINT8)(mSubBitBuf | (x >> (LoopVar8 -= mBitCount)));
    if (mDst < mDstUpperLimit) {
      *mDst++ = Temp;
    }
    mCompSize++;

    if (LoopVar8 < UINT8_BIT) {
      mSubBitBuf = x << (mBitCount = UINT8_BIT - LoopVar8);
    } else {

      Temp = (UINT8)(x >> (LoopVar8 - UINT8_BIT));
      if (mDst < mDstUpperLimit) {
        *mDst++ = Temp;
      }
      mCompSize++;

      mSubBitBuf = x << (mBitCount = 2 * UINT8_BIT - LoopVar8);
    }
  }
}

/**
  Encode a signed 32 bit number.

  @param[in] LoopVar5     The number to encode.
**/
VOID
EFIAPI
EncodeC (
  IN INT32 LoopVar5
  )
{
  PutBits (mCLen[LoopVar5], mCCode[LoopVar5]);
}

/**
  Encode a unsigned 32 bit number.

  @param[in] LoopVar7     The number to encode.
**/
VOID
EFIAPI
EncodeP (
  IN UINT32 LoopVar7
  )
{
  UINT32  LoopVar5;

  UINT32  LoopVar6;

  LoopVar5 = 0;
  LoopVar6 = LoopVar7;
  while (LoopVar6 != 0) {
    LoopVar6 >>= 1;
    LoopVar5++;
  }

  PutBits (mPTLen[LoopVar5], mPTCode[LoopVar5]);
  if (LoopVar5 > 1) {
    PutBits(LoopVar5 - 1, LoopVar7 & (0xFFFFU >> (17 - LoopVar5)));
  }
}

/**
  Count the frequencies for the Extra Set.

**/
VOID
EFIAPI
CountTFreq (
  VOID
  )
{
  INT32 LoopVar1;

  INT32 LoopVar3;

  INT32 LoopVar8;

  INT32 Count;

  for (LoopVar1 = 0; LoopVar1 < NT; LoopVar1++) {
    mTFreq[LoopVar1] = 0;
  }

  LoopVar8 = NC;
  while (LoopVar8 > 0 && mCLen[LoopVar8 - 1] == 0) {
    LoopVar8--;
  }

  LoopVar1 = 0;
  while (LoopVar1 < LoopVar8) {
    LoopVar3 = mCLen[LoopVar1++];
    if (LoopVar3 == 0) {
      Count = 1;
      while (LoopVar1 < LoopVar8 && mCLen[LoopVar1] == 0) {
        LoopVar1++;
        Count++;
      }

      if (Count <= 2) {
        mTFreq[0] = (UINT16) (mTFreq[0] + Count);
      } else if (Count <= 18) {
        mTFreq[1]++;
      } else if (Count == 19) {
        mTFreq[0]++;
        mTFreq[1]++;
      } else {
        mTFreq[2]++;
      }
    } else {
      ASSERT((LoopVar3+2)<(2 * NT - 1));
      mTFreq[LoopVar3 + 2]++;
    }
  }
}

/**
  Outputs the code length array for the Extra Set or the Position Set.

  @param[in] LoopVar8       The number of symbols.
  @param[in] nbit           The number of bits needed to represent 'LoopVar8'.
  @param[in] Special        The special symbol that needs to be take care of.

**/
VOID
EFIAPI
WritePTLen (
  IN INT32 LoopVar8,
  IN INT32 nbit,
  IN INT32 Special
  )
{
  INT32 LoopVar1;

  INT32 LoopVar3;

  while (LoopVar8 > 0 && mPTLen[LoopVar8 - 1] == 0) {
    LoopVar8--;
  }

  PutBits (nbit, LoopVar8);
  LoopVar1 = 0;
  while (LoopVar1 < LoopVar8) {
    LoopVar3 = mPTLen[LoopVar1++];
    if (LoopVar3 <= 6) {
      PutBits (3, LoopVar3);
    } else {
      PutBits (LoopVar3 - 3, (1U << (LoopVar3 - 3)) - 2);
    }

    if (LoopVar1 == Special) {
      while (LoopVar1 < 6 && mPTLen[LoopVar1] == 0) {
        LoopVar1++;
      }

      PutBits (2, (LoopVar1 - 3) & 3);
    }
  }
}

/**
  Outputs the code length array for Char&Length Set.
**/
VOID
EFIAPI
WriteCLen (
  VOID
  )
{
  INT32 LoopVar1;

  INT32 LoopVar3;

  INT32 LoopVar8;

  INT32 Count;

  LoopVar8 = NC;
  while (LoopVar8 > 0 && mCLen[LoopVar8 - 1] == 0) {
    LoopVar8--;
  }

  PutBits (CBIT, LoopVar8);
  LoopVar1 = 0;
  while (LoopVar1 < LoopVar8) {
    LoopVar3 = mCLen[LoopVar1++];
    if (LoopVar3 == 0) {
      Count = 1;
      while (LoopVar1 < LoopVar8 && mCLen[LoopVar1] == 0) {
        LoopVar1++;
        Count++;
      }

      if (Count <= 2) {
        for (LoopVar3 = 0; LoopVar3 < Count; LoopVar3++) {
          PutBits (mPTLen[0], mPTCode[0]);
        }
      } else if (Count <= 18) {
        PutBits (mPTLen[1], mPTCode[1]);
        PutBits (4, Count - 3);
      } else if (Count == 19) {
        PutBits (mPTLen[0], mPTCode[0]);
        PutBits (mPTLen[1], mPTCode[1]);
        PutBits (4, 15);
      } else {
        PutBits (mPTLen[2], mPTCode[2]);
        PutBits (CBIT, Count - 20);
      }
    } else {
      ASSERT((LoopVar3+2)<NPT);
      PutBits (mPTLen[LoopVar3 + 2], mPTCode[LoopVar3 + 2]);
    }
  }
}

/**
  Huffman code the block and output it.

**/
VOID
EFIAPI
SendBlock (
  VOID
  )
{
  UINT32  LoopVar1;

  UINT32  LoopVar3;

  UINT32  Flags;

  UINT32  Root;

  UINT32  Pos;

  UINT32  Size;
  Flags = 0;

  Root  = MakeTree (NC, mCFreq, mCLen, mCCode);
  Size  = mCFreq[Root];
  PutBits (16, Size);
  if (Root >= NC) {
    CountTFreq ();
    Root = MakeTree (NT, mTFreq, mPTLen, mPTCode);
    if (Root >= NT) {
      WritePTLen (NT, TBIT, 3);
    } else {
      PutBits (TBIT, 0);
      PutBits (TBIT, Root);
    }

    WriteCLen ();
  } else {
    PutBits (TBIT, 0);
    PutBits (TBIT, 0);
    PutBits (CBIT, 0);
    PutBits (CBIT, Root);
  }

  Root = MakeTree (NP, mPFreq, mPTLen, mPTCode);
  if (Root >= NP) {
    WritePTLen (NP, PBIT, -1);
  } else {
    PutBits (PBIT, 0);
    PutBits (PBIT, Root);
  }

  Pos = 0;
  for (LoopVar1 = 0; LoopVar1 < Size; LoopVar1++) {
    if (LoopVar1 % UINT8_BIT == 0) {
      Flags = mBuf[Pos++];
    } else {
      Flags <<= 1;
    }
    if ((Flags & (1U << (UINT8_BIT - 1))) != 0){
      EncodeC(mBuf[Pos++] + (1U << UINT8_BIT));
      LoopVar3 = mBuf[Pos++] << UINT8_BIT;
      LoopVar3 += mBuf[Pos++];

      EncodeP (LoopVar3);
    } else {
      EncodeC (mBuf[Pos++]);
    }
  }

  SetMem (mCFreq, NC * sizeof (UINT16), 0);
  SetMem (mPFreq, NP * sizeof (UINT16), 0);
}

/**
  Start the huffman encoding.

**/
VOID
EFIAPI
HufEncodeStart (
  VOID
  )
{
  SetMem (mCFreq, NC * sizeof (UINT16), 0);
  SetMem (mPFreq, NP * sizeof (UINT16), 0);

  mOutputPos = mOutputMask = 0;

  mBitCount   = UINT8_BIT;
  mSubBitBuf  = 0;
}

/**
  Outputs an Original Character or a Pointer.

  @param[in] LoopVar5     The original character or the 'String Length' element of 
                   a Pointer.
  @param[in] LoopVar7     The 'Position' field of a Pointer.
**/
VOID
EFIAPI
CompressOutput (
  IN UINT32 LoopVar5,
  IN UINT32 LoopVar7
  )
{
  STATIC UINT32 CPos;

  if ((mOutputMask >>= 1) == 0) {
    mOutputMask = 1U << (UINT8_BIT - 1);
    if (mOutputPos >= mBufSiz - 3 * UINT8_BIT) {
      SendBlock ();
      mOutputPos = 0;
    }

    CPos        = mOutputPos++;
    mBuf[CPos]  = 0;
  }
  mBuf[mOutputPos++] = (UINT8) LoopVar5;
  mCFreq[LoopVar5]++;
  if (LoopVar5 >= (1U << UINT8_BIT)) {
    mBuf[CPos] = (UINT8)(mBuf[CPos]|mOutputMask);
    mBuf[mOutputPos++] = (UINT8)(LoopVar7 >> UINT8_BIT);
    mBuf[mOutputPos++] = (UINT8) LoopVar7;
    LoopVar5                  = 0;
    while (LoopVar7!=0) {
      LoopVar7 >>= 1;
      LoopVar5++;
    }
    mPFreq[LoopVar5]++;
  }
}

/**
  End the huffman encoding.

**/
VOID
EFIAPI
HufEncodeEnd (
  VOID
  )
{
  SendBlock ();

  //
  // Flush remaining bits
  //
  PutBits (UINT8_BIT - 1, 0);
}

/**
  The main controlling routine for compression process.

  @retval EFI_SUCCESS           The compression is successful.
  @retval EFI_OUT_0F_RESOURCES  Not enough memory for compression process.
**/
EFI_STATUS
EFIAPI
Encode (
  VOID
  )
{
  EFI_STATUS  Status;
  INT32       LastMatchLen;
  NODE        LastMatchPos;

  Status = AllocateMemory ();
  if (EFI_ERROR (Status)) {
    FreeMemory ();
    return Status;
  }

  InitSlide ();

  HufEncodeStart ();

  mRemainder  = FreadCrc (&mText[WNDSIZ], WNDSIZ + MAXMATCH);

  mMatchLen   = 0;
  mPos        = WNDSIZ;
  InsertNode ();
  if (mMatchLen > mRemainder) {
    mMatchLen = mRemainder;
  }

  while (mRemainder > 0) {
    LastMatchLen  = mMatchLen;
    LastMatchPos  = mMatchPos;
    if (!GetNextMatch ()) {
      Status = EFI_OUT_OF_RESOURCES;
    }
    if (mMatchLen > mRemainder) {
      mMatchLen = mRemainder;
    }

    if (mMatchLen > LastMatchLen || LastMatchLen < THRESHOLD) {
      //
      // Not enough benefits are gained by outputting a pointer,
      // so just output the original character
      //
      CompressOutput(mText[mPos - 1], 0);
    } else {
      //
      // Outputting a pointer is beneficial enough, do it.
      //

      CompressOutput(LastMatchLen + (UINT8_MAX + 1 - THRESHOLD),
             (mPos - LastMatchPos - 2) & (WNDSIZ - 1));
      LastMatchLen--;
      while (LastMatchLen > 0) {
        if (!GetNextMatch ()) {
          Status = EFI_OUT_OF_RESOURCES;
        }
        LastMatchLen--;
      }

      if (mMatchLen > mRemainder) {
        mMatchLen = mRemainder;
      }
    }
  }

  HufEncodeEnd ();
  FreeMemory ();
  return (Status);
}

/**
  The compression routine.

  @param[in]       SrcBuffer     The buffer containing the source data.
  @param[in]       SrcSize       The number of bytes in SrcBuffer.
  @param[in]       DstBuffer     The buffer to put the compressed image in.
  @param[in, out]  DstSize       On input the size (in bytes) of DstBuffer, on
                                return the number of bytes placed in DstBuffer.

  @retval EFI_SUCCESS           The compression was sucessful.
  @retval EFI_BUFFER_TOO_SMALL  The buffer was too small.  DstSize is required.
**/
EFI_STATUS
EFIAPI
Compress (
  IN       VOID   *SrcBuffer,
  IN       UINT64 SrcSize,
  IN       VOID   *DstBuffer,
  IN OUT   UINT64 *DstSize
  )
{
  EFI_STATUS  Status;

  //
  // Initializations
  //
  mBufSiz         = 0;
  mBuf            = NULL;
  mText           = NULL;
  mLevel          = NULL;
  mChildCount     = NULL;
  mPosition       = NULL;
  mParent         = NULL;
  mPrev           = NULL;
  mNext           = NULL;

  mSrc            = SrcBuffer;
  mSrcUpperLimit  = mSrc + SrcSize;
  mDst            = DstBuffer;
  mDstUpperLimit  = mDst +*DstSize;

  PutDword (0L);
  PutDword (0L);

  MakeCrcTable ();

  mOrigSize             = mCompSize = 0;
  mCrc                  = INIT_CRC;

  //
  // Compress it
  //
  Status = Encode ();
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Null terminate the compressed data
  //
  if (mDst < mDstUpperLimit) {
    *mDst++ = 0;
  }
  //
  // Fill in compressed size and original size
  //
  mDst = DstBuffer;
  PutDword (mCompSize + 1);
  PutDword (mOrigSize);

  //
  // Return
  //
  if (mCompSize + 1 + 8 > *DstSize) {
    *DstSize = mCompSize + 1 + 8;
    return EFI_BUFFER_TOO_SMALL;
  } else {
    *DstSize = mCompSize + 1 + 8;
    return EFI_SUCCESS;
  }

}

