/** @file
  BitStream utility.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>

#include "BitStream.h"

/** Check whether a BitStream is NULL (null length).

  @param [in] Stream  The BitStream.

  @retval TRUE if the BitStream is NULL (null length).
  @retval FALSE otherwise.
**/
BOOLEAN
EFIAPI
IsBitStreamEmpty (
  IN  BIT_STREAM  *Stream
  )
{
  return ((Stream == NULL)       ||
          (Stream->BitLen == 0)  ||
          (Stream->Data == NULL));
}

/** Convert bits to bytes (rounds down).

  @param [in] Bits    Bits.

  @return Bytes.
**/
UINTN
EFIAPI
BitsToLowerBytes (
  IN  UINTN  Bits
  )
{
  return Bits >> 3;
}

/** Convert bits to bytes (rounds up).

  @param [in] Bits    Bits.

  @return Bytes.
**/
UINTN
EFIAPI
BitsToUpperBytes (
  IN  UINTN  Bits
  )
{
  return ((Bits + 0x7) >> 3);
}

/** Get the BitStream length (in bits).

  @param [in] Stream    The BitStream.

  @return Length of the BitStream (in bits).
**/
UINTN
EFIAPI
BitStreamBitLen (
  IN  BIT_STREAM  *Stream
  )
{
  if (Stream == NULL) {
    ASSERT (Stream != NULL);
    return 0;
  }

  return Stream->BitLen;
}

/** Get the BitStream length (in bytes).

  @param [in] Stream    The BitStream.

  @return Length of the BitStream (in bytes).
**/
UINTN
EFIAPI
BitStreamByteLen (
  IN  BIT_STREAM  *Stream
  )
{
  if (Stream == NULL) {
    ASSERT (Stream != NULL);
    return 0;
  }

  return Stream->ByteLen;
}

/** Get the BitStream data buffer.

  @param [in] Stream    The BitStream.

  @return Data buffer of the BitStream (can be NULL).
**/
UINT8 *
EFIAPI
BitStreamData (
  IN  BIT_STREAM  *Stream
  )
{
  if (Stream == NULL) {
    ASSERT (Stream != NULL);
    return 0;
  }

  return Stream->Data;
}

/** Clear the unsused bits of a Stream.

  For instance, if a stream is 5 bits long, then:
  - bits[7:5] must be cleared.
  - bits[4:0] must be preserved.

  @param [in, out]  Stream    The BitStream.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
BitStreamClearUnusedBits (
  IN  OUT BIT_STREAM  *Stream
  )
{
  UINT8  UsedBits;

  if (IsBitStreamEmpty (Stream)) {
    ASSERT (!IsBitStreamEmpty (Stream));
    return EFI_INVALID_PARAMETER;
  }

  // Clear the unsused bits of the Stream.
  // BitStream are big-endian, so MSByte is at index 0.
  UsedBits = Stream->BitLen & 0x7;
  if (UsedBits != 0) {
    Stream->Data[0] &= (0XFF >> (8 - UsedBits));
  }

  return EFI_SUCCESS;
}

/** Allocate a buffer of BitLen (bits) for BitStream.

  @param [in]   BitLen    Length of the buffer to allocate (in bits).
  @param [out]  Stream    The BitStream.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
BitStreamShallowAlloc (
  IN  UINTN       BitLen,
  OUT BIT_STREAM  *Stream
  )
{
  UINTN  ByteLen;

  if (Stream == NULL) {
    ASSERT (Stream != NULL);
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Stream, sizeof (BIT_STREAM));

  if (BitLen == 0) {
    return EFI_SUCCESS;
  }

  ByteLen      = BitsToUpperBytes (BitLen);
  Stream->Data = (UINT8 *)AllocateZeroPool (ByteLen);
  if (Stream->Data == NULL) {
    ASSERT (Stream->Data != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  Stream->BitLen  = BitLen;
  Stream->ByteLen = ByteLen;

  return EFI_SUCCESS;
}

/** Allocate a BitStream of BitLen (bits).

  @param [in]   BitLen    Length of the BitStream (in bits).
  @param [out]  Stream    The BitStream to allocate.
                          Must be NULL initialized.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BitStreamAlloc (
  IN  UINTN       BitLen,
  OUT BIT_STREAM  **Stream
  )
{
  EFI_STATUS  Status;
  BIT_STREAM  *LocStream;

  // Non NULL initialized pointers are considered invalid.
  if ((Stream == NULL)  ||
      (*Stream != NULL))
  {
    ASSERT (Stream != NULL);
    ASSERT (*Stream == NULL);
    return EFI_INVALID_PARAMETER;
  }

  LocStream = AllocateZeroPool (sizeof (BIT_STREAM));
  if (LocStream == NULL) {
    ASSERT (LocStream != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = BitStreamShallowAlloc (BitLen, LocStream);
  if (EFI_ERROR (Status)) {
    FreePool (LocStream);
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  *Stream = LocStream;

  return EFI_SUCCESS;
}

/** Free the buffer of the BitStream.

  This is a shallow free, so the BitStream structure itself if not freed.

  @param [in, out]  Stream  BitStream to free the buffer of.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
BitStreamShallowFree (
  IN  OUT BIT_STREAM  *Stream
  )
{
  if (Stream == NULL) {
    ASSERT (Stream != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (Stream->Data != NULL) {
    FreePool (Stream->Data);
  }

  ZeroMem (Stream, sizeof (BIT_STREAM));

  return EFI_SUCCESS;
}

/** Free a BitStream.

  @param [in, out]  Stream    BitStream to free.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
BitStreamFree (
  IN  OUT BIT_STREAM  **Stream
  )
{
  if (Stream == NULL) {
    ASSERT (Stream != NULL);
    return EFI_INVALID_PARAMETER;
  }

  BitStreamShallowFree (*Stream);
  FreePool (*Stream);
  *Stream = NULL;

  return EFI_SUCCESS;
}

/** Initialize a BitStream with a buffer.

  The input Buffer is copied to a BitStream buffer.

  @param [in]   Buffer    Buffer to init the Data of the BitStream with.
                          The Buffer must be big-endian (MSByte at index 0).
  @param [in]   BitLen    Length of the Buffer (in bits).
  @param [out]  Stream    BitStream to initialize.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BitStreamInit (
  IN  CONST UINT8       *Buffer,
  IN        UINTN       BitLen,
  OUT       BIT_STREAM  **Stream
  )
{
  EFI_STATUS  Status;

  // Non NULL initialized pointers are considered invalid.
  if ((Stream == NULL)    ||
      (*Stream != NULL)   ||
      ((Buffer == NULL) ^ (BitLen == 0)))
  {
    ASSERT (Stream != NULL);
    ASSERT (*Stream == NULL);
    ASSERT (!((Buffer == NULL) ^ (BitLen == 0)));
    return EFI_INVALID_PARAMETER;
  }

  Status = BitStreamAlloc (BitLen, Stream);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // BitStream are big-endian, i.e. MSByte is at index 0,
  // so just copy the input Buffer.
  CopyMem ((*Stream)->Data, Buffer, (*Stream)->ByteLen);

  return EFI_SUCCESS;
}

/** Fill a Buffer with a Stream Data.

  The Buffer will be big-endian.

  @param [in]   Stream  Stream to take the Data from.
  @param [out]  Buffer  Buffer where to write the Data.
                        Must be at least BitStreamByteLen (Stream)
                        bytes long.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
BitStreamToBuffer (
  IN  BIT_STREAM  *Stream,
  OUT UINT8       *Buffer
  )
{
  if (IsBitStreamEmpty (Stream)  ||
      (Buffer == NULL))
  {
    ASSERT (!IsBitStreamEmpty (Stream));
    ASSERT (Buffer != NULL);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (Buffer, Stream->Data, Stream->ByteLen);

  return EFI_SUCCESS;
}

/** Shallow clone a BitStream.

  @param [out]  StreamDest    Shallow cloned BiStream.
  @param [in]   StreamSrc     Source BitStream to shallow clone.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
BitStreamShallowClone (
  OUT BIT_STREAM  *StreamDest,
  IN  BIT_STREAM  *StreamSrc
  )
{
  if ((StreamDest == NULL)  ||
      (StreamSrc == NULL))
  {
    ASSERT (StreamDest != NULL);
    ASSERT (StreamSrc != NULL);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (StreamDest, StreamSrc, sizeof (BIT_STREAM));
  return EFI_SUCCESS;
}

/** Clone a BitStream.

  @param [out]  StreamDest    Cloned BiStream.
  @param [in]   StreamSrc     Source BitStream to clone.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BitStreamClone (
  OUT BIT_STREAM  **StreamDest,
  IN  BIT_STREAM  *StreamSrc
  )
{
  EFI_STATUS  Status;

  if ((StreamDest == NULL)  ||
      (StreamSrc == NULL))
  {
    ASSERT (StreamDest != NULL);
    ASSERT (StreamSrc != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = BitStreamInit (StreamSrc->Data, StreamSrc->BitLen, StreamDest);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    // Fall through.
  }

  return Status;
}

/** Replace an initialized BitStream with another.

  This function frees StreamRepl's Data if success.

  @param [in, out]  StreamRepl    Stream whose content is replaced.
  @param [in]       StreamData    Stream containing the Data to use.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BitStreamReplace (
  IN  OUT BIT_STREAM  *StreamRepl,
  IN      BIT_STREAM  *StreamData
  )
{
  EFI_STATUS  Status;

  if ((StreamRepl == NULL)  ||
      (StreamData == NULL))
  {
    ASSERT (StreamRepl != NULL);
    ASSERT (StreamData != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status = BitStreamShallowFree (StreamRepl);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = BitStreamShallowClone (StreamRepl, StreamData);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    // StreamRepl's content was freed. Can't do much now.
    // Fall through.
  }

  return Status;
}

/** Write a buffer to a BitStream.

  @param [in]       Buffer          Buffer to write to the BitStream.
                                    Buffer is big-endian.
  @param [in]       StartBitIndex   Bit index to start writing from.
  @param [in]       BitCount        Count of bits to write.
  @param [in, out]  Stream          BitStream to write to.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BitStreamWrite (
  IN      UINT8       *Buffer,
  IN      UINTN       StartBitIndex,
  IN      UINTN       BitCount,
  IN  OUT BIT_STREAM  *Stream
  )
{
  UINTN  EndByteIndex;
  UINTN  EndBitIndex;
  UINT8  StartBitRemainder;
  UINT8  EndBitRemainder;
  UINT8  *Data;
  UINT8  ShiftR;
  UINT8  ShiftL;
  UINT8  BitMask;
  UINTN  ByteCount;

  if (((Buffer != NULL) ^ (BitCount != 0))    ||
      IsBitStreamEmpty (Stream)                ||
      (StartBitIndex > (MAX_UINTN - BitCount))  ||
      ((StartBitIndex + BitCount) > Stream->BitLen))
  {
    ASSERT (!((Buffer != NULL) ^ (BitCount != 0)));
    ASSERT (!IsBitStreamEmpty (Stream));
    ASSERT (StartBitIndex <= (MAX_UINTN - BitCount));
    ASSERT ((StartBitIndex + BitCount) <= Stream->BitLen);
    return EFI_INVALID_PARAMETER;
  }

  ByteCount = BitsToUpperBytes (BitCount);

  if (BitCount == 0) {
    // Nothing to do.
    return EFI_SUCCESS;
  }

  EndByteIndex = Stream->ByteLen - 1 -
                 BitsToLowerBytes (StartBitIndex + BitCount - 1);
  EndBitIndex = StartBitIndex + BitCount;
  Data        = &Stream->Data[EndByteIndex];

  StartBitRemainder = StartBitIndex & 0x7;
  EndBitRemainder   = EndBitIndex & 0x7;

  ShiftL = StartBitRemainder;
  ShiftR = 8 - ShiftL;

  // BitCount might not be a multiple of 8. These MsBits can also
  // be spread on 2 bytes (in StreamIn).
  if ((StartBitRemainder < EndBitRemainder) || (EndBitRemainder == 0)) {
    BitMask = 0xFF << StartBitRemainder;
    if (EndBitRemainder != 0) {
      BitMask ^= 0xFF << EndBitRemainder;
    }

    *Data = (*Data & ~BitMask) | ((*Buffer++ << ShiftL) & BitMask);

    ByteCount--;
    BitCount -= (BitCount & 0x7);

    if (StartBitRemainder == 0) {
      Data++;
    }
  } else if (StartBitRemainder > EndBitRemainder) {
    BitMask = ~(0xFF << EndBitRemainder);
    *Data   = (*Data & ~BitMask) | ((*Buffer >> ShiftR) & BitMask);
    Data++;

    BitMask = 0xFF << StartBitRemainder;
    *Data   = (*Data & ~BitMask) | ((*Buffer++ << ShiftL) & BitMask);

    ByteCount--;
    BitCount -= (ShiftR + 8 - EndBitRemainder);
  }

  // else (StartBitRemainder == EndBitRemainder), nothing to do

  // From here, (BitCount % 8) == 0 so we copy whole bytes from the Buffer.
  // It doesn't mean we are byte-aligned, so check if the alignment of
  // StartBitIndex.
  if (ShiftL == 0) {
    // StartBitIndex is byte aligned,
    if (ByteCount) {
      CopyMem (Data, Buffer, ByteCount);
    }
  } else {
    // StartBitIndex is not byte aligned.
    BitMask = 0xFF << (StartBitIndex & 0x7);
    while (ByteCount--) {
      *Data = (*Data & BitMask) | ((*Buffer >> ShiftR) & ~BitMask);
      Data++;
      *Data = (*Data & ~BitMask) | ((*Buffer++ << ShiftL) & BitMask);
    }
  }

  return EFI_SUCCESS;
}

/** XoR two BitStreams.

  We must have len(StremA) = len(StreamB)

  @param [in]  StreamA   BitStream A.
  @param [in]  StreamB   BitStream B.
  @param [out] StreamOut Output BitStream.
                         Can be *StreamA or *StreamB to replace their
                         content with the new stream.
                         Otherwise, it is initialized and contains the
                         resulting stream.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BitStreamXor (
  IN  BIT_STREAM  *StreamA,
  IN  BIT_STREAM  *StreamB,
  OUT BIT_STREAM  **StreamOut
  )
{
  EFI_STATUS  Status;
  BIT_STREAM  *LocStream;
  UINTN       ByteLen;
  UINTN       *DataLocN;
  UINT8       *DataLoc8;
  UINTN       *DataAN;
  UINT8       *DataA8;
  UINTN       *DataBN;
  UINT8       *DataB8;
  UINTN       ByteLenN;
  UINTN       ByteLen8;
  UINTN       Offset;

  if ((StreamA == NULL)     ||
      (StreamB == NULL)     ||
      (StreamOut == NULL)   ||
      (StreamA->BitLen != StreamB->BitLen))
  {
    ASSERT (StreamA != NULL);
    ASSERT (StreamB != NULL);
    ASSERT (StreamOut != NULL);
    ASSERT (StreamA->BitLen == StreamB->BitLen);
    return EFI_INVALID_PARAMETER;
  }

  if (StreamB->BitLen == 0) {
    // Nothing to do.
    return EFI_SUCCESS;
  }

  if (*StreamOut == StreamA) {
    LocStream = StreamA;
  } else if (*StreamOut == StreamB) {
    LocStream = StreamB;
  } else {
    LocStream = NULL;

    Status = BitStreamAlloc (StreamA->BitLen, &LocStream);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  ByteLen = StreamA->ByteLen;

  DataLocN = (UINTN *)LocStream->Data;
  DataAN   = (UINTN *)StreamA->Data;
  DataBN   = (UINTN *)StreamB->Data;
  // Speed up the process by taking chunks of UINTN instead of UINT8.
  // ((sizeof (UINTN) >> 2) + 1) is 3 or 2 depending on the size of UINTN.
  ByteLenN = ByteLen >> (((sizeof (UINTN) >> 2) + 1));
  ByteLen8 = ByteLen & ((sizeof (UINTN) - 1));
  while (ByteLenN-- > 0) {
    DataLocN[ByteLenN] = DataAN[ByteLenN] ^ DataBN[ByteLenN];
  }

  // XOR remaining UINT8 chunks.
  if (ByteLen8 != 0) {
    Offset   = ByteLen - ByteLen8;
    DataLoc8 = (UINT8 *)DataLocN + Offset;
    DataA8   = (UINT8 *)DataAN + Offset;
    DataB8   = (UINT8 *)DataBN + Offset;
    while (ByteLen8-- > 0) {
      DataLoc8[ByteLen8] = DataA8[ByteLen8] ^ DataB8[ByteLen8];
    }
  }

  *StreamOut = LocStream;
  return EFI_SUCCESS;
}

/** Concatenate two BitStreams.

  @param [in]  StreamHigh  BitStream containing the MSBytes.
  @param [in]  StreamLow   BitStream containing the LSBytes.
  @param [out] StreamOut   Output BitStream.
                           Can be *StreamHigh or *StreamLow to replace their
                           content with the new concatenated stream.
                           Otherwise, it is initialized and contains the
                           resulting stream.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BitStreamConcat (
  IN  BIT_STREAM  *StreamHigh,
  IN  BIT_STREAM  *StreamLow,
  OUT BIT_STREAM  **StreamOut
  )
{
  EFI_STATUS  Status;
  BIT_STREAM  LocStream;
  UINTN       TotalBitLen;

  if ((StreamHigh == NULL)  ||
      (StreamLow == NULL)   ||
      (StreamOut == NULL))
  {
    ASSERT (StreamHigh != NULL);
    ASSERT (StreamLow != NULL);
    ASSERT (StreamOut != NULL);
    return EFI_INVALID_PARAMETER;
  }

  TotalBitLen = StreamHigh->BitLen + StreamLow->BitLen;

  Status = BitStreamShallowAlloc (TotalBitLen, &LocStream);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Write StreamLow data.
  Status = BitStreamWrite (
             StreamLow->Data,
             0,
             StreamLow->BitLen,
             &LocStream
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
  }

  // Write StreamHigh data.
  Status = BitStreamWrite (
             StreamHigh->Data,
             StreamLow->BitLen,
             StreamHigh->BitLen,
             &LocStream
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
  }

  if (*StreamOut == StreamHigh) {
    Status = BitStreamReplace (StreamHigh, &LocStream);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ErrorHandler;
    }
  } else if (*StreamOut == StreamLow) {
    Status = BitStreamReplace (StreamLow, &LocStream);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ErrorHandler;
    }
  } else {
    Status = BitStreamClone (StreamOut, &LocStream);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ErrorHandler;
    }
  }

  return Status;

ErrorHandler:
  BitStreamShallowFree (&LocStream);
  return Status;
}

/** Select bits in a BitStream.

  @param [in]   StreamIn        Input BitStream.
  @param [in]   StartBitIndex   Bit index to start the copy from.
  @param [in]   BitCount        Count of bits to copy.
  @param [out]  StreamOut       Output BitStream.
                                Can be *StreamIn to replace its
                                content with the new concatenated stream.
                                Otherwise, it is initialized and contains the
                                resulting stream.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BitStreamSelect (
  IN  BIT_STREAM  *StreamIn,
  IN  UINTN       StartBitIndex,
  IN  UINTN       BitCount,
  OUT BIT_STREAM  **StreamOut
  )
{
  EFI_STATUS  Status;
  BIT_STREAM  StreamDest;
  UINTN       ByteCount;
  UINTN       EndByteIndex;
  UINTN       EndBitCountRemainder;
  UINT8       *DataDest;
  UINT8       *DataIn;
  UINT8       ShiftR;
  UINT8       ShiftL;

  if ((StreamIn == NULL)                              ||
      (StartBitIndex > (MAX_UINTN - BitCount))        ||
      ((StartBitIndex + BitCount) > StreamIn->BitLen) ||
      (StreamOut == NULL))
  {
    ASSERT (StreamIn != NULL);
    ASSERT (StartBitIndex <= (MAX_UINTN - BitCount));
    ASSERT ((StartBitIndex + BitCount) <= StreamIn->BitLen);
    ASSERT (StreamOut != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (BitCount == 0) {
    if (*StreamOut == StreamIn) {
      Status = BitStreamShallowAlloc (0, &StreamDest);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      Status = BitStreamReplace (StreamIn, &StreamDest);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        BitStreamShallowFree (&StreamDest);
        // Fall through.
      }
    } else {
      Status = BitStreamAlloc (0, StreamOut);
      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
        // Fall through.
      }
    }

    return Status;
  }

  ByteCount = BitsToUpperBytes (BitCount);

  // Alloc StreamDest.
  Status = BitStreamShallowAlloc (BitCount, &StreamDest);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  DataDest = StreamDest.Data;

  EndByteIndex = StreamIn->ByteLen - 1 -
                 BitsToLowerBytes (StartBitIndex + BitCount - 1);
  DataIn = &StreamIn->Data[EndByteIndex];

  ShiftR = StartBitIndex & 0x7;
  if (ShiftR == 0) {
    // StartBitIndex is byte aligned.
    CopyMem (DataDest, DataIn, ByteCount);
    // Unused bits are cleared later.
  } else {
    // StartBitIndex is not byte aligned.
    ShiftL = 8 - ShiftR;

    // BitCount might not be a multiple of 8. These MsBits can also
    // be spread on 2 bytes (in StreamIn).
    EndBitCountRemainder = BitCount & 0x7;
    if ((ShiftR <= EndBitCountRemainder) || (EndBitCountRemainder == 0)) {
      *DataDest |= (*DataIn++ << ShiftL);
    }

    *DataDest++ |= (*DataIn >> ShiftR);
    ByteCount   -= 1;

    // Then copy 8 bits chunks.
    while (ByteCount--) {
      *DataDest   |= (*DataIn++ << ShiftL);
      *DataDest++ |= (*DataIn >> ShiftR);
    }
  }

  Status = BitStreamClearUnusedBits (&StreamDest);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorHandler;
  }

  if (*StreamOut == StreamIn) {
    Status = BitStreamReplace (StreamIn, &StreamDest);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ErrorHandler;
    }
  } else {
    Status = BitStreamClone (StreamOut, &StreamDest);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      // Fall through.
    }
  }

  return Status;

ErrorHandler:
  BitStreamShallowFree (&StreamDest);
  return Status;
}

/** Get leftmost (i.e. MSBytes) BitLen bits of a BitStream.

  @param [in]  StreamIn    Input BitStream.
  @param [in]  BitCount    Count of leftmost bits to copy.
  @param [out] StreamOut   Output BitStream.
                           Can be *StreamIn to replace its
                           content with the new concatenated stream.
                           Otherwise, it is initialized and contains the
                           resulting stream.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BitStreamLeftmost (
  IN  BIT_STREAM  *StreamIn,
  IN  UINTN       BitCount,
  OUT BIT_STREAM  **StreamOut
  )
{
  if ((StreamIn == NULL)    ||
      (BitCount > StreamIn->BitLen))
  {
    ASSERT (StreamIn != NULL);
    ASSERT (BitCount <= StreamIn->BitLen);
    return EFI_INVALID_PARAMETER;
  }

  return BitStreamSelect (
           StreamIn,
           StreamIn->BitLen - BitCount,
           BitCount,
           StreamOut
           );
}

/** Get rightmost (i.e. LSBytes) BitLen bits of a BitStream.

  @param [in]  StreamIn    Input BitStream.
  @param [in]  BitCount    Count of righttmost bits to copy.
  @param [out] StreamOut   Output BitStream.
                           Can be *StreamIn to replace its
                           content with the new concatenated stream.
                           Otherwise, it is initialized and contains the
                           resulting stream.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BitStreamRightmost (
  IN  BIT_STREAM  *StreamIn,
  IN  UINTN       BitCount,
  OUT BIT_STREAM  **StreamOut
  )
{
  return BitStreamSelect (StreamIn, 0, BitCount, StreamOut);
}

/** Add a value modulo 2^n to a BitStream.

  @param [in]       Val       Value to add.
  @param [in]       Modulo    Modulo of the addition (2^Modulo).
  @param [in, out]  Stream    BitStream where the addition happens.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BitStreamAddModulo (
  IN      UINTN       Val,
  IN      UINTN       Modulo,
  IN  OUT BIT_STREAM  *Stream
  )
{
  UINTN  Index;
  UINTN  ByteLen;
  UINT8  Carry;
  UINT8  Trunc8Val;
  UINT8  *Data;

  if (IsBitStreamEmpty (Stream)   ||
      (Stream->BitLen != Modulo))
  {
    // Additions with BitLen != Modulo are not handled for now.
    ASSERT (!IsBitStreamEmpty (Stream));
    ASSERT (Stream->BitLen == Modulo);
    return EFI_INVALID_PARAMETER;
  }

  Index   = 0;
  ByteLen = Stream->ByteLen;
  while ((Val != 0) && (Index < ByteLen)) {
    Data = &Stream->Data[ByteLen - Index - 1];

    // Only check for 8-bits overflow. If we exit the loop with Carry == 1,
    // then this was an actual addition modulo X.
    // [1-7]-bits overflows are cleared with BitStreamClearUnusedBits().
    Trunc8Val = (UINT8)Val;
    Carry     = (*Data > MAX_UINT8 - Trunc8Val) ? 1 : 0;
    *Data    += Trunc8Val;

    Val >>= 8;
    Val  += Carry;

    Index++;
  }

  return BitStreamClearUnusedBits (Stream);
}

/** Print a BitStream.

  @param [in]   Stream    Stream to print.
**/
VOID
EFIAPI
BitStreamPrint (
  IN  BIT_STREAM  *Stream
  )
{
  UINTN  ByteLen;
  UINTN  Index;
  UINT8  HeadBits;
  UINT8  Data;
  UINT8  Bit;

  if (Stream == NULL) {
    ASSERT (Stream != NULL);
    return;
  }

  HeadBits = Stream->BitLen & 0x7;
  ByteLen  = Stream->ByteLen;
  Print (L"BitStream(%lu): {\n", Stream->BitLen);
  Print (L"  [Index]     < 7 6 5 4 3 2 1 0 >\n");

  if (Stream->BitLen == 0) {
    return;
  }

  // Print most significant byte.
  Data = Stream->Data[0];

  Print (L"  [%02lu] (0x%02x) <", ByteLen - 1, Data);
  for (Index = 7; Index >= 0; Index--) {
    if ((Index >= HeadBits) && (HeadBits != 0)) {
      Bit = 'x';
    } else {
      Bit = '0' + ((Data & (1 << Index)) >> Index);
    }

    Print (L" %c", Bit);
  }

  Print (L" >\n");

  // Print other bytes.
  for (Index = 1; Index < ByteLen; Index++) {
    Data = Stream->Data[Index];
    Print (
      L"  [%02ld] (0x%02x) < %d %d %d %d %d %d %d %d >\n",
      ByteLen - Index - 1,
      Data,
      Data >> 7,
      (Data & 0x40) >> 6,
      (Data & 0x20) >> 5,
      (Data & 0x10) >> 4,
      (Data & 0x8) >> 3,
      (Data & 0x4) >> 2,
      (Data & 0x2) >> 1,
      (Data & 0x1)
      );
  }

  Print (L"}\n");
}
