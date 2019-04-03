/** @file
  Provide functions to provide tcg storage core spec related functions.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/TcgStorageCoreLib.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
//#include <Library/PrintLib.h>

/**
  Required to be called before calling any other Tcg functions with the TCG_CREATE_STRUCT.
  Initializes the packet variables to NULL.  Additionally, the buffer will be memset.

  @param [in/out]    CreateStruct    Structure to initialize
  @param [in]        Buffer          Buffer allocated by client of library.  It will contain the Tcg encoded packet.  This cannot be null.
  @param [in]        BufferSize      Size of buffer provided.  It cannot be 0.

  @retval       Return the action result.
**/
TCG_RESULT
EFIAPI
TcgInitTcgCreateStruct(
  TCG_CREATE_STRUCT      *CreateStruct,
  VOID                   *Buffer,
  UINT32                 BufferSize
  )
{
  NULL_CHECK(CreateStruct);
  NULL_CHECK(Buffer);

  if (BufferSize == 0) {
    DEBUG ((DEBUG_INFO, "BufferSize=0\n"));
    return (TcgResultFailureZeroSize);
  }

  ZeroMem(Buffer, BufferSize);
  CreateStruct->BufferSize = BufferSize;
  CreateStruct->Buffer = Buffer;
  CreateStruct->ComPacket = NULL;
  CreateStruct->CurPacket = NULL;
  CreateStruct->CurSubPacket = NULL;

  return (TcgResultSuccess);
}

/**

  Encodes the ComPacket header to the data structure.

  @param[in/out]    CreateStruct          Structure to initialize
  @param[in]        ComId                 ComID of the Tcg ComPacket.
  @param[in]        ComIdExtension        ComID Extension of the Tcg ComPacket.

**/
TCG_RESULT
EFIAPI
TcgStartComPacket(
  TCG_CREATE_STRUCT   *CreateStruct,
  UINT16              ComId,
  UINT16              ComIdExtension
  )
{
  NULL_CHECK(CreateStruct);

  if (CreateStruct->ComPacket != NULL ||
      CreateStruct->CurPacket != NULL ||
      CreateStruct->CurSubPacket != NULL
     ) {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", CreateStruct->ComPacket, CreateStruct->CurPacket,
    CreateStruct->CurSubPacket));
    return (TcgResultFailureInvalidAction);
  }

  if (sizeof(TCG_COM_PACKET) > CreateStruct->BufferSize) {
    DEBUG ((DEBUG_INFO, "BufferSize=0x%X\n", CreateStruct->BufferSize));
    return (TcgResultFailureBufferTooSmall);
  }

  CreateStruct->ComPacket = (TCG_COM_PACKET*)CreateStruct->Buffer;
  CreateStruct->ComPacket->ComIDBE = SwapBytes16(ComId);
  CreateStruct->ComPacket->ComIDExtensionBE = SwapBytes16(ComIdExtension);

  return (TcgResultSuccess);
}

/**

  Starts a new ComPacket in the Data structure.

  @param [in/out]     CreateStruct    Structure used to add Tcg Packet
  @param[in]          Tsn             Packet Tper session number
  @param[in]          Hsn             Packet Host session number
  @param[in]          SeqNumber       Packet Sequence Number
  @param[in]          AckType         Packet Acknowledge Type
  @param[in]          Ack             Packet Acknowledge

**/
TCG_RESULT
EFIAPI
TcgStartPacket(
  TCG_CREATE_STRUCT    *CreateStruct,
  UINT32               Tsn,
  UINT32               Hsn,
  UINT32               SeqNumber,
  UINT16               AckType,
  UINT32               Ack
  )
{
  UINT32 AddedSize;
  NULL_CHECK(CreateStruct);

  AddedSize = 0;

  if (CreateStruct->ComPacket == NULL ||
      CreateStruct->CurPacket != NULL ||
      CreateStruct->CurSubPacket != NULL
     ) {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", CreateStruct->ComPacket, CreateStruct->CurPacket, CreateStruct->CurSubPacket));
    return (TcgResultFailureInvalidAction);
  }

  // update TCG_COM_PACKET and packet lengths
  AddedSize = sizeof(TCG_PACKET);

  if ((SwapBytes32(CreateStruct->ComPacket->LengthBE) + AddedSize) > CreateStruct->BufferSize) {
    DEBUG ((DEBUG_INFO, "BufferSize=0x%X\n", CreateStruct->BufferSize));
    return (TcgResultFailureBufferTooSmall);
  }

  CreateStruct->CurPacket = (TCG_PACKET*)(CreateStruct->ComPacket->Payload + SwapBytes32(CreateStruct->ComPacket->LengthBE));

  CreateStruct->CurPacket->TperSessionNumberBE = SwapBytes32( Tsn );
  CreateStruct->CurPacket->HostSessionNumberBE = SwapBytes32( Hsn );
  CreateStruct->CurPacket->SequenceNumberBE = SwapBytes32( SeqNumber );
  CreateStruct->CurPacket->AckTypeBE = SwapBytes16( AckType );
  CreateStruct->CurPacket->AcknowledgementBE = SwapBytes32( Ack );

  CreateStruct->CurPacket->LengthBE = 0;

  // update TCG_COM_PACKET Length for next pointer
  CreateStruct->ComPacket->LengthBE = SwapBytes32( SwapBytes32(CreateStruct->ComPacket->LengthBE) + AddedSize );

  return (TcgResultSuccess);
}

/**

  Starts a new SubPacket in the Data structure.

  @param[in/out]    CreateStruct        Structure used to start Tcg SubPacket
  @param[in]        Kind                SubPacket kind

**/
TCG_RESULT
EFIAPI
TcgStartSubPacket(
  TCG_CREATE_STRUCT    *CreateStruct,
  UINT16               Kind
  )
{
  UINT32 AddedSize;

  NULL_CHECK(CreateStruct);

  AddedSize = 0;

  if (CreateStruct->ComPacket == NULL ||
      CreateStruct->CurPacket == NULL ||
      CreateStruct->CurSubPacket != NULL
     ) {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", CreateStruct->ComPacket, CreateStruct->CurPacket, CreateStruct->CurSubPacket));
    return (TcgResultFailureInvalidAction);
  }

  AddedSize = sizeof(TCG_SUB_PACKET);

  if ((SwapBytes32(CreateStruct->ComPacket->LengthBE) + AddedSize) > CreateStruct->BufferSize) {
    DEBUG ((DEBUG_INFO, "BufferSize=0x%X\n", CreateStruct->BufferSize));
    return (TcgResultFailureBufferTooSmall);
  }

  CreateStruct->CurSubPacket = (TCG_SUB_PACKET*)(CreateStruct->CurPacket->Payload + SwapBytes32(CreateStruct->CurPacket->LengthBE));
  CreateStruct->CurSubPacket->KindBE = SwapBytes16(Kind);

  // update lengths
  CreateStruct->CurSubPacket->LengthBE = 0;

  // update TCG_COM_PACKET and packet lengths
  CreateStruct->ComPacket->LengthBE = SwapBytes32(SwapBytes32(CreateStruct->ComPacket->LengthBE) + AddedSize);
  CreateStruct->CurPacket->LengthBE = SwapBytes32(SwapBytes32(CreateStruct->CurPacket->LengthBE) + AddedSize);

  return (TcgResultSuccess);
}

/**

  Ends the current SubPacket in the Data structure.  This function will also perform the 4-byte padding
  required for Subpackets.

  @param[in/out]       CreateStruct        Structure used to end the current Tcg SubPacket

**/
TCG_RESULT
EFIAPI
TcgEndSubPacket(
  TCG_CREATE_STRUCT   *CreateStruct
  )
{
  UINT32 PadSize;

  NULL_CHECK(CreateStruct);

  PadSize = 0;

  if (CreateStruct->ComPacket == NULL ||
      CreateStruct->CurPacket == NULL  ||
      CreateStruct->CurSubPacket == NULL
     ) {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", CreateStruct->ComPacket, CreateStruct->CurPacket, CreateStruct->CurSubPacket));
    return (TcgResultFailureInvalidAction);
  }

  // align to 4-byte boundaries, so shift padding
  // pad Size does not apply to subpacket Length
  PadSize = TCG_SUBPACKET_ALIGNMENT - (SwapBytes32(CreateStruct->CurSubPacket->LengthBE) & (TCG_SUBPACKET_ALIGNMENT - 1));

  if (PadSize == TCG_SUBPACKET_ALIGNMENT) {
    PadSize = 0;
  }

  if ((SwapBytes32(CreateStruct->ComPacket->LengthBE) + PadSize) > CreateStruct->BufferSize) {
    DEBUG ((DEBUG_INFO, "BufferSize=0x%X\n", CreateStruct->BufferSize));
    return (TcgResultFailureBufferTooSmall);
  }

  CreateStruct->CurPacket->LengthBE = SwapBytes32(SwapBytes32(CreateStruct->CurPacket->LengthBE) + PadSize);
  CreateStruct->ComPacket->LengthBE = SwapBytes32(SwapBytes32(CreateStruct->ComPacket->LengthBE) + PadSize);

  CreateStruct->CurSubPacket = NULL;

  return (TcgResultSuccess);
}

/**

  Ends the current Packet in the Data structure.

  @param[in/out]       CreateStruct        Structure used to end the current Tcg Packet

**/
TCG_RESULT
EFIAPI
TcgEndPacket(
  TCG_CREATE_STRUCT     *CreateStruct
  )
{
  NULL_CHECK(CreateStruct);

  if (CreateStruct->ComPacket == NULL ||
      CreateStruct->CurPacket == NULL ||
      CreateStruct->CurSubPacket != NULL
     ) {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", CreateStruct->ComPacket, CreateStruct->CurPacket, CreateStruct->CurSubPacket));
    return (TcgResultFailureInvalidAction);
  }

  CreateStruct->CurPacket = NULL;

  return (TcgResultSuccess);
}

/**

  Ends the ComPacket in the Data structure and ret

  @param [in/out]       CreateStruct       Structure used to end the Tcg ComPacket
  @param [in/out]       Size               Describes the Size of the entire ComPacket (Header and payload). Filled out by function.

**/
TCG_RESULT
EFIAPI
TcgEndComPacket(
  TCG_CREATE_STRUCT      *CreateStruct,
  UINT32                 *Size
  )
{
  NULL_CHECK(CreateStruct);
  NULL_CHECK(Size);

  if (CreateStruct->ComPacket == NULL ||
      CreateStruct->CurPacket != NULL ||
      CreateStruct->CurSubPacket != NULL
     ) {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", CreateStruct->ComPacket, CreateStruct->CurPacket, CreateStruct->CurSubPacket));
    return (TcgResultFailureInvalidAction);
  }

  *Size = SwapBytes32(CreateStruct->ComPacket->LengthBE) + sizeof(*CreateStruct->ComPacket);
  CreateStruct->ComPacket = NULL;

  return (TcgResultSuccess);
}

/**
  Adds raw Data with optional Header

  @param       CreateStruct       The create structure.
  @param       Header             The header structure.
  @param       HeaderSize         The header size.
  @param       Data               The data need to add.
  @param       DataSize           The data size.
  @param       ByteSwapData       Whether byte or swap data.

**/
TCG_RESULT
TcgAddRawTokenData(
  TCG_CREATE_STRUCT      *CreateStruct,
  const VOID             *Header,
  UINT8                  HeaderSize,
  const VOID             *Data,
  UINT32                 DataSize,
  BOOLEAN                ByteSwapData
  )
{
  UINT32 AddedSize;
  UINT8* Dest;
  const UINT8* DataBytes;
  UINT32 Index;

  AddedSize = 0;
  Index = 0;
  Dest = NULL;

  NULL_CHECK(CreateStruct);

  if ((HeaderSize != 0 && Header == NULL) ||
      (DataSize != 0 && Data == NULL)
     ) {
    DEBUG ((DEBUG_INFO, "HeaderSize=0x%X Header=%p DataSize=0x%X Data=%p\n", HeaderSize, Header, DataSize, Data));
    return (TcgResultFailureNullPointer);
  }

  if (CreateStruct->ComPacket == NULL ||
      CreateStruct->CurPacket == NULL ||
      CreateStruct->CurSubPacket == NULL
     ) {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", CreateStruct->ComPacket, CreateStruct->CurPacket, CreateStruct->CurSubPacket));
    return (TcgResultFailureInvalidAction);
  }

  // verify there is enough Buffer Size
  AddedSize = HeaderSize + DataSize;
  if ((SwapBytes32(CreateStruct->ComPacket->LengthBE) + AddedSize) > CreateStruct->BufferSize) {
    return (TcgResultFailureBufferTooSmall);
  }

  // Get a pointer to where the new bytes should go
  Dest = CreateStruct->ComPacket->Payload + SwapBytes32(CreateStruct->ComPacket->LengthBE);

  switch (HeaderSize) {
    case sizeof(TCG_SIMPLE_TOKEN_SHORT_ATOM):
    case sizeof(TCG_SIMPLE_TOKEN_MEDIUM_ATOM):
    case sizeof(TCG_SIMPLE_TOKEN_LONG_ATOM):
      CopyMem(Dest, Header, HeaderSize);
      Dest += HeaderSize;
    case 0: // no Header is valid
      break;
    // invalid Header Size
    default:
      DEBUG ((DEBUG_INFO, "unsupported HeaderSize=%u\n", HeaderSize));
      return TcgResultFailure;
  }

  // copy the Data bytes
  if (ByteSwapData) {
    DataBytes = (const UINT8*)Data;
    for (Index = 0; Index < DataSize; Index++) {
      Dest[Index] = DataBytes[DataSize - 1 - Index];
    }
  } else {
    CopyMem(Dest, Data, DataSize);
  }

  // Update all the packet sizes
  CreateStruct->ComPacket->LengthBE = SwapBytes32(SwapBytes32(CreateStruct->ComPacket->LengthBE) + AddedSize);
  CreateStruct->CurPacket->LengthBE = SwapBytes32(SwapBytes32(CreateStruct->CurPacket->LengthBE) + AddedSize);
  CreateStruct->CurSubPacket->LengthBE = SwapBytes32(SwapBytes32(CreateStruct->CurSubPacket->LengthBE) + AddedSize);

  return (TcgResultSuccess);
}

/**

  Adds a single raw token byte to the Data structure.

  @param[in/out]   CreateStruct        Structure used to add the byte
  @param[in]       Byte                Byte to add

**/
TCG_RESULT
EFIAPI
TcgAddRawByte(
  TCG_CREATE_STRUCT     *CreateStruct,
  UINT8                 Byte
  )
{
  return TcgAddRawTokenData(CreateStruct, NULL, 0, &Byte, 1, FALSE);
}


/**
   simple tokens - atoms: tiny, short, medium, long and empty atoms.
   tiny atom can be a signed or unsigned integer.
   short, medium, long can be a signed or unsigned integer OR a complete or non-final byte sequence.

  @param       CreateStruct       The create structure.
  @param       Data               The data need to add.
  @param       DataSize           The data size.
  @param       ByteOrInt,         Data format is byte or int.
  @param       SignOrCont         sign or cont.


**/
TCG_RESULT
TcgAddAtom(
  TCG_CREATE_STRUCT   *CreateStruct,
  const VOID          *Data,
  UINT32              DataSize,
  UINT8               ByteOrInt,
  UINT8               SignOrCont
  )
{
  const UINT8* DataBytes;
  TCG_SIMPLE_TOKEN_TINY_ATOM TinyAtom;
  TCG_SIMPLE_TOKEN_SHORT_ATOM ShortAtom;
  TCG_SIMPLE_TOKEN_MEDIUM_ATOM MediumAtom;
  TCG_SIMPLE_TOKEN_LONG_ATOM LongAtom;

  NULL_CHECK(CreateStruct);

  if (DataSize == 0) {
    if (ByteOrInt == TCG_ATOM_TYPE_INTEGER) {
      DEBUG ((DEBUG_INFO, "0-Size integer not allowed\n"));
      return TcgResultFailure;
    }
  } else {
    // if DataSize != 0, Data must be valid
    NULL_CHECK(Data);
  }

  // encode Data using the shortest possible atom
  DataBytes = (const UINT8*)Data;
  if ((DataSize == 1) &&
      (ByteOrInt == TCG_ATOM_TYPE_INTEGER) &&
      ((SignOrCont != 0 && ((TCG_TOKEN_TINYATOM_SIGNED_MIN_VALUE <= *(INT8*)Data) && (*(INT8*)Data <= TCG_TOKEN_TINYATOM_SIGNED_MAX_VALUE))) ||
       (SignOrCont == 0 && ((*DataBytes <= TCG_TOKEN_TINYATOM_UNSIGNED_MAX_VALUE))))
     ) {
    TinyAtom.TinyAtomBits.IsZero = 0;
    TinyAtom.TinyAtomBits.Sign = SignOrCont;
    TinyAtom.TinyAtomBits.Data = *DataBytes & TCG_TOKEN_TINYATOM_UNSIGNED_MAX_VALUE;
    return TcgAddRawTokenData(CreateStruct, NULL, 0, (UINT8*)&TinyAtom, sizeof(TCG_SIMPLE_TOKEN_TINY_ATOM), FALSE);
  }

  if (DataSize <= TCG_TOKEN_SHORTATOM_MAX_BYTE_SIZE) {
    ShortAtom.ShortAtomBits.IsOne = 1;
    ShortAtom.ShortAtomBits.IsZero = 0;
    ShortAtom.ShortAtomBits.ByteOrInt = ByteOrInt;
    ShortAtom.ShortAtomBits.SignOrCont = SignOrCont;
    ShortAtom.ShortAtomBits.Length = DataSize & 0x0F;
    return TcgAddRawTokenData(CreateStruct, &ShortAtom, sizeof(TCG_SIMPLE_TOKEN_SHORT_ATOM), Data, DataSize, ByteOrInt == TCG_ATOM_TYPE_INTEGER);
  }

  if (DataSize <= TCG_TOKEN_MEDIUMATOM_MAX_BYTE_SIZE) {
    MediumAtom.MediumAtomBits.IsOne1 = 1;
    MediumAtom.MediumAtomBits.IsOne2 = 1;
    MediumAtom.MediumAtomBits.IsZero = 0;
    MediumAtom.MediumAtomBits.ByteOrInt = ByteOrInt;
    MediumAtom.MediumAtomBits.SignOrCont = SignOrCont;
    MediumAtom.MediumAtomBits.LengthLow = DataSize & 0xFF;
    MediumAtom.MediumAtomBits.LengthHigh = (DataSize >> TCG_MEDIUM_ATOM_LENGTH_HIGH_SHIFT) & TCG_MEDIUM_ATOM_LENGTH_HIGH_MASK;
    return TcgAddRawTokenData(CreateStruct, &MediumAtom, sizeof(TCG_SIMPLE_TOKEN_MEDIUM_ATOM), Data, DataSize, ByteOrInt == TCG_ATOM_TYPE_INTEGER);
  }

  LongAtom.LongAtomBits.IsOne1 = 1;
  LongAtom.LongAtomBits.IsOne2 = 1;
  LongAtom.LongAtomBits.IsOne3 = 1;
  LongAtom.LongAtomBits.IsZero = 0;
  LongAtom.LongAtomBits.ByteOrInt = ByteOrInt;
  LongAtom.LongAtomBits.SignOrCont = SignOrCont;
  LongAtom.LongAtomBits.LengthLow = DataSize & 0xFF;
  LongAtom.LongAtomBits.LengthMid = (DataSize >> TCG_LONG_ATOM_LENGTH_MID_SHIFT) & 0xFF;
  LongAtom.LongAtomBits.LengthHigh = (DataSize >> TCG_LONG_ATOM_LENGTH_HIGH_SHIFT) & 0xFF;
  return TcgAddRawTokenData(CreateStruct, &LongAtom, sizeof(TCG_SIMPLE_TOKEN_LONG_ATOM), Data, DataSize, ByteOrInt == TCG_ATOM_TYPE_INTEGER);
}

/**

  Adds the Data parameter as a byte sequence to the Data structure.

  @param[in/out]    CreateStruct      Structure used to add the byte sequence
  @param[in]        Data              Byte sequence that will be encoded and copied into Data structure
  @param[in]        DataSize          Length of Data provided
  @param[in]        Continued         TRUE if byte sequence is continued or
                                      FALSE if the Data contains the entire byte sequence to be encoded

**/
TCG_RESULT
EFIAPI
TcgAddByteSequence(
  TCG_CREATE_STRUCT     *CreateStruct,
  const VOID            *Data,
  UINT32                DataSize,
  BOOLEAN               Continued
  )
{
  return TcgAddAtom(CreateStruct, Data, DataSize, TCG_ATOM_TYPE_BYTE, Continued ? 1 : 0);
}

/**

  Adds an arbitrary-Length integer to the Data structure.
  The integer will be encoded using the shortest possible atom.

  @param[in/out]     CreateStruct      Structure used to add the integer
  @param[in]         Data              Integer in host byte order that will be encoded and copied into Data structure
  @param[in]         DataSize          Length in bytes of the Data provided
  @param[in]         SignedInteger     TRUE if the integer is signed or FALSE if the integer is unsigned

**/
TCG_RESULT
EFIAPI
TcgAddInteger(
  TCG_CREATE_STRUCT  *CreateStruct,
  const VOID         *Data,
  UINT32             DataSize,
  BOOLEAN            SignedInteger
  )
{
  const UINT8* DataBytes;
  UINT32 ActualDataSize;
  BOOLEAN ValueIsNegative;

  NULL_CHECK(CreateStruct);
  NULL_CHECK(Data);

  if (DataSize == 0) {
    DEBUG ((DEBUG_INFO, "invalid DataSize=0\n"));
    return TcgResultFailure;
  }

  DataBytes = (const UINT8*)Data;

  // integer should be represented by smallest atom possible
  // so calculate real Data Size
  ValueIsNegative = SignedInteger && DataBytes[ DataSize - 1 ] & 0x80;

  // assumes native Data is little endian
  // shorten Data to smallest byte representation
  for (ActualDataSize = DataSize; ActualDataSize > 1; ActualDataSize--) {
    // ignore sign extended  FFs
    if (ValueIsNegative && (DataBytes[ActualDataSize - 1] != 0xFF)) {
      break;
    } else if (!ValueIsNegative && (DataBytes[ActualDataSize - 1] != 0)) {
      // ignore extended 00s
      break;
    }
  }

  return TcgAddAtom(CreateStruct, Data, ActualDataSize, TCG_ATOM_TYPE_INTEGER, SignedInteger ? 1 : 0);
}

/**
  Adds an 8-bit unsigned integer to the Data structure.

  @param[in/out]       CreateStruct        Structure used to add the integer
  @param[in]           Value               Integer Value to add

**/
TCG_RESULT
EFIAPI
TcgAddUINT8(
  TCG_CREATE_STRUCT   *CreateStruct,
  UINT8               Value
  )
{
  return TcgAddInteger(CreateStruct, &Value, sizeof(Value), FALSE);
}

/**

  Adds a 16-bit unsigned integer to the Data structure.

  @param[in/out]       CreateStruct        Structure used to add the integer
  @param[in]           Value               Integer Value to add

**/
TCG_RESULT
EFIAPI
TcgAddUINT16 (
  TCG_CREATE_STRUCT   *CreateStruct,
  UINT16              Value
  )
{
  return TcgAddInteger(CreateStruct, &Value, sizeof(Value), FALSE);
}

/**

  Adds a 32-bit unsigned integer to the Data structure.

  @param[in/out]        CreateStruct        Structure used to add the integer
  @param[in]            Value               Integer Value to add

**/
TCG_RESULT
EFIAPI
TcgAddUINT32(
  TCG_CREATE_STRUCT    *CreateStruct,
  UINT32               Value
  )
{
  return TcgAddInteger(CreateStruct, &Value, sizeof(Value), FALSE);
}


/**

  Adds a 64-bit unsigned integer to the Data structure.

  @param[in/out]      CreateStruct        Structure used to add the integer
  @param[in]          Value               Integer Value to add

**/
TCG_RESULT
EFIAPI
TcgAddUINT64(
  TCG_CREATE_STRUCT   *CreateStruct,
  UINT64              Value
  )
{
  return TcgAddInteger(CreateStruct, &Value, sizeof(Value), FALSE);
}

/**
  Adds a BOOLEAN to the Data structure.

  @param[in/out]       CreateStruct     Structure used to add the integer
  @param[in]           Value              BOOLEAN Value to add

**/
TCG_RESULT
EFIAPI
TcgAddBOOLEAN(
  TCG_CREATE_STRUCT    *CreateStruct,
  BOOLEAN              Value
  )
{
  return TcgAddInteger(CreateStruct, &Value, sizeof(Value), FALSE);
}

/**
  Add tcg uid info.

  @param [in/out]       CreateStruct       Structure used to add the integer
  @param                Uid                Input uid info.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgAddTcgUid(
  TCG_CREATE_STRUCT   *CreateStruct,
  TCG_UID             Uid
  )
{
  return TcgAddByteSequence(CreateStruct, &Uid, sizeof(TCG_UID), FALSE);
}

/**
  Add start list.

  @param [in/out]       CreateStruct       Structure used to add the integer

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgAddStartList(
  TCG_CREATE_STRUCT          *CreateStruct
  )
{
  return TcgAddRawByte(CreateStruct, TCG_TOKEN_STARTLIST);
}

/**
  Add end list.

  @param [in/out]       CreateStruct       Structure used to add the integer

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgAddEndList(
  TCG_CREATE_STRUCT       *CreateStruct
  )
{
  return TcgAddRawByte(CreateStruct, TCG_TOKEN_ENDLIST);
}

/**
  Add start name.

  @param [in/out]       CreateStruct       Structure used to add the integer

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgAddStartName(
  TCG_CREATE_STRUCT          *CreateStruct
  )
{
  return TcgAddRawByte(CreateStruct, TCG_TOKEN_STARTNAME);
}

/**
  Add end name.

  @param [in/out]       CreateStruct       Structure used to add the integer

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgAddEndName(
  TCG_CREATE_STRUCT          *CreateStruct
  )
{
  return TcgAddRawByte(CreateStruct, TCG_TOKEN_ENDNAME);
}

/**
  Add end call.

  @param [in/out]       CreateStruct       Structure used to add the integer

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgAddCall(
  TCG_CREATE_STRUCT      *CreateStruct
  )
{
  return TcgAddRawByte(CreateStruct, TCG_TOKEN_CALL);
}

/**
  Add end of data.

  @param [in/out]       CreateStruct       Structure used to add the integer

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgAddEndOfData(
  TCG_CREATE_STRUCT          *CreateStruct
  )
{
  return TcgAddRawByte(CreateStruct, TCG_TOKEN_ENDDATA);
}

/**
  Add end of session.

  @param [in/out]       CreateStruct       Structure used to add the integer

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgAddEndOfSession(
  TCG_CREATE_STRUCT              *CreateStruct
  )
{
  return TcgAddRawByte(CreateStruct, TCG_TOKEN_ENDSESSION);
}

/**
  Add start transaction.

  @param [in/out]       CreateStruct       Structure used to add the integer

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgAddStartTransaction(
  TCG_CREATE_STRUCT             *CreateStruct
  )
{
  return TcgAddRawByte(CreateStruct, TCG_TOKEN_STARTTRANSACTION);
}

/**
  Add end transaction.

  @param [in/out]       CreateStruct       Structure used to add the integer

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgAddEndTransaction(
  TCG_CREATE_STRUCT           *CreateStruct
  )
{
  return TcgAddRawByte(CreateStruct, TCG_TOKEN_ENDTRANSACTION);
}

/**
  Initial the tcg parse stucture.

  @param    ParseStruct    Input parse structure.
  @param    Buffer         Input buffer data.
  @param    BufferSize     Input buffer size.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgInitTcgParseStruct(
  TCG_PARSE_STRUCT          *ParseStruct,
  const VOID                *Buffer,
  UINT32                    BufferSize
  )
{
  UINT32 ComPacketLength;
  UINT32 PacketLength;

  NULL_CHECK(ParseStruct);
  NULL_CHECK(Buffer);

  if (BufferSize < sizeof(TCG_COM_PACKET)) {
    return (TcgResultFailureBufferTooSmall);
  }

  ParseStruct->ComPacket = (TCG_COM_PACKET*)Buffer;

  ComPacketLength = SwapBytes32(ParseStruct->ComPacket->LengthBE);

  if ((BufferSize - sizeof(TCG_COM_PACKET)) < ComPacketLength) {
    DEBUG ((DEBUG_INFO, "Buffer %u too small for ComPacket %u\n", BufferSize, ComPacketLength));
    return (TcgResultFailureBufferTooSmall);
  }

  ParseStruct->BufferSize = BufferSize;
  ParseStruct->Buffer = Buffer;

  ParseStruct->CurPacket = NULL;
  ParseStruct->CurSubPacket = NULL;
  ParseStruct->CurPtr = NULL;

  // if payload > 0, then must have a packet
  if (ComPacketLength != 0) {
    if (ComPacketLength < sizeof(TCG_PACKET)) {
      DEBUG ((DEBUG_INFO, "ComPacket too small for Packet\n"));
      return (TcgResultFailureBufferTooSmall);
    }
    ParseStruct->CurPacket = (TCG_PACKET*)ParseStruct->ComPacket->Payload;

    PacketLength = SwapBytes32(ParseStruct->CurPacket->LengthBE);

    if (PacketLength > 0) {
      if (PacketLength < sizeof(TCG_SUB_PACKET)) {
          DEBUG ((DEBUG_INFO, "Packet too small for SubPacket\n"));
          return (TcgResultFailureBufferTooSmall);
      }

      ParseStruct->CurSubPacket = (TCG_SUB_PACKET*)ParseStruct->CurPacket->Payload;
    }
  }

  //TODO should check for method status list at this point?

  return (TcgResultSuccess);
}

/**
  Get next token info.

  @param    ParseStruct      Input parse structure info.
  @param    TcgToken         return the tcg token info.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextToken(
  TCG_PARSE_STRUCT      *ParseStruct,
  TCG_TOKEN             *TcgToken
  )
{
  const UINT8* EndOfSubPacket;
  UINT8* TokenEnd;
  UINT8 Hdr;
  TCG_SIMPLE_TOKEN_SHORT_ATOM* TmpShort;
  const TCG_SIMPLE_TOKEN_MEDIUM_ATOM* TmpMed;
  const TCG_SIMPLE_TOKEN_LONG_ATOM* TmpLong;

  NULL_CHECK(ParseStruct);
  NULL_CHECK(TcgToken);

  if (ParseStruct->ComPacket == NULL ||
      ParseStruct->CurPacket == NULL ||
      ParseStruct->CurSubPacket == NULL
     ) {
    DEBUG ((DEBUG_INFO, "unexpected state: ComPacket=%p CurPacket=%p CurSubPacket=%p\n", ParseStruct->ComPacket, ParseStruct->CurPacket, ParseStruct->CurSubPacket));
    return TcgResultFailureInvalidAction;
  }

  // initial call, start at sub packet
  if (ParseStruct->CurPtr == NULL) {
    ParseStruct->CurPtr = ParseStruct->CurSubPacket->Payload;
  }

  EndOfSubPacket = ParseStruct->CurSubPacket->Payload + SwapBytes32(ParseStruct->CurSubPacket->LengthBE);
  TokenEnd = NULL;

  // confirmed that subpacket Length falls within end of Buffer and TCG_COM_PACKET,
  // so simply need to verify the loop stays within current subpacket
  if (ParseStruct->CurPtr >= EndOfSubPacket) {
    DEBUG ((DEBUG_INFO, "ParseStruct->CurPtr >= EndOfSubPacket\n"));
    return (TcgResultFailureEndBuffer);
  }

  Hdr = *ParseStruct->CurPtr;
  TcgToken->HdrStart = ParseStruct->CurPtr;

  // Tiny Atom range
  if (Hdr <= 0x7F) {
    // tiny atom Header is only 1 byte, so don't need to verify Size before cast and access
    TcgToken->Type = TcgTokenTypeTinyAtom;

    TokenEnd = TcgToken->HdrStart + sizeof(TCG_SIMPLE_TOKEN_TINY_ATOM);

    // verify caller will have enough Size to reference token
    if (TokenEnd >= EndOfSubPacket) {
      DEBUG ((DEBUG_INFO, "Tiny Atom TokenEnd >= EndOfSubPacket\n"));
      return (TcgResultFailureEndBuffer);
    }
  }
  // Short Atom Range
  else if (0x80 <= Hdr && Hdr <= 0xBF) {
    // short atom Header is only 1 byte, so don't need to verify Size before cast and access
    TmpShort = (TCG_SIMPLE_TOKEN_SHORT_ATOM*)(ParseStruct->CurPtr);
    TcgToken->Type = TcgTokenTypeShortAtom;

    TokenEnd = (TcgToken->HdrStart + sizeof(TCG_SIMPLE_TOKEN_SHORT_ATOM) + TmpShort->ShortAtomBits.Length);

    // verify caller will have enough Size to reference token
    if (TokenEnd >= EndOfSubPacket) {
      DEBUG ((DEBUG_INFO, "Short Atom TokenEnd >= EndOfSubPacket\n"));
      return (TcgResultFailureEndBuffer);
    }
  }
  // Medium Atom Range
  else if (0xC0 <= Hdr && Hdr <= 0xDF) {
    if (TcgToken->HdrStart + sizeof(TCG_SIMPLE_TOKEN_MEDIUM_ATOM) >= EndOfSubPacket) {
      return (TcgResultFailureEndBuffer);
    }
    TmpMed = (const TCG_SIMPLE_TOKEN_MEDIUM_ATOM*)ParseStruct->CurPtr;
    TcgToken->Type = TcgTokenTypeMediumAtom;
    TokenEnd = TcgToken->HdrStart + sizeof(TCG_SIMPLE_TOKEN_MEDIUM_ATOM) +
               ((TmpMed->MediumAtomBits.LengthHigh << TCG_MEDIUM_ATOM_LENGTH_HIGH_SHIFT) |
                TmpMed->MediumAtomBits.LengthLow);

    // verify caller will have enough Size to reference token
    if (TokenEnd >= EndOfSubPacket) {
      DEBUG ((DEBUG_INFO, "Medium Atom TokenEnd >= EndOfSubPacket\n"));
      return (TcgResultFailureEndBuffer);
    }
  }
  // Long Atom Range
  else if (0xE0 <= Hdr && Hdr <= 0xE3) {
    if (TcgToken->HdrStart + sizeof(TCG_SIMPLE_TOKEN_LONG_ATOM) >= EndOfSubPacket) {
      return (TcgResultFailureEndBuffer);
    }
    TmpLong = (const TCG_SIMPLE_TOKEN_LONG_ATOM*)ParseStruct->CurPtr;
    TcgToken->Type = TcgTokenTypeLongAtom;

    TokenEnd = TcgToken->HdrStart + sizeof(TCG_SIMPLE_TOKEN_LONG_ATOM) +
               ((TmpLong->LongAtomBits.LengthHigh << TCG_LONG_ATOM_LENGTH_HIGH_SHIFT) |
                (TmpLong->LongAtomBits.LengthMid << TCG_LONG_ATOM_LENGTH_MID_SHIFT)   |
                TmpLong->LongAtomBits.LengthLow);

    // verify caller will have enough Size to reference token
    if (TokenEnd >= EndOfSubPacket) {
      DEBUG ((DEBUG_INFO, "Long Atom TokenEnd >= EndOfSubPacket\n"));
      return (TcgResultFailureEndBuffer);
    }
  } else {
    // single byte tokens
    switch (Hdr) {
      case TCG_TOKEN_STARTLIST:
          TcgToken->Type = TcgTokenTypeStartList;
          break;
      case TCG_TOKEN_ENDLIST:
          TcgToken->Type = TcgTokenTypeEndList;
          break;
      case TCG_TOKEN_STARTNAME:
          TcgToken->Type = TcgTokenTypeStartName;
          break;
      case TCG_TOKEN_ENDNAME:
          TcgToken->Type = TcgTokenTypeEndName;
          break;
      case TCG_TOKEN_CALL:
          TcgToken->Type = TcgTokenTypeCall;
          break;
      case TCG_TOKEN_ENDDATA:
          TcgToken->Type = TcgTokenTypeEndOfData;
          break;
      case TCG_TOKEN_ENDSESSION:
          TcgToken->Type = TcgTokenTypeEndOfSession;
          break;
      case TCG_TOKEN_STARTTRANSACTION:
          TcgToken->Type = TcgTokenTypeStartTransaction;
          break;
      case TCG_TOKEN_ENDTRANSACTION:
          TcgToken->Type = TcgTokenTypeEndTransaction;
          break;
      case TCG_TOKEN_EMPTY:
          TcgToken->Type = TcgTokenTypeEmptyAtom;
          break;
      default:
          DEBUG ((DEBUG_INFO, "WARNING: reserved token Type 0x%02X\n", Hdr));
          TcgToken->Type = TcgTokenTypeReserved;
          break;
    }
    ParseStruct->CurPtr++;
    TokenEnd = TcgToken->HdrStart + 1;
  }

  // increment curptr for next call
  ParseStruct->CurPtr = TokenEnd;
  return (TcgResultSuccess);
}

/**
  Get atom info.

  @param    TcgToken          Input token info.
  @param    HeaderLength      return the header length.
  @param    DataLength        return the data length.
  @param    ByteOrInt         return the atom Type.
  @param    SignOrCont        return the sign or count info.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetAtomInfo(
  const TCG_TOKEN      *TcgToken,
  UINT32               *HeaderLength,
  UINT32               *DataLength,
  UINT8                *ByteOrInt,
  UINT8                *SignOrCont
  )
{
  TCG_SIMPLE_TOKEN_TINY_ATOM* TinyAtom;
  TCG_SIMPLE_TOKEN_SHORT_ATOM* ShortAtom;
  TCG_SIMPLE_TOKEN_MEDIUM_ATOM* MediumAtom;
  TCG_SIMPLE_TOKEN_LONG_ATOM* LongAtom;

  NULL_CHECK(TcgToken);
  NULL_CHECK(HeaderLength);
  NULL_CHECK(DataLength);
  NULL_CHECK(ByteOrInt);
  NULL_CHECK(SignOrCont);

  switch (TcgToken->Type) {
    case TcgTokenTypeTinyAtom: {
      TinyAtom = (TCG_SIMPLE_TOKEN_TINY_ATOM*)TcgToken->HdrStart;
      *ByteOrInt      = TCG_ATOM_TYPE_INTEGER;
      *SignOrCont     = TinyAtom->TinyAtomBits.Sign;
      *HeaderLength   = 0;
      *DataLength     = 0; // tiny atom must be handled as a special case - Header and Data in the same byte
      return TcgResultSuccess;
    }

    case TcgTokenTypeShortAtom: {
      ShortAtom = (TCG_SIMPLE_TOKEN_SHORT_ATOM*)TcgToken->HdrStart;
      *ByteOrInt      = ShortAtom->ShortAtomBits.ByteOrInt;
      *SignOrCont     = ShortAtom->ShortAtomBits.SignOrCont;
      *HeaderLength   = sizeof(TCG_SIMPLE_TOKEN_SHORT_ATOM);
      *DataLength     = ShortAtom->ShortAtomBits.Length;
      return TcgResultSuccess;
    }

    case TcgTokenTypeMediumAtom: {
      MediumAtom = (TCG_SIMPLE_TOKEN_MEDIUM_ATOM*)TcgToken->HdrStart;
      *ByteOrInt      = MediumAtom->MediumAtomBits.ByteOrInt;
      *SignOrCont     = MediumAtom->MediumAtomBits.SignOrCont;
      *HeaderLength   = sizeof(TCG_SIMPLE_TOKEN_MEDIUM_ATOM);
      *DataLength     = (MediumAtom->MediumAtomBits.LengthHigh << TCG_MEDIUM_ATOM_LENGTH_HIGH_SHIFT) | MediumAtom->MediumAtomBits.LengthLow;
      return TcgResultSuccess;
    }

    case TcgTokenTypeLongAtom: {
      LongAtom = (TCG_SIMPLE_TOKEN_LONG_ATOM*)TcgToken->HdrStart;
      *ByteOrInt      = LongAtom->LongAtomBits.ByteOrInt;
      *SignOrCont     = LongAtom->LongAtomBits.SignOrCont;
      *HeaderLength   = sizeof(TCG_SIMPLE_TOKEN_LONG_ATOM);
      *DataLength     = (LongAtom->LongAtomBits.LengthHigh << TCG_LONG_ATOM_LENGTH_HIGH_SHIFT) |
                        (LongAtom->LongAtomBits.LengthMid << TCG_LONG_ATOM_LENGTH_MID_SHIFT) |
                        LongAtom->LongAtomBits.LengthLow;
      return TcgResultSuccess;
    }

    default:
      DEBUG ((DEBUG_INFO, "Token Type is not simple atom (%d)\n", TcgToken->Type));
      return (TcgResultFailureInvalidType);
  }
}

/**
  Get token specified value.

  @param    TcgToken   Input token info.
  @param    Value      return the value.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetTokenUINT64(
  const TCG_TOKEN      *TcgToken,
  UINT64               *Value
  )
{
  UINT32 HdrLength;
  UINT32 DataLength;
  UINT8 ByteOrInt;
  UINT8 IsSigned;
  TCG_SIMPLE_TOKEN_TINY_ATOM* TmpTiny;
  const UINT8* Data;
  UINT32 Index;

  NULL_CHECK(TcgToken);
  NULL_CHECK(Value);

  Index = 0;
  *Value = 0;
  ERROR_CHECK(TcgGetAtomInfo(TcgToken, &HdrLength, &DataLength, &ByteOrInt, &IsSigned));

  if (ByteOrInt != TCG_ATOM_TYPE_INTEGER) {
    DEBUG ((DEBUG_INFO, "Invalid Type, expected integer not byte sequence\n"));
    return TcgResultFailureInvalidType;
  }

  if (IsSigned != 0) {
    DEBUG ((DEBUG_INFO, "Integer is signed, expected unsigned\n"));
    return TcgResultFailureInvalidType;
  }

  // special case for tiny atom
  // Header and Data are in one byte, so extract only the Data bitfield
  if (TcgToken->Type == TcgTokenTypeTinyAtom) {
    TmpTiny = (TCG_SIMPLE_TOKEN_TINY_ATOM*)TcgToken->HdrStart;
    *Value = TmpTiny->TinyAtomBits.Data;
    return TcgResultSuccess;
  }

  if (DataLength > sizeof(UINT64)) {
    DEBUG ((DEBUG_INFO, "Length %d is greater than Size of UINT64\n", DataLength));
    return TcgResultFailureBufferTooSmall;
  }

  // read big-endian integer
  Data = TcgToken->HdrStart + HdrLength;
  for (Index = 0; Index < DataLength; Index++) {
    *Value = LShiftU64(*Value, 8) | Data[Index];
  }

  return TcgResultSuccess;
}

/**
  Get token byte sequence.

  @param    TcgToken   Input token info.
  @param    Length     Input the length info.

  @retval   Return the value data.

**/
UINT8*
EFIAPI
TcgGetTokenByteSequence(
  const TCG_TOKEN     *TcgToken,
  UINT32              *Length
  )
{
  UINT32 HdrLength;
  UINT8 ByteOrInt;
  UINT8 SignOrCont;

  if (TcgToken == NULL || Length == NULL) {
    return NULL;
  }

  *Length = 0;
  if (TcgGetAtomInfo(TcgToken, &HdrLength, Length, &ByteOrInt, &SignOrCont) != TcgResultSuccess) {
    DEBUG ((DEBUG_INFO, "Failed to get simple token info\n"));
    return NULL;
  }

  if (ByteOrInt != TCG_ATOM_TYPE_BYTE) {
    DEBUG ((DEBUG_INFO, "Invalid Type, expected byte sequence not integer\n"));
    return NULL;
  }

  return (TcgToken->HdrStart + HdrLength);
}

/**
  Get next specify value.

  @param    ParseStruct   Input parse structure.
  @param    Value         Return vlaue.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextUINT8(
  TCG_PARSE_STRUCT      *ParseStruct,
  UINT8                 *Value
  )
{
  UINT64 Value64;
  TCG_TOKEN Tok;

  NULL_CHECK(Value);

  ERROR_CHECK(TcgGetNextToken(ParseStruct, &Tok));
  ERROR_CHECK(TcgGetTokenUINT64(&Tok, &Value64));

  if (Value64 > MAX_UINT8) {
    return TcgResultFailure;
  }

  *Value = (UINT8)Value64;

  return TcgResultSuccess;
}

/**
  Get next specify value.

  @param    ParseStruct   Input parse structure.
  @param    Value         Return vlaue.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextUINT16(
  TCG_PARSE_STRUCT     *ParseStruct,
  UINT16               *Value
  )
{
  UINT64 Value64;
  TCG_TOKEN Tok;

  NULL_CHECK(Value);

  ERROR_CHECK(TcgGetNextToken(ParseStruct, &Tok));
  ERROR_CHECK(TcgGetTokenUINT64(&Tok, &Value64));

  if (Value64 > MAX_UINT16) {
    return TcgResultFailure;
  }

  *Value = (UINT16)Value64;

  return TcgResultSuccess;
}

/**
  Get next specify value.

  @param    ParseStruct   Input parse structure.
  @param    Value         Return vlaue.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextUINT32(
  TCG_PARSE_STRUCT          *ParseStruct,
  UINT32                    *Value
  )
{
  UINT64 Value64;
  TCG_TOKEN Tok;

  NULL_CHECK(Value);

  ERROR_CHECK(TcgGetNextToken(ParseStruct, &Tok));
  ERROR_CHECK(TcgGetTokenUINT64(&Tok, &Value64));

  if (Value64 > MAX_UINT32) {
    return TcgResultFailure;
  }

  *Value = (UINT32)Value64;

  return TcgResultSuccess;
}

/**
  Get next specify value.

  @param    ParseStruct   Input parse structure.
  @param    Value         Return vlaue.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextUINT64(
  TCG_PARSE_STRUCT           *ParseStruct,
  UINT64                     *Value
  )
{
  TCG_TOKEN Tok;
  ERROR_CHECK(TcgGetNextToken(ParseStruct, &Tok));
  ERROR_CHECK(TcgGetTokenUINT64(&Tok, Value));
  return TcgResultSuccess;
}

/**
  Get next specify value.

  @param    ParseStruct   Input parse structure.
  @param    Value         Return vlaue.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextBOOLEAN(
  TCG_PARSE_STRUCT        *ParseStruct,
  BOOLEAN                 *Value
  )
{
  UINT64 Value64;
  TCG_TOKEN Tok;

  NULL_CHECK(Value);

  ERROR_CHECK(TcgGetNextToken(ParseStruct, &Tok));
  ERROR_CHECK(TcgGetTokenUINT64(&Tok, &Value64));

  if (Value64 > 1) {
    return TcgResultFailure;
  }

  *Value = (BOOLEAN)Value64;

  return TcgResultSuccess;
}

/**
  Get next tcg uid info.

  @param    ParseStruct    Input parse structure.
  @param    Uid            Get the uid info.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextTcgUid(
  TCG_PARSE_STRUCT         *ParseStruct,
  TCG_UID                  *Uid
  )
{
  TCG_TOKEN Tok;
  UINT32 Length;
  const UINT8* ByteSeq;

  NULL_CHECK(Uid);

  ERROR_CHECK(TcgGetNextToken(ParseStruct, &Tok));
  ByteSeq = TcgGetTokenByteSequence(&Tok, &Length);

  if (Length != sizeof(TCG_UID)) {
    DEBUG ((DEBUG_INFO, "Token Length %u != TCG_UID Size %u\n", Length, (UINT32)sizeof(TCG_UID)));
    return TcgResultFailure;
  }

  ASSERT (ByteSeq != NULL);

  CopyMem(Uid, ByteSeq, sizeof(TCG_UID));

  return TcgResultSuccess;
}

/**
  Get next byte sequence.

  @param    ParseStruct     Input parse structure.
  @param    Data            return the data.
  @param    Length          return the length.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextByteSequence(
  TCG_PARSE_STRUCT      *ParseStruct,
  const VOID            **Data,
  UINT32                *Length
  )
{
  TCG_TOKEN Tok;
  const UINT8* Bs;

  ERROR_CHECK(TcgGetNextToken(ParseStruct, &Tok));
  Bs = TcgGetTokenByteSequence(&Tok, Length);

  if (Bs == NULL) {
    return TcgResultFailure;
  }
  *Data = Bs;
  return TcgResultSuccess;
}

/**
  Get next token Type.

  @param    ParseStruct    Input parse structure.
  @param    Type           Input the type need to check.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextTokenType(
  TCG_PARSE_STRUCT        *ParseStruct,
  TCG_TOKEN_TYPE          Type
  )
{
  TCG_TOKEN Tok;
  ERROR_CHECK(TcgGetNextToken(ParseStruct, &Tok));
  if (Tok.Type != Type) {
    DEBUG ((DEBUG_INFO, "expected Type %u, got Type %u\n", Type, Tok.Type));
    return TcgResultFailure;
  }
  return TcgResultSuccess;
}

/**
  Get next start list.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextStartList(
  TCG_PARSE_STRUCT          *ParseStruct
  )
{
  return TcgGetNextTokenType(ParseStruct, TcgTokenTypeStartList);
}

/**
  Get next end list.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextEndList(
  TCG_PARSE_STRUCT             *ParseStruct
  )
{
  return TcgGetNextTokenType(ParseStruct, TcgTokenTypeEndList);
}

/**
  Get next start name.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextStartName(
  TCG_PARSE_STRUCT              *ParseStruct
  )
{
  return TcgGetNextTokenType(ParseStruct, TcgTokenTypeStartName);
}

/**
  Get next end name.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextEndName(
  TCG_PARSE_STRUCT               *ParseStruct
  )
{
  return TcgGetNextTokenType(ParseStruct, TcgTokenTypeEndName);
}

/**
  Get next call.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextCall(
  TCG_PARSE_STRUCT                   *ParseStruct
  )
{
  return TcgGetNextTokenType(ParseStruct, TcgTokenTypeCall);
}

/**
  Get next end data.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextEndOfData(
  TCG_PARSE_STRUCT                    *ParseStruct
  )
{
  return TcgGetNextTokenType(ParseStruct, TcgTokenTypeEndOfData);
}

/**
  Get next end of session.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextEndOfSession(
  TCG_PARSE_STRUCT                      *ParseStruct
  )
{
  return TcgGetNextTokenType(ParseStruct, TcgTokenTypeEndOfSession);
}

/**
  Get next start transaction.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextStartTransaction(
  TCG_PARSE_STRUCT                        *ParseStruct
  )
{
  return TcgGetNextTokenType(ParseStruct, TcgTokenTypeStartTransaction);
}

/**
  Get next end transaction.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextEndTransaction(
  TCG_PARSE_STRUCT                  *ParseStruct
  )
{
  return TcgGetNextTokenType(ParseStruct, TcgTokenTypeEndTransaction);
}
