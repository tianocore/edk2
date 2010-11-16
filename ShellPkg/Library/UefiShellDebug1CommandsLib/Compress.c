/** @file
  Main file for compression routine.

  Compression routine. The compression algorithm is a mixture of
  LZ77 and Huffman coding. LZ77 transforms the source data into a
  sequence of Original Characters and Pointers to repeated strings.
  This sequence is further divided into Blocks and Huffman codings
  are applied to each Block.

  Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
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
#define HASH(p, c)        ((p) + ((c) << (WNDBIT - 9)) + WNDSIZ * 2)
#define CRCPOLY           0xA001
#define UPDATE_CRC(c)     mCrc = mCrcTable[(mCrc ^ (c)) & 0xFF] ^ (mCrc >> UINT8_BIT)

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

EFI_STATUS
EFIAPI
AllocateMemory (
  VOID
  );

VOID
EFIAPI
FreeMemory (
  VOID
  );

VOID
EFIAPI
InitSlide (
  VOID
  );

NODE
EFIAPI
Child (
  IN NODE   q,
  IN UINT8  c
  );

VOID
EFIAPI
MakeChild (
  IN NODE   q,
  IN UINT8  c,
  IN NODE   r
  );

VOID
EFIAPI
Split (
  IN NODE Old
  );

VOID
EFIAPI
InsertNode (
  VOID
  );

VOID
EFIAPI
DeleteNode (
  VOID
  );

VOID
EFIAPI
GetNextMatch (
  VOID
  );

EFI_STATUS
EFIAPI
Encode (
  VOID
  );

VOID
EFIAPI
CountTFreq (
  VOID
  );

VOID
EFIAPI
WritePTLen (
  IN INT32 n,
  IN INT32 nbit,
  IN INT32 Special
  );

VOID
EFIAPI
WriteCLen (
  VOID
  );

VOID
EFIAPI
EncodeC (
  IN INT32 c
  );

VOID
EFIAPI
EncodeP (
  IN UINT32 p
  );

VOID
EFIAPI
SendBlock (
  VOID
  );

VOID
EFIAPI
CompressOutput (
  IN UINT32 c,
  IN UINT32 p
  );

VOID
EFIAPI
HufEncodeStart (
  VOID
  );

VOID
EFIAPI
HufEncodeEnd (
  VOID
  );

VOID
EFIAPI
MakeCrcTable (
  VOID
  );

VOID
EFIAPI
PutBits (
  IN INT32    n,
  IN UINT32   x
  );

INT32
EFIAPI
FreadCrc (
  OUT UINT8 *p,
  IN  INT32 n
  );

VOID
EFIAPI
InitPutBits (
  VOID
  );

VOID
EFIAPI
CountLen (
  IN INT32 i
  );

VOID
EFIAPI
MakeLen (
  IN INT32 Root
  );

VOID
EFIAPI
DownHeap (
  IN INT32 i
  );

VOID
EFIAPI
MakeCode (
  IN  INT32         n,
  IN  UINT8 Len[    ],
  OUT UINT16 Code[  ]
  );

INT32
EFIAPI
MakeTree (
  IN  INT32             NParm,
  IN  UINT16  FreqParm[ ],
  OUT UINT8   LenParm[  ],
  OUT UINT16  CodeParm[ ]
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

STATIC UINT16 *mFreq, *mSortPtr, mLenCnt[17], mLeft[2 * NC - 1], mRight[2 * NC - 1],
              mCrcTable[UINT8_MAX + 1], mCFreq[2 * NC - 1], mCTable[4096], mCCode[NC],
              mPFreq[2 * NP - 1], mPTCode[NPT], mTFreq[2 * NT - 1];

STATIC NODE   mPos, mMatchPos, mAvail, *mPosition, *mParent, *mPrev, *mNext = NULL;

//
// functions
//
/**
  The compression routine.

  @param[in]      SrcBuffer     The buffer containing the source data.
  @param[in]      SrcSizae      Number of bytes in SrcBuffer.
  @param[in]      DstBuffer     The buffer to put the compressed image in.
  @param[in,out]  DstSize       On input the size (in bytes) of DstBuffer, on
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

EFI_STATUS
EFIAPI
AllocateMemory (
  VOID
  )
/*++

Routine Description:

  Allocate memory spaces for data structures used in compression process

Arguments:

  None

Returns:

  EFI_SUCCESS           - Memory is allocated successfully
  EFI_OUT_OF_RESOURCES  - Allocation fails

**/
{
  mText       = AllocateZeroPool (WNDSIZ * 2 + MAXMATCH);
  mLevel      = AllocatePool ((WNDSIZ + UINT8_MAX + 1) * sizeof (*mLevel));
  mChildCount = AllocatePool ((WNDSIZ + UINT8_MAX + 1) * sizeof (*mChildCount));
  mPosition   = AllocatePool ((WNDSIZ + UINT8_MAX + 1) * sizeof (*mPosition));
  mParent     = AllocatePool (WNDSIZ * 2 * sizeof (*mParent));
  mPrev       = AllocatePool (WNDSIZ * 2 * sizeof (*mPrev));
  mNext       = AllocatePool ((MAX_HASH_VAL + 1) * sizeof (*mNext));

  mBufSiz     = BLKSIZ;
  mBuf        = AllocatePool (mBufSiz);
  while (mBuf == NULL) {
    mBufSiz = (mBufSiz / 10U) * 9U;
    if (mBufSiz < 4 * 1024U) {
      return EFI_OUT_OF_RESOURCES;
    }

    mBuf = AllocatePool (mBufSiz);
  }

  mBuf[0] = 0;

  return EFI_SUCCESS;
}

VOID
EFIAPI
FreeMemory (
  VOID
  )
/*++

Routine Description:

  Called when compression is completed to free memory previously allocated.

Arguments: (VOID)

Returns: (VOID)

**/
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

VOID
EFIAPI
InitSlide (
  VOID
  )
/*++

Routine Description:

  Initialize String Info Log data structures

Arguments: (VOID)

Returns: (VOID)

**/
{
  NODE  i;

  SetMem (mLevel + WNDSIZ, (UINT8_MAX + 1) * sizeof (UINT8), 1);
  SetMem (mPosition + WNDSIZ, (UINT8_MAX + 1) * sizeof (NODE), 0);

  SetMem (mParent + WNDSIZ, WNDSIZ * sizeof (NODE), 0);

  mAvail = 1;
  for (i = 1; i < WNDSIZ - 1; i++) {
    mNext[i] = (NODE) (i + 1);
  }

  mNext[WNDSIZ - 1] = NIL;
  SetMem (mNext + WNDSIZ * 2, (MAX_HASH_VAL - WNDSIZ * 2 + 1) * sizeof (NODE), 0);
}

NODE
EFIAPI
Child (
  IN NODE   q,
  IN UINT8  c
  )
/*++

Routine Description:

  Find child node given the parent node and the edge character

Arguments:

  q       - the parent node
  c       - the edge character

Returns:

  The child node (NIL if not found)

**/
{
  NODE  r;

  r             = mNext[HASH (q, c)];
  mParent[NIL]  = q;  /* sentinel */
  while (mParent[r] != q) {
    r = mNext[r];
  }

  return r;
}

VOID
EFIAPI
MakeChild (
  IN NODE   q,
  IN UINT8  c,
  IN NODE   r
  )
/*++

Routine Description:

  Create a new child for a given parent node.

Arguments:

  q       - the parent node
  c       - the edge character
  r       - the child node

Returns: (VOID)

**/
{
  NODE  h;

  NODE  t;

  h           = (NODE) HASH (q, c);
  t           = mNext[h];
  mNext[h]    = r;
  mNext[r]    = t;
  mPrev[t]    = r;
  mPrev[r]    = h;
  mParent[r]  = q;
  mChildCount[q]++;
}

VOID
EFIAPI
Split (
  NODE Old
  )
/*++

Routine Description:

  Split a node.

Arguments:

  Old     - the node to split

Returns: (VOID)

**/
{
  NODE  New;

  NODE  t;

  New               = mAvail;
  mAvail            = mNext[New];
  mChildCount[New]  = 0;
  t                 = mPrev[Old];
  mPrev[New]        = t;
  mNext[t]          = New;
  t                 = mNext[Old];
  mNext[New]        = t;
  mPrev[t]          = New;
  mParent[New]      = mParent[Old];
  mLevel[New]       = (UINT8) mMatchLen;
  mPosition[New]    = mPos;
  MakeChild (New, mText[mMatchPos + mMatchLen], Old);
  MakeChild (New, mText[mPos + mMatchLen], mPos);
}

VOID
EFIAPI
InsertNode (
  VOID
  )
/*++

Routine Description:

  Insert string info for current position into the String Info Log

Arguments: (VOID)

Returns: (VOID)

**/
{
  NODE  q;

  NODE  r;

  NODE  j;

  NODE  t;
  UINT8 c;
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
    r = (NODE) ((mMatchPos + 1) | WNDSIZ);
    q = mParent[r];
    while (q == NIL) {
      r = mNext[r];
      q = mParent[r];
    }

    while (mLevel[q] >= mMatchLen) {
      r = q;
      q = mParent[q];
    }

    t = q;
    while (mPosition[t] < 0) {
      mPosition[t]  = mPos;
      t             = mParent[t];
    }

    if (t < WNDSIZ) {
      mPosition[t] = (NODE) (mPos | PERC_FLAG);
    }
  } else {
    //
    // Locate the target tree
    //
    q = (NODE) (mText[mPos] + WNDSIZ);
    c = mText[mPos + 1];
    r = Child (q, c);
    if (r == NIL) {
      MakeChild (q, c, mPos);
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
    if (r >= WNDSIZ) {
      j         = MAXMATCH;
      mMatchPos = r;
    } else {
      j         = mLevel[r];
      mMatchPos = (NODE) (mPosition[r] & ~PERC_FLAG);
    }

    if (mMatchPos >= mPos) {
      mMatchPos -= WNDSIZ;
    }

    t1  = &mText[mPos + mMatchLen];
    t2  = &mText[mMatchPos + mMatchLen];
    while (mMatchLen < j) {
      if (*t1 != *t2) {
        Split (r);
        return ;
      }

      mMatchLen++;
      t1++;
      t2++;
    }

    if (mMatchLen >= MAXMATCH) {
      break;
    }

    mPosition[r]  = mPos;
    q             = r;
    r             = Child (q, *t1);
    if (r == NIL) {
      MakeChild (q, *t1, mPos);
      return ;
    }

    mMatchLen++;
  }

  t             = mPrev[r];
  mPrev[mPos]   = t;
  mNext[t]      = mPos;
  t             = mNext[r];
  mNext[mPos]   = t;
  mPrev[t]      = mPos;
  mParent[mPos] = q;
  mParent[r]    = NIL;

  //
  // Special usage of 'next'
  //
  mNext[r] = mPos;

}

VOID
EFIAPI
DeleteNode (
  VOID
  )
/*++

Routine Description:

  Delete outdated string info. (The Usage of PERC_FLAG
  ensures a clean deletion)

Arguments: (VOID)

Returns: (VOID)

**/
{
  NODE  q;

  NODE  r;

  NODE  s;

  NODE  t;

  NODE  u;

  if (mParent[mPos] == NIL) {
    return ;
  }

  r             = mPrev[mPos];
  s             = mNext[mPos];
  mNext[r]      = s;
  mPrev[s]      = r;
  r             = mParent[mPos];
  mParent[mPos] = NIL;
  if (r >= WNDSIZ) {
    return ;
  }

  mChildCount[r]--;
  if (mChildCount[r] > 1) {
    return ;
  }

  t = (NODE) (mPosition[r] & ~PERC_FLAG);
  if (t >= mPos) {
    t -= WNDSIZ;
  }

  s = t;
  q = mParent[r];
  u = mPosition[q];
  while ((u & PERC_FLAG) != 0){
    u &= ~PERC_FLAG;
    if (u >= mPos) {
      u -= WNDSIZ;
    }

    if (u > s) {
      s = u;
    }

    mPosition[q]  = (NODE) (s | WNDSIZ);
    q             = mParent[q];
    u             = mPosition[q];
  }

  if (q < WNDSIZ) {
    if (u >= mPos) {
      u -= WNDSIZ;
    }

    if (u > s) {
      s = u;
    }

    mPosition[q] = (NODE) (s | WNDSIZ | PERC_FLAG);
  }

  s           = Child (r, mText[t + mLevel[r]]);
  t           = mPrev[s];
  u           = mNext[s];
  mNext[t]    = u;
  mPrev[u]    = t;
  t           = mPrev[r];
  mNext[t]    = s;
  mPrev[s]    = t;
  t           = mNext[r];
  mPrev[t]    = s;
  mNext[s]    = t;
  mParent[s]  = mParent[r];
  mParent[r]  = NIL;
  mNext[r]    = mAvail;
  mAvail      = r;
}

VOID
EFIAPI
GetNextMatch (
  VOID
  )
/*++

Routine Description:

  Advance the current position (read in new data if needed).
  Delete outdated string info. Find a match string for current position.

Arguments: (VOID)

Returns: (VOID)

**/
{
  INT32 n;
  VOID  *Temp;

  mRemainder--;
  mPos++;
  if (mPos == WNDSIZ * 2) {
    Temp = AllocatePool (WNDSIZ + MAXMATCH);
    CopyMem (Temp, &mText[WNDSIZ], WNDSIZ + MAXMATCH);
    CopyMem (&mText[0], Temp, WNDSIZ + MAXMATCH);
    FreePool (Temp);
    n = FreadCrc (&mText[WNDSIZ + MAXMATCH], WNDSIZ);
    mRemainder += n;
    mPos = WNDSIZ;
  }

  DeleteNode ();
  InsertNode ();
}

EFI_STATUS
EFIAPI
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

**/
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
      CompressOutput(mText[mPos - 1], 0);
    } else {
      //
      // Outputting a pointer is beneficial enough, do it.
      //

      CompressOutput(LastMatchLen + (UINT8_MAX + 1 - THRESHOLD),
             (mPos - LastMatchPos - 2) & (WNDSIZ - 1));
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

VOID
EFIAPI
CountTFreq (
  VOID
  )
/*++

Routine Description:

  Count the frequencies for the Extra Set

Arguments: (VOID)

Returns: (VOID)

**/
{
  INT32 i;

  INT32 k;

  INT32 n;

  INT32 Count;

  for (i = 0; i < NT; i++) {
    mTFreq[i] = 0;
  }

  n = NC;
  while (n > 0 && mCLen[n - 1] == 0) {
    n--;
  }

  i = 0;
  while (i < n) {
    k = mCLen[i++];
    if (k == 0) {
      Count = 1;
      while (i < n && mCLen[i] == 0) {
        i++;
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
      ASSERT((k+2)<(2 * NT - 1));
      mTFreq[k + 2]++;
    }
  }
}

VOID
EFIAPI
WritePTLen (
  IN INT32 n,
  IN INT32 nbit,
  IN INT32 Special
  )
/*++

Routine Description:

  Outputs the code length array for the Extra Set or the Position Set.

Arguments:

  n       - the number of symbols
  nbit    - the number of bits needed to represent 'n'
  Special - the special symbol that needs to be take care of

Returns: (VOID)

**/
{
  INT32 i;

  INT32 k;

  while (n > 0 && mPTLen[n - 1] == 0) {
    n--;
  }

  PutBits (nbit, n);
  i = 0;
  while (i < n) {
    k = mPTLen[i++];
    if (k <= 6) {
      PutBits (3, k);
    } else {
      PutBits (k - 3, (1U << (k - 3)) - 2);
    }

    if (i == Special) {
      while (i < 6 && mPTLen[i] == 0) {
        i++;
      }

      PutBits (2, (i - 3) & 3);
    }
  }
}

VOID
EFIAPI
WriteCLen (
  VOID
  )
/*++

Routine Description:

  Outputs the code length array for Char&Length Set

Arguments: (VOID)

Returns: (VOID)

**/
{
  INT32 i;

  INT32 k;

  INT32 n;

  INT32 Count;

  n = NC;
  while (n > 0 && mCLen[n - 1] == 0) {
    n--;
  }

  PutBits (CBIT, n);
  i = 0;
  while (i < n) {
    k = mCLen[i++];
    if (k == 0) {
      Count = 1;
      while (i < n && mCLen[i] == 0) {
        i++;
        Count++;
      }

      if (Count <= 2) {
        for (k = 0; k < Count; k++) {
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
      ASSERT((k+2)<NPT);
      PutBits (mPTLen[k + 2], mPTCode[k + 2]);
    }
  }
}

VOID
EFIAPI
EncodeC (
  IN INT32 c
  )
{
  PutBits (mCLen[c], mCCode[c]);
}

VOID
EFIAPI
EncodeP (
  IN UINT32 p
  )
{
  UINT32  c;

  UINT32  q;

  c = 0;
  q = p;
  while (q != 0) {
    q >>= 1;
    c++;
  }

  PutBits (mPTLen[c], mPTCode[c]);
  if (c > 1) {
    PutBits(c - 1, p & (0xFFFFU >> (17 - c)));
  }
}

VOID
EFIAPI
SendBlock (
  VOID
  )
/*++

Routine Description:

  Huffman code the block and output it.

Arguments:

  None

Returns:

  None

**/
{
  UINT32  i;

  UINT32  k;

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
  for (i = 0; i < Size; i++) {
    if (i % UINT8_BIT == 0) {
      Flags = mBuf[Pos++];
    } else {
      Flags <<= 1;
    }
    if ((Flags & (1U << (UINT8_BIT - 1))) != 0){
      EncodeC(mBuf[Pos++] + (1U << UINT8_BIT));
      k = mBuf[Pos++] << UINT8_BIT;
      k += mBuf[Pos++];

      EncodeP (k);
    } else {
      EncodeC (mBuf[Pos++]);
    }
  }

  SetMem (mCFreq, NC * sizeof (UINT16), 0);
  SetMem (mPFreq, NP * sizeof (UINT16), 0);
}

VOID
EFIAPI
CompressOutput (
  IN UINT32 c,
  IN UINT32 p
  )
/*++

Routine Description:

  Outputs an Original Character or a Pointer

Arguments:

  c     - The original character or the 'String Length' element of a Pointer
  p     - The 'Position' field of a Pointer

Returns: (VOID)

**/
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
  mBuf[mOutputPos++] = (UINT8) c;
  mCFreq[c]++;
  if (c >= (1U << UINT8_BIT)) {
    mBuf[CPos] = (UINT8)(mBuf[CPos]|mOutputMask);
    mBuf[mOutputPos++] = (UINT8)(p >> UINT8_BIT);
    mBuf[mOutputPos++] = (UINT8) p;
    c                  = 0;
    while (p!=0) {
      p >>= 1;
      c++;
    }
    mPFreq[c]++;
  }
}

VOID
EFIAPI
HufEncodeStart (
  VOID
  )
{
  SetMem (mCFreq, NC * sizeof (UINT16), 0);
  SetMem (mPFreq, NP * sizeof (UINT16), 0);

  mOutputPos = mOutputMask = 0;
  InitPutBits ();
  return ;
}

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

  return ;
}

VOID
EFIAPI
MakeCrcTable (
  VOID
  )
{
  UINT32  i;

  UINT32  j;

  UINT32  r;

  for (i = 0; i <= UINT8_MAX; i++) {
    r = i;
    for (j = 0; j < UINT8_BIT; j++) {
      if ((r & 1) != 0) {
        r = (r >> 1) ^ CRCPOLY;
      } else {
        r >>= 1;
      }
    }

    mCrcTable[i] = (UINT16) r;
  }
}

VOID
EFIAPI
PutBits (
  IN INT32    n,
  IN UINT32   x
  )
/*++

Routine Description:

  Outputs rightmost n bits of x

Arguments:

  n   - the rightmost n bits of the data is used
  x   - the data

Returns:

  None

**/
{
  UINT8 Temp;

  if (n < mBitCount) {
    mSubBitBuf |= x << (mBitCount -= n);
  } else {

    Temp = (UINT8)(mSubBitBuf | (x >> (n -= mBitCount)));
    if (mDst < mDstUpperLimit) {
      *mDst++ = Temp;
    }
    mCompSize++;

    if (n < UINT8_BIT) {
      mSubBitBuf = x << (mBitCount = UINT8_BIT - n);
    } else {

      Temp = (UINT8)(x >> (n - UINT8_BIT));
      if (mDst < mDstUpperLimit) {
        *mDst++ = Temp;
      }
      mCompSize++;

      mSubBitBuf = x << (mBitCount = 2 * UINT8_BIT - n);
    }
  }
}

INT32
EFIAPI
FreadCrc (
  OUT UINT8 *p,
  IN  INT32 n
  )
/*++

Routine Description:

  Read in source data

Arguments:

  p   - the buffer to hold the data
  n   - number of bytes to read

Returns:

  number of bytes actually read

**/
{
  INT32 i;

  for (i = 0; mSrc < mSrcUpperLimit && i < n; i++) {
    *p++ = *mSrc++;
  }

  n = i;

  p -= n;
  mOrigSize += n;
  i--;
  while (i >= 0) {
    UPDATE_CRC (*p++);
    i--;
  }

  return n;
}

VOID
EFIAPI
InitPutBits (
  VOID
  )
{
  mBitCount   = UINT8_BIT;
  mSubBitBuf  = 0;
}

VOID
EFIAPI
CountLen (
  IN INT32 i
  )
/*++

Routine Description:

  Count the number of each code length for a Huffman tree.

Arguments:

  i   - the top node

Returns: (VOID)

**/
{
  STATIC INT32  Depth = 0;

  if (i < mN) {
    mLenCnt[(Depth < 16) ? Depth : 16]++;
  } else {
    Depth++;
    CountLen (mLeft[i]);
    CountLen (mRight[i]);
    Depth--;
  }
}

VOID
EFIAPI
MakeLen (
  IN INT32 Root
  )
/*++

Routine Description:

  Create code length array for a Huffman tree

Arguments:

  Root   - the root of the tree

Returns:

  None

**/
{
  INT32   i;

  INT32   k;
  UINT32  Cum;

  for (i = 0; i <= 16; i++) {
    mLenCnt[i] = 0;
  }

  CountLen (Root);

  //
  // Adjust the length count array so that
  // no code will be generated longer than its designated length
  //
  Cum = 0;
  for (i = 16; i > 0; i--) {
    Cum += mLenCnt[i] << (16 - i);
  }

  while (Cum != (1U << 16)) {
    mLenCnt[16]--;
    for (i = 15; i > 0; i--) {
      if (mLenCnt[i] != 0) {
        mLenCnt[i]--;
        mLenCnt[i + 1] += 2;
        break;
      }
    }

    Cum--;
  }

  for (i = 16; i > 0; i--) {
    k = mLenCnt[i];
    k--;
    while (k >= 0) {
      mLen[*mSortPtr++] = (UINT8) i;
      k--;
    }
  }
}

VOID
EFIAPI
DownHeap (
  IN INT32 i
  )
{
  INT32 j;

  INT32 k;

  //
  // priority queue: send i-th entry down heap
  //
  k = mHeap[i];
  j = 2 * i;
  while (j <= mHeapSize) {
    if (j < mHeapSize && mFreq[mHeap[j]] > mFreq[mHeap[j + 1]]) {
      j++;
    }

    if (mFreq[k] <= mFreq[mHeap[j]]) {
      break;
    }

    mHeap[i]  = mHeap[j];
    i         = j;
    j         = 2 * i;
  }

  mHeap[i] = (INT16) k;
}

VOID
EFIAPI
MakeCode (
  IN  INT32         n,
  IN  UINT8 Len[    ],
  OUT UINT16 Code[  ]
  )
/*++

Routine Description:

  Assign code to each symbol based on the code length array

Arguments:

  n     - number of symbols
  Len   - the code length array
  Code  - stores codes for each symbol

Returns:

  None

**/
{
  INT32   i;
  UINT16  Start[18];

  Start[1] = 0;
  for (i = 1; i <= 16; i++) {
    Start[i + 1] = (UINT16) ((Start[i] + mLenCnt[i]) << 1);
  }

  for (i = 0; i < n; i++) {
    Code[i] = Start[Len[i]]++;
  }
}

INT32
EFIAPI
MakeTree (
  IN  INT32             NParm,
  IN  UINT16  FreqParm[ ],
  OUT UINT8   LenParm[  ],
  OUT UINT16  CodeParm[ ]
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

**/
{
  INT32 i;

  INT32 j;

  INT32 k;

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
  for (i = 0; i < mN; i++) {
    mLen[i] = 0;
    if ((mFreq[i]) != 0) {
      mHeapSize++;
      mHeap[mHeapSize] = (INT16) i;
    }
  }

  if (mHeapSize < 2) {
    CodeParm[mHeap[1]] = 0;
    return mHeap[1];
  }

  for (i = mHeapSize / 2; i >= 1; i--) {
    //
    // make priority queue
    //
    DownHeap (i);
  }

  mSortPtr = CodeParm;
  do {
    i = mHeap[1];
    if (i < mN) {
      *mSortPtr++ = (UINT16) i;
    }

    mHeap[1] = mHeap[mHeapSize--];
    DownHeap (1);
    j = mHeap[1];
    if (j < mN) {
      *mSortPtr++ = (UINT16) j;
    }

    k         = Avail++;
    mFreq[k]  = (UINT16) (mFreq[i] + mFreq[j]);
    mHeap[1]  = (INT16) k;
    DownHeap (1);
    mLeft[k]  = (UINT16) i;
    mRight[k] = (UINT16) j;
  } while (mHeapSize > 1);

  mSortPtr = CodeParm;
  MakeLen (k);
  MakeCode (NParm, LenParm, CodeParm);

  //
  // return root
  //
  return k;
}
