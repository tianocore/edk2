/** @file

  SSIF instance of Manageability Transport Library

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <IndustryStandard/IpmiSsif.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <Library/ManageabilityTransportMctpLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmbusLib.h>
#include <Library/TimerLib.h>

#include "ManageabilityTransportSsif.h"

extern MANAGEABILITY_TRANSPORT_SSIF                *mSingleSessionToken;
extern MANAGEABILITY_TRANSPORT_SSIF_HARDWARE_INFO  mSsifHardwareInfo;

// BMC slave address exluding read/write bit.
#define IPMI_SSIF_BMC_SLAVE_ADDR_7BIT  SMBUS_LIB_SLAVE_ADDRESS (mSsifHardwareInfo.BmcSlaveAddress)

#define IPMI_SSIF_REQUEST_RETRY_COUNT      (FixedPcdGet8 (PcdIpmiSsifRequestRetryCount))
#define IPMI_SSIF_REQUEST_RETRY_INTERVAL   (FixedPcdGet32 (PcdIpmiSsifRequestRetryIntervalMicrosecond))
#define IPMI_SSIF_RESPONSE_RETRY_COUNT     (FixedPcdGet8 (PcdIpmiSsifResponseRetryCount))
#define IPMI_SSIF_RESPONSE_RETRY_INTERVAL  (FixedPcdGet32 (PcdIpmiSsifResponseRetryIntervalMicrosecond))

//
// SSIF Interface capabilities
//
extern BOOLEAN  mPecSupport;
extern UINT8    mMaxRequestSize;
extern UINT8    mMaxResponseSize;
extern UINT8    mTransactionSupport;

/**
  Write SSIF request to BMC.

  @param[in]   RequestData       Command Request Data.
  @param[in]   RequestDataSize   Size of Command Request Data.

  @retval EFI_SUCCESS            The command byte stream was successfully submit to the device
                                 and a response was successfully received.
  @retval EFI_INVALID_PARAMETER  RequestData is NULL or RequestDataSize is zero.
  @retval Others                 Failed to write data to the device.
**/
EFI_STATUS
SsifWriteRequest (
  IN UINT8   *RequestData,
  IN UINT32  RequestDataSize
  )
{
  EFI_STATUS  Status;
  BOOLEAN     IsMultiPartWrite;
  UINT8       Index;
  UINT8       MiddleCount;
  UINT8       SsifCmd;
  UINT8       WriteLen;

  if ((RequestData == NULL) || (RequestDataSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  IsMultiPartWrite = FALSE;
  Status           = EFI_SUCCESS;

  if (RequestDataSize > IPMI_SSIF_MAXIMUM_PACKET_SIZE_IN_BYTES) {
    IsMultiPartWrite = TRUE;
    MiddleCount      = ((RequestDataSize - 1) / IPMI_SSIF_MAXIMUM_PACKET_SIZE_IN_BYTES) - 1;

    if (  ((MiddleCount == 0) && (mTransactionSupport == IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_SSIF_TRANSACTION_SUPPORT_SINGLE_PARTITION_RW))
       || ((MiddleCount > 0) && (mTransactionSupport != IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_SSIF_TRANSACTION_SUPPORT_MULTI_PARTITION_RW_WITH_MIDDLE)))
    {
      DEBUG ((DEBUG_ERROR, "%a: Unsupported request transaction\n", __func__));
      return EFI_UNSUPPORTED;
    }
  }

  SsifCmd = IsMultiPartWrite ? IPMI_SSIF_SMBUS_CMD_MULTI_PART_WRITE_START
                               : IPMI_SSIF_SMBUS_CMD_SINGLE_PART_WRITE;
  WriteLen = IsMultiPartWrite ? IPMI_SSIF_MAXIMUM_PACKET_SIZE_IN_BYTES : RequestDataSize;

  SmBusWriteBlock (
    SMBUS_LIB_ADDRESS (
      IPMI_SSIF_BMC_SLAVE_ADDR_7BIT,
      SsifCmd,
      WriteLen,
      mPecSupport
      ),
    RequestData,
    &Status
    );

  if (  EFI_ERROR (Status)
     || !IsMultiPartWrite)
  {
    goto Exit;
  }

  for (Index = 1; Index <= MiddleCount; Index++) {
    SmBusWriteBlock (
      SMBUS_LIB_ADDRESS (
        IPMI_SSIF_BMC_SLAVE_ADDR_7BIT,
        IPMI_SSIF_SMBUS_CMD_MULTI_PART_WRITE_MIDDLE,
        WriteLen,
        mPecSupport
        ),
      &RequestData[Index * IPMI_SSIF_MAXIMUM_PACKET_SIZE_IN_BYTES],
      &Status
      );

    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  }

  //
  // Remain RequestData for END
  //
  WriteLen = RequestDataSize - (MiddleCount + 1) * IPMI_SSIF_MAXIMUM_PACKET_SIZE_IN_BYTES;
  ASSERT (WriteLen > 0);
  SmBusWriteBlock (
    SMBUS_LIB_ADDRESS (
      IPMI_SSIF_BMC_SLAVE_ADDR_7BIT,
      IPMI_SSIF_SMBUS_CMD_MULTI_PART_WRITE_END,
      WriteLen,
      mPecSupport
      ),
    &RequestData[(MiddleCount + 1) * IPMI_SSIF_MAXIMUM_PACKET_SIZE_IN_BYTES],
    &Status
    );

Exit:

  return Status;
}

/**
  Read SSIF response from BMC.

  @param[out]        ResponseData      Command Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize  Size of Command Response Data.

  @retval EFI_SUCCESS                  The command byte stream was successfully submit to the device
                                       and a response was successfully received.
  @retval EFI_INVALID_PARAMETER        ResponseData is NULL or ResponseDataSize is NULL.
  @retval Others                       Failed to write data to the device.
**/
EFI_STATUS
SsifReadResponse (
  OUT    UINT8   *ResponseData,
  IN OUT UINT32  *ResponseDataSize
  )
{
  EFI_STATUS  Status;
  BOOLEAN     IsMultiPartRead;
  UINT32      CopiedLen;
  UINT8       BlockNumber;
  UINT8       Offset;
  UINT8       ReadLen;
  UINT8       ResponseTemp[IPMI_SSIF_MAXIMUM_PACKET_SIZE_IN_BYTES];

  if ((ResponseData == NULL) || (ResponseDataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BlockNumber     = 0;
  CopiedLen       = 0;
  IsMultiPartRead = FALSE;
  Offset          = 2; // Ignore LUN and Command byte in return ResponseData
  Status          = EFI_SUCCESS;

  ReadLen = SmBusReadBlock (
              SMBUS_LIB_ADDRESS (
                IPMI_SSIF_BMC_SLAVE_ADDR_7BIT,
                IPMI_SSIF_SMBUS_CMD_SINGLE_PART_READ,
                0,  // Max block size
                mPecSupport
                ),
              ResponseTemp,
              &Status
              );

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if (ReadLen == 0) {
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  if (  (ReadLen == IPMI_SSIF_MAXIMUM_PACKET_SIZE_IN_BYTES)
     && (ResponseTemp[0] == IPMI_SSIF_MULTI_PART_READ_START_PATTERN1)
     && (ResponseTemp[1] == IPMI_SSIF_MULTI_PART_READ_START_PATTERN2))
  {
    Offset         += 2; // Ignore pattern1 and pattern2
    IsMultiPartRead = TRUE;
  }

  //
  // Copy ResponseData
  //
  ReadLen -= Offset;
  if (ReadLen > *ResponseDataSize) {
    ReadLen = *ResponseDataSize;
  }

  CopyMem (ResponseData, &ResponseTemp[Offset], ReadLen);
  CopiedLen = ReadLen;

  Offset = 1;  // Ignore block number
  while (IsMultiPartRead) {
    ReadLen = SmBusReadBlock (
                SMBUS_LIB_ADDRESS (
                  IPMI_SSIF_BMC_SLAVE_ADDR_7BIT,
                  IPMI_SSIF_SMBUS_CMD_MULTI_PART_READ_MIDDLE,
                  0,
                  mPecSupport
                  ),
                ResponseTemp,
                &Status
                );

    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    if (ReadLen == 0) {
      DEBUG ((DEBUG_ERROR, "%a: Response data error\n", __func__));
      Status = EFI_NOT_FOUND;
      goto Exit;
    }

    ReadLen -= Offset; // Ignore block number
    if (ReadLen > *ResponseDataSize - CopiedLen) {
      ReadLen = *ResponseDataSize - CopiedLen;
    }

    //
    // Copy to ResponseData if space is sufficient
    //
    if (ReadLen > 0) {
      CopyMem (&ResponseData[CopiedLen], &ResponseTemp[Offset], ReadLen);
      CopiedLen += ReadLen;
    }

    if (ResponseTemp[0] == IPMI_SSIF_MULTI_PART_READ_END_PATTERN) {
      break;
    }

    //
    // Verify BlockNumber
    //
    if (ResponseTemp[0] != BlockNumber++) {
      DEBUG ((DEBUG_ERROR, "%a: Block number is incorrect\n", __func__));
      Status = EFI_NOT_FOUND;
      goto Exit;
    }
  }

Exit:
  *ResponseDataSize = CopiedLen;

  return Status;
}

/**
  This function enables submitting IPMI command via SSIF interface.

  @param[in]         NetFunction       Net function of the command.
  @param[in]         Command           IPMI Command.
  @param[in]         RequestData       Command Request Data.
  @param[in]         RequestDataSize   Size of Command Request Data.
  @param[out]        ResponseData      Command Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize  Size of Command Response Data.

  @retval EFI_SUCCESS            The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_NOT_FOUND          The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_NOT_READY          Ipmi Device is not ready for Ipmi command access.
  @retval EFI_DEVICE_ERROR       Ipmi Device hardware error.
  @retval EFI_TIMEOUT            The command time out.
  @retval EFI_UNSUPPORTED        The command was not successfully sent to the device.
  @retval EFI_OUT_OF_RESOURCES   The resource allcation is out of resource or data size error.
**/
EFI_STATUS
SsifCommonSendCommand (
  IN     UINT8   NetFunction,
  IN     UINT8   Command,
  IN     UINT8   *RequestData,
  IN     UINT32  RequestDataSize,
  OUT    UINT8   *ResponseData,
  IN OUT UINT32  *ResponseDataSize
  )
{
  EFI_STATUS  Status;
  UINT32      TempLength;
  UINT8       *RequestTemp;
  UINT8       RetryCount;

  RequestTemp = AllocateZeroPool (RequestDataSize + sizeof (IPMI_SSIF_REQUEST_HEADER));
  if (RequestTemp == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ((IPMI_SSIF_REQUEST_HEADER *)RequestTemp)->NetFunc = (UINT8)((NetFunction << 2) | (MANAGEABILITY_IPMI_BMC_LUN & 0x3));
  ((IPMI_SSIF_REQUEST_HEADER *)RequestTemp)->Command = Command;

  TempLength = sizeof (IPMI_SSIF_REQUEST_HEADER);

  if (RequestData != NULL) {
    if (RequestDataSize > 0) {
      CopyMem (RequestTemp + TempLength, RequestData, RequestDataSize);
      TempLength += RequestDataSize;
    } else {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG ((DEBUG_ERROR, "%a: Invalid request info\n", __func__));
      goto Cleanup;
    }
  }

  if (TempLength > mMaxRequestSize) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Request size defeats BMC capability\n", __func__));
    goto Cleanup;
  }

  if (  (ResponseData == NULL)
     || (ResponseDataSize == NULL)
     || (*ResponseDataSize == 0))
  {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Invalid response info\n", __func__));
    goto Cleanup;
  }

  //
  // Write Request
  //
  RetryCount = 0;
  while (TRUE) {
    Status = SsifWriteRequest (RequestTemp, TempLength);
    if (!EFI_ERROR (Status)) {
      break;
    }

    if (++RetryCount > IPMI_SSIF_REQUEST_RETRY_COUNT) {
      DEBUG ((DEBUG_ERROR, "%a: Write request error %r\n", __func__, Status));
      goto Cleanup;
    }

    MicroSecondDelay (IPMI_SSIF_REQUEST_RETRY_INTERVAL);
    DEBUG ((DEBUG_INFO, "%a: Write request retry %d\n", __func__, RetryCount));
  }

  //
  // Read Response
  //
  TempLength = *ResponseDataSize; // Keep original DataSize

  RetryCount = 0;
  while (TRUE) {
    Status = SsifReadResponse (ResponseData, ResponseDataSize);
    if (!EFI_ERROR (Status)) {
      break;
    }

    if (++RetryCount > IPMI_SSIF_RESPONSE_RETRY_COUNT) {
      DEBUG ((DEBUG_ERROR, "%a: Read response error %r\n", __func__, Status));
      *ResponseDataSize = 0;
      goto Cleanup;
    }

    *ResponseDataSize = TempLength;
    MicroSecondDelay (IPMI_SSIF_RESPONSE_RETRY_INTERVAL);
    DEBUG ((DEBUG_INFO, "%a: Read response retry %d\n", __func__, RetryCount));
  }

Cleanup:
  FreePool (RequestTemp);

  return Status;
}

/**
  This retrieves the IPMI SSIF capabilities.

  @param[out]     PecSupport          PEC support.
  @param[out]     TransactionSupport  Transaction Support.
  @param[out]     MaxRequestSize      The maximum size of the data request.
  @param[out]     MaxResponseSize     The maximum size of the data response.

  @retval         EFI_SUCCESS             The operation completed successfully.
  @retval         EFI_INVALID_PARAMETER   An input parameter is NUL.

**/
EFI_STATUS
EFIAPI
SsifTransportGetCapabilities (
  OUT BOOLEAN  *PecSupport,
  OUT UINT8    *TransactionSupport,
  OUT UINT8    *MaxRequestSize,
  OUT UINT8    *MaxResponseSize
  )
{
  EFI_STATUS                                            Status;
  IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_REQUEST        SsifCapRequest;
  IPMI_GET_SYSTEM_INTERFACE_SSIF_CAPABILITIES_RESPONSE  SsifCapResponse;
  UINT32                                                ResponseSize;

  if (  (PecSupport == NULL) || (TransactionSupport == NULL)
     || (MaxRequestSize == NULL) || (MaxResponseSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  SsifCapRequest.Uint8 = IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_INTERFACE_TYPE_SSIF;
  ResponseSize         = sizeof (SsifCapResponse);
  Status               = SsifCommonSendCommand (
                           IPMI_NETFN_APP,
                           IPMI_APP_GET_SYSTEM_INTERFACE_CAPABILITIES,
                           (VOID *)&SsifCapRequest,
                           sizeof (SsifCapRequest),
                           (VOID *)&SsifCapResponse,
                           &ResponseSize
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Could not retrieve SSIF capabilities - %r\n", __func__, Status));
    return Status;
  }

  *PecSupport         = (SsifCapResponse.InterfaceCap.Bits.PecSupport == 0) ? FALSE : TRUE;
  *TransactionSupport = SsifCapResponse.InterfaceCap.Bits.TransactionSupport;
  *MaxRequestSize     = SsifCapResponse.InputMsgSize;
  *MaxResponseSize    = SsifCapResponse.OutputMsgSize;

  return EFI_SUCCESS;
}

/**
  This service communicates with BMC using SSIF protocol.

  @param[in]      TransmitHeader        SSIF packet header.
  @param[in]      TransmitHeaderSize    SSIF packet header size in byte.
  @param[in]      TransmitTrailer       SSIF packet trailer.
  @param[in]      TransmitTrailerSize   SSIF packet trailer size in byte.
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
  @retval         EFI_NOT_READY         IPMI Device is not ready for IPMI
                                        command access.
  @retval         EFI_DEVICE_ERROR      IPMI Device hardware error.
  @retval         EFI_TIMEOUT           The command time out.
  @retval         EFI_UNSUPPORTED       The command was not successfully sent to
                                        the device.
  @retval         EFI_OUT_OF_RESOURCES  The resource allocation is out of
                                        resource or data size error.
**/
EFI_STATUS
EFIAPI
SsifTransportSendCommand (
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
  UINT8  NetFunction;
  UINT8  Command;

  if (TransmitHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: TransmitHeader is NULL.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

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
    HelperManageabilityDebugPrint ((VOID *)TransmitHeader, (UINT32)TransmitHeaderSize, "SSIF Transmit Header:\n");
  }

  if (RequestData != NULL) {
    HelperManageabilityDebugPrint ((VOID *)RequestData, RequestDataSize, "SSIF Request Data:\n");
  }

  if ((TransmitTrailer != NULL) && (TransmitTrailerSize != 0)) {
    HelperManageabilityDebugPrint ((VOID *)TransmitTrailer, (UINT32)TransmitTrailerSize, "SSIF Transmit Trailer:\n");
  }

  NetFunction = ((MANAGEABILITY_IPMI_TRANSPORT_HEADER *)TransmitHeader)->NetFn;
  ASSERT (NetFunction <= MANAGEABILITY_IPMI_NET_FUNC_MAX);
  Command = ((MANAGEABILITY_IPMI_TRANSPORT_HEADER *)TransmitHeader)->Command;

  return SsifCommonSendCommand (NetFunction, Command, RequestData, RequestDataSize, ResponseData, ResponseDataSize);
}
