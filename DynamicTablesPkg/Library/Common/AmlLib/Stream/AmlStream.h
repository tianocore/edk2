/** @file
  AML Stream.

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_STREAM_H_
#define AML_STREAM_H_

#include <AmlInclude.h>

/** Stream direction.

  Enum to choose the direction the stream is progressing.
*/
typedef enum EAmlStreamDirection {
  EAmlStreamDirectionInvalid,     ///< Invalid AML Stream direction.
  EAmlStreamDirectionForward,     ///< Forward direction.
                                  ///  The Stream goes toward higher addresses.
  EAmlStreamDirectionBackward,    ///< Forward direction.
                                  ///  The Stream goes toward lower addresses.
  EAmlStreamDirectionMax,         ///< Max enum.
} EAML_STREAM_DIRECTION;

/** Stream.

  This structure is used as a wrapper around a buffer. It allows to do common
  buffer manipulations (read, write, etc.) while preventing buffer overflows.
*/
typedef struct AmlStream {
  /// Pointer to a buffer.
  UINT8     *Buffer;

  /// Size of Buffer.
  UINT32    MaxBufferSize;

  /// Index in the Buffer.
  /// The Index field allows to keep track of how many bytes have been
  /// read/written in the Buffer, and to retrieve the current stream position.
  /// 0 <= Index <= MaxBufferSize.
  /// If Index == MaxBufferSize, no more action is allowed on the stream.
  UINT32    Index;

  /// The direction the stream is progressing.
  /// If the stream goes backward (toward lower addresses), the bytes written
  /// to the stream are not reverted.
  /// In the example below, writing "Hello" to the stream will not revert
  /// the string. The end of the stream buffer will contain "Hello world!".
  /// Similarly, moving the stream position will be done according to the
  /// direction of the stream.
  /// Stream buffer:
  ///    +---------------+-----+-----+-----+-----+-----+-----+---- +------+
  ///    |-------------- | ' ' | 'w' | 'o' | 'r' | 'l' | 'd' | '!' | '\0' |
  ///    +---------------+-----+-----+-----+-----+-----+-----+---- +------+
  ///                       ^
  ///                Current position.
  EAML_STREAM_DIRECTION    Direction;
} AML_STREAM;

/** Check whether a StreamPtr is a valid Stream.

  @param  [in]  Stream   Pointer to a stream.

  @retval TRUE  Stream is a pointer to a stream.
  @retval FALSE Otherwise.
*/
#define IS_STREAM(Stream)  (                                                  \
          (((AML_STREAM*)Stream) != NULL)                                 &&  \
          (((AML_STREAM*)Stream)->Buffer != NULL))

/** Check whether a Stream is at the end of its buffer.

  @param  [in]  Stream   Pointer to a stream.

  @retval TRUE  Stream is a pointer to a non-full stream.
  @retval FALSE Otherwise.
*/
#define IS_END_OF_STREAM(Stream)  (                                           \
          (((AML_STREAM*)Stream)->Index ==                                    \
             ((AML_STREAM*)Stream)->MaxBufferSize))

/** Check Stream goes forward.

  @param  [in]  Stream    Pointer to a stream.

  @retval TRUE  Stream goes forward.
  @retval FALSE Otherwise.
*/
#define IS_STREAM_FORWARD(Stream)  (                                          \
    ((AML_STREAM*)Stream)->Direction == EAmlStreamDirectionForward)

/** Check Stream goes backward.

  @param  [in]  Stream   Pointer to a stream.

  @retval TRUE  Stream goes backward.
  @retval FALSE Otherwise.
*/
#define IS_STREAM_BACKWARD(Stream)  (                                         \
    ((AML_STREAM*)Stream)->Direction == EAmlStreamDirectionBackward)

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
  IN  OUT AML_STREAM             *Stream,
  IN      UINT8                  *Buffer,
  IN      UINT32                 MaxBufferSize,
  IN      EAML_STREAM_DIRECTION  Direction
  );

/** Clone a stream.

  Cloning a stream means copying all the values of the input Stream
  in the ClonedStream.

  @param  [in]  Stream          Pointer to the stream to clone.
  @param  [in]  ClonedStream    Pointer to the stream to initialize.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlStreamClone (
  IN  CONST AML_STREAM   *Stream,
  OUT        AML_STREAM  *ClonedStream
  );

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
  @param  [in]  SubStream   Pointer to the stream to initialize.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlStreamInitSubStream (
  IN  CONST AML_STREAM  *Stream,
  OUT       AML_STREAM  *SubStream
  );

/** Get the buffer of a stream.

  @param  [in]  Stream    Pointer to a stream.

  @return The stream's Buffer.
          NULL otherwise.
**/
UINT8 *
EFIAPI
AmlStreamGetBuffer (
  IN  CONST AML_STREAM  *Stream
  );

/** Get the size of Stream's Buffer.

  @param  [in]  Stream    Pointer to a stream.

  @return The Size of Stream's Buffer.
          Return 0 if Stream is invalid.
**/
UINT32
EFIAPI
AmlStreamGetMaxBufferSize (
  IN  CONST AML_STREAM  *Stream
  );

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
  IN  AML_STREAM  *Stream,
  IN  UINT32      Diff
  );

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
  IN  CONST AML_STREAM  *Stream
  );

/** Get Stream's Direction.

  @param  [in]  Stream    Pointer to a stream.

  @return Stream's Direction.
          Return EAmlStreamDirectionUnknown if Stream is invalid.
**/
EAML_STREAM_DIRECTION
EFIAPI
AmlStreamGetDirection (
  IN  CONST AML_STREAM  *Stream
  );

/** Return a pointer to the current position in the stream.

  @param  [in]  Stream    Pointer to a stream.

  @return The current position in the stream.
          Return NULL if error.
**/
UINT8 *
EFIAPI
AmlStreamGetCurrPos (
  IN  CONST AML_STREAM  *Stream
  );

/** Get the space available in the stream.

  @param  [in]  Stream    Pointer to a stream.

  @return Remaining space available in the stream.
          Zero in case of error or if the stream is at its end.
**/
UINT32
EFIAPI
AmlStreamGetFreeSpace (
  IN  CONST AML_STREAM  *Stream
  );

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
  IN  AML_STREAM  *Stream,
  IN  UINT32      Offset
  );

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
  IN  AML_STREAM  *Stream,
  IN  UINT32      Offset
  );

/** Reset the Stream (move the current position to the initial position).

  @param  [in]  Stream  Pointer to a stream.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlStreamReset (
  IN  AML_STREAM  *Stream
  );

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
  IN  AML_STREAM  *Stream,
  OUT UINT8       *OutByte
  );

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
  IN  AML_STREAM  *Stream,
  OUT UINT8       *OutByte
  );

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
  IN        AML_STREAM  *Stream,
  IN  CONST UINT8       *Buffer,
  IN        UINT32      Size
  );

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
  IN  CONST AML_STREAM  *Stream1,
  IN  CONST AML_STREAM  *Stream2,
  IN        UINT32      Size
  );

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
  OUT CHAR8       *DstBuffer,
  IN  UINT32      MaxDstBufferSize,
  IN  AML_STREAM  *Stream,
  IN  UINT32      Size
  );

#endif // AML_STREAM_H_
