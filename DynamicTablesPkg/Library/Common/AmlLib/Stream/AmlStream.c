/** @file
  AML Stream.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Stream/AmlStream.h>

/** Initialize a stream.

  @param  [in, out] Stream          Pointer to the stream to initialize.
  @param  [in]      Buffer          Buffer to initialize Stream with.
                                    Point to the beginning of the Buffer.
  @param  [in]      MaxBufferSize   Maximum size of Buffer.
  @param  [in]      Direction       Direction Stream is progressing
                                    (forward, backward).

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlStreamInit (
  IN  OUT AML_STREAM              * Stream,
  IN      UINT8                   * Buffer,
  IN      UINT32                    MaxBufferSize,
  IN      EAML_STREAM_DIRECTION     Direction
  )
{
  if ((Stream == NULL)                            ||
      (Buffer == NULL)                            ||
      (MaxBufferSize == 0)                        ||
      ((Direction != EAmlStreamDirectionForward)  &&
       (Direction != EAmlStreamDirectionBackward))) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Stream->Buffer = Buffer;
  Stream->MaxBufferSize = MaxBufferSize;
  Stream->Index = 0;
  Stream->Direction = Direction;

  return EFI_SUCCESS;
}

/** Clone a stream.

  Cloning a stream means copying all the values of the input Stream
  in the ClonedStream.

  @param  [in]  Stream          Pointer to the stream to clone.
  @param  [out] ClonedStream    Pointer to the stream to initialize.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlStreamClone (
  IN  CONST AML_STREAM    * Stream,
  OUT        AML_STREAM   * ClonedStream
  )
{
  if (!IS_STREAM (Stream)   ||
      (ClonedStream == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ClonedStream->Buffer = Stream->Buffer;
  ClonedStream->MaxBufferSize = Stream->MaxBufferSize;
  ClonedStream->Index = Stream->Index;
  ClonedStream->Direction = Stream->Direction;

  return EFI_SUCCESS;
}

/** Initialize a sub-stream from a stream.

  A sub-stream is a stream initialized at the current position of the input
  stream:
    - the Buffer field points to the current position of the input stream;
    - the Index field is set to 0;
    - the MaxBufferSize field is set to the remaining size of the input stream;
    - the direction is conserved;

  E.g.: For a forward stream:
                   +----------------+----------------+
                   |ABCD.........XYZ|   Free Space   |
                   +----------------+----------------+
                   ^                ^                ^
  Stream:        Buffer          CurrPos         EndOfBuff
  Sub-stream:                Buffer/CurrPos      EndOfBuff

  @param  [in]  Stream      Pointer to the stream from which a sub-stream is
                            created.
  @param  [out] SubStream   Pointer to the stream to initialize.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlStreamInitSubStream (
  IN  CONST AML_STREAM  * Stream,
  OUT       AML_STREAM  * SubStream
  )
{
  if (!IS_STREAM (Stream) ||
      (SubStream == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (IS_STREAM_FORWARD (Stream)) {
    SubStream->Buffer = AmlStreamGetCurrPos (Stream);
  } else if (IS_STREAM_BACKWARD (Stream)) {
    SubStream->Buffer = Stream->Buffer;
  } else {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  SubStream->MaxBufferSize = AmlStreamGetFreeSpace (Stream);
  SubStream->Index = 0;
  SubStream->Direction = Stream->Direction;

  return EFI_SUCCESS;
}

/** Get the buffer of a stream.

  @param  [in]  Stream    Pointer to a stream.

  @return The stream's Buffer.
          NULL otherwise.
**/
UINT8 *
EFIAPI
AmlStreamGetBuffer (
  IN  CONST AML_STREAM  * Stream
  )
{
  if (!IS_STREAM (Stream)) {
    ASSERT (0);
    return NULL;
  }
  return Stream->Buffer;
}

/** Get the size of Stream's Buffer.

  @param  [in]  Stream    Pointer to a stream.

  @return The Size of Stream's Buffer.
          Return 0 if Stream is invalid.
**/
UINT32
EFIAPI
AmlStreamGetMaxBufferSize (
  IN  CONST AML_STREAM  * Stream
  )
{
  if (!IS_STREAM (Stream)) {
    ASSERT (0);
    return 0;
  }
  return Stream->MaxBufferSize;
}

/** Reduce the maximal size of Stream's Buffer (MaxBufferSize field).

  @param  [in]  Stream    Pointer to a stream.
  @param  [in]  Diff      Value to subtract to the Stream's MaxBufferSize.
                          0 < x < MaxBufferSize - Index.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlStreamReduceMaxBufferSize (
  IN  AML_STREAM  * Stream,
  IN  UINT32        Diff
  )
{
  if (!IS_STREAM (Stream)       ||
      (Diff == 0)               ||
      ((Stream->MaxBufferSize - Diff) <= Stream->Index)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Stream->MaxBufferSize -= Diff;
  return EFI_SUCCESS;
}

/** Get Stream's Index.

  Stream's Index is incremented when writing data, reading data,
  or moving the position in the Stream.
  It can be seen as an index:
   - starting at the beginning of Stream's Buffer if the stream goes forward;
   - starting at the end of Stream's Buffer if the stream goes backward.

  @param  [in]  Stream    Pointer to a stream.

  @return Stream's Index.
          Return 0 if Stream is invalid.
**/
UINT32
EFIAPI
AmlStreamGetIndex (
  IN  CONST AML_STREAM  * Stream
  )
{
  if (!IS_STREAM (Stream)) {
    ASSERT (0);
    return 0;
  }
  return Stream->Index;
}

/** Get Stream's Direction.

  @param  [in]  Stream    Pointer to a stream.

  @return Stream's Direction.
          Return EAmlStreamDirectionUnknown if Stream is invalid.
**/
EAML_STREAM_DIRECTION
EFIAPI
AmlStreamGetDirection (
  IN  CONST AML_STREAM  * Stream
  )
{
  if (!IS_STREAM (Stream)) {
    ASSERT (0);
    return EAmlStreamDirectionInvalid;
  }
  return Stream->Direction;
}

/** Return a pointer to the current position in the stream.

  @param  [in]  Stream    Pointer to a stream.

  @return The current position in the stream.
          Return NULL if error.
**/
UINT8 *
EFIAPI
AmlStreamGetCurrPos (
  IN  CONST AML_STREAM  * Stream
  )
{
  if (!IS_STREAM (Stream)) {
    ASSERT (0);
    return NULL;
  }

  if (IS_STREAM_FORWARD (Stream)) {
    return Stream->Buffer + Stream->Index;
  } else if (IS_STREAM_BACKWARD (Stream)) {
    return Stream->Buffer + (Stream->MaxBufferSize - 1) - Stream->Index;
  } else {
    ASSERT (0);
    return NULL;
  }
}

/** Get the space available in the stream.

  @param  [in]  Stream    Pointer to a stream.

  @return Remaining space available in the stream.
          Zero in case of error or if the stream is at its end.
**/
UINT32
EFIAPI
AmlStreamGetFreeSpace (
  IN  CONST AML_STREAM  * Stream
  )
{
  if (!IS_STREAM (Stream)) {
    ASSERT (0);
    return 0;
  }

  if (Stream->Index > Stream->MaxBufferSize) {
    ASSERT (0);
    return 0;
  }

  return Stream->MaxBufferSize - Stream->Index;
}

/** Move Stream by Offset bytes.

  The stream current position is moved according to the stream direction
  (forward, backward).

  @param  [in]  Stream  Pointer to a stream.
                        The stream must not be at its end.
  @param  [in]  Offset  Offset to move the stream of.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
**/
EFI_STATUS
EFIAPI
AmlStreamProgress (
  IN  AML_STREAM  * Stream,
  IN  UINT32        Offset
  )
{
  if (!IS_STREAM (Stream)         ||
      IS_END_OF_STREAM  (Stream)  ||
      (Offset == 0)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (AmlStreamGetFreeSpace (Stream) < Offset) {
    ASSERT (0);
    return EFI_BUFFER_TOO_SMALL;
  }

  Stream->Index += Offset;

  return EFI_SUCCESS;
}

/** Rewind Stream of Offset bytes.

  The stream current position is rewound according to the stream direction
  (forward, backward). A stream going forward will be rewound backward.

  @param  [in]  Stream  Pointer to a stream.
  @param  [in]  Offset  Offset to rewind the stream of.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
**/
EFI_STATUS
EFIAPI
AmlStreamRewind (
  IN  AML_STREAM  * Stream,
  IN  UINT32        Offset
  )
{
  if (!IS_STREAM (Stream) ||
      (Offset == 0)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (AmlStreamGetIndex (Stream) < Offset) {
    ASSERT (0);
    return EFI_BUFFER_TOO_SMALL;
  }

  Stream->Index -= Offset;

  return EFI_SUCCESS;
}

/** Reset the Stream (move the current position to the initial position).

  @param  [in]  Stream  Pointer to a stream.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlStreamReset (
  IN  AML_STREAM  * Stream
  )
{
  if (!IS_STREAM (Stream)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Stream->Index = 0;

  return EFI_SUCCESS;
}

/** Peek one byte at Stream's current position.

  Stream's position is not moved when peeking.

  @param  [in]  Stream    Pointer to a stream.
                          The stream must not be at its end.
  @param  [out] OutByte   Pointer holding the byte value of
                          the stream current position.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid parameter.
  @retval EFI_BUFFER_TOO_SMALL  No space left in the buffer.
**/
EFI_STATUS
EFIAPI
AmlStreamPeekByte (
  IN  AML_STREAM  * Stream,
  OUT UINT8       * OutByte
  )
{
  UINT8   * CurPos;

  if (!IS_STREAM (Stream)       ||
      IS_END_OF_STREAM (Stream) ||
      (OutByte == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  CurPos = AmlStreamGetCurrPos (Stream);
  if (CurPos == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *OutByte = *CurPos;
  return EFI_SUCCESS;
}

/** Read one byte at Stream's current position.

  The stream current position is moved when reading.

  @param  [in]  Stream    Pointer to a stream.
                          The stream must not be at its end.
  @param  [out] OutByte   Pointer holding the byte value of
                          the stream current position.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid parameter.
  @retval EFI_BUFFER_TOO_SMALL  No space left in the buffer.
**/
EFI_STATUS
EFIAPI
AmlStreamReadByte (
  IN  AML_STREAM  * Stream,
  OUT UINT8       * OutByte
  )
{
  EFI_STATUS    Status;

  if (!IS_STREAM (Stream)       ||
      IS_END_OF_STREAM (Stream) ||
      (OutByte == NULL)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Stream is checked in the function call.
  Status = AmlStreamPeekByte (Stream, OutByte);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlStreamProgress (Stream, 1);
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Write Size bytes in the stream.

  If the stream goes backward (toward lower addresses), the bytes written
  to the stream are not reverted.
  In the example below, writing "Hello" to the stream will not revert
  the string. The end of the stream buffer will contain "Hello world!".
  Stream buffer:
     +---------------+-----+-----+-----+-----+-----+-----+---- +------+
     |         ..... | ' ' | 'w' | 'o' | 'r' | 'l' | 'd' | '!' | '\0' |
     +---------------+-----+-----+-----+-----+-----+-----+---- +------+
                        ^
                 Current position.

  @param  [in]  Stream  Pointer to a stream.
                        The stream must not be at its end.
  @param  [in]  Buffer  Pointer to the data to write.
  @param  [in]  Size    Number of bytes to write.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_BUFFER_TOO_SMALL    No space left in the buffer.
**/
EFI_STATUS
EFIAPI
AmlStreamWrite (
  IN        AML_STREAM  * Stream,
  IN  CONST UINT8       * Buffer,
  IN        UINT32        Size
  )
{
  UINT8   * CurrPos;

  if (!IS_STREAM (Stream)       ||
      IS_END_OF_STREAM (Stream) ||
      (Buffer == NULL)          ||
      (Size == 0)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (AmlStreamGetFreeSpace (Stream) < Size) {
    ASSERT (0);
    return EFI_BUFFER_TOO_SMALL;
  }

  CurrPos = AmlStreamGetCurrPos (Stream);

  // If the Stream goes backward, prepare some space to copy the data.
  if (IS_STREAM_BACKWARD (Stream)) {
    CurrPos -= Size;
  }

  CopyMem (CurrPos, Buffer, Size);
  Stream->Index += Size;

  return EFI_SUCCESS;
}

/** Compare Size bytes between Stream1 and Stream2 from their
    respective current position.

  Stream1 and Stream2 must go in the same direction.
  Stream1 and Stream2 are left unchanged.

  @param  [in]  Stream1   First stream to compare.
                          The stream must not be at its end.
  @param  [in]  Stream2   Second stream to compare.
                          The stream must not be at its end.
  @param  [in]  Size      Number of bytes to compare.
                          Must be lower than the minimum remaining space of
                          Stream1 and Stream2.
                          Must be non-zero.

  @retval TRUE  If Stream1 and Stream2 have Size bytes equal,
                from their respective current position.
                The function completed successfully.
  @retval FALSE Otherwise.
**/
BOOLEAN
EFIAPI
AmlStreamCmp (
  IN  CONST AML_STREAM    * Stream1,
  IN  CONST AML_STREAM    * Stream2,
  IN        UINT32          Size
  )
{
  UINT32          MinSize;
  UINT8         * CurrPosStream1;
  UINT8         * CurrPosStream2;

  if (!IS_STREAM (Stream1)                        ||
      IS_END_OF_STREAM (Stream1)                  ||
      !IS_STREAM (Stream2)                        ||
      IS_END_OF_STREAM (Stream2)                  ||
      (Stream1->Direction != Stream2->Direction)  ||
      (Size == 0)) {
    ASSERT (0);
    return FALSE;
  }

  // Check the Size is not longer than the remaining size of
  // Stream1 and Stream2.
  MinSize = MIN (
              AmlStreamGetFreeSpace (Stream1),
              AmlStreamGetFreeSpace (Stream2)
              );
  if (MinSize < Size) {
    ASSERT (0);
    return FALSE;
  }

  CurrPosStream1 = AmlStreamGetCurrPos (Stream1);
  if (CurrPosStream1 == NULL) {
    ASSERT (0);
    return FALSE;
  }
  CurrPosStream2 = AmlStreamGetCurrPos (Stream2);
  if (CurrPosStream2 == NULL) {
    ASSERT (0);
    return FALSE;
  }

  if (Stream1->Direction == EAmlStreamDirectionForward) {
    return (0 == CompareMem (CurrPosStream1, CurrPosStream2, MinSize));
  }

  // The stream is already pointing on the last byte, thus the (-1).
  //          +---------------------+
  // BStream  | | | | | | | |M|E|T|0|
  //          +---------------------+
  //                               ^
  //                             CurrPos
  return (0 == CompareMem (
                  CurrPosStream1 - (MinSize - 1),
                  CurrPosStream2 - (MinSize - 1),
                  MinSize
                  ));
}

/** Copy Size bytes of the stream's data to DstBuffer.

  For a backward stream, the bytes are copied starting from the
  current stream position.

  @param  [out] DstBuffer         Destination Buffer to copy the data to.
  @param  [in]  MaxDstBufferSize  Maximum size of DstBuffer.
                                  Must be non-zero.
  @param  [in]  Stream            Pointer to the stream to copy the data from.
  @param  [in]  Size              Number of bytes to copy from the stream
                                  buffer.
                                  Must be lower than MaxDstBufferSize.
                                  Must be lower than Stream's MaxBufferSize.
                                  Return success if zero.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlStreamCpyS (
  OUT CHAR8         * DstBuffer,
  IN  UINT32          MaxDstBufferSize,
  IN  AML_STREAM    * Stream,
  IN  UINT32          Size
  )
{
  CHAR8   * StreamBufferStart;

  // Stream is checked in the function call.
  if ((DstBuffer == NULL)       ||
      (MaxDstBufferSize == 0)   ||
      (Size > MaxDstBufferSize) ||
      (Size > AmlStreamGetMaxBufferSize (Stream))) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (Size == 0) {
    return EFI_SUCCESS;
  }

  // Find the address at which the data is starting.
  StreamBufferStart = (CHAR8*)(IS_STREAM_FORWARD (Stream) ?
                                 Stream->Buffer :
                                 AmlStreamGetCurrPos (Stream));

  CopyMem (DstBuffer, StreamBufferStart, Size);

  return EFI_SUCCESS;
}
