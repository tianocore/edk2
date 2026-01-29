/** @file
Compression routine. The compression algorithm is a mixture of LZ77 and Huffman
coding. LZ77 transforms the source data into a sequence of Original Characters
and Pointers to repeated strings.
This sequence is further divided into Blocks and Huffman codings are applied to
each Block.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Compress.h"
#include "Decompress.h"
#include "TianoCompress.h"
#include "EfiUtilityMsgs.h"
#include "ParseInf.h"
#include <stdio.h>
#include "assert.h"

//
// Macro Definitions
//
static BOOLEAN VerboseMode = FALSE;
static BOOLEAN QuietMode = FALSE;
#undef UINT8_MAX
#define UINT8_MAX     0xff
#define UINT8_BIT     8
#define THRESHOLD     3
#define INIT_CRC      0
#define WNDBIT        19
#define WNDSIZ        (1U << WNDBIT)
#define MAXMATCH      256
#define BLKSIZ        (1U << 14)  // 16 * 1024U
#define PERC_FLAG     0x80000000U
#define CODE_BIT      16
#define NIL           0
#define MAX_HASH_VAL  (3 * WNDSIZ + (WNDSIZ / 512 + 1) * UINT8_MAX)
#define HASH(p, c)    ((p) + ((c) << (WNDBIT - 9)) + WNDSIZ * 2)
#define CRCPOLY       0xA001
#define UPDATE_CRC(c) mCrc = mCrcTable[(mCrc ^ (c)) & 0xFF] ^ (mCrc >> UINT8_BIT)

//
// C: the Char&Len Set; P: the Position Set; T: the exTra Set
//
//#define NC    (UINT8_MAX + MAXMATCH + 2 - THRESHOLD)
#define CBIT  9
#define NP    (WNDBIT + 1)
#define PBIT  5
//#define NT    (CODE_BIT + 3)
//#define TBIT  5
//#if NT > NP
//#define NPT NT
//#else
//#define NPT NP
//#endif

//
//  Global Variables
//
STATIC BOOLEAN ENCODE = FALSE;
STATIC BOOLEAN DECODE = FALSE;
STATIC BOOLEAN UEFIMODE = FALSE;
STATIC UINT8  *mSrc, *mDst, *mSrcUpperLimit, *mDstUpperLimit;
STATIC UINT8  *mLevel, *mText, *mChildCount, *mBuf, mCLen[NC], mPTLen[NPT], *mLen;
STATIC INT16  mHeap[NC + 1];
STATIC INT32  mRemainder, mMatchLen, mBitCount, mHeapSize, mN;
STATIC UINT32 mBufSiz = 0, mOutputPos, mOutputMask, mSubBitBuf, mCrc;
STATIC UINT32 mCompSize, mOrigSize;

STATIC UINT16 *mFreq, *mSortPtr, mLenCnt[17], mLeft[2 * NC - 1], mRight[2 * NC - 1], mCrcTable[UINT8_MAX + 1],
  mCFreq[2 * NC - 1], mCCode[NC], mPFreq[2 * NP - 1], mPTCode[NPT], mTFreq[2 * NT - 1];

STATIC NODE   mPos, mMatchPos, mAvail, *mPosition, *mParent, *mPrev, *mNext = NULL;

static  UINT64     DebugLevel;
static  BOOLEAN    DebugMode;
//
// functions
//
EFI_STATUS
TianoCompress (
  IN      UINT8   *SrcBuffer,
  IN      UINT32  SrcSize,
  IN      UINT8   *DstBuffer,
  IN OUT  UINT32  *DstSize
  )
/*++

Routine Description:

  The internal implementation of [Efi/Tiano]Compress().

Arguments:

  SrcBuffer   - The buffer storing the source data
  SrcSize     - The size of source data
  DstBuffer   - The buffer to store the compressed data

  Version     - The version of de/compression algorithm.
                Version 1 for EFI 1.1 de/compression algorithm.
                Version 2 for Tiano de/compression algorithm.

Returns:

  EFI_BUFFER_TOO_SMALL  - The DstBuffer is too small. In this case,
                DstSize contains the size needed.
  EFI_SUCCESS           - Compression is successful.
  EFI_OUT_OF_RESOURCES  - No resource to complete function.
  EFI_INVALID_PARAMETER - Parameter supplied is wrong.

--*/
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

STATIC
VOID
PutDword (
  IN UINT32 Data
  )
/*++

Routine Description:

  Put a dword to output stream

Arguments:

  Data    - the dword to put

Returns: (VOID)

--*/
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

STATIC
EFI_STATUS
AllocateMemory (
  VOID
  )
/*++

Routine Description:

  Allocate memory spaces for data structures used in compression process

Arguments:
  VOID

Returns:

  EFI_SUCCESS           - Memory is allocated successfully
  EFI_OUT_OF_RESOURCES  - Allocation fails

--*/
{
  UINT32  Index;

  mText = malloc (WNDSIZ * 2 + MAXMATCH);
  if (mText == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
    return EFI_OUT_OF_RESOURCES;
  }
  for (Index = 0; Index < WNDSIZ * 2 + MAXMATCH; Index++) {
    mText[Index] = 0;
  }

  mLevel      = malloc ((WNDSIZ + UINT8_MAX + 1) * sizeof (*mLevel));
  mChildCount = malloc ((WNDSIZ + UINT8_MAX + 1) * sizeof (*mChildCount));
  mPosition   = malloc ((WNDSIZ + UINT8_MAX + 1) * sizeof (*mPosition));
  mParent     = malloc (WNDSIZ * 2 * sizeof (*mParent));
  mPrev       = malloc (WNDSIZ * 2 * sizeof (*mPrev));
  mNext       = malloc ((MAX_HASH_VAL + 1) * sizeof (*mNext));
  if (mLevel == NULL || mChildCount == NULL || mPosition == NULL ||
    mParent == NULL || mPrev == NULL || mNext == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
    return EFI_OUT_OF_RESOURCES;
  }

  mBufSiz     = BLKSIZ;
  mBuf        = malloc (mBufSiz);
  while (mBuf == NULL) {
    mBufSiz = (mBufSiz / 10U) * 9U;
    if (mBufSiz < 4 * 1024U) {
      return EFI_OUT_OF_RESOURCES;
    }

    mBuf = malloc (mBufSiz);
  }

  mBuf[0] = 0;

  return EFI_SUCCESS;
}

VOID
FreeMemory (
  VOID
  )
/*++

Routine Description:

  Called when compression is completed to free memory previously allocated.

Arguments: (VOID)

Returns: (VOID)

--*/
{
  if (mText != NULL) {
    free (mText);
  }

  if (mLevel != NULL) {
    free (mLevel);
  }

  if (mChildCount != NULL) {
    free (mChildCount);
  }

  if (mPosition != NULL) {
    free (mPosition);
  }

  if (mParent != NULL) {
    free (mParent);
  }

  if (mPrev != NULL) {
    free (mPrev);
  }

  if (mNext != NULL) {
    free (mNext);
  }

  if (mBuf != NULL) {
    free (mBuf);
  }

  return ;
}

STATIC
VOID
InitSlide (
  VOID
  )
/*++

Routine Description:

  Initialize String Info Log data structures

Arguments: (VOID)

Returns: (VOID)

--*/
{
  NODE  Index;

  for (Index = WNDSIZ; Index <= WNDSIZ + UINT8_MAX; Index++) {
    mLevel[Index]     = 1;
    mPosition[Index]  = NIL;  // sentinel
  }

  for (Index = WNDSIZ; Index < WNDSIZ * 2; Index++) {
    mParent[Index] = NIL;
  }

  mAvail = 1;
  for (Index = 1; Index < WNDSIZ - 1; Index++) {
    mNext[Index] = (NODE) (Index + 1);
  }

  mNext[WNDSIZ - 1] = NIL;
  for (Index = WNDSIZ * 2; Index <= MAX_HASH_VAL; Index++) {
    mNext[Index] = NIL;
  }
}

STATIC
NODE
Child (
  IN NODE  NodeQ,
  IN UINT8 CharC
  )
/*++

Routine Description:

  Find child node given the parent node and the edge character

Arguments:

  NodeQ       - the parent node
  CharC       - the edge character

Returns:

  The child node (NIL if not found)

--*/
{
  NODE  NodeR;

  NodeR = mNext[HASH (NodeQ, CharC)];
  //
  // sentinel
  //
  mParent[NIL] = NodeQ;
  while (mParent[NodeR] != NodeQ) {
    NodeR = mNext[NodeR];
  }

  return NodeR;
}

STATIC
VOID
MakeChild (
  IN NODE  Parent,
  IN UINT8 CharC,
  IN NODE  Child
  )
/*++

Routine Description:

  Create a new child for a given parent node.

Arguments:

  Parent       - the parent node
  CharC   - the edge character
  Child       - the child node

Returns: (VOID)

--*/
{
  NODE  Node1;
  NODE  Node2;

  Node1           = (NODE) HASH (Parent, CharC);
  Node2           = mNext[Node1];
  mNext[Node1]    = Child;
  mNext[Child]    = Node2;
  mPrev[Node2]    = Child;
  mPrev[Child]    = Node1;
  mParent[Child]  = Parent;
  mChildCount[Parent]++;
}

STATIC
VOID
Split (
  NODE Old
  )
/*++

Routine Description:

  Split a node.

Arguments:

  Old     - the node to split

Returns: (VOID)

--*/
{
  NODE  New;
  NODE  TempNode;

  New               = mAvail;
  mAvail            = mNext[New];
  mChildCount[New]  = 0;
  TempNode          = mPrev[Old];
  mPrev[New]        = TempNode;
  mNext[TempNode]   = New;
  TempNode          = mNext[Old];
  mNext[New]        = TempNode;
  mPrev[TempNode]   = New;
  mParent[New]      = mParent[Old];
  mLevel[New]       = (UINT8) mMatchLen;
  mPosition[New]    = mPos;
  MakeChild (New, mText[mMatchPos + mMatchLen], Old);
  MakeChild (New, mText[mPos + mMatchLen], mPos);
}

STATIC
VOID
InsertNode (
  VOID
  )
/*++

Routine Description:

  Insert string info for current position into the String Info Log

Arguments: (VOID)

Returns: (VOID)

--*/
{
  NODE  NodeQ;
  NODE  NodeR;
  NODE  Index2;
  NODE  NodeT;
  UINT8 CharC;
  UINT8 *t1;
  UINT8 *t2;

  if (mMatchLen >= 4) {
    //
    // We have just got a long match, the target tree
    // can be located by MatchPos + 1. Traverse the tree
    // from bottom up to get to a proper starting point.
    // The usage of PERC_FLAG ensures proper node deletion
    // in DeleteNode() later.
    //
    mMatchLen--;
    NodeR = (NODE) ((mMatchPos + 1) | WNDSIZ);
    NodeQ = mParent[NodeR];
    while (NodeQ == NIL) {
      NodeR = mNext[NodeR];
      NodeQ = mParent[NodeR];
    }

    while (mLevel[NodeQ] >= mMatchLen) {
      NodeR = NodeQ;
      NodeQ = mParent[NodeQ];
    }

    NodeT = NodeQ;
    while (mPosition[NodeT] < 0) {
      mPosition[NodeT]  = mPos;
      NodeT             = mParent[NodeT];
    }

    if (NodeT < WNDSIZ) {
      mPosition[NodeT] = (NODE) (mPos | (UINT32) PERC_FLAG);
    }
  } else {
    //
    // Locate the target tree
    //
    NodeQ = (NODE) (mText[mPos] + WNDSIZ);
    CharC = mText[mPos + 1];
    NodeR = Child (NodeQ, CharC);
    if (NodeR == NIL) {
      MakeChild (NodeQ, CharC, mPos);
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
    if (NodeR >= WNDSIZ) {
      Index2    = MAXMATCH;
      mMatchPos = NodeR;
    } else {
      Index2    = mLevel[NodeR];
      mMatchPos = (NODE) (mPosition[NodeR] & (UINT32)~PERC_FLAG);
    }

    if (mMatchPos >= mPos) {
      mMatchPos -= WNDSIZ;
    }

    t1  = &mText[mPos + mMatchLen];
    t2  = &mText[mMatchPos + mMatchLen];
    while (mMatchLen < Index2) {
      if (*t1 != *t2) {
        Split (NodeR);
        return ;
      }

      mMatchLen++;
      t1++;
      t2++;
    }

    if (mMatchLen >= MAXMATCH) {
      break;
    }

    mPosition[NodeR]  = mPos;
    NodeQ             = NodeR;
    NodeR             = Child (NodeQ, *t1);
    if (NodeR == NIL) {
      MakeChild (NodeQ, *t1, mPos);
      return ;
    }

    mMatchLen++;
  }

  NodeT           = mPrev[NodeR];
  mPrev[mPos]     = NodeT;
  mNext[NodeT]    = mPos;
  NodeT           = mNext[NodeR];
  mNext[mPos]     = NodeT;
  mPrev[NodeT]    = mPos;
  mParent[mPos]   = NodeQ;
  mParent[NodeR]  = NIL;

  //
  // Special usage of 'next'
  //
  mNext[NodeR] = mPos;

}

STATIC
VOID
DeleteNode (
  VOID
  )
/*++

Routine Description:

  Delete outdated string info. (The Usage of PERC_FLAG
  ensures a clean deletion)

Arguments: (VOID)

Returns: (VOID)

--*/
{
  NODE  NodeQ;
  NODE  NodeR;
  NODE  NodeS;
  NODE  NodeT;
  NODE  NodeU;

  if (mParent[mPos] == NIL) {
    return ;
  }

  NodeR         = mPrev[mPos];
  NodeS         = mNext[mPos];
  mNext[NodeR]  = NodeS;
  mPrev[NodeS]  = NodeR;
  NodeR         = mParent[mPos];
  mParent[mPos] = NIL;
  if (NodeR >= WNDSIZ) {
    return ;
  }

  mChildCount[NodeR]--;
  if (mChildCount[NodeR] > 1) {
    return ;
  }

  NodeT = (NODE) (mPosition[NodeR] & (UINT32)~PERC_FLAG);
  if (NodeT >= mPos) {
    NodeT -= WNDSIZ;
  }

  NodeS = NodeT;
  NodeQ = mParent[NodeR];
  NodeU = mPosition[NodeQ];
  while (NodeU & (UINT32) PERC_FLAG) {
    NodeU &= (UINT32)~PERC_FLAG;
    if (NodeU >= mPos) {
      NodeU -= WNDSIZ;
    }

    if (NodeU > NodeS) {
      NodeS = NodeU;
    }

    mPosition[NodeQ]  = (NODE) (NodeS | WNDSIZ);
    NodeQ             = mParent[NodeQ];
    NodeU             = mPosition[NodeQ];
  }

  if (NodeQ < WNDSIZ) {
    if (NodeU >= mPos) {
      NodeU -= WNDSIZ;
    }

    if (NodeU > NodeS) {
      NodeS = NodeU;
    }

    mPosition[NodeQ] = (NODE) (NodeS | WNDSIZ | (UINT32) PERC_FLAG);
  }

  NodeS           = Child (NodeR, mText[NodeT + mLevel[NodeR]]);
  NodeT           = mPrev[NodeS];
  NodeU           = mNext[NodeS];
  mNext[NodeT]    = NodeU;
  mPrev[NodeU]    = NodeT;
  NodeT           = mPrev[NodeR];
  mNext[NodeT]    = NodeS;
  mPrev[NodeS]    = NodeT;
  NodeT           = mNext[NodeR];
  mPrev[NodeT]    = NodeS;
  mNext[NodeS]    = NodeT;
  mParent[NodeS]  = mParent[NodeR];
  mParent[NodeR]  = NIL;
  mNext[NodeR]    = mAvail;
  mAvail          = NodeR;
}

STATIC
VOID
GetNextMatch (
  VOID
  )
/*++

Routine Description:

  Advance the current position (read in new data if needed).
  Delete outdated string info. Find a match string for current position.

Arguments: (VOID)

Returns: (VOID)

--*/
{
  INT32 Number;

  mRemainder--;
  mPos++;
  if (mPos == WNDSIZ * 2) {
    memmove (&mText[0], &mText[WNDSIZ], WNDSIZ + MAXMATCH);
    Number = FreadCrc (&mText[WNDSIZ + MAXMATCH], WNDSIZ);
    mRemainder += Number;
    mPos = WNDSIZ;
  }

  DeleteNode ();
  InsertNode ();
}

STATIC
EFI_STATUS
Encode (
  VOID
  )
/*++

Routine Description:

  The main controlling routine for compression process.

Arguments: (VOID)

Returns:

  EFI_SUCCESS           - The compression is successful
  EFI_OUT_0F_RESOURCES  - Not enough memory for compression process

--*/
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
    GetNextMatch ();
    if (mMatchLen > mRemainder) {
      mMatchLen = mRemainder;
    }

    if (mMatchLen > LastMatchLen || LastMatchLen < THRESHOLD) {
      //
      // Not enough benefits are gained by outputting a pointer,
      // so just output the original character
      //
      Output (mText[mPos - 1], 0);

    } else {

      if (LastMatchLen == THRESHOLD) {
        if (((mPos - LastMatchPos - 2) & (WNDSIZ - 1)) > (1U << 11)) {
          Output (mText[mPos - 1], 0);
          continue;
        }
      }
      //
      // Outputting a pointer is beneficial enough, do it.
      //
      Output (
        LastMatchLen + (UINT8_MAX + 1 - THRESHOLD),
        (mPos - LastMatchPos - 2) & (WNDSIZ - 1)
        );
      LastMatchLen--;
      while (LastMatchLen > 0) {
        GetNextMatch ();
        LastMatchLen--;
      }

      if (mMatchLen > mRemainder) {
        mMatchLen = mRemainder;
      }
    }
  }

  HufEncodeEnd ();
  FreeMemory ();
  return EFI_SUCCESS;
}

STATIC
VOID
CountTFreq (
  VOID
  )
/*++

Routine Description:

  Count the frequencies for the Extra Set

Arguments: (VOID)

Returns: (VOID)

--*/
{
  INT32 Index;
  INT32 Index3;
  INT32 Number;
  INT32 Count;

  for (Index = 0; Index < NT; Index++) {
    mTFreq[Index] = 0;
  }

  Number = NC;
  while (Number > 0 && mCLen[Number - 1] == 0) {
    Number--;
  }

  Index = 0;
  while (Index < Number) {
    Index3 = mCLen[Index++];
    if (Index3 == 0) {
      Count = 1;
      while (Index < Number && mCLen[Index] == 0) {
        Index++;
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
      mTFreq[Index3 + 2]++;
    }
  }
}

STATIC
VOID
WritePTLen (
  IN INT32 Number,
  IN INT32 nbit,
  IN INT32 Special
  )
/*++

Routine Description:

  Outputs the code length array for the Extra Set or the Position Set.

Arguments:

  Number       - the number of symbols
  nbit    - the number of bits needed to represent 'n'
  Special - the special symbol that needs to be take care of

Returns: (VOID)

--*/
{
  INT32 Index;
  INT32 Index3;

  while (Number > 0 && mPTLen[Number - 1] == 0) {
    Number--;
  }

  PutBits (nbit, Number);
  Index = 0;
  while (Index < Number) {
    Index3 = mPTLen[Index++];
    if (Index3 <= 6) {
      PutBits (3, Index3);
    } else {
      PutBits (Index3 - 3, (1U << (Index3 - 3)) - 2);
    }

    if (Index == Special) {
      while (Index < 6 && mPTLen[Index] == 0) {
        Index++;
      }

      PutBits (2, (Index - 3) & 3);
    }
  }
}

STATIC
VOID
WriteCLen (
  VOID
  )
/*++

Routine Description:

  Outputs the code length array for Char&Length Set

Arguments: (VOID)

Returns: (VOID)

--*/
{
  INT32 Index;
  INT32 Index3;
  INT32 Number;
  INT32 Count;

  Number = NC;
  while (Number > 0 && mCLen[Number - 1] == 0) {
    Number--;
  }

  PutBits (CBIT, Number);
  Index = 0;
  while (Index < Number) {
    Index3 = mCLen[Index++];
    if (Index3 == 0) {
      Count = 1;
      while (Index < Number && mCLen[Index] == 0) {
        Index++;
        Count++;
      }

      if (Count <= 2) {
        for (Index3 = 0; Index3 < Count; Index3++) {
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
      PutBits (mPTLen[Index3 + 2], mPTCode[Index3 + 2]);
    }
  }
}

STATIC
VOID
EncodeC (
  IN INT32 Value
  )
{
  PutBits (mCLen[Value], mCCode[Value]);
}

STATIC
VOID
EncodeP (
  IN UINT32 Value
  )
{
  UINT32  Index;
  UINT32  NodeQ;

  Index = 0;
  NodeQ = Value;
  while (NodeQ) {
    NodeQ >>= 1;
    Index++;
  }

  PutBits (mPTLen[Index], mPTCode[Index]);
  if (Index > 1) {
    PutBits (Index - 1, Value & (0xFFFFFFFFU >> (32 - Index + 1)));
  }
}

STATIC
VOID
SendBlock (
  VOID
  )
/*++

Routine Description:

  Huffman code the block and output it.

Arguments:
  (VOID)

Returns:
  (VOID)

--*/
{
  UINT32  Index;
  UINT32  Index2;
  UINT32  Index3;
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
  for (Index = 0; Index < Size; Index++) {
    if (Index % UINT8_BIT == 0) {
      Flags = mBuf[Pos++];
    } else {
      Flags <<= 1;
    }

    if (Flags & (1U << (UINT8_BIT - 1))) {
      EncodeC (mBuf[Pos++] + (1U << UINT8_BIT));
      Index3 = mBuf[Pos++];
      for (Index2 = 0; Index2 < 3; Index2++) {
        Index3 <<= UINT8_BIT;
        Index3 += mBuf[Pos++];
      }

      EncodeP (Index3);
    } else {
      EncodeC (mBuf[Pos++]);
    }
  }

  for (Index = 0; Index < NC; Index++) {
    mCFreq[Index] = 0;
  }

  for (Index = 0; Index < NP; Index++) {
    mPFreq[Index] = 0;
  }
}

STATIC
VOID
Output (
  IN UINT32 CharC,
  IN UINT32 Pos
  )
/*++

Routine Description:

  Outputs an Original Character or a Pointer

Arguments:

  CharC     - The original character or the 'String Length' element of a Pointer
  Pos     - The 'Position' field of a Pointer

Returns: (VOID)

--*/
{
  STATIC UINT32 CPos;

  if ((mOutputMask >>= 1) == 0) {
    mOutputMask = 1U << (UINT8_BIT - 1);
    //
    // Check the buffer overflow per outputing UINT8_BIT symbols
    // which is an Original Character or a Pointer. The biggest
    // symbol is a Pointer which occupies 5 bytes.
    //
    if (mOutputPos >= mBufSiz - 5 * UINT8_BIT) {
      SendBlock ();
      mOutputPos = 0;
    }

    CPos        = mOutputPos++;
    mBuf[CPos]  = 0;
  }

  mBuf[mOutputPos++] = (UINT8) CharC;
  mCFreq[CharC]++;
  if (CharC >= (1U << UINT8_BIT)) {
    mBuf[CPos] |= mOutputMask;
    mBuf[mOutputPos++]  = (UINT8) (Pos >> 24);
    mBuf[mOutputPos++]  = (UINT8) (Pos >> 16);
    mBuf[mOutputPos++]  = (UINT8) (Pos >> (UINT8_BIT));
    mBuf[mOutputPos++]  = (UINT8) Pos;
    CharC               = 0;
    while (Pos) {
      Pos >>= 1;
      CharC++;
    }

    mPFreq[CharC]++;
  }
}

STATIC
VOID
HufEncodeStart (
  VOID
  )
{
  INT32 Index;

  for (Index = 0; Index < NC; Index++) {
    mCFreq[Index] = 0;
  }

  for (Index = 0; Index < NP; Index++) {
    mPFreq[Index] = 0;
  }

  mOutputPos = mOutputMask = 0;
  InitPutBits ();
  return ;
}

STATIC
VOID
HufEncodeEnd (
  VOID
  )
{
  SendBlock ();

  //
  // Flush remaining bits
  //
  PutBits (UINT8_BIT - 1, 0);

  return ;
}

STATIC
VOID
MakeCrcTable (
  VOID
  )
{
  UINT32  Index;
  UINT32  Index2;
  UINT32  Temp;

  for (Index = 0; Index <= UINT8_MAX; Index++) {
    Temp = Index;
    for (Index2 = 0; Index2 < UINT8_BIT; Index2++) {
      if (Temp & 1) {
        Temp = (Temp >> 1) ^ CRCPOLY;
      } else {
        Temp >>= 1;
      }
    }

    mCrcTable[Index] = (UINT16) Temp;
  }
}

STATIC
VOID
PutBits (
  IN INT32  Number,
  IN UINT32 Value
  )
/*++

Routine Description:

  Outputs rightmost n bits of x

Arguments:

  Number   - the rightmost n bits of the data is used
  x   - the data

Returns: (VOID)

--*/
{
  UINT8 Temp;

  while (Number >= mBitCount) {
    //
    // Number -= mBitCount should never equal to 32
    //
    Temp = (UINT8) (mSubBitBuf | (Value >> (Number -= mBitCount)));

    if (mDst < mDstUpperLimit) {
      *mDst++ = Temp;
    }

    mCompSize++;
    mSubBitBuf  = 0;
    mBitCount   = UINT8_BIT;
  }

  mSubBitBuf |= Value << (mBitCount -= Number);
}

STATIC
INT32
FreadCrc (
  OUT UINT8 *Pointer,
  IN  INT32 Number
  )
/*++

Routine Description:

  Read in source data

Arguments:

  Pointer   - the buffer to hold the data
  Number   - number of bytes to read

Returns:

  number of bytes actually read

--*/
{
  INT32 Index;

  for (Index = 0; mSrc < mSrcUpperLimit && Index < Number; Index++) {
    *Pointer++ = *mSrc++;
  }

  Number = Index;

  Pointer -= Number;
  mOrigSize += Number;

  Index--;
  while (Index >= 0) {
    UPDATE_CRC (*Pointer++);
    Index--;
  }

  return Number;
}

STATIC
VOID
InitPutBits (
  VOID
  )
{
  mBitCount   = UINT8_BIT;
  mSubBitBuf  = 0;
}

STATIC
VOID
CountLen (
  IN INT32 Index
  )
/*++

Routine Description:

  Count the number of each code length for a Huffman tree.

Arguments:

  Index   - the top node

Returns: (VOID)

--*/
{
  STATIC INT32  Depth = 0;

  if (Index < mN) {
    mLenCnt[(Depth < 16) ? Depth : 16]++;
  } else {
    Depth++;
    CountLen (mLeft[Index]);
    CountLen (mRight[Index]);
    Depth--;
  }
}

STATIC
VOID
MakeLen (
  IN INT32 Root
  )
/*++

Routine Description:

  Create code length array for a Huffman tree

Arguments:

  Root   - the root of the tree

Returns:

  VOID

--*/
{
  INT32   Index;
  INT32   Index3;
  UINT32  Cum;

  for (Index = 0; Index <= 16; Index++) {
    mLenCnt[Index] = 0;
  }

  CountLen (Root);

  //
  // Adjust the length count array so that
  // no code will be generated longer than its designated length
  //
  Cum = 0;
  for (Index = 16; Index > 0; Index--) {
    Cum += mLenCnt[Index] << (16 - Index);
  }

  while (Cum != (1U << 16)) {
    mLenCnt[16]--;
    for (Index = 15; Index > 0; Index--) {
      if (mLenCnt[Index] != 0) {
        mLenCnt[Index]--;
        mLenCnt[Index + 1] += 2;
        break;
      }
    }

    Cum--;
  }

  for (Index = 16; Index > 0; Index--) {
    Index3 = mLenCnt[Index];
    Index3--;
    while (Index3 >= 0) {
      mLen[*mSortPtr++] = (UINT8) Index;
      Index3--;
    }
  }
}

STATIC
VOID
DownHeap (
  IN INT32 Index
  )
{
  INT32 Index2;
  INT32 Index3;

  //
  // priority queue: send Index-th entry down heap
  //
  Index3  = mHeap[Index];
  Index2  = 2 * Index;
  while (Index2 <= mHeapSize) {
    if (Index2 < mHeapSize && mFreq[mHeap[Index2]] > mFreq[mHeap[Index2 + 1]]) {
      Index2++;
    }

    if (mFreq[Index3] <= mFreq[mHeap[Index2]]) {
      break;
    }

    mHeap[Index]  = mHeap[Index2];
    Index         = Index2;
    Index2        = 2 * Index;
  }

  mHeap[Index] = (INT16) Index3;
}

STATIC
VOID
MakeCode (
  IN  INT32       Number,
  IN  UINT8 Len[  ],
  OUT UINT16 Code[]
  )
/*++

Routine Description:

  Assign code to each symbol based on the code length array

Arguments:

  Number     - number of symbols
  Len   - the code length array
  Code  - stores codes for each symbol

Returns: (VOID)

--*/
{
  INT32   Index;
  UINT16  Start[18];

  Start[1] = 0;
  for (Index = 1; Index <= 16; Index++) {
    Start[Index + 1] = (UINT16) ((Start[Index] + mLenCnt[Index]) << 1);
  }

  for (Index = 0; Index < Number; Index++) {
    Code[Index] = Start[Len[Index]]++;
  }
}

STATIC
INT32
MakeTree (
  IN  INT32            NParm,
  IN  UINT16  FreqParm[],
  OUT UINT8   LenParm[ ],
  OUT UINT16  CodeParm[]
  )
/*++

Routine Description:

  Generates Huffman codes given a frequency distribution of symbols

Arguments:

  NParm    - number of symbols
  FreqParm - frequency of each symbol
  LenParm  - code length for each symbol
  CodeParm - code for each symbol

Returns:

  Root of the Huffman tree.

--*/
{
  INT32 Index;
  INT32 Index2;
  INT32 Index3;
  INT32 Avail;

  //
  // make tree, calculate len[], return root
  //
  mN        = NParm;
  mFreq     = FreqParm;
  mLen      = LenParm;
  Avail     = mN;
  mHeapSize = 0;
  mHeap[1]  = 0;
  for (Index = 0; Index < mN; Index++) {
    mLen[Index] = 0;
    if (mFreq[Index]) {
      mHeapSize++;
      mHeap[mHeapSize] = (INT16) Index;
    }
  }

  if (mHeapSize < 2) {
    CodeParm[mHeap[1]] = 0;
    return mHeap[1];
  }

  for (Index = mHeapSize / 2; Index >= 1; Index--) {
    //
    // make priority queue
    //
    DownHeap (Index);
  }

  mSortPtr = CodeParm;
  do {
    Index = mHeap[1];
    if (Index < mN) {
      *mSortPtr++ = (UINT16) Index;
    }

    mHeap[1] = mHeap[mHeapSize--];
    DownHeap (1);
    Index2 = mHeap[1];
    if (Index2 < mN) {
      *mSortPtr++ = (UINT16) Index2;
    }

    Index3        = Avail++;
    mFreq[Index3] = (UINT16) (mFreq[Index] + mFreq[Index2]);
    mHeap[1]      = (INT16) Index3;
    DownHeap (1);
    mLeft[Index3]   = (UINT16) Index;
    mRight[Index3]  = (UINT16) Index2;
  } while (mHeapSize > 1);

  mSortPtr = CodeParm;
  MakeLen (Index3);
  MakeCode (NParm, LenParm, CodeParm);

  //
  // return root
  //
  return Index3;
}

EFI_STATUS
GetFileContents (
  IN char    *InputFileName,
  OUT UINT8   *FileBuffer,
  OUT UINT32  *BufferLength
  )
/*++

Routine Description:

  Get the contents of file specified in InputFileName
  into FileBuffer.

Arguments:

  InputFileName  - Name of the input file.

  FileBuffer     - Output buffer to contain data

  BufferLength   - Actual length of the data

Returns:

  EFI_SUCCESS on successful return
  EFI_ABORTED if unable to open input file.

--*/
{
  UINTN   Size;
  UINTN   FileSize;
  FILE    *InputFile;

  Size = 0;
  //
  // Copy the file contents to the output buffer.
  //
  InputFile = fopen (LongFilePath (InputFileName), "rb");
    if (InputFile == NULL) {
      Error (NULL, 0, 0001, "Error opening file: %s", InputFileName);
      return EFI_ABORTED;
    }

  fseek (InputFile, 0, SEEK_END);
  FileSize = ftell (InputFile);
  fseek (InputFile, 0, SEEK_SET);
    //
    // Now read the contents of the file into the buffer
    //
    if (FileSize > 0 && FileBuffer != NULL) {
      if (fread (FileBuffer, FileSize, 1, InputFile) != 1) {
        Error (NULL, 0, 0004, "Error reading contents of input file: %s", InputFileName);
        fclose (InputFile);
        return EFI_ABORTED;
      }
    }

  fclose (InputFile);
  Size += (UINTN) FileSize;
  *BufferLength = Size;

  if (FileBuffer != NULL) {
    return EFI_SUCCESS;
  } else {
    return EFI_BUFFER_TOO_SMALL;
  }
}

VOID
Version (
  VOID
  )
/*++

Routine Description:

  Displays the standard utility information to SDTOUT

Arguments:

  None

Returns:

  None

--*/
{
  fprintf (stdout, "%s Version %d.%d %s \n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
}

VOID
Usage (
  VOID
  )
/*++

Routine Description:

  Displays the utility usage syntax to STDOUT

Arguments:

  None

Returns:

  None

--*/
{
  //
  // Summary usage
  //
  fprintf (stdout, "Usage: %s -e|-d [options] <input_file>\n\n", UTILITY_NAME);

  //
  // Copyright declaration
  //
  fprintf (stdout, "Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.\n\n");

  //
  // Details Option
  //
  fprintf (stdout, "Options:\n");
  fprintf (stdout, "  --uefi\n\
            Enable UefiCompress, use TianoCompress when without this option\n");
  fprintf (stdout, "  -o FileName, --output FileName\n\
            File will be created to store the output content.\n");
  fprintf (stdout, "  -v, --verbose\n\
           Turn on verbose output with informational messages.\n");
  fprintf (stdout, "  -q, --quiet\n\
           Disable all messages except key message and fatal error\n");
  fprintf (stdout, "  --debug [0-9]\n\
           Enable debug messages, at input debug level.\n");
  fprintf (stdout, "  --version\n\
           Show program's version number and exit.\n");
  fprintf (stdout, "  -h, --help\n\
           Show this help message and exit.\n");
}


int
main (
  int  argc,
  char *argv[]
  )
/*++

Routine Description:

  Main

Arguments:

  command line parameters

Returns:

  EFI_SUCCESS    Section header successfully generated and section concatenated.
  EFI_ABORTED    Could not generate the section
  EFI_OUT_OF_RESOURCES  No resource to complete the operation.

--*/
{
  FILE       *OutputFile;
  char       *OutputFileName;
  char       *InputFileName;
  FILE       *InputFile;
  EFI_STATUS Status;
  UINT8      *FileBuffer;
  UINT8      *OutBuffer;
  UINT32     InputLength;
  UINT32     DstSize;
  SCRATCH_DATA      *Scratch;
  UINT8      *Src;
  UINT32     OrigSize;
  UINT32     CompSize;

  SetUtilityName(UTILITY_NAME);

  FileBuffer = NULL;
  Src = NULL;
  OutBuffer = NULL;
  Scratch   = NULL;
  OrigSize = 0;
  CompSize = 0;
  InputLength = 0;
  InputFileName = NULL;
  OutputFileName = NULL;
  InputFile = NULL;
  OutputFile = NULL;
  DstSize=0;
  DebugLevel = 0;
  DebugMode = FALSE;

  //
  // Verify the correct number of arguments
  //
  if (argc == 1) {
    Error (NULL, 0, 1001, "Missing options", "No input options specified.");
    Usage();
    return 0;
  }

  if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)) {
    Usage();
    return 0;
  }

  if ((strcmp(argv[1], "--version") == 0)) {
    Version();
    return 0;
  }

  argc--;
  argv++;
  if (strcmp(argv[0],"-e") == 0) {
    //
    // encode the input file
    //
    ENCODE = TRUE;
    argc--;
    argv++;
  } else if (strcmp(argv[0], "-d") == 0) {
    //
    // decode the input file
    //
    DECODE = TRUE;
    argc--;
    argv++;
  } else {
    //
    // Error command line
    //
    Error (NULL, 0, 1003, "Invalid option value", "the options specified are not recognized.");
    Usage();
    return 1;
  }

  while (argc > 0) {
    if ((strcmp(argv[0], "-v") == 0) || (stricmp(argv[0], "--verbose") == 0)) {
      VerboseMode = TRUE;
      argc--;
      argv++;
      continue;
    }

    if (stricmp(argv[0], "--uefi") == 0) {
      UEFIMODE = TRUE;
      argc--;
      argv++;
      continue;
    }

    if (stricmp (argv[0], "--debug") == 0) {
      argc-=2;
      argv++;
      Status = AsciiStringToUint64(argv[0], FALSE, &DebugLevel);
      if (DebugLevel > 9) {
        Error (NULL, 0 ,2000, "Invalid parameter", "Unrecognized argument %s", argv[0]);
        goto ERROR;
      }
      if (DebugLevel>=5 && DebugLevel <=9){
        DebugMode = TRUE;
      } else {
        DebugMode = FALSE;
      }
      argv++;
      continue;
    }

    if ((strcmp(argv[0], "-q") == 0) || (stricmp (argv[0], "--quiet") == 0)) {
      QuietMode = TRUE;
      argc--;
      argv++;
      continue;
    }

    if ((strcmp(argv[0], "-o") == 0) || (stricmp (argv[0], "--output") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Output File name is missing for -o option");
        goto ERROR;
      }
      OutputFileName = argv[1];
      argc -=2;
      argv +=2;
      continue;
    }

    if (argv[0][0]!='-') {
      InputFileName = argv[0];
      argc--;
      argv++;
      continue;
    }

    Error (NULL, 0, 1000, "Unknown option", argv[0]);
    goto ERROR;
  }

  if (InputFileName == NULL) {
    Error (NULL, 0, 1001, "Missing options", "No input files specified.");
    goto ERROR;
  }

//
// All Parameters has been parsed, now set the message print level
//
  if (QuietMode) {
    SetPrintLevel(40);
  } else if (VerboseMode) {
    SetPrintLevel(15);
  } else if (DebugMode) {
    SetPrintLevel(DebugLevel);
  }

  if (VerboseMode) {
    VerboseMsg("%s tool start.\n", UTILITY_NAME);
   }
  Scratch = (SCRATCH_DATA *)malloc(sizeof(SCRATCH_DATA));
  if (Scratch == NULL) {
    Error (NULL, 0, 4001, "Resource:", "Memory cannot be allocated!");
    goto ERROR;
  }

  InputFile = fopen (LongFilePath (InputFileName), "rb");
  if (InputFile == NULL) {
    Error (NULL, 0, 0001, "Error opening input file", InputFileName);
    goto ERROR;
  }

  Status = GetFileContents(
            InputFileName,
            FileBuffer,
            &InputLength);

  if (Status == EFI_BUFFER_TOO_SMALL) {
    FileBuffer = (UINT8 *) malloc (InputLength);
    if (FileBuffer == NULL) {
      Error (NULL, 0, 4001, "Resource:", "Memory cannot be allocated!");
      goto ERROR;
    }

    Status = GetFileContents (
              InputFileName,
              FileBuffer,
              &InputLength
              );
  }

  if (EFI_ERROR(Status)) {
    Error (NULL, 0, 0004, "Error getting contents of file: %s", InputFileName);
    goto ERROR;
  }

  if (OutputFileName == NULL) {
    OutputFileName = DEFAULT_OUTPUT_FILE;
  }
  OutputFile = fopen (LongFilePath (OutputFileName), "wb");
  if (OutputFile == NULL) {
    Error (NULL, 0, 0001, "Error opening output file for writing", OutputFileName);
    goto ERROR;
  }

  if (ENCODE) {
  //
  // First call TianoCompress to get DstSize
  //
  if (DebugMode) {
    DebugMsg(UTILITY_NAME, 0, DebugLevel, "Encoding", NULL);
  }
  if (UEFIMODE) {
    Status = EfiCompress ((UINT8 *)FileBuffer, InputLength, OutBuffer, &DstSize);
  } else {
    Status = TianoCompress ((UINT8 *)FileBuffer, InputLength, OutBuffer, &DstSize);
  }

  if (Status == EFI_BUFFER_TOO_SMALL) {
    OutBuffer = (UINT8 *) malloc (DstSize);
    if (OutBuffer == NULL) {
      Error (NULL, 0, 4001, "Resource:", "Memory cannot be allocated!");
      goto ERROR;
    }
  }

  if (UEFIMODE) {
    Status = EfiCompress ((UINT8 *)FileBuffer, InputLength, OutBuffer, &DstSize);
  } else {
    Status = TianoCompress ((UINT8 *)FileBuffer, InputLength, OutBuffer, &DstSize);
  }
  if (Status != EFI_SUCCESS) {
    Error (NULL, 0, 0007, "Error compressing file", NULL);
    goto ERROR;
  }

  if (OutBuffer == NULL) {
    Error (NULL, 0, 4001, "Resource:", "Memory cannot be allocated!");
    goto ERROR;
  }

  fwrite(OutBuffer,(size_t)DstSize, 1, OutputFile);
  fclose(OutputFile);
  fclose(InputFile);
  free(Scratch);
  free(FileBuffer);
  free(OutBuffer);

  if (DebugMode) {
    DebugMsg(UTILITY_NAME, 0, DebugLevel, "Encoding Successful!\n", NULL);
  }
  if (VerboseMode) {
    VerboseMsg("Encoding successful\n");
  }
  return 0;
  }
  else if (DECODE) {
  if (DebugMode) {
    DebugMsg(UTILITY_NAME, 0, DebugLevel, "Decoding\n", NULL);
  }

  if (UEFIMODE) {
    Status = Extract((VOID *)FileBuffer, InputLength, (VOID *)&OutBuffer, &DstSize, 1);
    if (Status != EFI_SUCCESS) {
      goto ERROR;
    }
    fwrite(OutBuffer, (size_t)(DstSize), 1, OutputFile);
  } else {
    if (InputLength < 8){
      Error (NULL, 0, 3000, "Invalid", "The input file %s is too small.", InputFileName);
      goto ERROR;
    }
    //
    // Get Compressed file original size
    //
    Src     = (UINT8 *)FileBuffer;
    OrigSize  = Src[4] + (Src[5] << 8) + (Src[6] << 16) + (Src[7] << 24);
    CompSize  = Src[0] + (Src[1] << 8) + (Src[2] <<16) + (Src[3] <<24);

    //
    // Allocate OutputBuffer
    //
    if (InputLength < CompSize + 8 || (CompSize + 8) < 8) {
      Error (NULL, 0, 3000, "Invalid", "The input file %s data is invalid.", InputFileName);
      goto ERROR;
    }
    OutBuffer = (UINT8 *)malloc(OrigSize);
    if (OutBuffer == NULL) {
      Error (NULL, 0, 4001, "Resource:", "Memory cannot be allocated!");
      goto ERROR;
     }

    Status = TDecompress((VOID *)FileBuffer, (VOID *)OutBuffer, (VOID *)Scratch, 2);
    if (Status != EFI_SUCCESS) {
      goto ERROR;
    }
    fwrite(OutBuffer, (size_t)(Scratch->mOrigSize), 1, OutputFile);
  }
  fclose(OutputFile);
  fclose(InputFile);
  if (Scratch != NULL) {
    free(Scratch);
  }
  if (FileBuffer != NULL) {
    free(FileBuffer);
  }
  if (OutBuffer != NULL) {
    free(OutBuffer);
  }

  if (DebugMode) {
    DebugMsg(UTILITY_NAME, 0, DebugLevel, "Encoding successful!\n", NULL);
  }

  if (VerboseMode) {
    VerboseMsg("Decoding successful\n");
  }
  return 0;
  }

ERROR:
  if (DebugMode) {
    if (ENCODE) {
      DebugMsg(UTILITY_NAME, 0, DebugLevel, "Encoding Error\n", NULL);
    } else if (DECODE) {
      DebugMsg(UTILITY_NAME, 0, DebugLevel, "Decoding Error\n", NULL);
    }
  }
  if (OutputFile != NULL) {
    fclose(OutputFile);
  }
  if (InputFile != NULL) {
    fclose (InputFile);
  }
  if (Scratch != NULL) {
    free(Scratch);
  }
  if (FileBuffer != NULL) {
    free(FileBuffer);
  }
  if (OutBuffer != NULL) {
    free(OutBuffer);
  }

  if (VerboseMode) {
    VerboseMsg("%s tool done with return code is 0x%x.\n", UTILITY_NAME, GetUtilityStatus ());
  }
  return GetUtilityStatus ();
}

VOID
FillBuf (
  IN  SCRATCH_DATA  *Sd,
  IN  UINT16        NumOfBits
  )
/*++

Routine Description:

  Shift mBitBuf NumOfBits left. Read in NumOfBits of bits from source.

Arguments:

  Sd        - The global scratch data
  NumOfBits  - The number of bits to shift and read.

Returns: (VOID)

--*/
{
  Sd->mBitBuf = (UINT32) (((UINT64)Sd->mBitBuf) << NumOfBits);

  while (NumOfBits > Sd->mBitCount) {

    Sd->mBitBuf |= (UINT32) (((UINT64)Sd->mSubBitBuf) << (NumOfBits = (UINT16) (NumOfBits - Sd->mBitCount)));

    if (Sd->mCompSize > 0) {
      //
      // Get 1 byte into SubBitBuf
      //
      Sd->mCompSize--;
      Sd->mSubBitBuf  = 0;
      Sd->mSubBitBuf  = Sd->mSrcBase[Sd->mInBuf++];
      Sd->mBitCount   = 8;

    } else {
      //
      // No more bits from the source, just pad zero bit.
      //
      Sd->mSubBitBuf  = 0;
      Sd->mBitCount   = 8;

    }
  }

  Sd->mBitCount = (UINT16) (Sd->mBitCount - NumOfBits);
  Sd->mBitBuf |= Sd->mSubBitBuf >> Sd->mBitCount;
}

UINT32
GetBits (
  IN  SCRATCH_DATA  *Sd,
  IN  UINT16        NumOfBits
  )
/*++

Routine Description:

  Get NumOfBits of bits out from mBitBuf. Fill mBitBuf with subsequent
  NumOfBits of bits from source. Returns NumOfBits of bits that are
  popped out.

Arguments:

  Sd            - The global scratch data.
  NumOfBits     - The number of bits to pop and read.

Returns:

  The bits that are popped out.

--*/
{
  UINT32  OutBits;

  OutBits = (UINT32) (Sd->mBitBuf >> (BITBUFSIZ - NumOfBits));

  FillBuf (Sd, NumOfBits);

  return OutBits;
}

UINT16
MakeTable (
  IN  SCRATCH_DATA  *Sd,
  IN  UINT16        NumOfChar,
  IN  UINT8         *BitLen,
  IN  UINT16        TableBits,
  OUT UINT16        *Table
  )
/*++

Routine Description:

  Creates Huffman Code mapping table according to code length array.

Arguments:

  Sd        - The global scratch data
  NumOfChar - Number of symbols in the symbol set
  BitLen    - Code length array
  TableBits - The width of the mapping table
  Table     - The table

Returns:

  0         - OK.
  BAD_TABLE - The table is corrupted.

--*/
{
  UINT16  Count[17];
  UINT16  Weight[17];
  UINT16  Start[18];
  UINT16  *Pointer;
  UINT16  Index3;
  UINT16  Index;
  UINT16  Len;
  UINT16  Char;
  UINT16  JuBits;
  UINT16  Avail;
  UINT16  NextCode;
  UINT16  Mask;
  UINT16  WordOfStart;
  UINT16  WordOfCount;
  UINT16  MaxTableLength;

  for (Index = 0; Index <= 16; Index++) {
    Count[Index] = 0;
  }

  for (Index = 0; Index < NumOfChar; Index++) {
    if (BitLen[Index] > 16) {
      return (UINT16) BAD_TABLE;
    }
    Count[BitLen[Index]]++;
  }

  Start[0] = 0;
  Start[1] = 0;

  for (Index = 1; Index <= 16; Index++) {
    WordOfStart = Start[Index];
    WordOfCount = Count[Index];
    Start[Index + 1] = (UINT16) (WordOfStart + (WordOfCount << (16 - Index)));
  }

  if (Start[17] != 0) {
    //
    //(1U << 16)
    //
    return (UINT16) BAD_TABLE;
  }

  JuBits = (UINT16) (16 - TableBits);

  Weight[0] = 0;
  for (Index = 1; Index <= TableBits; Index++) {
    Start[Index] >>= JuBits;
    Weight[Index] = (UINT16) (1U << (TableBits - Index));
  }

  while (Index <= 16) {
    Weight[Index] = (UINT16) (1U << (16 - Index));
    Index++;
  }

  Index = (UINT16) (Start[TableBits + 1] >> JuBits);

  if (Index != 0) {
    Index3 = (UINT16) (1U << TableBits);
    while (Index != Index3) {
      Table[Index++] = 0;
    }
  }

  Avail = NumOfChar;
  Mask  = (UINT16) (1U << (15 - TableBits));
  MaxTableLength = (UINT16) (1U << TableBits);

  for (Char = 0; Char < NumOfChar; Char++) {

    Len = BitLen[Char];
    if (Len == 0 || Len >= 17) {
      continue;
    }

    NextCode = (UINT16) (Start[Len] + Weight[Len]);

    if (Len <= TableBits) {

      if (Start[Len] >= NextCode || NextCode > MaxTableLength){
        return (UINT16) BAD_TABLE;
      }

      for (Index = Start[Len]; Index < NextCode; Index++) {
        Table[Index] = Char;
      }

    } else {

      Index3  = Start[Len];
      Pointer = &Table[Index3 >> JuBits];
      Index   = (UINT16) (Len - TableBits);

      while (Index != 0) {
        if (*Pointer == 0) {
          Sd->mRight[Avail]                     = Sd->mLeft[Avail] = 0;
          *Pointer = Avail++;
        }

        if (Index3 & Mask) {
          Pointer = &Sd->mRight[*Pointer];
        } else {
          Pointer = &Sd->mLeft[*Pointer];
        }

        Index3 <<= 1;
        Index--;
      }

      *Pointer = Char;

    }

    Start[Len] = NextCode;
  }
  //
  // Succeeds
  //
  return 0;
}

UINT32
DecodeP (
  IN  SCRATCH_DATA  *Sd
  )
/*++

Routine Description:

  Decodes a position value.

Arguments:

  Sd      - the global scratch data

Returns:

  The position value decoded.

--*/
{
  UINT16  Val;
  UINT32  Mask;
  UINT32  Pos;

  Val = Sd->mPTTable[Sd->mBitBuf >> (BITBUFSIZ - 8)];

  if (Val >= MAXNP) {
    Mask = 1U << (BITBUFSIZ - 1 - 8);

    do {

      if (Sd->mBitBuf & Mask) {
        Val = Sd->mRight[Val];
      } else {
        Val = Sd->mLeft[Val];
      }

      Mask >>= 1;
    } while (Val >= MAXNP);
  }
  //
  // Advance what we have read
  //
  FillBuf (Sd, Sd->mPTLen[Val]);

  Pos = Val;
  if (Val > 1) {
    Pos = (UINT32) ((1U << (Val - 1)) + GetBits (Sd, (UINT16) (Val - 1)));
  }

  return Pos;
}

UINT16
ReadPTLen (
  IN  SCRATCH_DATA  *Sd,
  IN  UINT16        nn,
  IN  UINT16        nbit,
  IN  UINT16        Special
  )
/*++

Routine Description:

  Reads code lengths for the Extra Set or the Position Set

Arguments:

  Sd        - The global scratch data
  nn        - Number of symbols
  nbit      - Number of bits needed to represent nn
  Special   - The special symbol that needs to be taken care of

Returns:

  0         - OK.
  BAD_TABLE - Table is corrupted.

--*/
{
  UINT16  Number;
  UINT16  CharC;
  volatile UINT16  Index;
  UINT32  Mask;

  assert (nn <= NPT);

  Number = (UINT16) GetBits (Sd, nbit);

  if (Number == 0) {
    CharC = (UINT16) GetBits (Sd, nbit);

    for (Index = 0; Index < 256; Index++) {
      Sd->mPTTable[Index] = CharC;
    }

    for (Index = 0; Index < nn; Index++) {
      Sd->mPTLen[Index] = 0;
    }

    return 0;
  }

  Index = 0;

  while (Index < Number) {

    CharC = (UINT16) (Sd->mBitBuf >> (BITBUFSIZ - 3));

    if (CharC == 7) {
      Mask = 1U << (BITBUFSIZ - 1 - 3);
      while (Mask & Sd->mBitBuf) {
        Mask >>= 1;
        CharC += 1;
      }
    }

    FillBuf (Sd, (UINT16) ((CharC < 7) ? 3 : CharC - 3));

    Sd->mPTLen[Index++] = (UINT8) CharC;

    if (Index == Special) {
      CharC = (UINT16) GetBits (Sd, 2);
      while ((INT16) (--CharC) >= 0) {
        Sd->mPTLen[Index++] = 0;
      }
    }
  }

  while (Index < nn) {
    Sd->mPTLen[Index++] = 0;
  }

  return MakeTable (Sd, nn, Sd->mPTLen, 8, Sd->mPTTable);
}

VOID
ReadCLen (
  SCRATCH_DATA  *Sd
  )
/*++

Routine Description:

  Reads code lengths for Char&Len Set.

Arguments:

  Sd    - the global scratch data

Returns: (VOID)

--*/
{
  UINT16  Number;
  UINT16  CharC;
  volatile UINT16  Index;
  UINT32  Mask;

  Number = (UINT16) GetBits (Sd, CBIT);

  if (Number == 0) {
    CharC = (UINT16) GetBits (Sd, CBIT);

    for (Index = 0; Index < NC; Index++) {
      Sd->mCLen[Index] = 0;
    }

    for (Index = 0; Index < 4096; Index++) {
      Sd->mCTable[Index] = CharC;
    }

    return ;
  }

  Index = 0;
  while (Index < Number) {

    CharC = Sd->mPTTable[Sd->mBitBuf >> (BITBUFSIZ - 8)];
    if (CharC >= NT) {
      Mask = 1U << (BITBUFSIZ - 1 - 8);

      do {

        if (Mask & Sd->mBitBuf) {
          CharC = Sd->mRight[CharC];
        } else {
          CharC = Sd->mLeft[CharC];
        }

        Mask >>= 1;

      } while (CharC >= NT);
    }
    //
    // Advance what we have read
    //
    FillBuf (Sd, Sd->mPTLen[CharC]);

    if (CharC <= 2) {

      if (CharC == 0) {
        CharC = 1;
      } else if (CharC == 1) {
        CharC = (UINT16) (GetBits (Sd, 4) + 3);
      } else if (CharC == 2) {
        CharC = (UINT16) (GetBits (Sd, CBIT) + 20);
      }

      while ((INT16) (--CharC) >= 0) {
        Sd->mCLen[Index++] = 0;
      }

    } else {

      Sd->mCLen[Index++] = (UINT8) (CharC - 2);

    }
  }

  while (Index < NC) {
    Sd->mCLen[Index++] = 0;
  }

  MakeTable (Sd, NC, Sd->mCLen, 12, Sd->mCTable);

  return ;
}

UINT16
DecodeC (
  SCRATCH_DATA  *Sd
  )
/*++

Routine Description:

  Decode a character/length value.

Arguments:

  Sd    - The global scratch data.

Returns:

  The value decoded.

--*/
{
  UINT16  Index2;
  UINT32  Mask;

  if (Sd->mBlockSize == 0) {
    //
    // Starting a new block
    //
    Sd->mBlockSize    = (UINT16) GetBits (Sd, 16);
    Sd->mBadTableFlag = ReadPTLen (Sd, NT, TBIT, 3);
    if (Sd->mBadTableFlag != 0) {
      return 0;
    }

    ReadCLen (Sd);

    Sd->mBadTableFlag = ReadPTLen (Sd, MAXNP, Sd->mPBit, (UINT16) (-1));
    if (Sd->mBadTableFlag != 0) {
      return 0;
    }
  }

  Sd->mBlockSize--;
  Index2 = Sd->mCTable[Sd->mBitBuf >> (BITBUFSIZ - 12)];

  if (Index2 >= NC) {
    Mask = 1U << (BITBUFSIZ - 1 - 12);

    do {
      if (Sd->mBitBuf & Mask) {
        Index2 = Sd->mRight[Index2];
      } else {
        Index2 = Sd->mLeft[Index2];
      }

      Mask >>= 1;
    } while (Index2 >= NC);
  }
  //
  // Advance what we have read
  //
  FillBuf (Sd, Sd->mCLen[Index2]);

  return Index2;
}

VOID
Decode (
  SCRATCH_DATA  *Sd
  )
/*++

Routine Description:

  Decode the source data and put the resulting data into the destination buffer.

Arguments:

  Sd            - The global scratch data

Returns: (VOID)

 --*/
{
  UINT16  BytesRemain;
  UINT32  DataIdx;
  UINT16  CharC;

  BytesRemain = (UINT16) (-1);

  DataIdx     = 0;

  for (;;) {
    CharC = DecodeC (Sd);
    if (Sd->mBadTableFlag != 0) {
      goto Done ;
    }

    if (CharC < 256) {
      //
      // Process an Original character
      //
      if (Sd->mOutBuf >= Sd->mOrigSize) {
        goto Done ;
      } else {
        Sd->mDstBase[Sd->mOutBuf++] = (UINT8) CharC;
      }

    } else {
      //
      // Process a Pointer
      //
      CharC       = (UINT16) (CharC - (UINT8_MAX + 1 - THRESHOLD));

      BytesRemain = CharC;

      DataIdx     = Sd->mOutBuf - DecodeP (Sd) - 1;

      BytesRemain--;

      while ((INT16) (BytesRemain) >= 0) {
        if (Sd->mOutBuf >= Sd->mOrigSize) {
          goto Done ;
        }
        if (DataIdx >= Sd->mOrigSize) {
          Sd->mBadTableFlag = (UINT16) BAD_TABLE;
          goto Done ;
        }
        Sd->mDstBase[Sd->mOutBuf++] = Sd->mDstBase[DataIdx++];

        BytesRemain--;
      }
      //
      // Once mOutBuf is fully filled, directly return
      //
      if (Sd->mOutBuf >= Sd->mOrigSize) {
        goto Done ;
      }
    }
  }

Done:
  return ;
}

RETURN_STATUS
EFIAPI
TDecompress (
  IN VOID  *Source,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch,
  IN UINT32      Version
  )
/*++

Routine Description:

  The internal implementation of Decompress().

Arguments:

  Source          - The source buffer containing the compressed data.
  Destination     - The destination buffer to store the decompressed data
  Scratch         - The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.
  Version         - 1 for EFI1.1 Decompress algorithm, 2 for Tiano Decompress algorithm

Returns:

  RETURN_SUCCESS           - Decompression is successful
  RETURN_INVALID_PARAMETER - The source data is corrupted

--*/
{
  volatile UINT32  Index;
  UINT32           CompSize;
  UINT32           OrigSize;
  SCRATCH_DATA     *Sd;
  CONST UINT8      *Src;
  UINT8            *Dst;

  //
  // Verify input is not NULL
  //
  assert(Source);
//  assert(Destination);
  assert(Scratch);

  Src     = (UINT8 *)Source;
  Dst     = (UINT8 *)Destination;

  Sd      = (SCRATCH_DATA *) Scratch;
  CompSize  = Src[0] + (Src[1] << 8) + (Src[2] << 16) + (Src[3] << 24);
  OrigSize  = Src[4] + (Src[5] << 8) + (Src[6] << 16) + (Src[7] << 24);

  //
  // If compressed file size is 0, return
  //
  if (OrigSize == 0) {
    return RETURN_SUCCESS;
  }

  Src = Src + 8;

  for (Index = 0; Index < sizeof (SCRATCH_DATA); Index++) {
    ((UINT8 *) Sd)[Index] = 0;
  }
  //
  // The length of the field 'Position Set Code Length Array Size' in Block Header.
  // For EFI 1.1 de/compression algorithm(Version 1), mPBit = 4
  // For Tiano de/compression algorithm(Version 2), mPBit = 5
  //
  switch (Version) {
    case 1 :
      Sd->mPBit = 4;
      break;
    case 2 :
      Sd->mPBit = 5;
      break;
    default:
      assert(FALSE);
  }
  Sd->mSrcBase  = (UINT8 *)Src;
  Sd->mDstBase  = Dst;
  Sd->mCompSize = CompSize;
  Sd->mOrigSize = OrigSize;

  //
  // Fill the first BITBUFSIZ bits
  //
  FillBuf (Sd, BITBUFSIZ);

  //
  // Decompress it
  //

  Decode (Sd);

  if (Sd->mBadTableFlag != 0) {
    //
    // Something wrong with the source
    //
    return RETURN_INVALID_PARAMETER;
  }

  return RETURN_SUCCESS;
}


