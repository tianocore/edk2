/** @file
  BitStream utility.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BIT_STREAM_H_
#define BIT_STREAM_H_

/** BitStream.

  Struct holding a buffer of data and allowing to do operations
  on them (concatenation, selecting rightmost bits, addition, ...).
  BitStream are big-endian (MSByte at index 0).
*/
typedef struct BitStream {
  /// Length of BitStream in bits.
  UINTN    BitLen;
  /// Length of BitStream in bytes.
  UINTN    ByteLen;
  /// Buffer holding the data.
  UINT8    *Data;
} BIT_STREAM;

/** Check whether a BitStream is NULL (null length).

  @param [in] Stream  The BitStream.

  @retval TRUE if the BitStream is NULL (null length).
  @retval FALSE otherwise.
**/
BOOLEAN
EFIAPI
IsBitStreamEmpty (
  IN  BIT_STREAM  *Stream
  );

/** Convert bits to bytes (rounds down).

  @param [in] Bits    Bits.

  @return Bytes.
**/
UINTN
EFIAPI
BitsToLowerBytes (
  IN  UINTN  Bits
  );

/** Convert bits to bytes (rounds up).

  @param [in] Bits    Bits.

  @return Bytes.
**/
UINTN
EFIAPI
BitsToUpperBytes (
  IN  UINTN  Bits
  );

/** Get the BitStream length (in bits).

  @param [in] Stream    The BitStream.

  @return Length of the BitStream (in bits).
**/
UINTN
EFIAPI
BitStreamBitLen (
  IN  BIT_STREAM  *Stream
  );

/** Get the BitStream length (in bytes).

  @param [in] Stream    The BitStream.

  @return Length of the BitStream (in bytes).
**/
UINTN
EFIAPI
BitStreamByteLen (
  IN  BIT_STREAM  *Stream
  );

/** Get the BitStream data buffer.

  @param [in] Stream    The BitStream.

  @return Data buffer of the BitStream (can be NULL).
**/
UINT8 *
EFIAPI
BitStreamData (
  IN  BIT_STREAM  *Stream
  );

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
  );

/** Free a BitStream.

  @param [in, out]  Stream    BitStream to free.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
BitStreamFree (
  IN  OUT BIT_STREAM  **Stream
  );

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
  );

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
  );

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
  );

/** Replace an initialized BitStream with another.

  This function frees StreamRepl's Data if success.

  @param [out]  StreamRepl    Stream whose content is replaced.
  @param [in]   StreamData    Stream containing the Data to use.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
**/
EFI_STATUS
EFIAPI
BitStreamReplace (
  IN  OUT BIT_STREAM  *StreamRepl,
  IN      BIT_STREAM  *StreamData
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/** Print a BitStream.

  @param [in]   Stream    Stream to print.
**/
VOID
EFIAPI
BitStreamPrint (
  IN  BIT_STREAM  *Stream
  );

#endif // BIT_STREAM_H_
