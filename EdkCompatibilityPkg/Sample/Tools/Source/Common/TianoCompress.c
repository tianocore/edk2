/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  TianoCompress.c

Abstract:

  Compression routine. The compression algorithm is a mixture of
  LZ77 and Huffman coding. LZ77 transforms the source data into a
  sequence of Original Characters and Pointers to repeated strings.
  This sequence is further divided into Blocks and Huffman codings
  are applied to each Block.

--*/

#include <string.h>
#include <stdlib.h>
#include "TianoCommon.h"
#include "Compress.h"

//
// Macro Definitions
//
typedef INT32 NODE;
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
#define NC    (UINT8_MAX + MAXMATCH + 2 - THRESHOLD)
#define CBIT  9
#define NP    (WNDBIT + 1)
#define PBIT  5
#define NT    (CODE_BIT + 3)
#define TBIT  5
#if NT > NP
#define NPT   NT
#else
#define NPT   NP
#endif
//
// Function Prototypes
//
STATIC
EFI_STATUS
Compress (
  IN      UINT8   *SrcBuffer,
  IN      UINT32  SrcSize,
  IN      UINT8   *DstBuffer,
  IN OUT  UINT32  *DstSize,
  IN      UINT8   Version
  );

STATIC
VOID
PutDword(
  IN UINT32 Data
  );

STATIC
EFI_STATUS
AllocateMemory (
  VOID
  );

STATIC
VOID
FreeMemory (
  VOID
  );

STATIC
VOID
InitSlide (
  VOID
  );

STATIC
NODE
Child (
  IN NODE   NodeQ,
  IN UINT8  CharC
  );

STATIC
VOID
MakeChild (
  IN NODE  NodeQ,
  IN UINT8 CharC,
  IN NODE  NodeR
  );

STATIC
VOID
Split (
  IN NODE Old
  );

STATIC
VOID
InsertNode (
  VOID
  );

STATIC
VOID
DeleteNode (
  VOID
  );

STATIC
VOID
GetNextMatch (
  VOID
  );

STATIC
EFI_STATUS
Encode (
  VOID
  );

STATIC
VOID
CountTFreq (
  VOID
  );

STATIC
VOID
WritePTLen (
  IN INT32 Number,
  IN INT32 nbit,
  IN INT32 Special
  );

STATIC
VOID
WriteCLen (
  VOID
  );

STATIC
VOID
EncodeC (
  IN INT32 Value
  );

STATIC
VOID
EncodeP (
  IN UINT32 Value
  );

STATIC
VOID
SendBlock (
  VOID
  );

STATIC
VOID
Output (
  IN UINT32 c,
  IN UINT32 p
  );

STATIC
VOID
HufEncodeStart (
  VOID
  );

STATIC
VOID
HufEncodeEnd (
  VOID
  );

STATIC
VOID
MakeCrcTable (
  VOID
  );

STATIC
VOID
PutBits (
  IN INT32  Number,
  IN UINT32 Value
  );

STATIC
INT32
FreadCrc (
  OUT UINT8 *Pointer,
  IN  INT32 Number
  );

STATIC
VOID
InitPutBits (
  VOID
  );

STATIC
VOID
CountLen (
  IN INT32 Index
  );

STATIC
VOID
MakeLen (
  IN INT32 Root
  );

STATIC
VOID
DownHeap (
  IN INT32 Index
  );

STATIC
VOID
MakeCode (
  IN  INT32       Number,
  IN  UINT8 Len[  ],
  OUT UINT16 Code[]
  );

STATIC
INT32
MakeTree (
  IN  INT32            NParm,
  IN  UINT16  FreqParm[],
  OUT UINT8   LenParm[ ],
  OUT UINT16  CodeParm[]
  );

//
//  Global Variables
//
STATIC UINT8  *mSrc, *mDst, *mSrcUpperLimit, *mDstUpperLimit;

STATIC UINT8  *mLevel, *mText, *mChildCount, *mBuf, mCLen[NC], mPTLen[NPT], *mLen;
STATIC INT16  mHeap[NC + 1];
STATIC INT32  mRemainder, mMatchLen, mBitCount, mHeapSize, mN;
STATIC UINT32 mBufSiz = 0, mOutputPos, mOutputMask, mSubBitBuf, mCrc;
STATIC UINT32 mCompSize, mOrigSize;

STATIC UINT16 *mFreq, *mSortPtr, mLenCnt[17], mLeft[2 * NC - 1], mRight[2 * NC - 1], mCrcTable[UINT8_MAX + 1],
  mCFreq[2 * NC - 1], mCTable[4096], mCCode[NC], mPFreq[2 * NP - 1], mPTCode[NPT], mTFreq[2 * NT - 1];

STATIC NODE   mPos, mMatchPos, mAvail, *mPosition, *mParent, *mPrev, *mNext = NULL;

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
  DstSize     - On input, the size of DstBuffer; On output,
                the size of the actual compressed data.
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
  mDstUpperLimit  = mDst + *DstSize;

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
  
Argements: 
  VOID

Returns:

  EFI_SUCCESS           - Memory is allocated successfully
  EFI_OUT_OF_RESOURCES  - Allocation fails

--*/
{
  UINT32  Index;

  mText = malloc (WNDSIZ * 2 + MAXMATCH);
  for (Index = 0; Index < WNDSIZ * 2 + MAXMATCH; Index++) {
    mText[Index] = 0;
  }

  mLevel      = malloc ((WNDSIZ + UINT8_MAX + 1) * sizeof (*mLevel));
  mChildCount = malloc ((WNDSIZ + UINT8_MAX + 1) * sizeof (*mChildCount));
  mPosition   = malloc ((WNDSIZ + UINT8_MAX + 1) * sizeof (*mPosition));
  mParent     = malloc (WNDSIZ * 2 * sizeof (*mParent));
  mPrev       = malloc (WNDSIZ * 2 * sizeof (*mPrev));
  mNext       = malloc ((MAX_HASH_VAL + 1) * sizeof (*mNext));

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
    mPosition[Index]  = NIL;  /* sentinel */
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
    // can be located by MatchPos + 1. Travese the tree
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
