/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  BaseUefiDecompressLib.c
  
Abstract: 

  UEFI Decompress Library.

--*/

#include "BaseUefiDecompressLibInternals.h"

/**
  Read NumOfBit of bits from source into mBitBuf

  Shift mBitBuf NumOfBits left. Read in NumOfBits of bits from source.

  @param  Sd        The global scratch data
  @param  NumOfBits The number of bits to shift and read.

**/
VOID
GlueFillBuf (
  IN  SCRATCH_DATA  *Sd,
  IN  UINT16        NumOfBits
  )
{
  //
  // Left shift NumOfBits of bits in advance
  //
  Sd->mBitBuf = (UINT32) (Sd->mBitBuf << NumOfBits);

  //
  // Copy data needed in bytes into mSbuBitBuf
  //
  while (NumOfBits > Sd->mBitCount) {

    Sd->mBitBuf |= (UINT32) (Sd->mSubBitBuf << (NumOfBits = (UINT16) (NumOfBits - Sd->mBitCount)));

    if (Sd->mCompSize > 0) {
      //
      // Get 1 byte into SubBitBuf
      //
      Sd->mCompSize--;
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

  //
  // Caculate additional bit count read to update mBitCount
  //
  Sd->mBitCount = (UINT16) (Sd->mBitCount - NumOfBits);
  
  //
  // Copy NumOfBits of bits from mSubBitBuf into mBitBuf
  //
  Sd->mBitBuf |= Sd->mSubBitBuf >> Sd->mBitCount;
}

/**
  Get NumOfBits of bits out from mBitBuf

  Get NumOfBits of bits out from mBitBuf. Fill mBitBuf with subsequent 
  NumOfBits of bits from source. Returns NumOfBits of bits that are 
  popped out.

  @param  Sd        The global scratch data.
  @param  NumOfBits The number of bits to pop and read.

  @return The bits that are popped out.

**/
UINT32
GlueGetBits (
  IN  SCRATCH_DATA  *Sd,
  IN  UINT16        NumOfBits
  )
{
  UINT32  OutBits;

  //
  // Pop NumOfBits of Bits from Left
  //  
  OutBits = (UINT32) (Sd->mBitBuf >> (BITBUFSIZ - NumOfBits));

  //
  // Fill up mBitBuf from source
  //
  FillBuf (Sd, NumOfBits);

  return OutBits;
}

/**
  Creates Huffman Code mapping table according to code length array.

  Creates Huffman Code mapping table for Extra Set, Char&Len Set 
  and Position Set according to code length array.

  @param  Sd        The global scratch data
  @param  NumOfChar Number of symbols in the symbol set
  @param  BitLen    Code length array
  @param  TableBits The width of the mapping table
  @param  Table     The table

  @retval  0 OK.
  @retval  BAD_TABLE The table is corrupted.

**/
UINT16
GlueMakeTable (
  IN  SCRATCH_DATA  *Sd,
  IN  UINT16        NumOfChar,
  IN  UINT8         *BitLen,
  IN  UINT16        TableBits,
  OUT UINT16        *Table
  )
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
    WordOfStart = Start[Index];
    WordOfCount = Count[Index];
    Start[Index + 1] = (UINT16) (WordOfStart + (WordOfCount << (16 - Index)));
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

/**
  Decodes a position value.

  Get a position value according to Position Huffman Table.
  
  @param  Sd the global scratch data

  @return The position value decoded.

**/
UINT32
GlueDecodeP (
  IN  SCRATCH_DATA  *Sd
  )
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

/**
  Reads code lengths for the Extra Set or the Position Set.

  Read in the Extra Set or Pointion Set Length Arrary, then
  generate the Huffman code mapping for them.

  @param  Sd      The global scratch data.
  @param  nn      Number of symbols.
  @param  nbit    Number of bits needed to represent nn.
  @param  Special The special symbol that needs to be taken care of.

  @retval  0 OK.
  @retval  BAD_TABLE Table is corrupted.

**/
UINT16
GlueReadPTLen (
  IN  SCRATCH_DATA  *Sd,
  IN  UINT16        nn,
  IN  UINT16        nbit,
  IN  UINT16        Special
  )
{
  UINT16  Number;
  UINT16  CharC;
  volatile UINT16  Index;
  UINT32  Mask;

  //
  // Read Extra Set Code Length Array size 
  //
  Number = (UINT16) GetBits (Sd, nbit);

  if ((Number > sizeof (Sd->mPTLen)) || (nn > sizeof (Sd->mPTLen))) {
    //
    // Fail if Number or nn is greater than size of mPTLen
    //
    return (UINT16) BAD_TABLE;
  }

  if (Number == 0) {
    //
    // This represents only Huffman code used
    //
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

    //
    // If a code length is less than 7, then it is encoded as a 3-bit
    // value. Or it is encoded as a series of "1"s followed by a 
    // terminating "0". The number of "1"s = Code length - 4.
    //
    if (CharC == 7) {
      Mask = 1U << (BITBUFSIZ - 1 - 3);
      while (Mask & Sd->mBitBuf) {
        Mask >>= 1;
        CharC += 1;
      }
    }
    
    FillBuf (Sd, (UINT16) ((CharC < 7) ? 3 : CharC - 3));

    Sd->mPTLen[Index++] = (UINT8) CharC;
 
    //
    // For Code&Len Set, 
    // After the third length of the code length concatenation,
    // a 2-bit value is used to indicated the number of consecutive 
    // zero lengths after the third length.
    //
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

/**
  Reads code lengths for Char&Len Set.
  
  Read in and decode the Char&Len Set Code Length Array, then
  generate the Huffman Code mapping table for the Char&Len Set.

  @param  Sd the global scratch data

**/
VOID
GlueReadCLen (
  SCRATCH_DATA  *Sd
  )
{
  UINT16           Number;
  UINT16           CharC;
  volatile UINT16  Index;
  UINT32           Mask;

  Number = (UINT16) GetBits (Sd, CBIT);

  if (Number == 0) {
    //
    // This represents only Huffman code used
    //
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

/**
  Decode a character/length value.
  
  Read one value from mBitBuf, Get one code from mBitBuf. If it is at block boundary, generates
  Huffman code mapping table for Extra Set, Code&Len Set and
  Position Set.

  @param  Sd The global scratch data.

  @return The value decoded.

**/
UINT16
GlueDecodeC (
  SCRATCH_DATA  *Sd
  )
{
  UINT16  Index2;
  UINT32  Mask;

  if (Sd->mBlockSize == 0) {
    //
    // Starting a new block
    // Read BlockSize from block header
    // 
    Sd->mBlockSize    = (UINT16) GetBits (Sd, 16);

    //
    // Read in the Extra Set Code Length Arrary,
    // Generate the Huffman code mapping table for Extra Set.
    //
    Sd->mBadTableFlag = ReadPTLen (Sd, NT, TBIT, 3);
    if (Sd->mBadTableFlag != 0) {
      return 0;
    }

    //
    // Read in and decode the Char&Len Set Code Length Arrary,
    // Generate the Huffman code mapping table for Char&Len Set.
    //
    ReadCLen (Sd);

    //
    // Read in the Position Set Code Length Arrary, 
    // Generate the Huffman code mapping table for the Position Set.
    //
    Sd->mBadTableFlag = ReadPTLen (Sd, MAXNP, Sd->mPBit, (UINT16) (-1));
    if (Sd->mBadTableFlag != 0) {
      return 0;
    }
  }

  //
  // Get one code according to Code&Set Huffman Table
  //
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

/**
  Decode the source data and put the resulting data into the destination buffer.

  Decode the source data and put the resulting data into the destination buffer.
  
  @param  Sd The global scratch data

**/
VOID
GlueDecode (
  SCRATCH_DATA  *Sd
  )
{
  UINT16  BytesRemain;
  UINT32  DataIdx;
  UINT16  CharC;

  BytesRemain = (UINT16) (-1);

  DataIdx     = 0;

  for (;;) {
    //
    // Get one code from mBitBuf
    // 
    CharC = DecodeC (Sd);
    if (Sd->mBadTableFlag != 0) {
      goto Done;
    }

    if (CharC < 256) {
      //
      // Process an Original character
      //
      if (Sd->mOutBuf >= Sd->mOrigSize) {
        goto Done;
      } else {
        //
        // Write orignal character into mDstBase
        //
        Sd->mDstBase[Sd->mOutBuf++] = (UINT8) CharC;
      }

    } else {
      //
      // Process a Pointer
      //
      CharC       = (UINT16) (CharC - (UINT8_MAX + 1 - THRESHOLD));
 
      //
      // Get string length
      //
      BytesRemain = CharC;

      //
      // Locate string position
      //
      DataIdx     = Sd->mOutBuf - DecodeP (Sd) - 1;

      //
      // Write BytesRemain of bytes into mDstBase
      //
      BytesRemain--;
      while ((INT16) (BytesRemain) >= 0) {
        Sd->mDstBase[Sd->mOutBuf++] = Sd->mDstBase[DataIdx++];
        if (Sd->mOutBuf >= Sd->mOrigSize) {
          goto Done;
        }

        BytesRemain--;
      }
    }
  }

Done:
  return ;
}

/**
  Retrieves the size of the uncompressed buffer and the size of the scratch buffer.

  Retrieves the size of the uncompressed buffer and the temporary scratch buffer 
  required to decompress the buffer specified by Source and SourceSize.
  If the size of the uncompressed buffer or the size of the scratch buffer cannot
  be determined from the compressed data specified by Source and SourceData, 
  then RETURN_INVALID_PARAMETER is returned.  Otherwise, the size of the uncompressed
  buffer is returned in DestinationSize, the size of the scratch buffer is returned
  in ScratchSize, and RETURN_SUCCESS is returned.
  This function does not have scratch buffer available to perform a thorough 
  checking of the validity of the source data.  It just retrieves the "Original Size"
  field from the beginning bytes of the source data and output it as DestinationSize.
  And ScratchSize is specific to the decompression implementation.

  If Source is NULL, then ASSERT().
  If DestinationSize is NULL, then ASSERT().
  If ScratchSize is NULL, then ASSERT().

  @param  Source          The source buffer containing the compressed data.
  @param  SourceSize      The size, in bytes, of the source buffer.
  @param  DestinationSize A pointer to the size, in bytes, of the uncompressed buffer
                          that will be generated when the compressed buffer specified
                          by Source and SourceSize is decompressed..
  @param  ScratchSize     A pointer to the size, in bytes, of the scratch buffer that
                          is required to decompress the compressed buffer specified 
                          by Source and SourceSize.

  @retval  RETURN_SUCCESS The size of destination buffer and the size of scratch 
                          buffer are successull retrieved.
  @retval  RETURN_INVALID_PARAMETER The source data is corrupted

**/
RETURN_STATUS
EFIAPI
UefiDecompressGetInfo (
  IN  CONST VOID  *Source,
  IN  UINT32      SourceSize,
  OUT UINT32      *DestinationSize,
  OUT UINT32      *ScratchSize
  )
{
  UINT32  CompressedSize;

  ASSERT (Source != NULL);
  ASSERT (DestinationSize != NULL);
  ASSERT (ScratchSize != NULL);

  *ScratchSize  = sizeof (SCRATCH_DATA);

  if (SourceSize < 8) {
    return RETURN_INVALID_PARAMETER;
  }

  CopyMem (&CompressedSize, Source, sizeof (UINT32));
  CopyMem (DestinationSize, (VOID *)((UINT8 *)Source + 4), sizeof (UINT32));

  if (SourceSize < (CompressedSize + 8)) {
    return RETURN_INVALID_PARAMETER;
  }

  return RETURN_SUCCESS;
}

/**
  Decompresses a compressed source buffer.

  This function is designed so that the decompression algorithm can be implemented
  without using any memory services.  As a result, this function is not allowed to
  call any memory allocation services in its implementation.  It is the caller's r
  esponsibility to allocate and free the Destination and Scratch buffers.
  If the compressed source data specified by Source is sucessfully decompressed 
  into Destination, then RETURN_SUCCESS is returned.  If the compressed source data 
  specified by Source is not in a valid compressed data format,
  then RETURN_INVALID_PARAMETER is returned.

  If Source is NULL, then ASSERT().
  If Destination is NULL, then ASSERT().
  If the required scratch buffer size > 0 and Scratch is NULL, then ASSERT().

  @param  Source      The source buffer containing the compressed data.
  @param  Destination The destination buffer to store the decompressed data
  @param  Scratch     A temporary scratch buffer that is used to perform the decompression.
                      This is an optional parameter that may be NULL if the 
                      required scratch buffer size is 0.
                     
  @retval  RETURN_SUCCESS Decompression is successfull
  @retval  RETURN_INVALID_PARAMETER The source data is corrupted

**/
RETURN_STATUS
EFIAPI
UefiDecompress (
  IN CONST VOID  *Source,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  )
{
  volatile UINT32  Index;
  UINT32           CompSize;
  UINT32           OrigSize;
  SCRATCH_DATA     *Sd;
  CONST UINT8      *Src;
  UINT8            *Dst;

  ASSERT (Source != NULL);
  ASSERT (Destination != NULL);
  ASSERT (Scratch != NULL);

  Src     = Source;
  Dst     = Destination;

  Sd = (SCRATCH_DATA *) Scratch;

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
  Sd->mPBit     = 4;
  Sd->mSrcBase  = (UINT8 *)Src;
  Sd->mDstBase  = Dst;
  //
  // CompSize and OrigSize are caculated in bytes
  //
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
