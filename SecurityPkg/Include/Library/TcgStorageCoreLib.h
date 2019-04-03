/** @file
  Public API for the Tcg Core library to perform the lowest level TCG Data encoding.

  (TCG Storage Architecture Core Specification, Version 2.01, Revision 1.00,
  https://trustedcomputinggroup.org/tcg-storage-architecture-core-specification/)

  Check http://trustedcomputinggroup.org for latest specification updates.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TCG_CORE_H_
#define _TCG_CORE_H_

#include <IndustryStandard/TcgStorageCore.h>

#define ERROR_CHECK(arg)                                                         \
  {                                                                              \
    TCG_RESULT ret = (arg);                                                      \
    if (ret != TcgResultSuccess) {                                               \
      DEBUG ((DEBUG_INFO, "ERROR_CHECK failed at %a:%u\n", __FILE__, __LINE__)); \
      return ret;                                                                \
    }                                                                            \
  }

#define METHOD_STATUS_ERROR_CHECK(arg, failRet)                                                  \
  if ((arg) != TCG_METHOD_STATUS_CODE_SUCCESS) {                                                 \
    DEBUG ((DEBUG_INFO, "Method Status error: 0x%02X (%a)\n", arg, TcgMethodStatusString(arg))); \
    return (failRet);                                                                            \
  }

#define NULL_CHECK(arg)                                                                   \
  do {                                                                                    \
    if ((arg) == NULL) {                                                                  \
      DEBUG ((DEBUG_INFO, "NULL_CHECK(%a) failed at %a:%u\n", #arg, __FILE__, __LINE__)); \
      return TcgResultFailureNullPointer;                                                 \
    }                                                                                     \
  } while (0)

#pragma pack(1)

/**
Tcg result codes.

The result code indicates if the Tcg function call was successful or not
**/
typedef enum {
  //
  // This is the return result upon successful completion of a Tcg function call
  //
  TcgResultSuccess,

  //
  // This is the return "catchall" result for the failure of a Tcg function call
  //
  TcgResultFailure,

  //
  // This is the return result if a required parameter was Null for a Tcg function call
  //
  TcgResultFailureNullPointer,

  //
  // This is the return result if a required buffersize was 0 for a Tcg function call
  //
  TcgResultFailureZeroSize,

  //
  // This is the return result if a Tcg function call was executed out of order.
  // For instance, starting a Tcg subpacket before starting its Tcg packet.
  //
  TcgResultFailureInvalidAction,

  //
  // This is the return result if the buffersize provided is not big enough to add a requested Tcg encoded item.
  //
  TcgResultFailureBufferTooSmall,

  //
  // This is the return result for a Tcg parse function if the end of the parsed Buffer is reached, yet Data is still attempted to be retrieved.
  // For instance, attempting to retrieve another Tcg token from the Buffer after it has reached the end of the Tcg subpacket payload.
  //
  TcgResultFailureEndBuffer,

  //
  // This is the return result for a Tcg parse function if the Tcg Token item requested is not the expected type.
  // For instance, the caller requested to receive an integer and the Tcg token was a byte sequence.
  //
  TcgResultFailureInvalidType,
} TCG_RESULT;

//
// Structure that is used to build the Tcg ComPacket.  It contains the start Buffer pointer and the current position of the
// Tcg ComPacket, current Tcg Packet and Tcg SubPacket. This structure must be initialized
// by calling tcgInitTcgCreateStruct before it is used as parameter to any other Tcg function.
// This structure should NOT be directly modified by the client of this library.
//
//  NOTE:  WE MAY MAKE THIS AN ABSTRACT STRUCTURE WITH A DEFINED SIZE AND KEEP THE VARIABLES
//         INTERNAL AND ONLY KNOWN TO THE TCG LIBRARY
//
// tcgInitTcgCreateStruct
//
typedef struct {
  //
  // Buffer allocated and freed by the client of the Tcg library.
  // This is the Buffer that shall contain the final Tcg encoded compacket.
  //
  VOID              *Buffer;

  //
  // Size of the Buffer provided.
  //
  UINT32            BufferSize;

  //
  //Pointer to the start of the Tcg ComPacket.  It should point to a location within Buffer.
  //
  TCG_COM_PACKET    *ComPacket;

  //
  // Current Tcg Packet that is being created.  It should point to a location within Buffer.
  //
  TCG_PACKET        *CurPacket;

  //
  // Current Tcg SubPacket that is being created.  It should point to a location within Buffer.
  //
  TCG_SUB_PACKET    *CurSubPacket;

  //
  // Flag used to indicate if the Buffer of the structure should be filled out.
  // This is intended to be used to support a use-case where the client of library
  // can perform all the desired tcg calls to determine what the actual Size of the final compacket will be.
  // Then the client can allocate the required Buffer Size and re-run the tcg calls.
  // THIS MAY NOT BE IMPLEMENTED... REQUIRES MORE THOUGHT BECAUSE YOU CANNOT SOLVE ISSUE FOR RECEIVE
  //
  BOOLEAN          DryRun;
} TCG_CREATE_STRUCT;

//
// Structure that is used to parse the Tcg response received.  It contains the response Buffer pointer
// and the current position of the Tcg ComPacket, current Tcg Packet and Tcg SubPacket being parsed.
// This structure must be initialized by calling tcgInitTcgParseStruct before it is used as parameter to any other Tcg parse function.
// This structure should NOT be directly modified by the client of this library.
//
//  NOTE:  WE MAY MAKE THIS AN ABSTRACT STRUCTURE WITH A DEFINED SIZE AND KEEP THE VARIABLES
//         INTERNAL AND ONLY KNOWN TO THE TCG LIBRARY
//
// @sa tcgInitTcgParseStruct
//
typedef struct  {
  //
  // Buffer allocated and freed by the client of the Tcg library.
  // This is the Buffer that contains the Tcg response to decode/parse.
  //
  const VOID*         Buffer;

  //
  //Size of the Buffer provided.
  //
  UINT32              BufferSize;

  //
  // Pointer to the start of the Tcg ComPacket.  It should point to a location within Buffer.
  //
  TCG_COM_PACKET      *ComPacket;

  //
  // Current Tcg Packet that is being created.  It should point to a location within Buffer.
  //
  TCG_PACKET          *CurPacket;

  //
  // Current Tcg SubPacket that is being created.  It should point to a location within Buffer.
  //
  TCG_SUB_PACKET      *CurSubPacket;

  //
  // Current pointer within the current subpacket payload.
  //
  UINT8               *CurPtr;
} TCG_PARSE_STRUCT ;


//
// Structure that is used to represent a Tcg Token that is retrieved by Tcg parse functions.
//
typedef struct {
  //
  // Describes the type of Tcg token the Hdr start points to.
  //
  TCG_TOKEN_TYPE    Type;

  //
  // Pointer to the beginning of the Header of the Tcg token
  //
  UINT8             *HdrStart;
} TCG_TOKEN ;

/**

  Required to be called before calling any other Tcg functions with the TCG_CREATE_STRUCT.
  Initializes the packet variables to NULL.  Additionally, the buffer will be memset.

  @param[in/out]   CreateStruct   Structure to initialize
  @param[in]       Buffer         Buffer allocated by client of library.  It will contain the Tcg encoded packet.  This cannot be null.
  @param[in]       BufferSize     Size of buffer provided.  It cannot be 0.

**/
TCG_RESULT
EFIAPI
TcgInitTcgCreateStruct(
  TCG_CREATE_STRUCT      *CreateStruct,
  VOID                   *Buffer,
  UINT32                 BufferSize
  );


/**

  Encodes the ComPacket header to the data structure.

  @param[in/out]    CreateStruct       Structure to initialize
  @param[in]        ComId              ComID of the Tcg ComPacket.
  @param[in]        ComIdExtension     ComID Extension of the Tcg ComPacket.

**/
TCG_RESULT
EFIAPI
TcgStartComPacket(
  TCG_CREATE_STRUCT   *CreateStruct,
  UINT16              ComId,
  UINT16              ComIdExtension
  );


/**

  Starts a new ComPacket in the Data structure.

  @param[in/out]    CreateStruct       Structure used to add Tcg Packet
  @param[in]        Tsn                Packet Tper session number
  @param[in]        Hsn                Packet Host session number
  @param[in]        SeqNumber          Packet Sequence Number
  @param[in]        AckType            Packet Acknowledge Type
  @param[in]        Ack                Packet Acknowledge

**/
TCG_RESULT
EFIAPI
TcgStartPacket(
  TCG_CREATE_STRUCT       *CreateStruct,
  UINT32                  Tsn,
  UINT32                  Hsn,
  UINT32                  SeqNumber,
  UINT16                  AckType,
  UINT32                  Ack
  );

/**

  Starts a new SubPacket in the Data structure.

  @param[in/out]    CreateStruct       Structure used to start Tcg SubPacket
  @param[in]        Kind               SubPacket kind

**/
TCG_RESULT
EFIAPI
TcgStartSubPacket(
  TCG_CREATE_STRUCT   *CreateStruct,
  UINT16              Kind
  );


/**

  Ends the current SubPacket in the Data structure.  This function will also perform the 4-byte padding
  required for Subpackets.

  @param[in/out]        CreateStruct       Structure used to end the current Tcg SubPacket

**/
TCG_RESULT
EFIAPI
TcgEndSubPacket(
  TCG_CREATE_STRUCT   *CreateStruct
  );


/**

  Ends the current Packet in the Data structure.

  @param[in/out]       CreateStruct        Structure used to end the current Tcg Packet

**/
TCG_RESULT
EFIAPI
TcgEndPacket(
  TCG_CREATE_STRUCT     *CreateStruct
  );


/**

  Ends the ComPacket in the Data structure and ret

  @param[in/out]       CreateStruct    Structure used to end the Tcg ComPacket
  @param[in/out]       Size                Describes the Size of the entire ComPacket (Header and payload). Filled out by function.

**/
TCG_RESULT
EFIAPI
TcgEndComPacket(
  TCG_CREATE_STRUCT   *CreateStruct,
  UINT32              *Size
  );

/**
  Adds a single raw token byte to the Data structure.

  @param[in/out]   CreateStruct      Structure used to add the byte
  @param [in]      Byte              Byte to add

**/
TCG_RESULT
EFIAPI
TcgAddRawByte(
  TCG_CREATE_STRUCT  *CreateStruct,
  UINT8              Byte
  );


/**

  Adds the Data parameter as a byte sequence to the Data structure.

  @param [in/out]    CreateStruct   Structure used to add the byte sequence
  @param[in]         Data           Byte sequence that will be encoded and copied into Data structure
  @param[in]         DataSize       Length of Data provided
  @param[in]         Continued      TRUE if byte sequence is continued or
                                    FALSE if the Data contains the entire byte sequence to be encoded

**/
TCG_RESULT
EFIAPI
TcgAddByteSequence(
  TCG_CREATE_STRUCT     *CreateStruct,
  const VOID            *Data,
  UINT32                DataSize,
  BOOLEAN               Continued
  );


/**

  Adds an arbitrary-Length integer to the Data structure.

  The integer will be encoded using the shortest possible atom.

  @param[in/out]     CreateStruct        Structure used to add the integer
  @param[in]         Data                Integer in host byte order that will be encoded and copied into Data structure
  @param[in]         DataSize            Length in bytes of the Data provided
  @param[in]         SignedInteger       TRUE if the integer is signed or FALSE if the integer is unsigned

**/
TCG_RESULT
EFIAPI
TcgAddInteger(
  TCG_CREATE_STRUCT  *CreateStruct,
  const VOID         *Data,
  UINT32             DataSize,
  BOOLEAN            SignedInteger
  );


/**
  Adds an 8-bit unsigned integer to the Data structure.

  @param[in/out]     CreateStruct        Structure used to add the integer
  @param[in]         Value               Integer Value to add

**/
TCG_RESULT
EFIAPI
TcgAddUINT8(
  TCG_CREATE_STRUCT   *CreateStruct,
  UINT8               Value
  );

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
  );

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
  );


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
  );

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
  );

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
  );

/**
 Adds a Start List token to the Data structure.

 @param[in/out]   CreateStruct      Structure used to add the token

**/
TCG_RESULT
EFIAPI
TcgAddStartList(
  TCG_CREATE_STRUCT    *CreateStruct
  );


/**

 Adds an End List token to the Data structure.

 @param [in/out]    CreateStruct      Structure used to add the token

**/
TCG_RESULT
EFIAPI
TcgAddEndList(
  TCG_CREATE_STRUCT     *CreateStruct
  );


/**
 Adds a Start Name token to the Data structure.

 @param[in/out]    CreateStruct    Structure used to add the token

**/
TCG_RESULT
EFIAPI
TcgAddStartName(
  TCG_CREATE_STRUCT      *CreateStruct
  );


/**

 Adds an End Name token to the Data structure.

 @param [in/out]   CreateStruct      Structure used to add the token

**/
TCG_RESULT
EFIAPI
TcgAddEndName(
  TCG_CREATE_STRUCT            *CreateStruct
  );


/**
 Adds a Call token to the Data structure.

 @param  [in/out]    CreateStruct    Structure used to add the token

**/
TCG_RESULT
EFIAPI
TcgAddCall(
  TCG_CREATE_STRUCT            *CreateStruct
  );


/**

Adds an End of Data token to the Data structure.

@param[in/out]   CreateStruct    Structure used to add the token

**/
TCG_RESULT
EFIAPI
TcgAddEndOfData(
  TCG_CREATE_STRUCT            *CreateStruct
  );


/**

Adds an End of Session token to the Data structure.

@param [in/out]    CreateStruct  Structure used to add the token

**/
TCG_RESULT
EFIAPI
TcgAddEndOfSession(
  TCG_CREATE_STRUCT             *CreateStruct
  );


/**
 Adds a Start Transaction token to the Data structure.

 @param [in/out]    CreateStruct  Structure used to add the token

**/
TCG_RESULT
EFIAPI
TcgAddStartTransaction(
  TCG_CREATE_STRUCT              *CreateStruct
  );


/**
 Adds an End Transaction token to the Data structure.

 @param[in/out]   CreateStruct   Structure used to add the token

**/
TCG_RESULT
EFIAPI
TcgAddEndTransaction(
  TCG_CREATE_STRUCT             *CreateStruct
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );


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
  );


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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Get next start list.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextStartList(
  TCG_PARSE_STRUCT          *ParseStruct
  );

/**
  Get next end list.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextEndList(
  TCG_PARSE_STRUCT             *ParseStruct
  );

/**
  Get next start name.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextStartName(
  TCG_PARSE_STRUCT              *ParseStruct
  );

/**
  Get next end name.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextEndName(
  TCG_PARSE_STRUCT               *ParseStruct
  );

/**
  Get next call.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextCall(
  TCG_PARSE_STRUCT                   *ParseStruct
  );

/**
  Get next end data.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextEndOfData(
  TCG_PARSE_STRUCT                    *ParseStruct
  );

/**
  Get next end of session.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextEndOfSession(
  TCG_PARSE_STRUCT                      *ParseStruct
  );

/**
  Get next start transaction.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextStartTransaction(
  TCG_PARSE_STRUCT                        *ParseStruct
  );

/**
  Get next end transaction.

  @param    ParseStruct   Input parse structure.

  @retval   return the action result.

**/
TCG_RESULT
EFIAPI
TcgGetNextEndTransaction(
  TCG_PARSE_STRUCT                  *ParseStruct
  );

// end of parse functions


typedef
BOOLEAN
(EFIAPI* TCG_LEVEL0_ENUM_CALLBACK) (
  const TCG_LEVEL0_DISCOVERY_HEADER      *DiscoveryHeader,
  TCG_LEVEL0_FEATURE_DESCRIPTOR_HEADER   *Feature,
  UINTN                                  FeatureSize, // includes header
  VOID                                   *Context
);

/**
  Adds call token and method Header (invoking id, and method id).

  @param    CreateStruct             The input create structure.
  @param    InvokingId               Invoking id.
  @param    MethodId                 Method id.

**/
TCG_RESULT
EFIAPI
TcgStartMethodCall(
  TCG_CREATE_STRUCT   *CreateStruct,
  TCG_UID             InvokingId,
  TCG_UID             MethodId
  );

/**
  Adds START LIST token.

  @param    CreateStruct        The input create structure.

**/
TCG_RESULT
EFIAPI
TcgStartParameters(
  TCG_CREATE_STRUCT           *CreateStruct
  );

/**
  Adds END LIST token.

  @param    CreateStruct        The input create structure.

**/
TCG_RESULT
EFIAPI
TcgEndParameters(
  TCG_CREATE_STRUCT   *CreateStruct
  );

/**
  Adds END Data token and method list.

  @param    CreateStruct        The input create structure.

**/
TCG_RESULT
EFIAPI
TcgEndMethodCall(
  TCG_CREATE_STRUCT      *CreateStruct
  );

/**

  Adds Start Session call to the data structure.  This creates the entire ComPacket structure and
  returns the size of the entire compacket in the size parameter.

  @param [in/out]    CreateStruct               Structure used to add the start session call
  @param [in/out]    Size                       Describes the size of the entire ComPacket (header and payload). Filled out by function.
  @param [in]        ComId                      ComID for the ComPacket
  @param [in]        ComIdExtension             Extended ComID for the ComPacket
  @param [in]        HostSessionId              Host Session ID
  @param [in]        SpId                       Security Provider to start session with
  @param [in]        Write                      Write option for start session.  TRUE = start session requests write access
  @param [in]        HostChallengeLength        Length of the host challenge.  Length should be 0 if hostChallenge is NULL
  @param [in]        HostChallenge              Host challenge for Host Signing Authority.  If NULL, then no Host Challenge shall be sent.
  @param [in]        HostSigningAuthority       Host Signing Authority used for start session.  If NULL, then no Host Signing Authority shall be sent.

**/
TCG_RESULT
EFIAPI
TcgCreateStartSession(
  TCG_CREATE_STRUCT     *CreateStruct,
  UINT32                *Size,
  UINT16                ComId,
  UINT16                ComIdExtension,
  UINT32                HostSessionId,
  TCG_UID               SpId,
  BOOLEAN               Write,
  UINT32                HostChallengeLength,
  const VOID            *HostChallenge,
  TCG_UID               HostSigningAuthority
  );

/**
  Creates ComPacket with a Method call that sets the PIN column for the row specified.
  This assumes a start session has already been opened with the desired SP.

  @param [in/out]   CreateStruct           Structure used to add method call.
  @param [in/out]   Size                   Describes the size of the entire ComPacket (header and payload). Filled out by function.
  @param [in]       ComId                  ComID for the ComPacket
  @param [in]       ComIdExtension         Extended ComID for the ComPacket
  @param [in]       TperSession            Tper Session ID for the Packet
  @param [in]       HostSession            Host Session ID for the Packet
  @param [in]       SidRow                 UID of row of current SP to set PIN column
  @param [in]       Password               value of PIN to set
  @param [in]       PasswordSize           Size of PIN

**/
TCG_RESULT
EFIAPI
TcgCreateSetCPin(
  TCG_CREATE_STRUCT       *CreateStruct,
  UINT32                  *Size,
  UINT16                  ComId,
  UINT16                  ComIdExtension,
  UINT32                  TperSession,
  UINT32                  HostSession,
  TCG_UID                 SidRow,
  const VOID              *Password,
  UINT32                  PasswordSize
  );

/**
 Creates ComPacket with a Method call that sets the "Enabled" column for the row specified using the value specified.
 This assumes a start session has already been opened with the desired SP.

 @param [in/out]  CreateStruct          Structure used to add method call
 @param [in/out]  Size                  Describes the size of the entire ComPacket (header and payload). Filled out by function.
 @param [in]      ComId                 ComID for the ComPacket
 @param [in]      ComIdExtension        Extended ComID for the ComPacket
 @param [in]      TperSession           Tper Session ID for the Packet
 @param [in]      HostSession           Host Session ID for the Packet
 @param [in]      AuthorityUid          Authority UID to modify the "Enabled" column for
 @param [in]      Enabled               Value to set the "Enabled" column to

**/
TCG_RESULT
EFIAPI
TcgSetAuthorityEnabled(
  TCG_CREATE_STRUCT           *CreateStruct,
  UINT32                      *Size,
  UINT16                      ComId,
  UINT16                      ComIdExtension,
  UINT32                      TperSession,
  UINT32                      HostSession,
  TCG_UID                     AuthorityUid,
  BOOLEAN                     Enabled
  );

/**

  Creates ComPacket with EndSession.
  This assumes a start session has already been opened.

  @param  [in/out]    CreateStruct        Structure used to add Endsession
  @param  [in/out]    Size                Describes the size of the entire ComPacket (header and payload). Filled out by function.
  @param  [in]        ComId               ComID for the ComPacket
  @param  [in]        ComIdExtension      Extended ComID for the ComPacket
  @param  [in]        HostSessionId         Host Session ID for the Packet
  @param  [in]        TpSessionId         Tper Session ID for the Packet

**/
TCG_RESULT
EFIAPI
TcgCreateEndSession(
  TCG_CREATE_STRUCT   *CreateStruct,
  UINT32              *Size,
  UINT16              ComId,
  UINT16              ComIdExtension,
  UINT32              HostSessionId,
  UINT32              TpSessionId
  );


/**

 Retrieves human-readable token type name.

 @param[in]   Type  Token type to retrieve

**/
CHAR8*
EFIAPI
TcgTokenTypeString(
  TCG_TOKEN_TYPE  Type
  );

/**
 Returns the method status of the current subpacket.  Does not affect the current position
 in the ComPacket.  In other words, it can be called whenever you have a valid SubPacket.

 @param [in/out]  ParseStruct       Structure used to parse received TCG response
 @param [in/out]  MethodStatus      Method status retrieved of the current SubPacket

**/
TCG_RESULT
EFIAPI
TcgGetMethodStatus(
  const TCG_PARSE_STRUCT            *ParseStruct,
  UINT8                             *MethodStatus
  );

/**
  Returns a human-readable string representing a method status return code.

  @param[in]  MethodStatus   Method status to translate to a string


  @retval   return the string info.
**/
CHAR8*
EFIAPI
TcgMethodStatusString(
  UINT8 MethodStatus
  );


/**
  Retrieves the comID and Extended comID of the ComPacket in the Tcg response.
  It is intended to be used to confirm the received Tcg response is intended for user that received it.

  @param [in]        ParseStruct        Structure used to parse received TCG response.
  @param [in/out]    ComId              comID retrieved from received ComPacket.
  @param [in/out]    ComIdExtension     Extended comID retrieved from received ComPacket

**/
TCG_RESULT
EFIAPI
TcgGetComIds(
  const TCG_PARSE_STRUCT     *ParseStruct,
  UINT16                     *ComId,
  UINT16                     *ComIdExtension
  );

/**
  Checks if the ComIDs of the response match the expected values.

  @param[in]   ParseStruct               Structure used to parse received TCG response
  @param[in]   ExpectedComId             Expected comID
  @param[in]   ExpectedComIdExtension    Expected extended comID

**/
TCG_RESULT
EFIAPI
TcgCheckComIds(
  const TCG_PARSE_STRUCT     *ParseStruct,
  UINT16                     ExpectedComId,
  UINT16                     ExpectedComIdExtension
  );

/**
 Parses the Sync Session response contained in the parseStruct to retrieve Tper session ID.  If the Sync Session response
 parameters do not match the comID, extended ComID and host session ID then a failure is returned.

 @param[in/out]   ParseStruct          Structure used to parse received TCG response, contains Sync Session response.
 @param[in]       ComId                Expected ComID that is compared to actual ComID of response
 @param[in]       ComIdExtension       Expected Extended ComID that is compared to actual Extended ComID of response
 @param[in]       HostSessionId        Expected Host Session ID that is compared to actual  Host Session ID of response
 @param[in/out]   TperSessionId        Tper Session ID retrieved from the Sync Session response.

**/
TCG_RESULT
EFIAPI
TcgParseSyncSession(
  const TCG_PARSE_STRUCT  *ParseStruct,
  UINT16                  ComId,
  UINT16                  ComIdExtension,
  UINT32                  HostSessionId,
  UINT32                  *TperSessionId
  );

/**
  Create set ace.

  @param     CreateStruct      Input create structure.
  @param     Size              size info.
  @param     ComId             ComId info.
  @param     ComIdExtension    ComId extension info.
  @param     TperSession       Tper session data.
  @param     HostSession       Host session data.
  @param     AceRow            Ace row info.
  @param     Authority1        Authority 1 info.
  @param     LogicalOperator   Logiccal operator info.
  @param     Authority2        Authority 2 info.

  @retval    Return the action result.

**/
TCG_RESULT
EFIAPI
TcgCreateSetAce(
  TCG_CREATE_STRUCT        *CreateStruct,
  UINT32                   *Size,
  UINT16                   ComId,
  UINT16                   ComIdExtension,
  UINT32                   TperSession,
  UINT32                   HostSession,
  TCG_UID                  AceRow,
  TCG_UID                  Authority1,
  BOOLEAN                  LogicalOperator,
  TCG_UID                  Authority2
  );

/**
  Enum level 0 discovery.

  @param     DiscoveryHeader   Discovery header.
  @param     Callback          Callback function.
  @param     Context           The context for the function.

  @retval    return true if the callback return TRUE, else return FALSE.

**/
BOOLEAN
EFIAPI
TcgEnumLevel0Discovery(
  const TCG_LEVEL0_DISCOVERY_HEADER  *DiscoveryHeader,
  TCG_LEVEL0_ENUM_CALLBACK           Callback,
  VOID                               *Context
  );

/**
  Get Feature code from the header.

  @param     DiscoveryHeader    The discovery header.
  @param     FeatureCode        reutrn the Feature code.
  @param     FeatureSize        return the Feature size.

  @retval    return the Feature code data.
**/
TCG_LEVEL0_FEATURE_DESCRIPTOR_HEADER*
EFIAPI
TcgGetFeature(
  const TCG_LEVEL0_DISCOVERY_HEADER  *DiscoveryHeader,
  UINT16                             FeatureCode,
  UINTN                              *FeatureSize
  );

/**
  Determines if the protocol provided is part of the provided supported protocol list.

  @param[in]  ProtocolList     Supported protocol list to investigate
  @param[in]  Protocol         Protocol value to determine if supported

  @return TRUE = protocol is supported, FALSE = protocol is not supported
**/
BOOLEAN
EFIAPI
TcgIsProtocolSupported(
  const TCG_SUPPORTED_SECURITY_PROTOCOLS   *ProtocolList,
  UINT16                                   Protocol
  );

/**
  Determines if the Locking Feature "Locked" bit is set in the level 0 discovery response.

  @param[in]  Discovery              Level 0 discovery response

  @return TRUE = Locked is set, FALSE = Locked is false

**/
BOOLEAN
EFIAPI
TcgIsLocked(
  const TCG_LEVEL0_DISCOVERY_HEADER      *Discovery
  );

#pragma pack()


#endif // _TCG_CORE_H_
