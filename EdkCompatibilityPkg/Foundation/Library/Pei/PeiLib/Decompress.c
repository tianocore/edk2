/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Decompress.c

Abstract:

  Decompressor. Algorithm Ported from OPSD code (Decomp.asm)
  
--*/

#include "TianoCommon.h"
#include EFI_PROTOCOL_DEFINITION (Decompress)
#include EFI_PROTOCOL_DEFINITION (TianoDecompress)

EFI_STATUS
EFIAPI
EfiGetInfo (
  IN      EFI_DECOMPRESS_PROTOCOL *This,
  IN      VOID                    *Source,
  IN      UINT32                  SrcSize,
  OUT     UINT32                  *DstSize,
  OUT     UINT32                  *ScratchSize
  );

EFI_STATUS
EFIAPI
EfiDecompress (
  IN      EFI_DECOMPRESS_PROTOCOL *This,
  IN      VOID                    *Source,
  IN      UINT32                  SrcSize,
  IN OUT  VOID                    *Destination,
  IN      UINT32                  DstSize,
  IN OUT  VOID                    *Scratch,
  IN      UINT32                  ScratchSize
  );

EFI_STATUS
EFIAPI
TianoGetInfo (
  IN      EFI_TIANO_DECOMPRESS_PROTOCOL *This,
  IN      VOID                          *Source,
  IN      UINT32                        SrcSize,
  OUT     UINT32                        *DstSize,
  OUT     UINT32                        *ScratchSize
  );

EFI_STATUS
EFIAPI
TianoDecompress (
  IN      EFI_TIANO_DECOMPRESS_PROTOCOL *This,
  IN      VOID                          *Source,
  IN      UINT32                        SrcSize,
  IN OUT  VOID                          *Destination,
  IN      UINT32                        DstSize,
  IN OUT  VOID                          *Scratch,
  IN      UINT32                        ScratchSize
  );

//
// The protocol instance
//

EFI_DECOMPRESS_PROTOCOL       mEfiDecompress = {
  EfiGetInfo,
  EfiDecompress
};

EFI_TIANO_DECOMPRESS_PROTOCOL mTianoDecompress = {
  TianoGetInfo,
  TianoDecompress
};

EFI_STATUS
InstallEfiDecompress (
  IN OUT  EFI_DECOMPRESS_PROTOCOL  **This
  )
/*++

Routine Description:

  Install EFI decompress protocol.

Arguments:

  This  - Pointer to get decompress protocol as output

Returns:

  EFI_SUCCESS - EFI decompress protocol successfully installed.

--*/
{
  *This = &mEfiDecompress;
  return EFI_SUCCESS;
}

EFI_STATUS
InstallTianoDecompress (
  EFI_TIANO_DECOMPRESS_PROTOCOL  **This
  )
/*++

Routine Description:

  Install Tiano decompress protocol.

Arguments:

  This  - Pointer to get decompress protocol as output

Returns:

  EFI_SUCCESS - Tiano decompress protocol successfully installed.

--*/
{
  *This = &mTianoDecompress;
  return EFI_SUCCESS;
}
//
// Decompression algorithm begins here
//
#define BITBUFSIZ 32
#define MAXMATCH  256
#define THRESHOLD 3
#define CODE_BIT  16
#ifndef UINT8_MAX
#define UINT8_MAX 0xff
#endif
#define BAD_TABLE - 1

//
// C: Char&Len Set; P: Position Set; T: exTra Set
//
#define NC      (0xff + MAXMATCH + 2 - THRESHOLD)
#define CBIT    9
#define MAXPBIT 5
#define TBIT    5
#define MAXNP   ((1U << MAXPBIT) - 1)
#define NT      (CODE_BIT + 3)
#if NT > MAXNP
#define NPT NT
#else
#define NPT MAXNP
#endif

typedef struct {
  UINT8   *mSrcBase;  // Starting address of compressed data
  UINT8   *mDstBase;  // Starting address of decompressed data
  UINT32  mOutBuf;
  UINT32  mInBuf;

  UINT16  mBitCount;
  UINT32  mBitBuf;
  UINT32  mSubBitBuf;
  UINT16  mBlockSize;
  UINT32  mCompSize;
  UINT32  mOrigSize;

  UINT16  mBadTableFlag;

  UINT16  mLeft[2 * NC - 1];
  UINT16  mRight[2 * NC - 1];
  UINT8   mCLen[NC];
  UINT8   mPTLen[NPT];
  UINT16  mCTable[4096];
  UINT16  mPTTable[256];

  //
  // The length of the field 'Position Set Code Length Array Size' in Block Header.
  // For EFI 1.1 de/compression algorithm, mPBit = 4
  // For Tiano de/compression algorithm, mPBit = 5
  //
  UINT8   mPBit;
} SCRATCH_DATA;

STATIC
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
  Sd->mBitBuf = (UINT32) (Sd->mBitBuf << NumOfBits);

  while (NumOfBits > Sd->mBitCount) {

    Sd->mBitBuf |= (UINT32) (Sd->mSubBitBuf << (NumOfBits = (UINT16) (NumOfBits - Sd->mBitCount)));

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

STATIC
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

STATIC
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

  //
  // TableBits should not be greater than 16.
  //
  if (TableBits >= (sizeof (Count)/sizeof (UINT16))) {
    return (UINT16) BAD_TABLE;
  }
 
  //
  // Initialize Count array starting from Index 0, as there is a possibility of Count array being uninitialized.
  //
  for (Index = 0; Index <= 16; Index++) {
    Count[Index] = 0;
  }

  for (Index = 0; Index < NumOfChar; Index++) {
    //
    // Count array index should not be greater than or equal to its size.
    //
    if (BitLen[Index] < (sizeof (Count)/sizeof (UINT16))) {
      Count[BitLen[Index]]++;
    } else {
      return (UINT16) BAD_TABLE;
    }
  }

  Start[0] = 0;
  Start[1] = 0;

  for (Index = 1; Index <= 16; Index++) {
    Start[Index + 1] = (UINT16) (Start[Index] + (Count[Index] << (16 - Index)));
  }

  if (Start[17] != 0) {
    /*(1U << 16)*/
    return (UINT16) BAD_TABLE;
  }

  JuBits = (UINT16) (16 - TableBits);

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

  for (Char = 0; Char < NumOfChar; Char++) {

    Len = BitLen[Char];
    if (Len == 0 || Len >= 17) {
      continue;
    }

    NextCode = (UINT16) (Start[Len] + Weight[Len]);

    if (Len <= TableBits) {

      for (Index = Start[Len]; Index < NextCode; Index++) {
        Table[Index] = Char;
      }

    } else {

      Index3  = Start[Len];
      Pointer = &Table[Index3 >> JuBits];
      Index   = (UINT16) (Len - TableBits);

      while (Index != 0) {
        //
        // Avail should be lesser than size of mRight and mLeft to prevent buffer overflow.
        //
        if ((*Pointer == 0) && (Avail < sizeof (Sd->mRight)/sizeof (UINT16)) && (Avail < sizeof (Sd->mLeft)/sizeof (UINT16))) {
          Sd->mRight[Avail]                     = Sd->mLeft[Avail] = 0;
          *Pointer = Avail++;
        }

        //
        // *Pointer should be lesser than size of mRight and mLeft to prevent buffer overflow.
        //
        if ((Index3 & Mask) && (*Pointer < (sizeof (Sd->mRight)/sizeof (UINT16)))) {
          Pointer = &Sd->mRight[*Pointer];
        } else if (*Pointer < (sizeof (Sd->mLeft)/sizeof (UINT16))) {
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

STATIC
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

STATIC
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
  UINT16  Index;
  UINT32  Mask;

  Number = (UINT16) GetBits (Sd, nbit);

  if ((Number > sizeof (Sd->mPTLen)) || (nn > sizeof (Sd->mPTLen))) {
    //
    // Fail if Number or nn is greater than size of mPTLen
    //
    return (UINT16) BAD_TABLE;
  }

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
        if (Index >= sizeof (Sd->mPTLen)) {
          //
          // Fail if Index is greater than or equal to mPTLen
          //
          return (UINT16) BAD_TABLE;
        }
        Sd->mPTLen[Index++] = 0;
      }
    }
  }

  while (Index < nn) {
    Sd->mPTLen[Index++] = 0;
  }

  return MakeTable (Sd, nn, Sd->mPTLen, 8, Sd->mPTTable);
}

STATIC
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
  UINT16  Index;
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

STATIC
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

STATIC
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
      return ;
    }

    if (CharC < 256) {
      //
      // Process an Original character
      //
      if (Sd->mOutBuf >= Sd->mOrigSize) {
        return ;
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
        Sd->mDstBase[Sd->mOutBuf++] = Sd->mDstBase[DataIdx++];
        if (Sd->mOutBuf >= Sd->mOrigSize) {
          return ;
        }

        BytesRemain--;
      }
    }
  }
}

EFI_STATUS
GetInfo (
  IN      VOID    *Source,
  IN      UINT32  SrcSize,
  OUT     UINT32  *DstSize,
  OUT     UINT32  *ScratchSize
  )
/*++

Routine Description:

  The internal implementation of *_DECOMPRESS_PROTOCOL.GetInfo().

Arguments:

  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  DstSize     - The size of destination buffer.
  ScratchSize - The size of scratch buffer.

Returns:

  EFI_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  EFI_INVALID_PARAMETER - The source data is corrupted

--*/
{
  UINT8 *Src;

  *ScratchSize  = sizeof (SCRATCH_DATA);

  Src           = Source;
  if (SrcSize < 8) {
    return EFI_INVALID_PARAMETER;
  }

  *DstSize = Src[4] + (Src[5] << 8) + (Src[6] << 16) + (Src[7] << 24);
  return EFI_SUCCESS;
}

EFI_STATUS
Decompress (
  IN      VOID    *Source,
  IN      UINT32  SrcSize,
  IN OUT  VOID    *Destination,
  IN      UINT32  DstSize,
  IN OUT  VOID    *Scratch,
  IN      UINT32  ScratchSize,
  IN      UINT8   Version
  )
/*++

Routine Description:

  The internal implementation of *_DECOMPRESS_PROTOCOL.Decompress().

Arguments:

  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  Destination - The destination buffer to store the decompressed data
  DstSize     - The size of destination buffer.
  Scratch     - The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.
  ScratchSize - The size of scratch buffer.
  Version     - The version of de/compression algorithm.
                Version 1 for EFI 1.1 de/compression algorithm.
                Version 2 for Tiano de/compression algorithm.

Returns:

  EFI_SUCCESS           - Decompression is successfull
  EFI_INVALID_PARAMETER - The source data is corrupted

--*/
{
  UINT32        Index;
  UINT32        CompSize;
  UINT32        OrigSize;
  EFI_STATUS    Status;
  SCRATCH_DATA  *Sd;
  UINT8         *Src;
  UINT8         *Dst;

  Status  = EFI_SUCCESS;
  Src     = Source;
  Dst     = Destination;

  if (ScratchSize < sizeof (SCRATCH_DATA)) {
    return EFI_INVALID_PARAMETER;
  }

  Sd = (SCRATCH_DATA *) Scratch;

  if (SrcSize < 8) {
    return EFI_INVALID_PARAMETER;
  }

  CompSize  = Src[0] + (Src[1] << 8) + (Src[2] << 16) + (Src[3] << 24);
  OrigSize  = Src[4] + (Src[5] << 8) + (Src[6] << 16) + (Src[7] << 24);

  //
  // If compressed file size is 0, return
  //
  if (OrigSize == 0) {
    return Status;
  }

  if (SrcSize < CompSize + 8) {
    return EFI_INVALID_PARAMETER;
  }

  if (DstSize != OrigSize) {
    return EFI_INVALID_PARAMETER;
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
  case 1:
    Sd->mPBit = 4;
    break;

  case 2:
    Sd->mPBit = 5;
    break;

  default:
    //
    // Currently, only have 2 versions
    //
    return EFI_INVALID_PARAMETER;
  }

  Sd->mSrcBase  = Src;
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
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

EFI_STATUS
EFIAPI
EfiGetInfo (
  IN      EFI_DECOMPRESS_PROTOCOL *This,
  IN      VOID                    *Source,
  IN      UINT32                  SrcSize,
  OUT     UINT32                  *DstSize,
  OUT     UINT32                  *ScratchSize
  )
/*++

Routine Description:

  The implementation of EFI_DECOMPRESS_PROTOCOL.GetInfo().

Arguments:

  This        - The protocol instance pointer
  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  DstSize     - The size of destination buffer.
  ScratchSize - The size of scratch buffer.

Returns:

  EFI_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  EFI_INVALID_PARAMETER - The source data is corrupted

--*/
{
  return GetInfo (
          Source,
          SrcSize,
          DstSize,
          ScratchSize
          );
}

EFI_STATUS
EFIAPI
EfiDecompress (
  IN      EFI_DECOMPRESS_PROTOCOL *This,
  IN      VOID                    *Source,
  IN      UINT32                  SrcSize,
  IN OUT  VOID                    *Destination,
  IN      UINT32                  DstSize,
  IN OUT  VOID                    *Scratch,
  IN      UINT32                  ScratchSize
  )
/*++

Routine Description:

  The implementation of EFI_DECOMPRESS_PROTOCOL.Decompress().

Arguments:

  This        - The protocol instance pointer
  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  Destination - The destination buffer to store the decompressed data
  DstSize     - The size of destination buffer.
  Scratch     - The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.
  ScratchSize - The size of scratch buffer.

Returns:

  EFI_SUCCESS           - Decompression is successfull
  EFI_INVALID_PARAMETER - The source data is corrupted

--*/
{
  //
  // For EFI 1.1 de/compression algorithm, the version is 1.
  //
  return Decompress (
          Source,
          SrcSize,
          Destination,
          DstSize,
          Scratch,
          ScratchSize,
          1
          );
}

EFI_STATUS
EFIAPI
TianoGetInfo (
  IN      EFI_TIANO_DECOMPRESS_PROTOCOL *This,
  IN      VOID                          *Source,
  IN      UINT32                        SrcSize,
  OUT     UINT32                        *DstSize,
  OUT     UINT32                        *ScratchSize
  )
/*++

Routine Description:

  The implementation of EFI_TIANO_DECOMPRESS_PROTOCOL.GetInfo().

Arguments:

  This        - The protocol instance pointer
  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  DstSize     - The size of destination buffer.
  ScratchSize - The size of scratch buffer.

Returns:

  EFI_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  EFI_INVALID_PARAMETER - The source data is corrupted

--*/
{
  return GetInfo (
          Source,
          SrcSize,
          DstSize,
          ScratchSize
          );
}

EFI_STATUS
EFIAPI
TianoDecompress (
  IN      EFI_TIANO_DECOMPRESS_PROTOCOL *This,
  IN      VOID                          *Source,
  IN      UINT32                        SrcSize,
  IN OUT  VOID                          *Destination,
  IN      UINT32                        DstSize,
  IN OUT  VOID                          *Scratch,
  IN      UINT32                        ScratchSize
  )
/*++

Routine Description:

  The implementation of EFI_TIANO_DECOMPRESS_PROTOCOL.Decompress().

Arguments:

  This        - The protocol instance pointer
  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  Destination - The destination buffer to store the decompressed data
  DstSize     - The size of destination buffer.
  Scratch     - The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.
  ScratchSize - The size of scratch buffer.

Returns:

  EFI_SUCCESS           - Decompression is successfull
  EFI_INVALID_PARAMETER - The source data is corrupted

--*/
{
  //
  // For Tiano de/compression algorithm, the version is 2.
  //
  return Decompress (
          Source,
          SrcSize,
          Destination,
          DstSize,
          Scratch,
          ScratchSize,
          2
          );
}
