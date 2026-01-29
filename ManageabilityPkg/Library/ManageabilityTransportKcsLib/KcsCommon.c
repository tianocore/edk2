/** @file

  KCS instance of Manageability Transport Library

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <IndustryStandard/IpmiKcs.h>
#include <IndustryStandard/Mctp.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <Library/ManageabilityTransportMctpLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>

#include "ManageabilityTransportKcs.h"

extern MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO  mKcsHardwareInfo;
extern MANAGEABILITY_TRANSPORT_KCS                *mSingleSessionToken;

/**
  This function waits for parameter Flag to set.
  Checks status flag in every 1ms internal till 5 seconds elapses.

  @param[in]  Flag        KCS Flag to test.
  @retval     EFI_SUCCESS The KCS flag under test is set.
  @retval     EFI_TIMEOUT The KCS flag didn't set in 5 second windows.
**/
EFI_STATUS
WaitStatusSet (
  IN  UINT8  Flag
  )
{
  UINT64  Timeout = 0;

  while (!(KcsRegisterRead8 (KCS_REG_STATUS) & Flag)) {
    MicroSecondDelay (IPMI_KCS_TIMEOUT_1MS);
    Timeout = Timeout + IPMI_KCS_TIMEOUT_1MS;
    if (Timeout >= IPMI_KCS_TIMEOUT_5_SEC) {
      return EFI_TIMEOUT;
    }
  }

  return EFI_SUCCESS;
}

/**
  This function waits for parameter Flag to get cleared.
  Checks status flag in every 1ms internal till 5 seconds elapses.

  @param[in]  Flag        KCS Flag to test.

  @retval     EFI_SUCCESS The KCS flag under test is clear.
  @retval     EFI_TIMEOUT The KCS flag didn't cleared in 5 second windows.
**/
EFI_STATUS
WaitStatusClear (
  IN  UINT8  Flag
  )
{
  UINT64  Timeout = 0;

  while (KcsRegisterRead8 (KCS_REG_STATUS) & Flag) {
    MicroSecondDelay (IPMI_KCS_TIMEOUT_1MS);
    Timeout = Timeout + IPMI_KCS_TIMEOUT_1MS;
    if (Timeout >= IPMI_KCS_TIMEOUT_5_SEC) {
      return EFI_TIMEOUT;
    }
  }

  return EFI_SUCCESS;
}

/**
  This function validates KCS OBF bit.
  Checks whether OBF bit is set or not.

  @retval EFI_SUCCESS    OBF bit is set.
  @retval EFI_NOT_READY  OBF bit is not set.
**/
EFI_STATUS
ClearOBF (
  VOID
  )
{
  if (KcsRegisterRead8 (KCS_REG_STATUS) & IPMI_KCS_OBF) {
    KcsRegisterRead8 (KCS_REG_DATA_IN); // read the data to clear the OBF
    if (KcsRegisterRead8 (KCS_REG_STATUS) & IPMI_KCS_OBF) {
      return EFI_NOT_READY;
    }
  }

  return EFI_SUCCESS;
}

/**
  This function writes/sends data to the KCS port.
  Algorithm is based on flow chart provided in IPMI spec 2.0
  Figure 9-6, KCS Interface BMC to SMS Write Transfer Flow Chart

  @param[in]      TransmitHeader        KCS packet header.
  @param[in]      TransmitHeaderSize    KCS packet header size in byte.
  @param[in]      TransmitTrailer       KCS packet trailer.
  @param[in]      TransmitTrailerSize   KCS packet trailer size in byte.
  @param[in]      RequestData           Command Request Data, could be NULL.
                                        RequestDataSize must be zero, if RequestData
                                        is NULL.
  @param[in]      RequestDataSize       Size of Command Request Data.

  @retval     EFI_SUCCESS           The command byte stream was successfully
                                    submit to the device and a response was
                                    successfully received.
  @retval     EFI_NOT_FOUND         The command was not successfully sent to the
                                    device or a response was not successfully
                                    received from the device.
  @retval     EFI_NOT_READY         Ipmi Device is not ready for Ipmi command
                                    access.
  @retval     EFI_DEVICE_ERROR      Ipmi Device hardware error.
  @retval     EFI_TIMEOUT           The command time out.
  @retval     EFI_UNSUPPORTED       The command was not successfully sent to
                                    the device.
  @retval     EFI_OUT_OF_RESOURCES  The resource allocation is out of resource or
                                    data size error.
**/
EFI_STATUS
KcsTransportWrite (
  IN  MANAGEABILITY_TRANSPORT_HEADER   TransmitHeader,
  IN  UINT16                           TransmitHeaderSize,
  IN  MANAGEABILITY_TRANSPORT_TRAILER  TransmitTrailer OPTIONAL,
  IN  UINT16                           TransmitTrailerSize,
  IN  UINT8                            *RequestData OPTIONAL,
  IN  UINT32                           RequestDataSize
  )
{
  EFI_STATUS  Status;
  UINT32      Length;
  UINT8       *Buffer;
  UINT8       *BufferPtr;

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

  Length = TransmitHeaderSize;
  if (RequestData != NULL) {
    Length = Length + RequestDataSize;
  }

  if ((TransmitTrailer != NULL) && (TransmitTrailerSize != 0)) {
    Length += TransmitTrailerSize;
  }

  Buffer = AllocateZeroPool (Length);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Buffer[0..(TransmitHeaderSize - 1)] = TransmitHeader
  // Buffer [TransmitHeader..(TransmitHeader + RequestDataSize - 1)] = RequestData
  // Buffer [(TransmitHeader + RequestDataSize)..(TransmitHeader + RequestDataSize + TransmitTrailerSize - 1)] = TransmitTrailer
  //
  BufferPtr = Buffer;
  CopyMem ((VOID *)BufferPtr, (VOID *)TransmitHeader, TransmitHeaderSize);
  BufferPtr += TransmitHeaderSize;
  if (RequestData != NULL) {
    CopyMem (BufferPtr, RequestData, RequestDataSize);
  }

  BufferPtr += RequestDataSize;
  if (TransmitTrailer != NULL) {
    CopyMem (BufferPtr, (VOID *)TransmitTrailer, TransmitTrailerSize);
  }

  BufferPtr = Buffer;

  // Step 1. wait for IBF to get clear
  Status = WaitStatusClear (IPMI_KCS_IBF);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return Status;
  }

  // Step 2. clear OBF
  if (EFI_ERROR (ClearOBF ())) {
    FreePool (Buffer);
    return EFI_NOT_READY;
  }

  // Step 3. WR_START to CMD, phase=wr_start
  KcsRegisterWrite8 (KCS_REG_COMMAND, IPMI_KCS_CONTROL_CODE_WRITE_START);

  // Step 4. wait for IBF to get clear
  Status = WaitStatusClear (IPMI_KCS_IBF);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return Status;
  }

  // Step 5. check state it should be WRITE_STATE, else exit with error
  if (IPMI_KCS_GET_STATE (KcsRegisterRead8 (KCS_REG_STATUS)) != IpmiKcsWriteState) {
    FreePool (Buffer);
    return EFI_NOT_READY;
  }

  // Step 6, Clear OBF
  if (EFI_ERROR (ClearOBF ())) {
    FreePool (Buffer);
    return EFI_NOT_READY;
  }

  while (Length > 1) {
    // Step 7, phase wr_data, write one byte of Data
    KcsRegisterWrite8 (KCS_REG_DATA_OUT, *BufferPtr);
    Length--;
    BufferPtr++;

    // Step 8. wait for IBF clear
    Status = WaitStatusClear (IPMI_KCS_IBF);
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      return Status;
    }

    // Step 9. check state it should be WRITE_STATE, else exit with error
    if (IPMI_KCS_GET_STATE (KcsRegisterRead8 (KCS_REG_STATUS)) != IpmiKcsWriteState) {
      FreePool (Buffer);
      return EFI_NOT_READY;
    }

    // Step 10
    if (EFI_ERROR (ClearOBF ())) {
      FreePool (Buffer);
      return EFI_NOT_READY;
    }

    //
    // Step 11, check for DATA completion if more than one byte;
    // if still need to be transferred then go to step 7 and repeat
    //
  }

  // Step 12, WR_END  to CMD
  KcsRegisterWrite8 (KCS_REG_COMMAND, IPMI_KCS_CONTROL_CODE_WRITE_END);

  // Step 13. wait for IBF to get clear
  Status = WaitStatusClear (IPMI_KCS_IBF);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return Status;
  }

  // Step 14. check state it should be WRITE_STATE, else exit with error
  if (IPMI_KCS_GET_STATE (KcsRegisterRead8 (KCS_REG_STATUS)) != IpmiKcsWriteState) {
    FreePool (Buffer);
    return EFI_NOT_READY;
  }

  // Step 15
  if (EFI_ERROR (ClearOBF ())) {
    FreePool (Buffer);
    return EFI_NOT_READY;
  }

  // Step 16, write the last byte
  KcsRegisterWrite8 (KCS_REG_DATA_OUT, *BufferPtr);
  FreePool (Buffer);
  return EFI_SUCCESS;
}

/**
  This function sends/receives data from KCS port.
  Algorithm is based on flow chart provided in IPMI spec 2.0
  Figure 9-7, KCS Interface BMC to SMS Read Transfer Flow Chart

  @param [in]       DataBytes             Buffer to hold the read Data.
  @param [in, out]  Length                Number of Bytes read from KCS port.
  @retval           EFI_SUCCESS           The command byte stream was
                                          successfully submit to the device and
                                          a response was successfully received.
  @retval           EFI_NOT_FOUND         The command was not successfully sent
                                          to the device or a response was not
                                          successfully received from the
                                          device.
  @retval           EFI_NOT_READY         Ipmi Device is not ready for Ipmi
                                          command access.
  @retval           EFI_DEVICE_ERROR      Ipmi Device hardware error.
  @retval           EFI_TIMEOUT           The command time out.
  @retval           EFI_UNSUPPORTED       The command was not successfully set
                                          to the device.
  @retval           EFI_OUT_OF_RESOURCES  The resource allocation is out of
                                          resource or data size error.
**/
EFI_STATUS
KcsTransportRead (
  OUT    UINT8   *DataByte,
  IN OUT UINT32  *Length
  )
{
  EFI_STATUS  Status;
  UINT32      ReadLength;

  if ((DataByte == NULL) || (*Length == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: Either DataByte is NULL or Length is 0.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  ReadLength = 0;
  while (ReadLength < *Length) {
    // Step 1. wait for IBF to get clear
    Status = WaitStatusClear (IPMI_KCS_IBF);
    if (EFI_ERROR (Status)) {
      *Length = ReadLength;
      return Status;
    }

    // Step 2. check state it should be READ_STATE, else exit with error
    if (IPMI_KCS_GET_STATE (KcsRegisterRead8 (KCS_REG_STATUS)) == IpmiKcsReadState) {
      // Step 2.1.1 check of OBF to get clear
      Status = WaitStatusSet (IPMI_KCS_OBF);
      if (EFI_ERROR (Status)) {
        *Length = ReadLength;
        return Status;
      }

      // Step 2.1.2 read data from data out
      DataByte[ReadLength++] = KcsRegisterRead8 (KCS_REG_DATA_IN);
      Status                 = WaitStatusClear (IPMI_KCS_IBF);
      if (EFI_ERROR (Status)) {
        *Length = ReadLength;
        return Status;
      }

      // Step 2.1.3 Write READ byte to data in register.
      KcsRegisterWrite8 (KCS_REG_DATA_OUT, IPMI_KCS_CONTROL_CODE_READ);
    } else if (IPMI_KCS_GET_STATE (KcsRegisterRead8 (KCS_REG_STATUS)) == IpmiKcsIdleState) {
      // Step 2.2.1
      Status = WaitStatusSet (IPMI_KCS_OBF);
      if (EFI_ERROR (Status)) {
        *Length = ReadLength;
        return Status;
      }

      // Step 2.2.2 read dummy data
      KcsRegisterRead8 (KCS_REG_DATA_IN); // Dummy read as per IPMI spec
      *Length = ReadLength;
      return EFI_SUCCESS;
    } else {
      *Length = ReadLength;
      return EFI_DEVICE_ERROR;
    }
  }

  *Length = ReadLength;
  return EFI_SUCCESS;
}

/**
  This funciton checks the KCS response data according to
  manageability protocol.

  @param[in]      ResponseData        Pointer to response data.
  @param[in]      ResponseDataSize    Size of response data.
  @param[out]     AdditionalStatus    Pointer to receive the additional status.

  @retval         EFI_SUCCESS         KCS response header is checked without error
  @retval         EFI_DEVICE_ERROR    KCS response header has problem.
**/
EFI_STATUS
KcsCheckResponseData (
  IN UINT8                                       *ResponseData,
  IN UINT32                                      ResponseDataSize,
  OUT MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *AdditionalStatus
  )
{
  EFI_STATUS                      Status;
  MANAGEABILITY_MCTP_KCS_TRAILER  MctpKcsPec;
  UINT32                          PecSize;
  UINT8                           CalculatedPec;
  CHAR16                          *CompletionCodeStr;

  Status            = EFI_SUCCESS;
  *AdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NO_ERRORS;
  if (CompareGuid (&gManageabilityProtocolMctpGuid, mSingleSessionToken->Token.ManageabilityProtocolSpecification)) {
    //
    // For MCTP over KCS, check PEC
    //
    PecSize = sizeof (MANAGEABILITY_MCTP_KCS_TRAILER) + 1;  // +1 to read last dummy byte that finishes KCS transfer
    Status  = KcsTransportRead (&MctpKcsPec.Pec, &PecSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Error! Failed to read PEC with Status(%r)\n",
        __func__,
        Status
        ));
      *AdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_ERROR;
      return Status;
    }

    if (PecSize != sizeof (MctpKcsPec)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Error! Received PEC size is %d instead of %d\n",
        __func__,
        PecSize,
        sizeof (MctpKcsPec)
        ));
      *AdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_ERROR;
      return EFI_DEVICE_ERROR;
    }

    HelperManageabilityDebugPrint ((VOID *)&MctpKcsPec.Pec, PecSize - 1, "MCTP over KCS Response PEC:\n");
    CalculatedPec = HelperManageabilityGenerateCrc8 (MCTP_KCS_PACKET_ERROR_CODE_POLY, 0, ResponseData, ResponseDataSize);
    if (CalculatedPec != MctpKcsPec.Pec) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Error! Received PEC is 0x%02x instead of 0x%02x\n",
        __func__,
        MctpKcsPec.Pec,
        CalculatedPec
        ));
      Status = EFI_DEVICE_ERROR;
    }
  } else if (CompareGuid (&gManageabilityProtocolIpmiGuid, mSingleSessionToken->Token.ManageabilityProtocolSpecification)) {
    //
    // For IPMI over KCS
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
  This funciton reads the KCS response header according to
  manageability protocol. Caller has to free the memory
  allocated for response header.

  @param[in]      ResponseHeader         Pointer to receive the response header.
  @param[out]     AdditionalStatus       Pointer to receive the additional status.

  @retval         EFI_SUCCESS            KCS response header is checked and returned
                                         to caller.
  @retval         EFI_INVALID_PARAMETER  One of the given parameter is incorrect.
  @retval         EFI_OUT_OF_RESOURCE    Memory allocation is failed for ResponseHeader.
  @retval         EFI_DEVICE_ERROR       Incorrect response header.
**/
EFI_STATUS
KcsReadResponseHeader (
  IN   UINT8                                      **ResponseHeader,
  OUT  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *AdditionalStatus
  )
{
  EFI_STATUS  Status;
  UINT32      RspHeaderSize;
  UINT32      ExpectedHeaderSize;
  UINT8       *RspHeader;

  if ((ResponseHeader == NULL) || (AdditionalStatus == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *ResponseHeader = NULL;
  if (CompareGuid (&gManageabilityProtocolMctpGuid, mSingleSessionToken->Token.ManageabilityProtocolSpecification)) {
    // For MCTP over KCS
    ExpectedHeaderSize = sizeof (MANAGEABILITY_MCTP_KCS_HEADER);
    DEBUG ((
      DEBUG_MANAGEABILITY_INFO,
      "%a: Reading MCTP over KCS response header.\n",
      __func__
      ));
  } else if (CompareGuid (&gManageabilityProtocolIpmiGuid, mSingleSessionToken->Token.ManageabilityProtocolSpecification)) {
    // For IPMI over KCS

    ExpectedHeaderSize = sizeof (IPMI_KCS_RESPONSE_HEADER);
    DEBUG ((
      DEBUG_MANAGEABILITY_INFO,
      "%a: Reading IPMI over KCS response header.\n",
      __func__
      ));
  } else {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error! Unsupportted manageability protocol over KCS: %g.\n",
      __func__,
      mSingleSessionToken->Token.ManageabilityProtocolSpecification
      ));
    return EFI_INVALID_PARAMETER;
  }

  RspHeader = (UINT8 *)AllocateZeroPool (ExpectedHeaderSize);
  if (RspHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "Memory allocation failed for KCS response header!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  RspHeaderSize = ExpectedHeaderSize;
  Status        = KcsTransportRead (RspHeader, &RspHeaderSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Error! Failed to read KCS response header Status(%r)\n",
      __func__,
      Status
      ));
    FreePool (RspHeader);
    *AdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_ERROR;
    return Status;
  }

  if (RspHeaderSize != 0) {
    HelperManageabilityDebugPrint ((VOID *)RspHeader, RspHeaderSize, "KCS Response Header:\n");
  }

  if (ExpectedHeaderSize != RspHeaderSize) {
    DEBUG ((
      DEBUG_ERROR,
      "The size (%d bytes) of returned resposne header is not the same as expection (%d bytes)!\n",
      RspHeaderSize,
      ExpectedHeaderSize
      ));
    FreePool (RspHeader);
    return EFI_DEVICE_ERROR;
  }

  if (CompareGuid (&gManageabilityProtocolMctpGuid, mSingleSessionToken->Token.ManageabilityProtocolSpecification)) {
    //
    // MCTP over KCS
    //
    if (((MANAGEABILITY_MCTP_KCS_HEADER *)RspHeader)->NetFunc != MCTP_KCS_NETFN_LUN) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Error! MANAGEABILITY_MCTP_KCS_HEADER.NetFunc is equal 0x%02x instead of 0x%02x\n",
        __func__,
        ((MANAGEABILITY_MCTP_KCS_HEADER *)RspHeader)->NetFunc,
        MCTP_KCS_NETFN_LUN
        ));
      FreePool (RspHeader);
      *AdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_ERROR;
      return EFI_DEVICE_ERROR;
    }

    if (((MANAGEABILITY_MCTP_KCS_HEADER *)RspHeader)->DefiningBody != DEFINING_BODY_DMTF_PRE_OS_WORKING_GROUP) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Error! MANAGEABILITY_MCTP_KCS_HEADER.DefiningBody is equal 0x%02x instead of 0x%02x\n",
        __func__,
        ((MANAGEABILITY_MCTP_KCS_HEADER *)RspHeader)->DefiningBody,
        DEFINING_BODY_DMTF_PRE_OS_WORKING_GROUP
        ));
      FreePool (RspHeader);
      *AdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_ERROR;
      return EFI_DEVICE_ERROR;
    }
  }

  *ResponseHeader   = RspHeader;
  *AdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NO_ERRORS;
  return EFI_SUCCESS;
}

/**
  This service communicates with BMC using KCS protocol.

  @param[in]      TransmitHeader        KCS packet header.
  @param[in]      TransmitHeaderSize    KCS packet header size in byte.
  @param[in]      TransmitTrailer       KCS packet trailer.
  @param[in]      TransmitTrailerSize   KCS packet trailer size in byte.
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
KcsTransportSendCommand (
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
  UINT8       *RspHeader;
  UINT32      ExpectedResponseDataSize;

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
    HelperManageabilityDebugPrint ((VOID *)TransmitHeader, (UINT32)TransmitHeaderSize, "KCS Transmit Header:\n");
  }

  if (RequestData != NULL) {
    HelperManageabilityDebugPrint ((VOID *)RequestData, RequestDataSize, "KCS Request Data:\n");
  }

  if ((TransmitTrailer != NULL) && (TransmitTrailerSize != 0)) {
    HelperManageabilityDebugPrint ((VOID *)TransmitTrailer, (UINT32)TransmitTrailerSize, "KCS Transmit Trailer:\n");
  }

  if ((TransmitHeader != NULL) || (RequestData != NULL)) {
    Status = KcsTransportWrite (
               TransmitHeader,
               TransmitHeaderSize,
               TransmitTrailer,
               TransmitTrailerSize,
               RequestData,
               RequestDataSize
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "KCS Write Failed with Status(%r)\n", Status));
      return Status;
    }
  }

  if ((ResponseData != NULL) && (ResponseDataSize != NULL) && (*ResponseDataSize != 0)) {
    //
    // Read the response header
    //
    Status = KcsReadResponseHeader (&RspHeader, AdditionalStatus);
    if (EFI_ERROR (Status)) {
      return (Status);
    }

    //
    // Override ResposeDataSize if the manageability protocol is MCTP.
    //
    if (CompareGuid (&gManageabilityProtocolMctpGuid, mSingleSessionToken->Token.ManageabilityProtocolSpecification)) {
      if (*ResponseDataSize < ((MANAGEABILITY_MCTP_KCS_HEADER *)RspHeader)->ByteCount) {
        DEBUG ((
          DEBUG_ERROR,
          "%a: Error! MANAGEABILITY_MCTP_KCS_HEADER.ByteCount (0x%02x) is bigger than provided buffer (0x%02x)\n",
          __func__,
          ((MANAGEABILITY_MCTP_KCS_HEADER *)RspHeader)->ByteCount,
          *ResponseDataSize
          ));
        return EFI_INVALID_PARAMETER;
      }

      *ResponseDataSize = ((MANAGEABILITY_MCTP_KCS_HEADER *)RspHeader)->ByteCount;
    }

    FreePool (RspHeader);

    ExpectedResponseDataSize = *ResponseDataSize;
    Status                   = KcsTransportRead (ResponseData, ResponseDataSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "KCS response read Failed with Status(%r)\n", Status));
    }

    // Print out the response payloads.
    if (*ResponseDataSize != 0) {
      if (ExpectedResponseDataSize != *ResponseDataSize) {
        DEBUG ((
          DEBUG_ERROR,
          "Expected KCS response size : %d is not matched to returned size : %d.\n",
          ExpectedResponseDataSize,
          *ResponseDataSize
          ));
        return EFI_DEVICE_ERROR;
      }

      HelperManageabilityDebugPrint ((VOID *)ResponseData, (UINT32)*ResponseDataSize, "KCS Response Data:\n");
      Status = KcsCheckResponseData (ResponseData, *ResponseDataSize, AdditionalStatus);
    } else {
      DEBUG ((DEBUG_ERROR, "No response, can't determine Completion Code.\n"));
    }
  } else {
    *ResponseDataSize = 0;
  }

  return Status;
}

/**
  This function reads 8-bit value from register address.

  @param[in]      Address               This represents either 16-bit IO address
                                        or 32-bit memory mapped address.

  @retval         UINT8                 8-bit value.
**/
UINT8
KcsRegisterRead8 (
  MANAGEABILITY_TRANSPORT_HARDWARE_IO  Address
  )
{
  UINT8  Value;

  if (mKcsHardwareInfo.MemoryMap == MANAGEABILITY_TRANSPORT_KCS_MEMORY_MAP_IO) {
    // Read 8-bit value from 32-bit Memory mapped address.
    Value = MmioRead8 ((UINTN)Address.IoAddress32);
  } else {
    // Read 8-bit value from 16-bit I/O address
    Value = IoRead8 ((UINTN)Address.IoAddress16);
  }

  return Value;
}

/**
  This function writes 8-bit value to register address.

  @param[in]      Address               This represents either 16-bit IO address
                                        or 32-bit memory mapped address.
  @param[in]      Value                 8-bit value write to register address

**/
VOID
KcsRegisterWrite8 (
  MANAGEABILITY_TRANSPORT_HARDWARE_IO  Address,
  UINT8                                Value
  )
{
  if (mKcsHardwareInfo.MemoryMap == MANAGEABILITY_TRANSPORT_KCS_MEMORY_MAP_IO) {
    // Write 8-bit value to 32-bit Memory mapped address.
    MmioWrite8 ((UINTN)Address.IoAddress32, Value);
  } else {
    // Write 8-bit value to 16-bit I/O address
    IoWrite8 ((UINTN)Address.IoAddress16, Value);
  }
}
