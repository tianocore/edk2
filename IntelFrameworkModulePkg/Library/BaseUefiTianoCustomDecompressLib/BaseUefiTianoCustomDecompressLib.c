/**@file
  UEFI and Custom Decompress Library 
  The function of UefiTianoDecompress() is interface for this module,
  it will do tiano or uefi decompress with different verison parameter.
  See EFI specification 1.1 Chapter 17 to get LZ77 compress/decompress.
  
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <Guid/CustomDecompress.h>
#include "BaseUefiTianoCustomDecompressLibInternals.h"

/**
  Shift mBitBuf NumOfBits left. Read in NumOfBits of bits from source.
  
  @param Sd         The global scratch data
  @param NumOfBits  The number of bits to shift and read.  
**/
VOID
FillBuf (
  IN  SCRATCH_DATA  *Sd,
  IN  UINT16        NumOfBits
  )
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
GetBits (
  IN  SCRATCH_DATA  *Sd,
  IN  UINT16        NumOfBits
  )
{
  UINT32  OutBits;

  OutBits = (UINT32) (Sd->mBitBuf >> (BITBUFSIZ - NumOfBits));

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
MakeTable (
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
  volatile UINT16  Index;
  UINT16  Len;
  UINT16  Char;
  UINT16  JuBits;
  UINT16  Avail;
  UINT16  NextCode;
  UINT16  Mask;
  UINT16  WordOfStart;
  UINT16  WordOfCount;

  for (Index = 1; Index <= 16; Index++) {
    Count[Index] = 0;
  }

  for (Index = 0; Index < NumOfChar; Index++) {
    Count[BitLen[Index]]++;
  }

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
    if (Len == 0) {
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

/**
  Decodes a position value.
  
  @param Sd      the global scratch data
  
  @return The position value decoded.
**/
UINT32
DecodeP (
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
ReadPTLen (
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

/**
  Reads code lengths for Char&Len Set.
  
  Read in and decode the Char&Len Set Code Length Array, then
  generate the Huffman Code mapping table for the Char&Len Set.

  @param  Sd the global scratch data

**/
VOID
ReadCLen (
  SCRATCH_DATA  *Sd
  )
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

/**
  Decode a character/length value.
  
  Read one value from mBitBuf, Get one code from mBitBuf. If it is at block boundary, generates
  Huffman code mapping table for Extra Set, Code&Len Set and
  Position Set.

  @param  Sd The global scratch data.

  @return The value decoded.

**/
UINT16
DecodeC (
  SCRATCH_DATA  *Sd
  )
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

/**
  Decode the source data ad put the resulting data into the destination buffer.
  
  @param Sd            - The global scratch data
**/
VOID
Decode (
  SCRATCH_DATA  *Sd
  )
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
        Sd->mDstBase[Sd->mOutBuf++] = Sd->mDstBase[DataIdx++];
        if (Sd->mOutBuf >= Sd->mOrigSize) {
          goto Done ;
        }

        BytesRemain--;
      }
    }
  }

Done:
  return ;
}

/**
  The internal implementation of *_DECOMPRESS_PROTOCOL.GetInfo().
  
  @param Source           The source buffer containing the compressed data.
  @param SourceSize       The size of source buffer
  @param DestinationSize  The size of destination buffer.
  @param ScratchSize      The size of scratch buffer.

  @retval RETURN_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  @retval RETURN_INVALID_PARAMETER - The source data is corrupted
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

  if (SourceSize < 8) {
    return RETURN_INVALID_PARAMETER;
  }

  CompressedSize   = *(UINT32 *) Source;
  if (SourceSize < (CompressedSize + 8)) {
    return RETURN_INVALID_PARAMETER;
  }

  *ScratchSize  = sizeof (SCRATCH_DATA);
  *DestinationSize = *((UINT32 *) Source + 1);

  return RETURN_SUCCESS;
}

/**
  The internal implementation of *_DECOMPRESS_PROTOCOL.Decompress().

  @param Source           The source buffer containing the compressed data.
  @param Destination      The destination buffer to store the decompressed data
  @param Scratch          The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.
  @param Version          1 for UEFI Decompress algoruthm, 2 for Tiano Decompess algorithm

  @retval RETURN_SUCCESS            Decompression is successfull
  @retval RETURN_INVALID_PARAMETER  The source data is corrupted
**/
RETURN_STATUS
EFIAPI
UefiTianoDecompress (
  IN CONST VOID  *Source,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch,
  IN UINT32      Version
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
  // For UEFI 2.0 de/compression algorithm(Version 1), mPBit = 4
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
      ASSERT (FALSE);
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

/**
  The internal implementation of *_DECOMPRESS_PROTOCOL.Decompress().
  
  @param Source          - The source buffer containing the compressed data.
  @param Destination     - The destination buffer to store the decompressed data
  @param Scratch         - The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.

  @retval RETURN_SUCCESS           - Decompression is successfull
  @retval RETURN_INVALID_PARAMETER - The source data is corrupted  
**/
RETURN_STATUS
EFIAPI
UefiDecompress (
  IN CONST VOID  *Source,
  IN OUT VOID    *Destination,
  IN OUT VOID    *Scratch
  )
{
  return UefiTianoDecompress (Source, Destination, Scratch, 1);
}

/**
  The internal implementation of Tiano decompress GetInfo.

  @param InputSection          Buffer containing the input GUIDed section to be processed. 
  @param OutputBufferSize      The size of OutputBuffer.
  @param ScratchBufferSize     The size of ScratchBuffer.
  @param SectionAttribute      The attribute of the input guided section.

  @retval RETURN_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  @retval RETURN_INVALID_PARAMETER - The source data is corrupted
                             The GUID in InputSection does not match this instance guid.
**/
RETURN_STATUS
EFIAPI
TianoDecompressGetInfo (
  IN  CONST VOID  *InputSection,
  OUT UINT32      *OutputBufferSize,
  OUT UINT32      *ScratchBufferSize,
  OUT UINT16      *SectionAttribute
  )

{
  ASSERT (SectionAttribute != NULL);

  if (InputSection == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if (!CompareGuid (
        &gTianoCustomDecompressGuid, 
        &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid))) {
    return RETURN_INVALID_PARAMETER;
  }
  //
  // Get guid attribute of guid section. 
  //
  *SectionAttribute = ((EFI_GUID_DEFINED_SECTION *) InputSection)->Attributes;

  //
  // Call Tiano GetInfo to get the required size info.
  //
  return UefiDecompressGetInfo (
          (UINT8 *) InputSection + ((EFI_GUID_DEFINED_SECTION *) InputSection)->DataOffset,
          (*(UINT32 *) (((EFI_COMMON_SECTION_HEADER *) InputSection)->Size) & 0x00ffffff) - ((EFI_GUID_DEFINED_SECTION *) InputSection)->DataOffset,
          OutputBufferSize,
          ScratchBufferSize
         );
}

/**
  The implementation of Tiano Decompress().

  @param InputSection           Buffer containing the input GUIDed section to be processed. 
  @param OutputBuffer           OutputBuffer to point to the start of the section's contents.
                                if guided data is not prcessed. Otherwise,
                                OutputBuffer to contain the output data, which is allocated by the caller.
  @param ScratchBuffer          A pointer to a caller-allocated buffer for function internal use. 
  @param AuthenticationStatus   A pointer to a caller-allocated UINT32 that indicates the
                                authentication status of the output buffer. 

  @retval RETURN_SUCCESS            Decompression is successfull
  @retval RETURN_INVALID_PARAMETER  The source data is corrupted, or
                                    The GUID in InputSection does not match this instance guid.

**/
RETURN_STATUS
EFIAPI
TianoDecompress (
  IN CONST  VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  IN        VOID    *ScratchBuffer,        OPTIONAL
  OUT       UINT32  *AuthenticationStatus
  )
{
  ASSERT (OutputBuffer != NULL);

  if (InputSection == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if (!CompareGuid (
        &gTianoCustomDecompressGuid, 
        &(((EFI_GUID_DEFINED_SECTION *) InputSection)->SectionDefinitionGuid))) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Set Authentication to Zero.
  //
  *AuthenticationStatus = 0;
  
  //
  // Call Tiano Decompress to get the raw data
  //
  return UefiTianoDecompress (
          (UINT8 *) InputSection + ((EFI_GUID_DEFINED_SECTION *) InputSection)->DataOffset,
          *OutputBuffer,
          ScratchBuffer,
          2
         );
}

/**
  Register TianoDecompress handler.

  @retval  RETURN_SUCCESS            Register successfully.
  @retval  RETURN_OUT_OF_RESOURCES   No enough memory to store this handler.
**/
EFI_STATUS
EFIAPI
TianoDecompressLibConstructor (
)
{
  return ExtractGuidedSectionRegisterHandlers (
          &gTianoCustomDecompressGuid,
          TianoDecompressGetInfo,
          TianoDecompress
          );      
}
