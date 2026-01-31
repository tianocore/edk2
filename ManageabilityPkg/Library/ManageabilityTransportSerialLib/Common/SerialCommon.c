/** @file

  Serial instance of Manageability Transport Library

  Copyright (c) 2024, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <IndustryStandard/Mctp.h>
#include <Library/SerialPortLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <Library/ManageabilityTransportMctpLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>

#include "ManageabilityTransportSerial.h"

extern MANAGEABILITY_TRANSPORT_SERIAL_HARDWARE_INFO  mSerialHardwareInfo;
extern MANAGEABILITY_TRANSPORT_SERIAL                *mSingleSessionToken;

struct IpmiSerialSpecialChar  mIpmiSerialSpecialChar[] = {
  { BASIC_MODE_START,     BASIC_MODE_START_ENCODED_BYTE     }, /* start */
  { BASIC_MODE_STOP,      BASIC_MODE_STOP_ENCODED_BYTE      }, /* stop */
  { BASIC_MODE_HANDSHAKE, BASIC_MODE_HANDSHAKE_ENCODED_BYTE }, /* packet handshake */
  { BASIC_MODE_ESCAPE,    BASIC_MODE_ESCAPE_ENCODED_BYTE    }, /* data escape */
  { BASIC_MODE_ESC_CHAR,  BASIC_MODE_ESC_CHAR_ENCODED_BYTE  }  /* escape */
};

UINT8  mNumberOfEscapeCharacters = ARRAY_SIZE (mIpmiSerialSpecialChar);

/**
  Return escaped character for the given one

  @param[in]      Character           Character data.

  @retval         Character           Return escaped character if it is.
                                      Otherwise, return the given one.
**/
static
UINT8
IpmiSerialGetEscapedCharacter (
  UINT8  Character
  )
{
  UINT8  Index;

  for (Index = 0; Index < mNumberOfEscapeCharacters; Index++) {
    if (mIpmiSerialSpecialChar[Index].Character == Character) {
      return mIpmiSerialSpecialChar[Index].Escape;
    }
  }

  return Character;
}

/**
  Return unescaped character for the given one

  @param[in]      Character           Character data.

  @retval         Character           Return unescaped character if it is.
                                      Otherwise, return the given one.
**/
static
UINT8
IpmiSerialGetUnescapedCharacter (
  UINT8  Character
  )
{
  UINT8  Index;

  for (Index = 0; Index < mNumberOfEscapeCharacters; Index++) {
    if (mIpmiSerialSpecialChar[Index].Escape == Character) {
      return mIpmiSerialSpecialChar[Index].Character;
    }
  }

  return Character;
}

/**
  Parse IPMI Serial incoming data in basic mode format to IPMI message

  @param[in]        Buffer                Data buffer with data byte escaped.
  @param[in]        BufferSize            Data buffer size.
  @param[out]       ResponseData          Data buffer without data byte escaped.
  @param[in, out]   ResponseDataSize      Data buffer size.

  @retval           EFI_SUCCESS           The command byte stream was successfully
                                          submit to the device and a response was
                                          successfully received.
  @retval           EFI_PROTOCOL_ERROR    The command byte stream cannot encode
                                          successfully.
  @retval           EFI_OUT_OF_RESOURCES  The resource allocation is out of resource or
                                          data size error.
**/
EFI_STATUS
IpmiSerialParseIncomingBuffer (
  IN        UINT8   *Buffer,
  IN        UINT32  BufferSize,
  OUT       UINT8   *ResponseData,
  IN OUT    UINT32  *ResponseDataSize
  )
{
  UINT8   *ResponsePtr;
  UINT8   Character;
  UINT8   UnescapedCharacter;
  UINT32  Count;
  UINT32  Index;
  UINT8   CtxState;
  UINT8   CtxEscape;

  if ((BufferSize == 0) || (Buffer == NULL)) {
    DEBUG ((DEBUG_ERROR, "Buffer Data insufficient. \n"));
    return EFI_OUT_OF_RESOURCES;
  }

  if ((ResponseDataSize == NULL) ||
      (BufferSize > *ResponseDataSize) ||
      (ResponseData == NULL))
  {
    DEBUG ((DEBUG_ERROR, "Response Data insufficient. \n"));
    return EFI_OUT_OF_RESOURCES;
  }

  CtxState    = MSG_IDLE;
  CtxEscape   = 0;
  ResponsePtr = ResponseData;
  Count       = 0;
  for (Index = 0; Index < BufferSize; Index++) {
    Character = Buffer[Index];
    if ((Character == BASIC_MODE_START) && (CtxState == MSG_IDLE)) {
      // START
      CtxState  = MSG_IN_PROGRESS;
      CtxEscape = 0;
    } else if (CtxState != MSG_IN_PROGRESS) {
      continue;
    } else if (CtxEscape != 0) {
      UnescapedCharacter = IpmiSerialGetUnescapedCharacter (Character);

      if (UnescapedCharacter == Character) {
        // error, then reset
        CtxState = MSG_IDLE;
        continue;
      }

      *ResponsePtr++ = UnescapedCharacter;
      Count++;
      CtxEscape = 0;
    } else if (Character == BASIC_MODE_ESCAPE) {
      CtxEscape = 1;
      continue;
    } else if (Character == BASIC_MODE_STOP) {
      // STOP
      CtxState          = MSG_IDLE;
      *ResponseDataSize = Count;
      return EFI_SUCCESS;
    } else if (Character == BASIC_MODE_HANDSHAKE) {
      // just skip it
      continue;
    } else if (CtxState == MSG_IN_PROGRESS) {
      *ResponsePtr++ = Character;
      Count++;
    }
  }

  return EFI_PROTOCOL_ERROR;
}

/**
  This function writes/sends data to the Serial port.
  Basic mode message fields in IPMI spec 2.0 Figure 14.

  @param[in]      TransmitHeader        Serial packet header.
  @param[in]      TransmitHeaderSize    Serial packet header size in byte.
  @param[in]      TransmitTrailer       Serial packet trailer.
  @param[in]      TransmitTrailerSize   Serial packet trailer size in byte.
  @param[in]      RequestData           Command Request Data, could be NULL.
                                        RequestDataSize must be zero, if RequestData
                                        is NULL.
  @param[in]      RequestDataSize       Size of Command Request Data.

  @retval         EFI_SUCCESS           The command byte stream was successfully
                                        submit to the device and a response was
                                        successfully received.
  @retval         EFI_NOT_FOUND         The command was not successfully sent to the
                                        device or a response was not successfully
                                        received from the device.
  @retval         EFI_NOT_READY         Ipmi Device is not ready for Ipmi command
                                        access.
  @retval         EFI_DEVICE_ERROR      Ipmi Device hardware error.
  @retval         EFI_TIMEOUT           The command time out.
  @retval         EFI_UNSUPPORTED       The command was not successfully sent to
                                        the device.
  @retval         EFI_OUT_OF_RESOURCES  The resource allocation is out of resource or
                                        data size error.
**/
EFI_STATUS
SerialTransportWrite (
  IN  MANAGEABILITY_TRANSPORT_HEADER   TransmitHeader,
  IN  UINT16                           TransmitHeaderSize,
  IN  MANAGEABILITY_TRANSPORT_TRAILER  TransmitTrailer OPTIONAL,
  IN  UINT16                           TransmitTrailerSize,
  IN  UINT8                            *RequestData OPTIONAL,
  IN  UINT32                           RequestDataSize
  )
{
  EFI_STATUS          Status;
  IPMI_SERIAL_HEADER  *HeaderPtr;
  UINT8               *BufferPtr;
  UINT8               *Buffer;
  UINT32              BufferLength;
  UINT8               *Request;
  UINT32              RequestLength;
  UINT8               RetryCount;
  UINT8               Index;
  UINT8               Character;
  UINT8               EscapedCharacterCount;
  UINT8               NetFunction;

  // Validation on RequestData and RequestDataSize.
  if (((RequestData == NULL) && (RequestDataSize != 0)) ||
      ((RequestData != NULL) && (RequestDataSize == 0))
      )
  {
    DEBUG ((DEBUG_ERROR, "%a: Mismatched values of RequestData or RequestDataSize.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // Validation on TransmitHeader and TransmitHeaderSize.
  if (((TransmitHeader == NULL) && (TransmitHeaderSize != 0)) ||
      ((TransmitHeader != NULL) && (TransmitHeaderSize == 0))
      )
  {
    DEBUG ((DEBUG_ERROR, "%a: Mismatched values of TransmitHeader or TransmitHeaderSize.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // Validation on TransmitHeader and TransmitHeaderSize.
  if (((TransmitTrailer == NULL) && (TransmitTrailerSize != 0)) ||
      ((TransmitTrailer != NULL) && (TransmitTrailerSize == 0))
      )
  {
    DEBUG ((DEBUG_ERROR, "%a: Mismatched values of TransmitTrailer or TransmitTrailerSize.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  NetFunction = ((MANAGEABILITY_IPMI_TRANSPORT_HEADER *)TransmitHeader)->NetFn;
  ASSERT (NetFunction <= MANAGEABILITY_IPMI_NET_FUNC_MAX);

  BufferLength = RequestDataSize + IPMI_SERIAL_MIN_REQUEST_LENGTH;
  Buffer       = AllocateZeroPool (BufferLength);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HeaderPtr = (IPMI_SERIAL_HEADER *)Buffer;

  // Fill IPMI Serial basic format data bytes
  HeaderPtr->ResponderAddress  = IPMI_SERIAL_RS_ADDRESS;
  HeaderPtr->ResponderNetFnLun = (UINT8)((NetFunction << 2) | (IPMI_SERIAL_RS_LUN & IPMI_MAX_LUN));
  HeaderPtr->CheckSum          = CalculateCheckSum8 (&Buffer[0], IPMI_SERIAL_CONNECTION_HEADER_LENGTH - 1);
  HeaderPtr->RequesterAddress  = IPMI_SERIAL_RQ_ADDRESS;
  HeaderPtr->RequesterSeqLun   = IPMI_SERIAL_RQ_LUN;
  HeaderPtr->Command           = ((MANAGEABILITY_IPMI_TRANSPORT_HEADER *)TransmitHeader)->Command;

  // Calculate data checksum
  if ((RequestData != NULL) && (RequestDataSize > 0)) {
    CopyMem (HeaderPtr->Data, RequestData, RequestDataSize);
    Buffer[BufferLength - 1] = CalculateCheckSum8 (
                                                   &Buffer[IPMI_SERIAL_CONNECTION_HEADER_LENGTH],
                                                   RequestDataSize + IPMI_SERIAL_REQUEST_DATA_HEADER_LENGTH
                                                   );
  } else {
    Buffer[BufferLength - 1] = CalculateCheckSum8 (&Buffer[IPMI_SERIAL_CONNECTION_HEADER_LENGTH], IPMI_SERIAL_REQUEST_DATA_HEADER_LENGTH);
  }

  // Calculate escapted character count
  EscapedCharacterCount = 0;
  for (Index = 0; Index < BufferLength; Index++) {
    if (IpmiSerialGetEscapedCharacter (Buffer[Index]) != Buffer[Index]) {
      EscapedCharacterCount++;
    }
  }

  // Allocate memory buffer include escaped characters
  RequestLength = BufferLength + EscapedCharacterCount + 2; // start + stop byte
  Request       = AllocateZeroPool (RequestLength);
  if (Request == NULL) {
    DEBUG ((DEBUG_ERROR, "Out Of Resource \n"));
    return EFI_OUT_OF_RESOURCES;
  }

  BufferPtr = Request;

  // start character
  *BufferPtr++ = BASIC_MODE_START;

  // Fill the request data with data byte escaped
  for (Index = 0; Index < BufferLength; Index++) {
    Character = IpmiSerialGetEscapedCharacter (Buffer[Index]);
    if (Character != Buffer[Index]) {
      *BufferPtr++ = BASIC_MODE_ESCAPE;
    }

    *BufferPtr++ = Character;
  }

  // stop character
  *BufferPtr++ = BASIC_MODE_STOP;

  //
  // Write Request
  //
  RetryCount = 0;
  while (TRUE) {
    Status = SerialPortWrite (Request, RequestLength);
    if (Status == RequestLength) {
      break;
    }

    if (++RetryCount > IPMI_SERIAL_RETRY_COUNT) {
      DEBUG ((DEBUG_ERROR, "%a: Write Request error %r\n", __func__, Status));
      break;
    }

    MicroSecondDelay (IPMI_SERIAL_REQUEST_RETRY_INTERVAL);
  }

  FreePool (Buffer);
  FreePool (Request);

  return EFI_SUCCESS;
}

/**
  Read Serial response from BMC.

  @param[out]        ResponseData      Command Response Data.
  @param[in, out]    ResponseDataSize  Size of Command Response Data.

  @retval EFI_SUCCESS                  The command byte stream was successfully submit to the device
                                       and a response was successfully received.
  @retval EFI_PROTOCOL_ERROR           The command byte stream cannot encode
                                       successfully.
**/
EFI_STATUS
SerialReadResponse (
  OUT    UINT8   *ResponseData,
  IN OUT UINT32  *ResponseDataSize
  )
{
  EFI_STATUS  Status;
  UINT8       Character;
  UINT8       Buffer[IPMI_SERIAL_MAXIMUM_PACKET_SIZE_IN_BYTES];
  UINT32      BufferSize;
  UINT8       ReadBuffer[IPMI_SERIAL_MAXIMUM_PACKET_SIZE_IN_BYTES];
  UINT32      ReadBytes;
  UINT32      RetryCount;

  ASSERT (ResponseData != NULL && ResponseDataSize != NULL);

  //
  // Keep polling until retryCount timeout
  //
  RetryCount = 0;
  ReadBytes  = 0;
  while (ReadBytes < IPMI_SERIAL_MAXIMUM_PACKET_SIZE_IN_BYTES) {
    if (++RetryCount > IPMI_SERIAL_RETRY_COUNT) {
      break;
    }

    while (SerialPortPoll ()) {
      SerialPortRead (&Character, sizeof (Character));
      ReadBuffer[ReadBytes++] = Character;
    }

    MicroSecondDelay (IPMI_SERIAL_RESPONSE_RETRY_INTERVAL);
  }

  // Parse IPMI Serial format request data
  BufferSize = sizeof (Buffer);
  Status     = IpmiSerialParseIncomingBuffer (
                                              ReadBuffer,
                                              ReadBytes,
                                              Buffer,
                                              &BufferSize
                                              );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Parse Response Error\n"));
    return Status;
  }

  // Header checksum verify
  if (CalculateCheckSum8 (Buffer, IPMI_SERIAL_CONNECTION_HEADER_LENGTH) != 0) {
    DEBUG ((DEBUG_ERROR, "Bad checksum - header\n"));
    return EFI_PROTOCOL_ERROR;
  }

  // Data checksum verify
  if (CalculateCheckSum8 (
                          &Buffer[IPMI_SERIAL_CONNECTION_HEADER_LENGTH],
                          BufferSize - IPMI_SERIAL_CONNECTION_HEADER_LENGTH
                          ) != 0)
  {
    DEBUG ((DEBUG_ERROR, "Bad checksum - data byte\n"));
    return EFI_PROTOCOL_ERROR;
  }

  *ResponseDataSize = BufferSize - (sizeof (IPMI_SERIAL_HEADER) + 1); // Header (6) + Checksum (1)
  CopyMem (ResponseData, &Buffer[sizeof (IPMI_SERIAL_HEADER)], *ResponseDataSize);

  return Status;
}

/**
  This function sends/receives data from Serial port.

  @param [in]       DataBytes             Buffer to hold the read Data.
  @param [in, out]  Length                Number of Bytes read from Serial port.
  @retval           EFI_SUCCESS           The command byte stream was
                                          successfully submit to the device and
                                          a response was successfully received.
  @retval           EFI_NOT_FOUND         The command was not successfully sent
                                          to the device or a response was not
                                          successfully received from the
                                          device.
  @retval           EFI_NOT_READY         Ipmi Device is not ready for Ipmi
                                          command access.
  @retval           EFI_OUT_OF_RESOURCES  The resource allocation is out of
                                          resource or data size error.
  @retval           EFI_PROTOCOL_ERROR    The command byte stream cannot encode
                                          successfully.
**/
EFI_STATUS
SerialTransportRead (
  OUT    UINT8   *DataByte,
  IN OUT UINT32  *Length
  )
{
  EFI_STATUS  Status;

  if ((DataByte == NULL) || (*Length == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: Either DataByte is NULL or Length is 0.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Status = SerialReadResponse (DataByte, Length);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Read Response error %r\n", __func__, Status));
    *Length = 0;
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This funciton checks the Serial response data according to
  manageability protocol.

  @param[in]      ResponseData        Pointer to response data.
  @param[in]      ResponseDataSize    Size of response data.
  @param[out]     AdditionalStatus    Pointer to receive the additional status.

  @retval         EFI_SUCCESS         Serial response header is checked without error
  @retval         EFI_DEVICE_ERROR    Serial response header has problem.
**/
EFI_STATUS
SerialCheckResponseData (
  IN UINT8                                       *ResponseData,
  IN UINT32                                      ResponseDataSize,
  OUT MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *AdditionalStatus
  )
{
  EFI_STATUS  Status;
  CHAR16      *CompletionCodeStr;

  Status            = EFI_SUCCESS;
  *AdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NO_ERRORS;
  if (CompareGuid (&gManageabilityProtocolIpmiGuid, mSingleSessionToken->Token.ManageabilityProtocolSpecification)) {
    //
    // For IPMI Serial
    // Check and print Completion Code
    //
    Status = IpmiHelperCheckCompletionCode (*ResponseData, &CompletionCodeStr, AdditionalStatus);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_MANAGEABILITY_INFO, "Cc: %02x %s.\n", *((UINT8 *)ResponseData), CompletionCodeStr));
    } else if (Status == EFI_NOT_FOUND) {
      DEBUG ((DEBUG_ERROR, "Cc: %02x not defined in IpmiCompletionCodeMapping or invalid.\n", *((UINT8 *)ResponseData)));
    }
  }

  return Status;
}

/**
  This service communicates with BMC using Serial protocol.

  @param[in]      TransmitHeader        Serial packet header.
  @param[in]      TransmitHeaderSize    Serial packet header size in byte.
  @param[in]      TransmitTrailer       Serial packet trailer.
  @param[in]      TransmitTrailerSize   Serial packet trailer size in byte.
  @param[in]      RequestData           Command Request Data.
  @param[in]      RequestDataSize       Size of Command Request Data.
  @param[out]     ResponseData          Command Response Data. The completion
                                        code is the first byte of response
                                        data.
  @param[in, out] ResponseDataSize      Size of Command Response Data.
  @param[out]     AdditionalStatus       Additional status of this transaction.

  @retval         EFI_SUCCESS           The command byte stream was
                                        successfully submit to the device and a
                                        response was successfully received.
  @retval         EFI_NOT_FOUND         The command was not successfully sent
                                        to the device or a response was not
                                        successfully received from the device.
  @retval         EFI_NOT_READY         Ipmi Device is not ready for Ipmi
                                        command access.
  @retval         EFI_DEVICE_ERROR      Ipmi Device hardware error.
  @retval         EFI_TIMEOUT           The command time out.
  @retval         EFI_UNSUPPORTED       The command was not successfully sent to
                                        the device.
  @retval         EFI_OUT_OF_RESOURCES  The resource allocation is out of
                                        resource or data size error.
**/
EFI_STATUS
EFIAPI
SerialTransportSendCommand (
  IN  MANAGEABILITY_TRANSPORT_HEADER              TransmitHeader OPTIONAL,
  IN  UINT16                                      TransmitHeaderSize,
  IN  MANAGEABILITY_TRANSPORT_TRAILER             TransmitTrailer OPTIONAL,
  IN  UINT16                                      TransmitTrailerSize,
  IN  UINT8                                       *RequestData OPTIONAL,
  IN  UINT32                                      RequestDataSize,
  OUT UINT8                                       *ResponseData OPTIONAL,
  IN  OUT UINT32                                  *ResponseDataSize OPTIONAL,
  OUT  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *AdditionalStatus
  )
{
  EFI_STATUS  Status;

  if ((RequestData != NULL) && (RequestDataSize == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: Mismatched values of RequestData and RequestDataSize\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((ResponseData != NULL) && ((ResponseDataSize != NULL) && (*ResponseDataSize == 0))) {
    DEBUG ((DEBUG_ERROR, "%a: Mismatched values of ResponseData and ResponseDataSize\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (AdditionalStatus == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: AdditionalStatus is NULL.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  // Print out the request payloads.
  if ((TransmitHeader != NULL) && (TransmitHeaderSize != 0)) {
    HelperManageabilityDebugPrint ((VOID *)TransmitHeader, (UINT32)TransmitHeaderSize, "Serial Transmit Header:\n");
  }

  if (RequestData != NULL) {
    HelperManageabilityDebugPrint ((VOID *)RequestData, RequestDataSize, "Serial Request Data:\n");
  }

  if ((TransmitTrailer != NULL) && (TransmitTrailerSize != 0)) {
    HelperManageabilityDebugPrint ((VOID *)TransmitTrailer, (UINT32)TransmitTrailerSize, "Serial Transmit Trailer:\n");
  }

  if ((TransmitHeader != NULL) || (RequestData != NULL)) {
    Status = SerialTransportWrite (
                                   TransmitHeader,
                                   TransmitHeaderSize,
                                   TransmitTrailer,
                                   TransmitTrailerSize,
                                   RequestData,
                                   RequestDataSize
                                   );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Serial Write Failed with Status(%r)\n", Status));
      return Status;
    }
  }

  if ((ResponseData != NULL) && (ResponseDataSize != NULL) && (*ResponseDataSize != 0)) {
    Status = SerialTransportRead (ResponseData, ResponseDataSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Serial response read Failed with Status(%r)\n", Status));
    }

    // Print out the response payloads.
    if (*ResponseDataSize != 0) {
      HelperManageabilityDebugPrint ((VOID *)ResponseData, (UINT32)*ResponseDataSize, "Serial Response Data:\n");
      Status = SerialCheckResponseData (ResponseData, *ResponseDataSize, AdditionalStatus);
    } else {
      DEBUG ((DEBUG_ERROR, "No response, can't determine Completion Code.\n"));
    }
  } else {
    *ResponseDataSize = 0;
  }

  return Status;
}
