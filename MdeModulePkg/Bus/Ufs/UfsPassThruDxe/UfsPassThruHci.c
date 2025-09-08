/** @file
  UfsPassThruDxe driver is used to produce EFI_EXT_SCSI_PASS_THRU protocol interface
  for upper layer application to execute UFS-supported SCSI cmds.

  Copyright (c) 2014 - 2022, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UfsPassThru.h"

/**
  Read 32bits data from specified UFS MMIO register.

  @param[in]  Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  Offset        The offset within the UFS Host Controller MMIO space to start
                            the memory operation.
  @param[out] Value         The data buffer to store.

  @retval EFI_TIMEOUT       The operation is time out.
  @retval EFI_SUCCESS       The operation succeeds.
  @retval Others            The operation fails.

**/
EFI_STATUS
UfsMmioRead32 (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN     UINTN                       Offset,
  OUT UINT32                         *Value
  )
{
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *UfsHc;
  EFI_STATUS                          Status;

  UfsHc = Private->UfsHostController;

  Status = UfsHc->Read (UfsHc, EfiUfsHcWidthUint32, Offset, 1, Value);

  return Status;
}

/**
  Write 32bits data to specified UFS MMIO register.

  @param[in] Private        The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in] Offset         The offset within the UFS Host Controller MMIO space to start
                            the memory operation.
  @param[in] Value          The data to write.

  @retval EFI_TIMEOUT       The operation is time out.
  @retval EFI_SUCCESS       The operation succeeds.
  @retval Others            The operation fails.

**/
EFI_STATUS
UfsMmioWrite32 (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN  UINTN                       Offset,
  IN  UINT32                      Value
  )
{
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *UfsHc;
  EFI_STATUS                          Status;

  UfsHc = Private->UfsHostController;

  Status = UfsHc->Write (UfsHc, EfiUfsHcWidthUint32, Offset, 1, &Value);

  return Status;
}

/**
  Wait for the value of the specified system memory set to the test value.

  @param[in]  Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  Offset        The offset within the UFS Host Controller MMIO space to start
                            the memory operation.
  @param[in]  MaskValue     The mask value of memory.
  @param[in]  TestValue     The test value of memory.
  @param[in]  Timeout       The time out value for wait memory set, uses 100ns as a unit.

  @retval EFI_TIMEOUT       The system memory setting is time out.
  @retval EFI_SUCCESS       The system memory is correct set.
  @retval Others            The operation fails.

**/
EFI_STATUS
UfsWaitMemSet (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN  UINTN                       Offset,
  IN  UINT32                      MaskValue,
  IN  UINT32                      TestValue,
  IN  UINT64                      Timeout
  )
{
  UINT32      Value;
  UINT64      Delay;
  BOOLEAN     InfiniteWait;
  EFI_STATUS  Status;

  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  Delay = DivU64x32 (Timeout, 10) + 1;

  do {
    //
    // Access PCI MMIO space to see if the value is the tested one.
    //
    Status = UfsMmioRead32 (Private, Offset, &Value);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Value &= MaskValue;

    if (Value == TestValue) {
      return EFI_SUCCESS;
    }

    //
    // Stall for 1 microseconds.
    //
    MicroSecondDelay (1);

    Delay--;
  } while (InfiniteWait || (Delay > 0));

  return EFI_TIMEOUT;
}

/**
  Dump UIC command execution result for debugging.

  @param[in]   UicOpcode  The executed UIC opcode.
  @param[in]   Result     The result to be parsed.

**/
VOID
DumpUicCmdExecResult (
  IN  UINT8  UicOpcode,
  IN  UINT8  Result
  )
{
  if (UicOpcode <= UfsUicDmePeerSet) {
    switch (Result) {
      case 0x00:
        break;
      case 0x01:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - INVALID_MIB_ATTRIBUTE\n"));
        break;
      case 0x02:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - INVALID_MIB_ATTRIBUTE_VALUE\n"));
        break;
      case 0x03:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - READ_ONLY_MIB_ATTRIBUTE\n"));
        break;
      case 0x04:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - WRITE_ONLY_MIB_ATTRIBUTE\n"));
        break;
      case 0x05:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - BAD_INDEX\n"));
        break;
      case 0x06:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - LOCKED_MIB_ATTRIBUTE\n"));
        break;
      case 0x07:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - BAD_TEST_FEATURE_INDEX\n"));
        break;
      case 0x08:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - PEER_COMMUNICATION_FAILURE\n"));
        break;
      case 0x09:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - BUSY\n"));
        break;
      case 0x0A:
        DEBUG ((DEBUG_VERBOSE, "UIC configuration command fails - DME_FAILURE\n"));
        break;
      default:
        ASSERT (FALSE);
        break;
    }
  } else {
    switch (Result) {
      case 0x00:
        break;
      case 0x01:
        DEBUG ((DEBUG_VERBOSE, "UIC control command fails - FAILURE\n"));
        break;
      default:
        ASSERT (FALSE);
        break;
    }
  }
}

/**
  Dump QUERY RESPONSE UPIU result for debugging.

  @param[in]   Result  The result to be parsed.

**/
VOID
DumpQueryResponseResult (
  IN  UINT8  Result
  )
{
  switch (Result) {
    case 0xF6:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Parameter Not Readable\n"));
      break;
    case 0xF7:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Parameter Not Writeable\n"));
      break;
    case 0xF8:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Parameter Already Written\n"));
      break;
    case 0xF9:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Invalid Length\n"));
      break;
    case 0xFA:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Invalid Value\n"));
      break;
    case 0xFB:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Invalid Selector\n"));
      break;
    case 0xFC:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Invalid Index\n"));
      break;
    case 0xFD:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Invalid Idn\n"));
      break;
    case 0xFE:
      DEBUG ((DEBUG_VERBOSE, "Query Response with Invalid Opcode\n"));
      break;
    case 0xFF:
      DEBUG ((DEBUG_VERBOSE, "Query Response with General Failure\n"));
      break;
    default:
      ASSERT (FALSE);
      break;
  }
}

/**
  Swap little endian to big endian.

  @param[in, out] Buffer      The data buffer. In input, it contains little endian data.
                              In output, it will become big endian.
  @param[in]      BufferSize  The length of converted data.

**/
VOID
SwapLittleEndianToBigEndian (
  IN OUT UINT8   *Buffer,
  IN     UINT32  BufferSize
  )
{
  UINT32  Index;
  UINT8   Temp;
  UINT32  SwapCount;

  SwapCount = BufferSize / 2;
  for (Index = 0; Index < SwapCount; Index++) {
    Temp                           = Buffer[Index];
    Buffer[Index]                  = Buffer[BufferSize - 1 - Index];
    Buffer[BufferSize - 1 - Index] = Temp;
  }
}

/**
  Fill TSF field of QUERY REQUEST UPIU.

  @param[in, out] TsfBase      The base address of TSF field of QUERY REQUEST UPIU.
  @param[in]      Opcode       The opcode of request.
  @param[in]      DescId       The descriptor ID of request.
  @param[in]      Index        The index of request.
  @param[in]      Selector     The selector of request.
  @param[in]      Length       The length of transferred data. The maximum is 4.
  @param[in]      Value        The value of transferred data.

**/
VOID
UfsFillTsfOfQueryReqUpiu (
  IN OUT UTP_UPIU_TSF  *TsfBase,
  IN     UINT8         Opcode,
  IN     UINT8         DescId    OPTIONAL,
  IN     UINT8         Index     OPTIONAL,
  IN     UINT8         Selector  OPTIONAL,
  IN     UINT16        Length    OPTIONAL,
  IN     UINT32        Value     OPTIONAL
  )
{
  ASSERT (TsfBase != NULL);
  ASSERT (Opcode <= UtpQueryFuncOpcodeTogFlag);

  TsfBase->Opcode = Opcode;
  if (Opcode != UtpQueryFuncOpcodeNop) {
    TsfBase->DescId   = DescId;
    TsfBase->Index    = Index;
    TsfBase->Selector = Selector;

    if ((Opcode == UtpQueryFuncOpcodeRdDesc) || (Opcode == UtpQueryFuncOpcodeWrDesc)) {
      SwapLittleEndianToBigEndian ((UINT8 *)&Length, sizeof (Length));
      TsfBase->Length = Length;
    }

    if (Opcode == UtpQueryFuncOpcodeWrAttr) {
      SwapLittleEndianToBigEndian ((UINT8 *)&Value, sizeof (Value));
      TsfBase->Value = Value;
    }
  }
}

/**
  Initialize COMMAND UPIU.

  @param[in, out] Command         The base address of COMMAND UPIU.
  @param[in]      Lun             The Lun on which the SCSI command is executed.
  @param[in]      TaskTag         The task tag of request.
  @param[in]      Cdb             The cdb buffer containing SCSI command.
  @param[in]      CdbLength       The cdb length.
  @param[in]      DataDirection   The direction of data transfer.
  @param[in]      ExpDataTranLen  The expected transfer data length.

  @retval EFI_SUCCESS     The initialization succeed.

**/
EFI_STATUS
UfsInitCommandUpiu (
  IN OUT UTP_COMMAND_UPIU    *Command,
  IN     UINT8               Lun,
  IN     UINT8               TaskTag,
  IN     UINT8               *Cdb,
  IN     UINT8               CdbLength,
  IN     UFS_DATA_DIRECTION  DataDirection,
  IN     UINT32              ExpDataTranLen
  )
{
  UINT8  Flags;

  ASSERT ((Command != NULL) && (Cdb != NULL));

  //
  // Task attribute is hard-coded to Ordered.
  //
  if (DataDirection == UfsDataIn) {
    Flags = BIT0 | BIT6;
  } else if (DataDirection == UfsDataOut) {
    Flags = BIT0 | BIT5;
  } else {
    Flags = BIT0;
  }

  //
  // Fill UTP COMMAND UPIU associated fields.
  //
  Command->TransCode = 0x01;
  Command->Flags     = Flags;
  Command->Lun       = Lun;
  Command->TaskTag   = TaskTag;
  Command->CmdSet    = 0x00;
  SwapLittleEndianToBigEndian ((UINT8 *)&ExpDataTranLen, sizeof (ExpDataTranLen));
  Command->ExpDataTranLen = ExpDataTranLen;

  CopyMem (Command->Cdb, Cdb, CdbLength);

  return EFI_SUCCESS;
}

/**
  Initialize UTP PRDT for data transfer.

  @param[in] Prdt         The base address of PRDT.
  @param[in] Buffer       The buffer to be read or written.
  @param[in] BufferSize   The data size to be read or written.

  @retval EFI_SUCCESS     The initialization succeed.

**/
EFI_STATUS
UfsInitUtpPrdt (
  IN  UTP_TR_PRD  *Prdt,
  IN  VOID        *Buffer,
  IN  UINT32      BufferSize
  )
{
  UINT32  PrdtIndex;
  UINT32  RemainingLen;
  UINT8   *Remaining;
  UINTN   PrdtNumber;

  ASSERT (((UINTN)Buffer & (BIT0 | BIT1)) == 0);
  ASSERT ((BufferSize & (BIT1 | BIT0)) == 0);

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  RemainingLen = BufferSize;
  Remaining    = Buffer;
  PrdtNumber   = (UINTN)DivU64x32 ((UINT64)BufferSize + UFS_MAX_DATA_LEN_PER_PRD - 1, UFS_MAX_DATA_LEN_PER_PRD);

  for (PrdtIndex = 0; PrdtIndex < PrdtNumber; PrdtIndex++) {
    if (RemainingLen < UFS_MAX_DATA_LEN_PER_PRD) {
      Prdt[PrdtIndex].DbCount = (UINT32)RemainingLen - 1;
    } else {
      Prdt[PrdtIndex].DbCount = UFS_MAX_DATA_LEN_PER_PRD - 1;
    }

    Prdt[PrdtIndex].DbAddr  = (UINT32)RShiftU64 ((UINT64)(UINTN)Remaining, 2);
    Prdt[PrdtIndex].DbAddrU = (UINT32)RShiftU64 ((UINT64)(UINTN)Remaining, 32);
    RemainingLen           -= UFS_MAX_DATA_LEN_PER_PRD;
    Remaining              += UFS_MAX_DATA_LEN_PER_PRD;
  }

  return EFI_SUCCESS;
}

/**
  Initialize QUERY REQUEST UPIU.

  @param[in, out] QueryReq      The base address of QUERY REQUEST UPIU.
  @param[in]      TaskTag       The task tag of request.
  @param[in]      Opcode        The opcode of request.
  @param[in]      DescId        The descriptor ID of request.
  @param[in]      Index         The index of request.
  @param[in]      Selector      The selector of request.
  @param[in]      DataSize      The data size to be read or written.
  @param[in]      Data          The buffer to be read or written.

  @retval EFI_SUCCESS           The initialization succeed.

**/
EFI_STATUS
UfsInitQueryRequestUpiu (
  IN OUT UTP_QUERY_REQ_UPIU  *QueryReq,
  IN     UINT8               TaskTag,
  IN     UINT8               Opcode,
  IN     UINT8               DescId,
  IN     UINT8               Index,
  IN     UINT8               Selector,
  IN     UINTN               DataSize   OPTIONAL,
  IN     UINT8               *Data      OPTIONAL
  )
{
  ASSERT (QueryReq != NULL);

  QueryReq->TransCode = 0x16;
  QueryReq->TaskTag   = TaskTag;
  if ((Opcode == UtpQueryFuncOpcodeRdDesc) || (Opcode == UtpQueryFuncOpcodeRdFlag) || (Opcode == UtpQueryFuncOpcodeRdAttr)) {
    QueryReq->QueryFunc = QUERY_FUNC_STD_READ_REQ;
  } else {
    QueryReq->QueryFunc = QUERY_FUNC_STD_WRITE_REQ;
  }

  if (Opcode == UtpQueryFuncOpcodeWrAttr) {
    UfsFillTsfOfQueryReqUpiu (&QueryReq->Tsf, Opcode, DescId, Index, Selector, 0, *(UINT32 *)Data);
  } else if ((Opcode == UtpQueryFuncOpcodeRdDesc) || (Opcode == UtpQueryFuncOpcodeWrDesc)) {
    UfsFillTsfOfQueryReqUpiu (&QueryReq->Tsf, Opcode, DescId, Index, Selector, (UINT16)DataSize, 0);
  } else {
    UfsFillTsfOfQueryReqUpiu (&QueryReq->Tsf, Opcode, DescId, Index, Selector, 0, 0);
  }

  if (Opcode == UtpQueryFuncOpcodeWrDesc) {
    CopyMem (QueryReq + 1, Data, DataSize);

    SwapLittleEndianToBigEndian ((UINT8 *)&DataSize, sizeof (UINT16));
    QueryReq->DataSegLen = (UINT16)DataSize;
  }

  return EFI_SUCCESS;
}

/**
  Allocate COMMAND/RESPONSE UPIU for filling UTP TRD's command descriptor field.

  @param[in]  Private           The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  Lun               The Lun on which the SCSI command is executed.
  @param[in]  Packet            The pointer to the EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET data structure.
  @param[in]  Trd               The pointer to the UTP Transfer Request Descriptor.
  @param[out] CmdDescHost       A pointer to store the base system memory address of the allocated range.
  @param[out] CmdDescMapping    A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The creation succeed.
  @retval EFI_DEVICE_ERROR      The creation failed.
  @retval EFI_OUT_OF_RESOURCES  The memory resource is insufficient.

**/
EFI_STATUS
UfsCreateScsiCommandDesc (
  IN     UFS_PASS_THRU_PRIVATE_DATA                  *Private,
  IN     UINT8                                       Lun,
  IN     EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet,
  IN     UTP_TRD                                     *Trd,
  OUT VOID                                           **CmdDescHost,
  OUT VOID                                           **CmdDescMapping
  )
{
  UINTN                 TotalLen;
  UINTN                 PrdtNumber;
  UTP_COMMAND_UPIU      *CommandUpiu;
  EFI_PHYSICAL_ADDRESS  CmdDescPhyAddr;
  EFI_STATUS            Status;
  UINT32                DataLen;
  UFS_DATA_DIRECTION    DataDirection;

  ASSERT ((Private != NULL) && (Packet != NULL) && (Trd != NULL));

  if (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ) {
    DataLen       = Packet->InTransferLength;
    DataDirection = UfsDataIn;
  } else {
    DataLen       = Packet->OutTransferLength;
    DataDirection = UfsDataOut;
  }

  if (DataLen == 0) {
    DataDirection = UfsNoData;
  }

  PrdtNumber = (UINTN)DivU64x32 ((UINT64)DataLen + UFS_MAX_DATA_LEN_PER_PRD - 1, UFS_MAX_DATA_LEN_PER_PRD);

  TotalLen = ROUNDUP8 (sizeof (UTP_COMMAND_UPIU)) + ROUNDUP8 (sizeof (UTP_RESPONSE_UPIU)) + PrdtNumber * sizeof (UTP_TR_PRD);

  Status = UfsAllocateAlignCommonBuffer (Private, TotalLen, CmdDescHost, &CmdDescPhyAddr, CmdDescMapping);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CommandUpiu = (UTP_COMMAND_UPIU *)*CmdDescHost;

  UfsInitCommandUpiu (CommandUpiu, Lun, Private->TaskTag++, Packet->Cdb, Packet->CdbLength, DataDirection, DataLen);

  //
  // Fill UTP_TRD associated fields
  // NOTE: Some UFS host controllers request the Response UPIU and the Physical Region Description Table
  // *MUST* be located at a 64-bit aligned boundary.
  //
  Trd->Int    = UFS_INTERRUPT_COMMAND;
  Trd->Dd     = DataDirection;
  Trd->Ct     = UFS_STORAGE_COMMAND_TYPE;
  Trd->Ocs    = UFS_HC_TRD_OCS_INIT_VALUE;
  Trd->UcdBa  = (UINT32)RShiftU64 ((UINT64)CmdDescPhyAddr, 7);
  Trd->UcdBaU = (UINT32)RShiftU64 ((UINT64)CmdDescPhyAddr, 32);
  Trd->RuL    = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_RESPONSE_UPIU)), sizeof (UINT32));
  Trd->RuO    = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_COMMAND_UPIU)), sizeof (UINT32));
  Trd->PrdtL  = (UINT16)PrdtNumber;
  Trd->PrdtO  = (UINT16)DivU64x32 ((UINT64)(ROUNDUP8 (sizeof (UTP_COMMAND_UPIU)) + ROUNDUP8 (sizeof (UTP_RESPONSE_UPIU))), sizeof (UINT32));
  return EFI_SUCCESS;
}

/**
  Allocate QUERY REQUEST/QUERY RESPONSE UPIU for filling UTP TRD's command descriptor field.

  @param[in]  Private           The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  Packet            The pointer to the UFS_DEVICE_MANAGEMENT_REQUEST_PACKET data structure.
  @param[in]  Trd               The pointer to the UTP Transfer Request Descriptor.
  @param[out] CmdDescHost       A pointer to store the base system memory address of the allocated range.
  @param[out] CmdDescMapping    A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The creation succeed.
  @retval EFI_DEVICE_ERROR      The creation failed.
  @retval EFI_OUT_OF_RESOURCES  The memory resource is insufficient.
  @retval EFI_INVALID_PARAMETER The parameter passed in is invalid.

**/
EFI_STATUS
UfsCreateDMCommandDesc (
  IN     UFS_PASS_THRU_PRIVATE_DATA            *Private,
  IN     UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  *Packet,
  IN     UTP_TRD                               *Trd,
  OUT VOID                                     **CmdDescHost,
  OUT VOID                                     **CmdDescMapping
  )
{
  UINTN                 TotalLen;
  UTP_QUERY_REQ_UPIU    *QueryReqUpiu;
  UINT8                 Opcode;
  UINT32                DataSize;
  UINT8                 *Data;
  UINT8                 DataDirection;
  EFI_PHYSICAL_ADDRESS  CmdDescPhyAddr;
  EFI_STATUS            Status;

  ASSERT ((Private != NULL) && (Packet != NULL) && (Trd != NULL));

  Opcode = Packet->Opcode;
  if ((Opcode > UtpQueryFuncOpcodeTogFlag) || (Opcode == UtpQueryFuncOpcodeNop)) {
    return EFI_INVALID_PARAMETER;
  }

  DataDirection = Packet->DataDirection;
  DataSize      = Packet->TransferLength;
  Data          = Packet->DataBuffer;

  if ((Opcode == UtpQueryFuncOpcodeWrDesc) || (Opcode == UtpQueryFuncOpcodeRdDesc)) {
    if ((DataSize == 0) || (Data == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    TotalLen = ROUNDUP8 (sizeof (UTP_QUERY_REQ_UPIU)) + ROUNDUP8 (sizeof (UTP_QUERY_RESP_UPIU)) + ROUNDUP8 (DataSize);
  } else {
    TotalLen = ROUNDUP8 (sizeof (UTP_QUERY_REQ_UPIU)) + ROUNDUP8 (sizeof (UTP_QUERY_RESP_UPIU));
  }

  Status = UfsAllocateAlignCommonBuffer (Private, TotalLen, CmdDescHost, &CmdDescPhyAddr, CmdDescMapping);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize UTP QUERY REQUEST UPIU
  //
  QueryReqUpiu = (UTP_QUERY_REQ_UPIU *)*CmdDescHost;
  ASSERT (QueryReqUpiu != NULL);
  UfsInitQueryRequestUpiu (
    QueryReqUpiu,
    Private->TaskTag++,
    Opcode,
    Packet->DescId,
    Packet->Index,
    Packet->Selector,
    DataSize,
    Data
    );

  //
  // Fill UTP_TRD associated fields
  // NOTE: Some UFS host controllers request the Query Response UPIU *MUST* be located at a 64-bit aligned boundary.
  //
  Trd->Int    = UFS_INTERRUPT_COMMAND;
  Trd->Dd     = DataDirection;
  Trd->Ct     = UFS_STORAGE_COMMAND_TYPE;
  Trd->Ocs    = UFS_HC_TRD_OCS_INIT_VALUE;
  Trd->UcdBa  = (UINT32)RShiftU64 ((UINT64)CmdDescPhyAddr, 7);
  Trd->UcdBaU = (UINT32)RShiftU64 ((UINT64)CmdDescPhyAddr, 32);
  if (Opcode == UtpQueryFuncOpcodeWrDesc) {
    Trd->RuL = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_QUERY_RESP_UPIU)), sizeof (UINT32));
    Trd->RuO = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_QUERY_REQ_UPIU)) + ROUNDUP8 (DataSize), sizeof (UINT32));
  } else {
    Trd->RuL = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_QUERY_RESP_UPIU)) + ROUNDUP8 (DataSize), sizeof (UINT32));
    Trd->RuO = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_QUERY_REQ_UPIU)), sizeof (UINT32));
  }

  return EFI_SUCCESS;
}

/**
  Allocate NOP IN and NOP OUT UPIU for filling UTP TRD's command descriptor field.

  @param[in]  Private           The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  Trd               The pointer to the UTP Transfer Request Descriptor.
  @param[out] CmdDescHost       A pointer to store the base system memory address of the allocated range.
  @param[out] CmdDescMapping    A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The creation succeed.
  @retval EFI_DEVICE_ERROR      The creation failed.
  @retval EFI_OUT_OF_RESOURCES  The memory resource is insufficient.

**/
EFI_STATUS
UfsCreateNopCommandDesc (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN     UTP_TRD                     *Trd,
  OUT VOID                           **CmdDescHost,
  OUT VOID                           **CmdDescMapping
  )
{
  UINTN                 TotalLen;
  UTP_NOP_OUT_UPIU      *NopOutUpiu;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  CmdDescPhyAddr;

  ASSERT ((Private != NULL) && (Trd != NULL));

  TotalLen = ROUNDUP8 (sizeof (UTP_NOP_OUT_UPIU)) + ROUNDUP8 (sizeof (UTP_NOP_IN_UPIU));
  Status   = UfsAllocateAlignCommonBuffer (Private, TotalLen, CmdDescHost, &CmdDescPhyAddr, CmdDescMapping);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NopOutUpiu = (UTP_NOP_OUT_UPIU *)*CmdDescHost;
  ASSERT (NopOutUpiu != NULL);
  NopOutUpiu->TaskTag = Private->TaskTag++;

  //
  // Fill UTP_TRD associated fields
  // NOTE: Some UFS host controllers request the Nop Out UPIU *MUST* be located at a 64-bit aligned boundary.
  //
  Trd->Int    = UFS_INTERRUPT_COMMAND;
  Trd->Dd     = 0x00;
  Trd->Ct     = UFS_STORAGE_COMMAND_TYPE;
  Trd->Ocs    = UFS_HC_TRD_OCS_INIT_VALUE;
  Trd->UcdBa  = (UINT32)RShiftU64 ((UINT64)CmdDescPhyAddr, 7);
  Trd->UcdBaU = (UINT32)RShiftU64 ((UINT64)CmdDescPhyAddr, 32);
  Trd->RuL    = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_NOP_IN_UPIU)), sizeof (UINT32));
  Trd->RuO    = (UINT16)DivU64x32 ((UINT64)ROUNDUP8 (sizeof (UTP_NOP_OUT_UPIU)), sizeof (UINT32));

  return EFI_SUCCESS;
}

/**
  Find out available slot in transfer list of a UFS device.

  @param[in]  Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[out] Slot          The available slot.

  @retval EFI_SUCCESS       The available slot was found successfully.
  @retval EFI_NOT_READY     No slot is available at this moment.

**/
EFI_STATUS
UfsFindAvailableSlotInTrl (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  OUT UINT8                          *Slot
  )
{
  UINT8       Nutrs;
  UINT8       Index;
  UINT32      Data;
  EFI_STATUS  Status;

  ASSERT ((Private != NULL) && (Slot != NULL));

  Status = UfsMmioRead32 (Private, UFS_HC_UTRLDBR_OFFSET, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Nutrs = (UINT8)((Private->UfsHcInfo.Capabilities & UFS_HC_CAP_NUTRS) + 1);

  for (Index = 0; Index < Nutrs; Index++) {
    if ((Data & (BIT0 << Index)) == 0) {
      *Slot = Index;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_READY;
}

/**
  Start specified slot in transfer list of a UFS device.

  @param[in]  Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  Slot          The slot to be started.

**/
EFI_STATUS
UfsStartExecCmd (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN  UINT8                       Slot
  )
{
  UINT32      Data;
  EFI_STATUS  Status;

  Status = UfsMmioRead32 (Private, UFS_HC_UTRLRSR_OFFSET, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Data & UFS_HC_UTRLRSR) != UFS_HC_UTRLRSR) {
    Status = UfsMmioWrite32 (Private, UFS_HC_UTRLRSR_OFFSET, UFS_HC_UTRLRSR);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = UfsMmioWrite32 (Private, UFS_HC_UTRLDBR_OFFSET, BIT0 << Slot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Stop specified slot in transfer list of a UFS device.

  @param[in]  Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  Slot          The slot to be stop.

**/
EFI_STATUS
UfsStopExecCmd (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN  UINT8                       Slot
  )
{
  UINT32      Data;
  EFI_STATUS  Status;

  Status = UfsMmioRead32 (Private, UFS_HC_UTRLDBR_OFFSET, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Data & (BIT0 << Slot)) != 0) {
    Status = UfsMmioRead32 (Private, UFS_HC_UTRLCLR_OFFSET, &Data);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = UfsMmioWrite32 (Private, UFS_HC_UTRLCLR_OFFSET, Data & ~(BIT0 << Slot));
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Extracts return data from query response upiu.

  @param[in] Packet     Pointer to the UFS_DEVICE_MANAGEMENT_REQUEST_PACKET.
  @param[in] QueryResp  Pointer to the query response.

  @retval EFI_INVALID_PARAMETER Packet or QueryResp are empty or opcode is invalid.
  @retval EFI_DEVICE_ERROR      Data returned from device is invalid.
  @retval EFI_SUCCESS           Data extracted.

**/
EFI_STATUS
UfsGetReturnDataFromQueryResponse (
  IN UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  *Packet,
  IN UTP_QUERY_RESP_UPIU                   *QueryResp
  )
{
  UINT16  ReturnDataSize;
  UINT32  ReturnData;

  if ((Packet == NULL) || (QueryResp == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Packet->Opcode) {
    case UtpQueryFuncOpcodeRdDesc:
      ReturnDataSize = QueryResp->Tsf.Length;
      SwapLittleEndianToBigEndian ((UINT8 *)&ReturnDataSize, sizeof (UINT16));
      //
      // Make sure the hardware device does not return more data than expected.
      //
      if (ReturnDataSize > Packet->TransferLength) {
        return EFI_DEVICE_ERROR;
      }

      CopyMem (Packet->DataBuffer, (QueryResp + 1), ReturnDataSize);
      Packet->TransferLength = ReturnDataSize;
      break;
    case UtpQueryFuncOpcodeWrDesc:
      ReturnDataSize = QueryResp->Tsf.Length;
      SwapLittleEndianToBigEndian ((UINT8 *)&ReturnDataSize, sizeof (UINT16));
      Packet->TransferLength = ReturnDataSize;
      break;
    case UtpQueryFuncOpcodeRdFlag:
    case UtpQueryFuncOpcodeSetFlag:
    case UtpQueryFuncOpcodeClrFlag:
    case UtpQueryFuncOpcodeTogFlag:
      //
      // The 'FLAG VALUE' field is at byte offset 3 of QueryResp->Tsf.Value
      //
      *((UINT8 *)(Packet->DataBuffer)) = *((UINT8 *)&(QueryResp->Tsf.Value) + 3);
      break;
    case UtpQueryFuncOpcodeRdAttr:
    case UtpQueryFuncOpcodeWrAttr:
      ReturnData = QueryResp->Tsf.Value;
      SwapLittleEndianToBigEndian ((UINT8 *)&ReturnData, sizeof (UINT32));
      CopyMem (Packet->DataBuffer, &ReturnData, sizeof (UINT32));
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Creates Transfer Request descriptor and sends Query Request to the device.

  @param[in] Private Pointer to the UFS_PASS_THRU_PRIVATE_DATA.
  @param[in] Packet  Pointer to the UFS_DEVICE_MANAGEMENT_REQUEST_PACKET.

  @retval EFI_SUCCESS           The device descriptor was read/written successfully.
  @retval EFI_INVALID_PARAMETER The DescId, Index and Selector fields in Packet are invalid
                                combination to point to a type of UFS device descriptor.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to r/w the device descriptor.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of r/w the device descriptor.

**/
EFI_STATUS
UfsSendDmRequestRetry (
  IN UFS_PASS_THRU_PRIVATE_DATA            *Private,
  IN UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  *Packet
  )
{
  UINT8                               Slot;
  UTP_TRD                             *Trd;
  VOID                                *CmdDescHost;
  VOID                                *CmdDescMapping;
  UINT32                              CmdDescSize;
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *UfsHc;
  UTP_QUERY_RESP_UPIU                 *QueryResp;
  EFI_STATUS                          Status;

  //
  // Find out which slot of transfer request list is available.
  //
  Status = UfsFindAvailableSlotInTrl (Private, &Slot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Trd = ((UTP_TRD *)Private->UtpTrlBase) + Slot;
  //
  // Fill transfer request descriptor to this slot.
  //
  Status = UfsCreateDMCommandDesc (Private, Packet, Trd, &CmdDescHost, &CmdDescMapping);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to create DM command descriptor\n"));
    return Status;
  }

  UfsHc     = Private->UfsHostController;
  QueryResp = (UTP_QUERY_RESP_UPIU *)((UINT8 *)CmdDescHost + Trd->RuO * sizeof (UINT32));
  ASSERT (QueryResp != NULL);
  CmdDescSize = Trd->RuO * sizeof (UINT32) + Trd->RuL * sizeof (UINT32);

  //
  // Start to execute the transfer request.
  //
  UfsStartExecCmd (Private, Slot);

  //
  // Wait for the completion of the transfer request.
  //
  Status = UfsWaitMemSet (Private, UFS_HC_UTRLDBR_OFFSET, BIT0, 0, Packet->Timeout);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if ((Trd->Ocs != 0) || (QueryResp->QueryResp != UfsUtpQueryResponseSuccess)) {
    DEBUG ((DEBUG_ERROR, "Failed to send query request, OCS = %X, QueryResp = %X\n", Trd->Ocs, QueryResp->QueryResp));
    DumpQueryResponseResult (QueryResp->QueryResp);

    if ((QueryResp->QueryResp == UfsUtpQueryResponseInvalidSelector) ||
        (QueryResp->QueryResp == UfsUtpQueryResponseInvalidIndex) ||
        (QueryResp->QueryResp == UfsUtpQueryResponseInvalidIdn))
    {
      Status = EFI_INVALID_PARAMETER;
    } else {
      Status = EFI_DEVICE_ERROR;
    }

    goto Exit;
  }

  Status = UfsGetReturnDataFromQueryResponse (Packet, QueryResp);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get return data from query response\n"));
    goto Exit;
  }

Exit:
  UfsHc->Flush (UfsHc);

  UfsStopExecCmd (Private, Slot);

  if (CmdDescMapping != NULL) {
    UfsHc->Unmap (UfsHc, CmdDescMapping);
  }

  if (CmdDescHost != NULL) {
    UfsHc->FreeBuffer (UfsHc, EFI_SIZE_TO_PAGES (CmdDescSize), CmdDescHost);
  }

  return Status;
}

/**
  Sends Query Request to the device. Query is sent until device responds correctly or counter runs out.

  @param[in] Private Pointer to the UFS_PASS_THRU_PRIVATE_DATA.
  @param[in] Packet  Pointer to the UFS_DEVICE_MANAGEMENT_PACKET.

  @retval EFI_SUCCESS           The device responded correctly to the Query request.
  @retval EFI_INVALID_PARAMETER The DescId, Index and Selector fields in Packet are invalid
                                combination to point to a type of UFS device descriptor.
  @retval EFI_DEVICE_ERROR      A device error occurred while waiting for the response from the device.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of the operation.

**/
EFI_STATUS
UfsSendDmRequest (
  IN UFS_PASS_THRU_PRIVATE_DATA            *Private,
  IN UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  *Packet
  )
{
  EFI_STATUS  Status;
  UINT8       Retry;

  Status = EFI_SUCCESS;

  for (Retry = 0; Retry < 5; Retry++) {
    Status = UfsSendDmRequestRetry (Private, Packet);
    if (!EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
  }

  DEBUG ((DEBUG_ERROR, "Failed to get response from the device after %d retries\n", Retry));
  return Status;
}

/**
  Read or write specified device descriptor of a UFS device.

  @param[in]      Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      DescId        The ID of device descriptor.
  @param[in]      Index         The Index of device descriptor.
  @param[in]      Selector      The Selector of device descriptor.
  @param[in, out] Descriptor    The buffer of device descriptor to be read or written.
  @param[in, out] DescSize      The size of device descriptor buffer. On input, the size, in bytes,
                                of the data buffer specified by Descriptor. On output, the number
                                of bytes that were actually transferred.

  @retval EFI_SUCCESS           The device descriptor was read/written successfully.
  @retval EFI_INVALID_PARAMETER DescId, Index and Selector are invalid combination to point to a
                                type of UFS device descriptor.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to r/w the device descriptor.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of r/w the device descriptor.

**/
EFI_STATUS
UfsRwDeviceDesc (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN     BOOLEAN                     Read,
  IN     UINT8                       DescId,
  IN     UINT8                       Index,
  IN     UINT8                       Selector,
  IN OUT VOID                        *Descriptor,
  IN OUT UINT32                      *DescSize
  )
{
  UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  Packet;
  EFI_STATUS                            Status;

  if (DescSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&Packet, sizeof (UFS_DEVICE_MANAGEMENT_REQUEST_PACKET));

  if (Read) {
    Packet.DataDirection = UfsDataIn;
    Packet.Opcode        = UtpQueryFuncOpcodeRdDesc;
  } else {
    Packet.DataDirection = UfsDataOut;
    Packet.Opcode        = UtpQueryFuncOpcodeWrDesc;
  }

  Packet.DataBuffer     = Descriptor;
  Packet.TransferLength = *DescSize;
  Packet.DescId         = DescId;
  Packet.Index          = Index;
  Packet.Selector       = Selector;
  Packet.Timeout        = UFS_TIMEOUT;

  Status = UfsSendDmRequest (Private, &Packet);
  if (EFI_ERROR (Status)) {
    *DescSize = 0;
  } else {
    *DescSize = Packet.TransferLength;
  }

  return Status;
}

/**
  Read or write specified attribute of a UFS device.

  @param[in]      Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      AttrId        The ID of Attribute.
  @param[in]      Index         The Index of Attribute.
  @param[in]      Selector      The Selector of Attribute.
  @param[in, out] Attributes    The value of Attribute to be read or written.

  @retval EFI_SUCCESS           The Attribute was read/written successfully.
  @retval EFI_INVALID_PARAMETER AttrId, Index and Selector are invalid combination to point to a
                                type of UFS device descriptor.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to r/w the Attribute.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of r/w the Attribute.

**/
EFI_STATUS
UfsRwAttributes (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN     BOOLEAN                     Read,
  IN     UINT8                       AttrId,
  IN     UINT8                       Index,
  IN     UINT8                       Selector,
  IN OUT UINT32                      *Attributes
  )
{
  UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  Packet;

  ZeroMem (&Packet, sizeof (UFS_DEVICE_MANAGEMENT_REQUEST_PACKET));

  if (Read) {
    Packet.DataDirection = UfsDataIn;
    Packet.Opcode        = UtpQueryFuncOpcodeRdAttr;
  } else {
    Packet.DataDirection = UfsDataOut;
    Packet.Opcode        = UtpQueryFuncOpcodeWrAttr;
  }

  Packet.DataBuffer = Attributes;
  Packet.DescId     = AttrId;
  Packet.Index      = Index;
  Packet.Selector   = Selector;
  Packet.Timeout    = UFS_TIMEOUT;

  return UfsSendDmRequest (Private, &Packet);
}

/**
  Read or write specified flag of a UFS device.

  @param[in]      Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      FlagId        The ID of flag to be read or written.
  @param[in, out] Value         The value to set or clear flag.

  @retval EFI_SUCCESS           The flag was read/written successfully.
  @retval EFI_INVALID_PARAMETER FlagId is an invalid UFS flag ID.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to r/w the flag.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of r/w the flag.

**/
EFI_STATUS
UfsRwFlags (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN     BOOLEAN                     Read,
  IN     UINT8                       FlagId,
  IN OUT UINT8                       *Value
  )
{
  UFS_DEVICE_MANAGEMENT_REQUEST_PACKET  Packet;

  if (Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&Packet, sizeof (UFS_DEVICE_MANAGEMENT_REQUEST_PACKET));

  if (Read) {
    ASSERT (Value != NULL);
    Packet.DataDirection = UfsDataIn;
    Packet.Opcode        = UtpQueryFuncOpcodeRdFlag;
  } else {
    Packet.DataDirection = UfsDataOut;
    if (*Value == 1) {
      Packet.Opcode = UtpQueryFuncOpcodeSetFlag;
    } else if (*Value == 0) {
      Packet.Opcode = UtpQueryFuncOpcodeClrFlag;
    } else {
      return EFI_INVALID_PARAMETER;
    }
  }

  Packet.DataBuffer = Value;
  Packet.DescId     = FlagId;
  Packet.Index      = 0;
  Packet.Selector   = 0;
  Packet.Timeout    = UFS_TIMEOUT;

  return UfsSendDmRequest (Private, &Packet);
}

/**
  Set specified flag to 1 on a UFS device.

  @param[in]  Private           The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  FlagId            The ID of flag to be set.

  @retval EFI_SUCCESS           The flag was set successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to set the flag.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of setting the flag.

**/
EFI_STATUS
UfsSetFlag (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN  UINT8                       FlagId
  )
{
  EFI_STATUS  Status;
  UINT8       Value;

  Value  = 1;
  Status = UfsRwFlags (Private, FALSE, FlagId, &Value);

  return Status;
}

/**
  Read specified flag from a UFS device.

  @param[in]  Private           The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  FlagId            The ID of flag to be read.
  @param[out] Value             The flag's value.

  @retval EFI_SUCCESS           The flag was read successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to read the flag.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of reading the flag.

**/
EFI_STATUS
UfsReadFlag (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN     UINT8                       FlagId,
  OUT UINT8                          *Value
  )
{
  EFI_STATUS  Status;

  Status = UfsRwFlags (Private, TRUE, FlagId, Value);

  return Status;
}

/**
  Sends NOP IN cmd to a UFS device for initialization process request.
  For more details, please refer to UFS 2.0 spec Figure 13.3.

  @param[in]  Private           The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS           The NOP IN command was sent by the host. The NOP OUT response was
                                received successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to execute NOP IN command.
  @retval EFI_OUT_OF_RESOURCES  The resource for transfer is not available.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the NOP IN command to execute.

**/
EFI_STATUS
UfsExecNopCmds (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS                          Status;
  UINT8                               Slot;
  UTP_TRD                             *Trd;
  UTP_NOP_IN_UPIU                     *NopInUpiu;
  UINT32                              CmdDescSize;
  VOID                                *CmdDescHost;
  VOID                                *CmdDescMapping;
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *UfsHc;

  //
  // Find out which slot of transfer request list is available.
  //
  Status = UfsFindAvailableSlotInTrl (Private, &Slot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Trd    = ((UTP_TRD *)Private->UtpTrlBase) + Slot;
  Status = UfsCreateNopCommandDesc (Private, Trd, &CmdDescHost, &CmdDescMapping);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check the transfer request result.
  //
  UfsHc     = Private->UfsHostController;
  NopInUpiu = (UTP_NOP_IN_UPIU *)((UINT8 *)CmdDescHost + Trd->RuO * sizeof (UINT32));
  ASSERT (NopInUpiu != NULL);
  CmdDescSize = Trd->RuO * sizeof (UINT32) + Trd->RuL * sizeof (UINT32);

  //
  // Start to execute the transfer request.
  //
  UfsStartExecCmd (Private, Slot);

  //
  // Wait for the completion of the transfer request.
  //
  Status = UfsWaitMemSet (Private, UFS_HC_UTRLDBR_OFFSET, BIT0 << Slot, 0, UFS_TIMEOUT);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if (NopInUpiu->Resp != 0) {
    Status = EFI_DEVICE_ERROR;
  } else {
    Status = EFI_SUCCESS;
  }

Exit:
  UfsHc->Flush (UfsHc);

  UfsStopExecCmd (Private, Slot);

  if (CmdDescMapping != NULL) {
    UfsHc->Unmap (UfsHc, CmdDescMapping);
  }

  if (CmdDescHost != NULL) {
    UfsHc->FreeBuffer (UfsHc, EFI_SIZE_TO_PAGES (CmdDescSize), CmdDescHost);
  }

  return Status;
}

/**
  Cleanup data buffers after data transfer. This function
  also takes care to copy all data to user memory pool for
  unaligned data transfers.

  @param[in] Private   Pointer to the UFS_PASS_THRU_PRIVATE_DATA
  @param[in] TransReq  Pointer to the transfer request
**/
VOID
UfsReconcileDataTransferBuffer (
  IN UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN UFS_PASS_THRU_TRANS_REQ     *TransReq
  )
{
  if (TransReq->DataBufMapping != NULL) {
    Private->UfsHostController->Unmap (
                                  Private->UfsHostController,
                                  TransReq->DataBufMapping
                                  );
  }

  //
  // Check if unaligned transfer was performed. If it was and we read
  // data from device copy memory to user data buffers before cleanup.
  // The assumption is if auxiliary aligned data buffer is not NULL then
  // unaligned transfer has been performed.
  //
  if (TransReq->AlignedDataBuf != NULL) {
    if (TransReq->Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ) {
      CopyMem (TransReq->Packet->InDataBuffer, TransReq->AlignedDataBuf, TransReq->Packet->InTransferLength);
    }

    //
    // Wipe out the transfer buffer in case it contains sensitive data.
    //
    ZeroMem (TransReq->AlignedDataBuf, TransReq->AlignedDataBufSize);
    FreeAlignedPages (TransReq->AlignedDataBuf, EFI_SIZE_TO_PAGES (TransReq->AlignedDataBufSize));
    TransReq->AlignedDataBuf = NULL;
  }
}

/**
  Prepare data buffer for transfer.

  @param[in]      Private   Pointer to the UFS_PASS_THRU_PRIVATE_DATA
  @param[in, out] TransReq  Pointer to the transfer request

  @retval EFI_DEVICE_ERROR  Failed to prepare buffer for transfer
  @retval EFI_SUCCESS       Buffer ready for transfer
**/
EFI_STATUS
UfsPrepareDataTransferBuffer (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN OUT UFS_PASS_THRU_TRANS_REQ     *TransReq
  )
{
  EFI_STATUS                           Status;
  VOID                                 *DataBuf;
  UINT32                               DataLen;
  UINTN                                MapLength;
  EFI_PHYSICAL_ADDRESS                 DataBufPhyAddr;
  EDKII_UFS_HOST_CONTROLLER_OPERATION  Flag;
  UTP_TR_PRD                           *PrdtBase;

  DataBufPhyAddr = 0;
  DataBuf        = NULL;

  //
  // For unaligned data transfers we allocate auxiliary DWORD aligned memory pool.
  // When command is finished auxiliary memory pool is copied into actual user memory.
  // This is requiered to assure data transfer safety(DWORD alignment required by UFS spec.)
  //
  if (TransReq->Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ) {
    if (((UINTN)TransReq->Packet->InDataBuffer % 4 != 0) || (TransReq->Packet->InTransferLength % 4 != 0)) {
      DataLen = TransReq->Packet->InTransferLength + (4 - (TransReq->Packet->InTransferLength % 4));
      DataBuf = AllocateAlignedPages (EFI_SIZE_TO_PAGES (DataLen), 4);
      if (DataBuf == NULL) {
        return EFI_DEVICE_ERROR;
      }

      ZeroMem (DataBuf, DataLen);
      TransReq->AlignedDataBuf     = DataBuf;
      TransReq->AlignedDataBufSize = DataLen;
    } else {
      DataLen = TransReq->Packet->InTransferLength;
      DataBuf = TransReq->Packet->InDataBuffer;
    }

    Flag = EdkiiUfsHcOperationBusMasterWrite;
  } else {
    if (((UINTN)TransReq->Packet->OutDataBuffer % 4 != 0) || (TransReq->Packet->OutTransferLength % 4 != 0)) {
      DataLen = TransReq->Packet->OutTransferLength + (4 - (TransReq->Packet->OutTransferLength % 4));
      DataBuf = AllocateAlignedPages (EFI_SIZE_TO_PAGES (DataLen), 4);
      if (DataBuf == NULL) {
        return EFI_DEVICE_ERROR;
      }

      CopyMem (DataBuf, TransReq->Packet->OutDataBuffer, TransReq->Packet->OutTransferLength);
      TransReq->AlignedDataBuf     = DataBuf;
      TransReq->AlignedDataBufSize = DataLen;
    } else {
      DataLen = TransReq->Packet->OutTransferLength;
      DataBuf = TransReq->Packet->OutDataBuffer;
    }

    Flag = EdkiiUfsHcOperationBusMasterRead;
  }

  if (DataLen != 0) {
    MapLength = DataLen;
    Status    = Private->UfsHostController->Map (
                                              Private->UfsHostController,
                                              Flag,
                                              DataBuf,
                                              &MapLength,
                                              &DataBufPhyAddr,
                                              &TransReq->DataBufMapping
                                              );

    if (EFI_ERROR (Status) || (DataLen != MapLength)) {
      if (TransReq->AlignedDataBuf != NULL) {
        //
        // Wipe out the transfer buffer in case it contains sensitive data.
        //
        ZeroMem (TransReq->AlignedDataBuf, TransReq->AlignedDataBufSize);
        FreeAlignedPages (TransReq->AlignedDataBuf, EFI_SIZE_TO_PAGES (TransReq->AlignedDataBufSize));
        TransReq->AlignedDataBuf = NULL;
      }

      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Fill PRDT table of Command UPIU for executed SCSI cmd.
  //
  PrdtBase = (UTP_TR_PRD *)((UINT8 *)TransReq->CmdDescHost + ROUNDUP8 (sizeof (UTP_COMMAND_UPIU)) + ROUNDUP8 (sizeof (UTP_RESPONSE_UPIU)));
  ASSERT (PrdtBase != NULL);
  UfsInitUtpPrdt (PrdtBase, (VOID *)(UINTN)DataBufPhyAddr, DataLen);

  return EFI_SUCCESS;
}

/**
  Sends a UFS-supported SCSI Request Packet to a UFS device that is attached to the UFS host controller.

  @param[in]      Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]      Lun           The LUN of the UFS device to send the SCSI Request Packet.
  @param[in, out] Packet        A pointer to the SCSI Request Packet to send to a specified Lun of the
                                UFS device.
  @param[in]      Event         If nonblocking I/O is not supported then Event is ignored, and blocking
                                I/O is performed. If Event is NULL, then blocking I/O is performed. If
                                Event is not NULL and non blocking I/O is supported, then
                                nonblocking I/O is performed, and Event will be signaled when the
                                SCSI Request Packet completes.

  @retval EFI_SUCCESS           The SCSI Request Packet was sent by the host. For bi-directional
                                commands, InTransferLength bytes were transferred from
                                InDataBuffer. For write and bi-directional commands,
                                OutTransferLength bytes were transferred by
                                OutDataBuffer.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send the SCSI Request
                                Packet.
  @retval EFI_OUT_OF_RESOURCES  The resource for transfer is not available.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the SCSI Request Packet to execute.

**/
EFI_STATUS
UfsExecScsiCmds (
  IN     UFS_PASS_THRU_PRIVATE_DATA                  *Private,
  IN     UINT8                                       Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet,
  IN     EFI_EVENT                                   Event    OPTIONAL
  )
{
  EFI_STATUS                          Status;
  UTP_RESPONSE_UPIU                   *Response;
  UINT16                              SenseDataLen;
  UINT32                              ResTranCount;
  EFI_TPL                             OldTpl;
  UFS_PASS_THRU_TRANS_REQ             *TransReq;
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *UfsHc;

  TransReq = AllocateZeroPool (sizeof (UFS_PASS_THRU_TRANS_REQ));
  if (TransReq == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TransReq->Signature     = UFS_PASS_THRU_TRANS_REQ_SIG;
  TransReq->TimeoutRemain = Packet->Timeout;
  TransReq->Packet        = Packet;

  UfsHc = Private->UfsHostController;
  //
  // Find out which slot of transfer request list is available.
  //
  Status = UfsFindAvailableSlotInTrl (Private, &TransReq->Slot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TransReq->Trd = ((UTP_TRD *)Private->UtpTrlBase) + TransReq->Slot;

  //
  // Fill transfer request descriptor to this slot.
  //
  Status = UfsCreateScsiCommandDesc (
             Private,
             Lun,
             Packet,
             TransReq->Trd,
             &TransReq->CmdDescHost,
             &TransReq->CmdDescMapping
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TransReq->CmdDescSize = TransReq->Trd->PrdtO * sizeof (UINT32) + TransReq->Trd->PrdtL * sizeof (UTP_TR_PRD);

  Status = UfsPrepareDataTransferBuffer (Private, TransReq);
  if (EFI_ERROR (Status)) {
    goto Exit1;
  }

  //
  // Insert the async SCSI cmd to the Async I/O list
  //
  if (Event != NULL) {
    OldTpl                = gBS->RaiseTPL (TPL_NOTIFY);
    TransReq->CallerEvent = Event;
    InsertTailList (&Private->Queue, &TransReq->TransferList);
    gBS->RestoreTPL (OldTpl);
  }

  //
  // Start to execute the transfer request.
  //
  UfsStartExecCmd (Private, TransReq->Slot);

  //
  // Immediately return for async I/O.
  //
  if (Event != NULL) {
    return EFI_SUCCESS;
  }

  //
  // Wait for the completion of the transfer request.
  //
  Status = UfsWaitMemSet (Private, UFS_HC_UTRLDBR_OFFSET, BIT0 << TransReq->Slot, 0, Packet->Timeout);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Get sense data if exists
  //
  Response = (UTP_RESPONSE_UPIU *)((UINT8 *)TransReq->CmdDescHost + TransReq->Trd->RuO * sizeof (UINT32));
  ASSERT (Response != NULL);
  SenseDataLen = Response->SenseDataLen;
  SwapLittleEndianToBigEndian ((UINT8 *)&SenseDataLen, sizeof (UINT16));

  if ((Packet->SenseDataLength != 0) && (Packet->SenseData != NULL)) {
    //
    // Make sure the hardware device does not return more data than expected.
    //
    if (SenseDataLen <= Packet->SenseDataLength) {
      CopyMem (Packet->SenseData, Response->SenseData, SenseDataLen);
      Packet->SenseDataLength = (UINT8)SenseDataLen;
    } else {
      Packet->SenseDataLength = 0;
    }
  }

  //
  // Check the transfer request result.
  //
  Packet->TargetStatus = Response->Status;
  if (Response->Response != 0) {
    DEBUG ((DEBUG_ERROR, "UfsExecScsiCmds() fails with Target Failure\n"));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  if (TransReq->Trd->Ocs == 0) {
    if (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ) {
      if ((Response->Flags & BIT5) == BIT5) {
        ResTranCount = Response->ResTranCount;
        SwapLittleEndianToBigEndian ((UINT8 *)&ResTranCount, sizeof (UINT32));
        Packet->InTransferLength -= ResTranCount;
      }
    } else {
      if ((Response->Flags & BIT5) == BIT5) {
        ResTranCount = Response->ResTranCount;
        SwapLittleEndianToBigEndian ((UINT8 *)&ResTranCount, sizeof (UINT32));
        Packet->OutTransferLength -= ResTranCount;
      }
    }
  } else {
    Status = EFI_DEVICE_ERROR;
  }

Exit:
  UfsHc->Flush (UfsHc);

  UfsStopExecCmd (Private, TransReq->Slot);

  UfsReconcileDataTransferBuffer (Private, TransReq);

Exit1:
  if (TransReq->CmdDescMapping != NULL) {
    UfsHc->Unmap (UfsHc, TransReq->CmdDescMapping);
  }

  if (TransReq->CmdDescHost != NULL) {
    UfsHc->FreeBuffer (UfsHc, EFI_SIZE_TO_PAGES (TransReq->CmdDescSize), TransReq->CmdDescHost);
  }

  if (TransReq != NULL) {
    FreePool (TransReq);
  }

  return Status;
}

/**
  Send UIC command.

  @param[in]      Private     The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in, out] UicCommand  UIC command descriptor. On exit contains UIC command results.

  @return EFI_SUCCESS      Successfully execute this UIC command and detect attached UFS device.
  @return EFI_DEVICE_ERROR Fail to execute this UIC command and detect attached UFS device.

**/
EFI_STATUS
UfsExecUicCommands (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN OUT EDKII_UIC_COMMAND        *UicCommand
  )
{
  EFI_STATUS  Status;
  UINT32      Data;

  Status = UfsMmioRead32 (Private, UFS_HC_IS_OFFSET, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Data & UFS_HC_IS_UCCS) == UFS_HC_IS_UCCS) {
    //
    // Clear IS.BIT10 UIC Command Completion Status (UCCS) at first.
    //
    Status = UfsMmioWrite32 (Private, UFS_HC_IS_OFFSET, Data);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // When programming UIC command registers, host software shall set the register UICCMD
  // only after all the UIC command argument registers (UICCMDARG1, UICCMDARG2 and UICCMDARG3)
  // are set.
  //
  Status = UfsMmioWrite32 (Private, UFS_HC_UCMD_ARG1_OFFSET, UicCommand->Arg1);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UfsMmioWrite32 (Private, UFS_HC_UCMD_ARG2_OFFSET, UicCommand->Arg2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UfsMmioWrite32 (Private, UFS_HC_UCMD_ARG3_OFFSET, UicCommand->Arg3);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Host software shall only set the UICCMD if HCS.UCRDY is set to 1.
  //
  Status = UfsWaitMemSet (Private, UFS_HC_STATUS_OFFSET, UFS_HC_HCS_UCRDY, UFS_HC_HCS_UCRDY, UFS_TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UfsMmioWrite32 (Private, UFS_HC_UIC_CMD_OFFSET, UicCommand->Opcode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // UFS 2.0 spec section 5.3.1 Offset:0x20 IS.Bit10 UIC Command Completion Status (UCCS)
  // This bit is set to '1' by the host controller upon completion of a UIC command.
  //
  Status = UfsWaitMemSet (Private, UFS_HC_IS_OFFSET, UFS_HC_IS_UCCS, UFS_HC_IS_UCCS, UFS_TIMEOUT);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (UicCommand->Opcode != UfsUicDmeReset) {
    Status = UfsMmioRead32 (Private, UFS_HC_UCMD_ARG2_OFFSET, &UicCommand->Arg2);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = UfsMmioRead32 (Private, UFS_HC_UCMD_ARG3_OFFSET, &UicCommand->Arg3);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((UicCommand->Arg2 & 0xFF) != 0) {
      DEBUG_CODE_BEGIN ();
      DumpUicCmdExecResult ((UINT8)UicCommand->Opcode, (UINT8)(UicCommand->Arg2 & 0xFF));
      DEBUG_CODE_END ();
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**
  Allocate common buffer for host and UFS bus master access simultaneously.

  @param[in]  Private                The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  Size                   The length of buffer to be allocated.
  @param[out] CmdDescHost            A pointer to store the base system memory address of the allocated range.
  @param[out] CmdDescPhyAddr         The resulting map address for the UFS bus master to use to access the hosts CmdDescHost.
  @param[out] CmdDescMapping         A resulting value to pass to Unmap().

  @retval EFI_SUCCESS                The common buffer was allocated successfully.
  @retval EFI_DEVICE_ERROR           The allocation fails.
  @retval EFI_OUT_OF_RESOURCES       The memory resource is insufficient.

**/
EFI_STATUS
UfsAllocateAlignCommonBuffer (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN     UINTN                       Size,
  OUT VOID                           **CmdDescHost,
  OUT EFI_PHYSICAL_ADDRESS           *CmdDescPhyAddr,
  OUT VOID                           **CmdDescMapping
  )
{
  EFI_STATUS                          Status;
  UINTN                               Bytes;
  BOOLEAN                             Is32BitAddr;
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *UfsHc;

  if ((Private->UfsHcInfo.Capabilities & UFS_HC_CAP_64ADDR) == UFS_HC_CAP_64ADDR) {
    Is32BitAddr = FALSE;
  } else {
    Is32BitAddr = TRUE;
  }

  UfsHc  = Private->UfsHostController;
  Status = UfsHc->AllocateBuffer (
                    UfsHc,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES (Size),
                    CmdDescHost,
                    0
                    );
  if (EFI_ERROR (Status)) {
    *CmdDescMapping = NULL;
    *CmdDescHost    = NULL;
    *CmdDescPhyAddr = 0;
    return EFI_OUT_OF_RESOURCES;
  }

  Bytes  = EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (Size));
  Status = UfsHc->Map (
                    UfsHc,
                    EdkiiUfsHcOperationBusMasterCommonBuffer,
                    *CmdDescHost,
                    &Bytes,
                    CmdDescPhyAddr,
                    CmdDescMapping
                    );

  if (EFI_ERROR (Status) || (Bytes != EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (Size)))) {
    UfsHc->FreeBuffer (
             UfsHc,
             EFI_SIZE_TO_PAGES (Size),
             *CmdDescHost
             );
    *CmdDescHost = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  if (Is32BitAddr && ((*CmdDescPhyAddr) > 0x100000000ULL)) {
    //
    // The UFS host controller doesn't support 64bit addressing, so should not get a >4G UFS bus master address.
    //
    UfsHc->Unmap (
             UfsHc,
             *CmdDescMapping
             );
    UfsHc->FreeBuffer (
             UfsHc,
             EFI_SIZE_TO_PAGES (Size),
             *CmdDescHost
             );
    *CmdDescMapping = NULL;
    *CmdDescHost    = NULL;
    return EFI_DEVICE_ERROR;
  }

  ZeroMem (*CmdDescHost, EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (Size)));
  return EFI_SUCCESS;
}

/**
  Enable the UFS host controller for accessing.

  @param[in] Private                 The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The UFS host controller enabling was executed successfully.
  @retval EFI_DEVICE_ERROR           A device error occurred while enabling the UFS host controller.

**/
EFI_STATUS
UfsEnableHostController (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;
  UINT32      Data;

  if ((mUfsHcPlatform != NULL) && (mUfsHcPlatform->Callback != NULL)) {
    Status = mUfsHcPlatform->Callback (Private->Handle, EdkiiUfsHcPreHce, &Private->UfsHcDriverInterface);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failure from platform driver during EdkiiUfsHcPreHce, Status = %r\n", Status));
      return Status;
    }
  }

  if ((mUfsHcPlatform == NULL) ||
      ((mUfsHcPlatform->Version >= 3) && !mUfsHcPlatform->SkipHceReenable))
  {
    //
    // UFS 2.0 spec section 7.1.1 - Host Controller Initialization
    //
    // Reinitialize the UFS host controller if HCE bit of HC register is set.
    //
    Status = UfsMmioRead32 (Private, UFS_HC_ENABLE_OFFSET, &Data);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((Data & UFS_HC_HCE_EN) == UFS_HC_HCE_EN) {
      //
      // Write a 0 to the HCE register at first to disable the host controller.
      //
      Status = UfsMmioWrite32 (Private, UFS_HC_ENABLE_OFFSET, 0);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      //
      // Wait until HCE is read as '0' before continuing.
      //
      Status = UfsWaitMemSet (Private, UFS_HC_ENABLE_OFFSET, UFS_HC_HCE_EN, 0, UFS_TIMEOUT);
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }
    }

    //
    // Write a 1 to the HCE register to enable the UFS host controller.
    //
    Status = UfsMmioWrite32 (Private, UFS_HC_ENABLE_OFFSET, UFS_HC_HCE_EN);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Wait until HCE is read as '1' before continuing.
  //
  Status = UfsWaitMemSet (Private, UFS_HC_ENABLE_OFFSET, UFS_HC_HCE_EN, UFS_HC_HCE_EN, UFS_TIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if ((mUfsHcPlatform != NULL) && (mUfsHcPlatform->Callback != NULL)) {
    Status = mUfsHcPlatform->Callback (Private->Handle, EdkiiUfsHcPostHce, &Private->UfsHcDriverInterface);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failure from platform driver during EdkiiUfsHcPostHce, Status = %r\n", Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Detect if a UFS device attached.

  @param[in] Private                 The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The UFS device detection was executed successfully.
  @retval EFI_NOT_FOUND              Not found a UFS device attached.
  @retval EFI_DEVICE_ERROR           A device error occurred while detecting the UFS device.

**/
EFI_STATUS
UfsDeviceDetection (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private
  )
{
  UINTN              Retry;
  EFI_STATUS         Status;
  UINT32             Data;
  EDKII_UIC_COMMAND  LinkStartupCommand;

  if ((mUfsHcPlatform != NULL) && (mUfsHcPlatform->Callback != NULL)) {
    Status = mUfsHcPlatform->Callback (Private->Handle, EdkiiUfsHcPreLinkStartup, &Private->UfsHcDriverInterface);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failure from platform driver during EdkiiUfsHcPreLinkStartup, Status = %r\n", Status));
      return Status;
    }
  }

  if ((mUfsHcPlatform != NULL) && (mUfsHcPlatform->Version >= 3) && mUfsHcPlatform->SkipLinkStartup) {
    return EFI_SUCCESS;
  }

  //
  // Start UFS device detection.
  // Try up to 3 times for establishing data link with device.
  //
  for (Retry = 0; Retry < 3; Retry++) {
    LinkStartupCommand.Opcode = UfsUicDmeLinkStartup;
    LinkStartupCommand.Arg1   = 0;
    LinkStartupCommand.Arg2   = 0;
    LinkStartupCommand.Arg3   = 0;
    Status                    = UfsExecUicCommands (Private, &LinkStartupCommand);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Status = UfsMmioRead32 (Private, UFS_HC_STATUS_OFFSET, &Data);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    if ((Data & UFS_HC_HCS_DP) == 0) {
      Status = UfsWaitMemSet (Private, UFS_HC_IS_OFFSET, UFS_HC_IS_ULSS, UFS_HC_IS_ULSS, UFS_TIMEOUT);
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }
    } else {
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Initialize UFS task management request list related h/w context.

  @param[in] Private                 The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The UFS task management list was initialzed successfully.
  @retval EFI_DEVICE_ERROR           The initialization fails.

**/
EFI_STATUS
UfsInitTaskManagementRequestList (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private
  )
{
  UINT8                 Nutmrs;
  VOID                  *CmdDescHost;
  EFI_PHYSICAL_ADDRESS  CmdDescPhyAddr;
  VOID                  *CmdDescMapping;
  EFI_STATUS            Status;

  //
  // Initial h/w and s/w context for future operations.
  //
  CmdDescHost    = NULL;
  CmdDescMapping = NULL;
  CmdDescPhyAddr = 0;

  //
  // Allocate and initialize UTP Task Management Request List.
  //
  Nutmrs = (UINT8)(RShiftU64 ((Private->UfsHcInfo.Capabilities & UFS_HC_CAP_NUTMRS), 16) + 1);
  Status = UfsAllocateAlignCommonBuffer (Private, Nutmrs * sizeof (UTP_TMRD), &CmdDescHost, &CmdDescPhyAddr, &CmdDescMapping);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Program the UTP Task Management Request List Base Address and UTP Task Management
  // Request List Base Address with a 64-bit address allocated at step 6.
  //
  Status = UfsMmioWrite32 (Private, UFS_HC_UTMRLBA_OFFSET, (UINT32)(UINTN)CmdDescPhyAddr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UfsMmioWrite32 (Private, UFS_HC_UTMRLBAU_OFFSET, (UINT32)RShiftU64 ((UINT64)CmdDescPhyAddr, 32));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private->UtpTmrlBase = CmdDescHost;
  Private->Nutmrs      = Nutmrs;
  Private->TmrlMapping = CmdDescMapping;

  //
  // Enable the UTP Task Management Request List by setting the UTP Task Management
  // Request List RunStop Register (UTMRLRSR) to '1'.
  //
  Status = UfsMmioWrite32 (Private, UFS_HC_UTMRLRSR_OFFSET, UFS_HC_UTMRLRSR);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Initialize UFS transfer request list related h/w context.

  @param[in] Private                 The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The UFS transfer list was initialzed successfully.
  @retval EFI_DEVICE_ERROR           The initialization fails.

**/
EFI_STATUS
UfsInitTransferRequestList (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private
  )
{
  UINT8                 Nutrs;
  VOID                  *CmdDescHost;
  EFI_PHYSICAL_ADDRESS  CmdDescPhyAddr;
  VOID                  *CmdDescMapping;
  EFI_STATUS            Status;

  //
  // Initial h/w and s/w context for future operations.
  //
  CmdDescHost    = NULL;
  CmdDescMapping = NULL;
  CmdDescPhyAddr = 0;

  //
  // Allocate and initialize UTP Transfer Request List.
  //
  Nutrs  = (UINT8)((Private->UfsHcInfo.Capabilities & UFS_HC_CAP_NUTRS) + 1);
  Status = UfsAllocateAlignCommonBuffer (Private, Nutrs * sizeof (UTP_TRD), &CmdDescHost, &CmdDescPhyAddr, &CmdDescMapping);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Program the UTP Transfer Request List Base Address and UTP Transfer Request List
  // Base Address with a 64-bit address allocated at step 8.
  //
  Status = UfsMmioWrite32 (Private, UFS_HC_UTRLBA_OFFSET, (UINT32)(UINTN)CmdDescPhyAddr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UfsMmioWrite32 (Private, UFS_HC_UTRLBAU_OFFSET, (UINT32)RShiftU64 ((UINT64)CmdDescPhyAddr, 32));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private->UtpTrlBase = CmdDescHost;
  Private->Nutrs      = Nutrs;
  Private->TrlMapping = CmdDescMapping;

  //
  // Enable the UTP Transfer Request List by setting the UTP Transfer Request List
  // RunStop Register (UTRLRSR) to '1'.
  //
  Status = UfsMmioWrite32 (Private, UFS_HC_UTRLRSR_OFFSET, UFS_HC_UTRLRSR);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Initialize the UFS host controller.

  @param[in] Private                 The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The Ufs Host Controller is initialized successfully.
  @retval Others                     A device error occurred while initializing the controller.

**/
EFI_STATUS
UfsControllerInit (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;

  Status = UfsEnableHostController (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UfsControllerInit: Enable Host Controller Fails, Status = %r\n", Status));
    return Status;
  }

  Status = UfsDeviceDetection (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UfsControllerInit: Device Detection Fails, Status = %r\n", Status));
    return Status;
  }

  Status = UfsInitTaskManagementRequestList (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UfsControllerInit: Task management list initialization Fails, Status = %r\n", Status));
    return Status;
  }

  Status = UfsInitTransferRequestList (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UfsControllerInit: Transfer list initialization Fails, Status = %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "UfsControllerInit Finished\n"));
  return EFI_SUCCESS;
}

/**
  Stop the UFS host controller.

  @param[in] Private                 The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The Ufs Host Controller is stopped successfully.
  @retval Others                     A device error occurred while stopping the controller.

**/
EFI_STATUS
UfsControllerStop (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;
  UINT32      Data;

  //
  // Enable the UTP Task Management Request List by setting the UTP Task Management
  // Request List RunStop Register (UTMRLRSR) to '1'.
  //
  Status = UfsMmioWrite32 (Private, UFS_HC_UTMRLRSR_OFFSET, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Enable the UTP Transfer Request List by setting the UTP Transfer Request List
  // RunStop Register (UTRLRSR) to '1'.
  //
  Status = UfsMmioWrite32 (Private, UFS_HC_UTRLRSR_OFFSET, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Write a 0 to the HCE register in order to disable the host controller.
  //
  Status = UfsMmioRead32 (Private, UFS_HC_ENABLE_OFFSET, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT ((Data & UFS_HC_HCE_EN) == UFS_HC_HCE_EN);

  Status = UfsMmioWrite32 (Private, UFS_HC_ENABLE_OFFSET, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Wait until HCE is read as '0' before continuing.
  //
  Status = UfsWaitMemSet (Private, UFS_HC_ENABLE_OFFSET, UFS_HC_HCE_EN, 0, UFS_TIMEOUT);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((DEBUG_INFO, "UfsController is stopped\n"));

  return EFI_SUCCESS;
}

/**
  Internal helper function which will signal the caller event and clean up
  resources.

  @param[in] Private   The pointer to the UFS_PASS_THRU_PRIVATE_DATA data
                       structure.
  @param[in] TransReq  The pointer to the UFS_PASS_THRU_TRANS_REQ data
                       structure.

**/
VOID
EFIAPI
SignalCallerEvent (
  IN UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN UFS_PASS_THRU_TRANS_REQ     *TransReq
  )
{
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *UfsHc;
  EFI_EVENT                           CallerEvent;

  ASSERT ((Private != NULL) && (TransReq != NULL));

  UfsHc       = Private->UfsHostController;
  CallerEvent = TransReq->CallerEvent;

  RemoveEntryList (&TransReq->TransferList);

  UfsHc->Flush (UfsHc);

  UfsStopExecCmd (Private, TransReq->Slot);

  UfsReconcileDataTransferBuffer (Private, TransReq);

  if (TransReq->CmdDescMapping != NULL) {
    UfsHc->Unmap (UfsHc, TransReq->CmdDescMapping);
  }

  if (TransReq->CmdDescHost != NULL) {
    UfsHc->FreeBuffer (
             UfsHc,
             EFI_SIZE_TO_PAGES (TransReq->CmdDescSize),
             TransReq->CmdDescHost
             );
  }

  FreePool (TransReq);

  gBS->SignalEvent (CallerEvent);
  return;
}

/**
  Call back function when the timer event is signaled.

  @param[in]  Event     The Event this notify function registered to.
  @param[in]  Context   Pointer to the context data registered to the Event.

**/
VOID
EFIAPI
ProcessAsyncTaskList (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UFS_PASS_THRU_PRIVATE_DATA                  *Private;
  LIST_ENTRY                                  *Entry;
  LIST_ENTRY                                  *NextEntry;
  UFS_PASS_THRU_TRANS_REQ                     *TransReq;
  EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet;
  UTP_RESPONSE_UPIU                           *Response;
  UINT16                                      SenseDataLen;
  UINT32                                      ResTranCount;
  UINT32                                      SlotsMap;
  UINT32                                      Value;
  EFI_STATUS                                  Status;

  Private  = (UFS_PASS_THRU_PRIVATE_DATA *)Context;
  SlotsMap = 0;

  //
  // Check the entries in the async I/O queue are done or not.
  //
  if (!IsListEmpty (&Private->Queue)) {
    BASE_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Private->Queue) {
      TransReq = UFS_PASS_THRU_TRANS_REQ_FROM_THIS (Entry);
      Packet   = TransReq->Packet;

      if ((SlotsMap & (BIT0 << TransReq->Slot)) != 0) {
        return;
      }

      SlotsMap |= BIT0 << TransReq->Slot;

      Status = UfsMmioRead32 (Private, UFS_HC_UTRLDBR_OFFSET, &Value);
      if (EFI_ERROR (Status)) {
        //
        // TODO: Should find/add a proper host adapter return status for this
        // case.
        //
        Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_PHASE_ERROR;
        DEBUG ((DEBUG_VERBOSE, "ProcessAsyncTaskList(): Signal Event %p UfsMmioRead32() Error.\n", TransReq->CallerEvent));
        SignalCallerEvent (Private, TransReq);
        continue;
      }

      if ((Value & (BIT0 << TransReq->Slot)) != 0) {
        //
        // Scsi cmd not finished yet.
        //
        if (TransReq->TimeoutRemain > UFS_HC_ASYNC_TIMER) {
          TransReq->TimeoutRemain -= UFS_HC_ASYNC_TIMER;
          continue;
        } else {
          //
          // Timeout occurs.
          //
          Packet->HostAdapterStatus = EFI_EXT_SCSI_STATUS_HOST_ADAPTER_TIMEOUT_COMMAND;
          DEBUG ((DEBUG_VERBOSE, "ProcessAsyncTaskList(): Signal Event %p EFI_TIMEOUT.\n", TransReq->CallerEvent));
          SignalCallerEvent (Private, TransReq);
          continue;
        }
      } else {
        //
        // Scsi cmd finished.
        //
        // Get sense data if exists
        //
        Response = (UTP_RESPONSE_UPIU *)((UINT8 *)TransReq->CmdDescHost + TransReq->Trd->RuO * sizeof (UINT32));
        ASSERT (Response != NULL);
        SenseDataLen = Response->SenseDataLen;
        SwapLittleEndianToBigEndian ((UINT8 *)&SenseDataLen, sizeof (UINT16));

        if ((Packet->SenseDataLength != 0) && (Packet->SenseData != NULL)) {
          //
          // Make sure the hardware device does not return more data than expected.
          //
          if (SenseDataLen <= Packet->SenseDataLength) {
            CopyMem (Packet->SenseData, Response->SenseData, SenseDataLen);
            Packet->SenseDataLength = (UINT8)SenseDataLen;
          } else {
            Packet->SenseDataLength = 0;
          }
        }

        //
        // Check the transfer request result.
        //
        Packet->TargetStatus = Response->Status;
        if (Response->Response != 0) {
          DEBUG ((DEBUG_VERBOSE, "ProcessAsyncTaskList(): Signal Event %p Target Failure.\n", TransReq->CallerEvent));
          SignalCallerEvent (Private, TransReq);
          continue;
        }

        if (TransReq->Trd->Ocs == 0) {
          if (Packet->DataDirection == EFI_EXT_SCSI_DATA_DIRECTION_READ) {
            if ((Response->Flags & BIT5) == BIT5) {
              ResTranCount = Response->ResTranCount;
              SwapLittleEndianToBigEndian ((UINT8 *)&ResTranCount, sizeof (UINT32));
              Packet->InTransferLength -= ResTranCount;
            }
          } else {
            if ((Response->Flags & BIT5) == BIT5) {
              ResTranCount = Response->ResTranCount;
              SwapLittleEndianToBigEndian ((UINT8 *)&ResTranCount, sizeof (UINT32));
              Packet->OutTransferLength -= ResTranCount;
            }
          }
        } else {
          DEBUG ((DEBUG_VERBOSE, "ProcessAsyncTaskList(): Signal Event %p Target Device Error.\n", TransReq->CallerEvent));
          SignalCallerEvent (Private, TransReq);
          continue;
        }

        DEBUG ((DEBUG_VERBOSE, "ProcessAsyncTaskList(): Signal Event %p Success.\n", TransReq->CallerEvent));
        SignalCallerEvent (Private, TransReq);
      }
    }
  }
}

/**
  Execute UIC command.

  @param[in]      This        Pointer to driver interface produced by the UFS controller.
  @param[in, out] UicCommand  Descriptor of the command that will be executed.

  @retval EFI_SUCCESS            Command executed successfully.
  @retval EFI_INVALID_PARAMETER  This or UicCommand is NULL.
  @retval Others                 Command failed to execute.
**/
EFI_STATUS
EFIAPI
UfsHcDriverInterfaceExecUicCommand (
  IN     EDKII_UFS_HC_DRIVER_INTERFACE  *This,
  IN OUT EDKII_UIC_COMMAND              *UicCommand
  )
{
  UFS_PASS_THRU_PRIVATE_DATA  *Private;

  if ((This == NULL) || (UicCommand == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = UFS_PASS_THRU_PRIVATE_DATA_FROM_DRIVER_INTF (This);

  return UfsExecUicCommands (Private, UicCommand);
}

/**
  Initializes UfsHcInfo field in private data.

  @param[in] Private  Pointer to host controller private data.

  @retval EFI_SUCCESS  UfsHcInfo initialized successfully.
  @retval Others       Failed to initalize UfsHcInfo.
**/
EFI_STATUS
GetUfsHcInfo (
  IN UFS_PASS_THRU_PRIVATE_DATA  *Private
  )
{
  UINT32      Data;
  EFI_STATUS  Status;

  Status = UfsMmioRead32 (Private, UFS_HC_VER_OFFSET, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private->UfsHcInfo.Version = Data;

  Status = UfsMmioRead32 (Private, UFS_HC_CAP_OFFSET, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private->UfsHcInfo.Capabilities = Data;

  if ((mUfsHcPlatform != NULL) && (mUfsHcPlatform->OverrideHcInfo != NULL)) {
    Status = mUfsHcPlatform->OverrideHcInfo (Private->Handle, &Private->UfsHcInfo);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failure from platform on OverrideHcInfo, Status = %r\n", Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}
