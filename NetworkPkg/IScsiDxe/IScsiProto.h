/** @file
  The header file of iSCSI Protocol that defines many specific data structures.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ISCSI_PROTO_H_
#define _ISCSI_PROTO_H_

//
// RFC 1982 Serial Number Arithmetic, SERIAL_BITS = 32
//
#define ISCSI_SEQ_EQ(s1, s2)  ((s1) == (s2))
#define ISCSI_SEQ_LT(s1, s2) \
    ( \
      (((INT32) (s1) < (INT32) (s2)) && (s2 - s1) < ((UINT32) 1 << 31)) || \
      (((INT32) (s1) > (INT32) (s2)) && (s1 - s2) > ((UINT32) 1 << 31)) \
    )
#define ISCSI_SEQ_GT(s1, s2) \
    ( \
      (((INT32) (s1) < (INT32) (s2)) && (s2 - s1) > ((UINT32) 1 << 31)) || \
      (((INT32) (s1) > (INT32) (s2)) && (s1 - s2) < ((UINT32) 1 << 31)) \
    )

#define ISCSI_WELL_KNOWN_PORT                   3260
#define ISCSI_MAX_CONNS_PER_SESSION             1

#define DEFAULT_MAX_RECV_DATA_SEG_LEN           8192
#define MAX_RECV_DATA_SEG_LEN_IN_FFP            65536
#define DEFAULT_MAX_OUTSTANDING_R2T             1

#define ISCSI_VERSION_MAX                       0x00
#define ISCSI_VERSION_MIN                       0x00

#define ISCSI_CHECK_MEDIA_LOGIN_WAITING_TIME       EFI_TIMER_PERIOD_SECONDS(20)
#define ISCSI_CHECK_MEDIA_GET_DHCP_WAITING_TIME    EFI_TIMER_PERIOD_SECONDS(20)

#define ISCSI_REDIRECT_ADDR_START_DELIMITER     '['
#define ISCSI_REDIRECT_ADDR_END_DELIMITER       ']'

#define ISCSI_KEY_AUTH_METHOD                   "AuthMethod"
#define ISCSI_KEY_HEADER_DIGEST                 "HeaderDigest"
#define ISCSI_KEY_DATA_DIGEST                   "DataDigest"
#define ISCSI_KEY_MAX_CONNECTIONS               "MaxConnections"
#define ISCSI_KEY_TARGET_NAME                   "TargetName"
#define ISCSI_KEY_INITIATOR_NAME                "InitiatorName"
#define ISCSI_KEY_TARGET_ALIAS                  "TargetAlias"
#define ISCSI_KEY_INITIATOR_ALIAS               "InitiatorAlias"
#define ISCSI_KEY_TARGET_ADDRESS                "TargetAddress"
#define ISCSI_KEY_INITIAL_R2T                   "InitialR2T"
#define ISCSI_KEY_IMMEDIATE_DATA                "ImmediateData"
#define ISCSI_KEY_TARGET_PORTAL_GROUP_TAG       "TargetPortalGroupTag"
#define ISCSI_KEY_MAX_BURST_LENGTH              "MaxBurstLength"
#define ISCSI_KEY_FIRST_BURST_LENGTH            "FirstBurstLength"
#define ISCSI_KEY_DEFAULT_TIME2WAIT             "DefaultTime2Wait"
#define ISCSI_KEY_DEFAULT_TIME2RETAIN           "DefaultTime2Retain"
#define ISCSI_KEY_MAX_OUTSTANDING_R2T           "MaxOutstandingR2T"
#define ISCSI_KEY_DATA_PDU_IN_ORDER             "DataPDUInOrder"
#define ISCSI_KEY_DATA_SEQUENCE_IN_ORDER        "DataSequenceInOrder"
#define ISCSI_KEY_ERROR_RECOVERY_LEVEL          "ErrorRecoveryLevel"
#define ISCSI_KEY_SESSION_TYPE                  "SessionType"
#define ISCSI_KEY_MAX_RECV_DATA_SEGMENT_LENGTH  "MaxRecvDataSegmentLength"

#define ISCSI_KEY_VALUE_NONE                    "None"

///
/// connection state for initiator
///

#define CONN_STATE_FREE                         0
#define CONN_STATE_XPT_WAIT                     1
#define CONN_STATE_IN_LOGIN                     2
#define CONN_STATE_LOGGED_IN                    3
#define CONN_STATE_IN_LOGOUT                    4
#define CONN_STATE_LOGOUT_REQUESTED             5
#define CONN_STATE_CLEANUP_WAIT                 6
#define CONN_STATE_IN_CLEANUP                   7

///
/// session state for initiator
///
#define SESSION_STATE_FREE                      0
#define SESSION_STATE_LOGGED_IN                 1
#define SESSION_STATE_FAILED                    2

#define ISCSI_RESERVED_TAG                      0xffffffff

#define ISCSI_REQ_IMMEDIATE                     0x40
#define ISCSI_OPCODE_MASK                       0x3F

#define ISCSI_SET_OPCODE(PduHdr, Op, Flgs)  ((((ISCSI_BASIC_HEADER *) (PduHdr))->OpCode) = ((Op) | (Flgs)))
#define ISCSI_GET_OPCODE(PduHdr)            ((((ISCSI_BASIC_HEADER *) (PduHdr))->OpCode) & ISCSI_OPCODE_MASK)
#define ISCSI_CHECK_OPCODE(PduHdr, Op)      ((((PduHdr)->OpCode) & ISCSI_OPCODE_MASK) == (Op))
#define ISCSI_IMMEDIATE_ON(PduHdr)          ((PduHdr)->OpCode & ISCSI_REQ_IMMEDIATE)
#define ISCSI_SET_FLAG(PduHdr, Flag)        (((ISCSI_BASIC_HEADER *) (PduHdr))->Flags |= (BOOLEAN)(Flag))
#define ISCSI_CLEAR_FLAG(PduHdr, Flag)      (((ISCSI_BASIC_HEADER *) (PduHdr))->Flags &= ~(Flag))
#define ISCSI_FLAG_ON(PduHdr, Flag)         ((BOOLEAN) ((((ISCSI_BASIC_HEADER *) (PduHdr))->Flags & (Flag)) == (Flag)))
#define ISCSI_SET_STAGES(PduHdr, Cur, Nxt)  ((PduHdr)->Flags = (UINT8) ((PduHdr)->Flags | ((Cur) << 2 | (Nxt))))
#define ISCSI_GET_CURRENT_STAGE(PduHdr)     ((UINT8) (((PduHdr)->Flags >> 2) & 0x3))
#define ISCSI_GET_NEXT_STAGE(PduHdr)        ((UINT8) (((PduHdr)->Flags) & 0x3))

#define ISCSI_GET_PAD_LEN(DataLen)          ((~(DataLen) + 1) & 0x3)
#define ISCSI_ROUNDUP(DataLen)              (((DataLen) + 3) &~(0x3))

#define HTON24(Dst, Src) \
  do { \
    (Dst)[0]  = (UINT8) ((UINT8) ((Src) >> 16) & 0xFF); \
    (Dst)[1]  = (UINT8) ((UINT8) ((Src) >> 8) & 0xFF); \
    (Dst)[2]  = (UINT8) ((UINT8) (Src) & 0xFF); \
  } while (0);

#define NTOH24(src)                         (((src)[0] << 16) | ((src)[1] << 8) | ((src)[2]))

#define ISCSI_GET_DATASEG_LEN(PduHdr)       NTOH24 (((ISCSI_BASIC_HEADER *) (PduHdr))->DataSegmentLength)
#define ISCSI_SET_DATASEG_LEN(PduHdr, Len)  HTON24 (((ISCSI_BASIC_HEADER *) (PduHdr))->DataSegmentLength, (Len))
#define ISCSI_GET_BUFFER_OFFSET(PduHdr)     NTOHL  (((ISCSI_SCSI_DATA_IN *) (PduHdr))->BufferOffset)

//
// Initiator opcodes.
//
#define ISCSI_OPCODE_NOP_OUT        0x00
#define ISCSI_OPCODE_SCSI_CMD       0x01
#define ISCSI_OPCODE_SCSI_TMF_REQ   0x02
#define ISCSI_OPCODE_LOGIN_REQ      0x03
#define ISCSI_OPCODE_TEXT_REQ       0x04
#define ISCSI_OPCODE_SCSI_DATA_OUT  0x05
#define ISCSI_OPCODE_LOGOUT_REQ     0x06
#define ISCSI_OPCODE_SNACK_REQ      0x10
#define ISCSI_OPCODE_VENDOR_I0      0x1c
#define ISCSI_OPCODE_VENDOR_I1      0x1d
#define ISCSI_OPCODE_VENDOR_I2      0x1e

//
// Target opcodes.
//
#define ISCSI_OPCODE_NOP_IN       0x20
#define ISCSI_OPCODE_SCSI_RSP     0x21
#define ISCSI_OPCODE_SCSI_TMF_RSP 0x22
#define ISCSI_OPCODE_LOGIN_RSP    0x23
#define ISCSI_OPCODE_TEXT_RSP     0x24
#define ISCSI_OPCODE_SCSI_DATA_IN 0x25
#define ISCSI_OPCODE_LOGOUT_RSP   0x26
#define ISCSI_OPCODE_R2T          0x31
#define ISCSI_OPCODE_ASYNC_MSG    0x32
#define ISCSI_OPCODE_VENDOR_T0    0x3c
#define ISCSI_OPCODE_VENDOR_T1    0x3d
#define ISCSI_OPCODE_VENDOR_T2    0x3e
#define ISCSI_OPCODE_REJECT       0x3f

#define ISCSI_BHS_FLAG_FINAL      0x80

//
// Defined AHS types, others are reserved.
//
#define ISCSI_AHS_TYPE_EXT_CDB              0x1
#define ISCSI_AHS_TYPE_BI_EXP_READ_DATA_LEN 0x2

#define SCSI_CMD_PDU_FLAG_READ        0x40
#define SCSI_CMD_PDU_FLAG_WRITE       0x20

#define ISCSI_CMD_PDU_TASK_ATTR_MASK  0x07

//
// task attributes
//
#define ISCSI_TASK_ATTR_UNTAGGED  0x00
#define ISCSI_TASK_ATTR_SIMPLE    0x01
#define ISCSI_TASK_ATTR_ORDERD    0x02
#define ISCSI_TASK_ATTR_HOQ       0x03
#define ISCSI_TASK_ATTR_ACA       0x04

//
// Flag bit definitions in SCSI response.
//
#define SCSI_RSP_PDU_FLAG_BI_READ_OVERFLOW  0x10
#define SCSI_RSP_PDU_FLAG_BI_READ_UNDERFLOW 0x08
#define SCSI_RSP_PDU_FLAG_OVERFLOW          0x04
#define SCSI_RSP_PDU_FLAG_UNDERFLOW         0x02

//
// iSCSI service response codes.
//
#define ISCSI_SERVICE_RSP_COMMAND_COMPLETE_AT_TARGET  0x00
#define ISCSI_SERVICE_RSP_TARGET_FAILURE              0x01

#define ISCSI_TMF_RSP_PDU_RSP_FUNCTION_COMPLETE           0
#define ISCSI_TMF_RSP_PDU_RSP_TASK_NOT_EXIST              1
#define ISCSI_TMF_RSP_PDU_RSP_LUN_NOT_EXIST               2
#define ISCSI_TMF_RSP_PDU_RSP_TASK_STILL_ALLEGIANT        3
#define ISCSI_TMF_RSP_PDU_RSP_TASK_REASSGIN_NOT_SUPPORTED 4
#define ISCSI_TMF_RSP_PDU_RSP_NOT_SUPPORTED               5
#define ISCSI_TMF_RSP_PDU_RSP_FUNCTION_AHTH_FAILED        6
#define ISCSI_TMF_RSP_PDU_RSP_FUNCTION_REJECTED           255

#define SCSI_DATA_IN_PDU_FLAG_ACKKNOWLEDGE  0x40
#define SCSI_DATA_IN_PDU_FLAG_OVERFLOW      SCSI_RSP_PDU_FLAG_OVERFLOW
#define SCSI_DATA_IN_PDU_FLAG_UNDERFLOW     SCSI_RSP_PDU_FLAG_UNDERFLOW
#define SCSI_DATA_IN_PDU_FLAG_STATUS_VALID  0x01

#define ISCSI_LOGIN_REQ_PDU_FLAG_TRANSIT  0x80
#define ISCSI_LOGIN_REQ_PDU_FLAG_CONTINUE 0x40

#define ISCSI_LOGIN_RSP_PDU_FLAG_TRANSIT    ISCSI_LOGIN_REQ_PDU_FLAG_TRANSIT
#define ISCSI_LOGIN_RSP_PDU_FLAG_CONTINUE   ISCSI_LOGIN_REQ_PDU_FLAG_CONTINUE

#define ISCSI_LOGIN_STATUS_SUCCESS          0
#define ISCSI_LOGIN_STATUS_REDIRECTION      1
#define ISCSI_LOGIN_STATUS_INITIATOR_ERROR  2
#define ISCSI_LOGIN_STATUS_TARGET_ERROR     3

#define ISCSI_LOGOUT_REASON_CLOSE_SESSION                   0
#define ISCSI_LOGOUT_REASON_CLOSE_CONNECTION                1
#define ISCSI_LOGOUT_REASON_REMOVE_CONNECTION_FOR_RECOVERY  2

#define ISCSI_LOGOUT_RESPONSE_SESSION_CLOSED_SUCCESS  0
#define ISCSI_LOGOUT_RESPONSE_CID_NOT_FOUND           1
#define ISCSI_LOGOUT_RESPONSE_RECOVERY_NOT_SUPPORTED  2
#define ISCSI_LOGOUT_RESPONSE_CLEANUP_FAILED          3

#define ISCSI_SNACK_REQUEST_TYPE_DATA_OR_R2T  0
#define ISCSI_SNACK_REQUEST_TYPE_STATUS       1
#define ISCSI_SNACK_REQUEST_TYPE_DATA_ACK     2
#define ISCSI_SNACK_REQUEST_TYPE_RDATA        3

#define ISCSI_SECURITY_NEGOTIATION          0
#define ISCSI_LOGIN_OPERATIONAL_NEGOTIATION 1
#define ISCSI_FULL_FEATURE_PHASE            3

typedef struct _ISCSI_SESSION               ISCSI_SESSION;
typedef struct _ISCSI_CONNECTION            ISCSI_CONNECTION;

typedef enum {
  DataIn  = 0,
  DataOut = 1,
  DataBi  = 2
} DATA_DIRECTION;

///
/// iSCSI Basic Header Segment
///
typedef struct _ISCSI_BASIC_HEADER {
  UINT8   OpCode;
  UINT8   Flags;
  UINT16  OpCodeSpecific1;
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Lun[8];
  UINT32  InitiatorTaskTag;
  UINT32  OpCodeSpecific2[7];
} ISCSI_BASIC_HEADER;

typedef struct _ISCSI_ADDTIONAL_HEADER {
  UINT16  Length;
  UINT8   Type;
  UINT8   TypeSpecific[1];
} ISCSI_ADDITIONAL_HEADER;

typedef struct _ISCSI_BI_EXP_READ_DATA_LEN_AHS {
  UINT16  Length;
  UINT8   Type;
  UINT8   Reserved;
  UINT32  ExpReadDataLength;
} ISCSI_BI_EXP_READ_DATA_LEN_AHS;

///
/// SCSI Command
///
typedef struct _SCSI_COMMAND {
  UINT8   OpCode;
  UINT8   Flags;
  UINT16  Reserved;
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Lun[8];
  UINT32  InitiatorTaskTag;
  UINT32  ExpDataXferLength;
  UINT32  CmdSN;
  UINT32  ExpStatSN;
  UINT8   Cdb[16];
} SCSI_COMMAND;

///
/// SCSI Response
///
typedef struct _SCSI_RESPONSE {
  UINT8   OpCode;
  UINT8   Flags;
  UINT8   Response;
  UINT8   Status;
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Reserved[8];
  UINT32  InitiatorTaskTag;
  UINT32  SNACKTag;
  UINT32  StatSN;
  UINT32  ExpCmdSN;
  UINT32  MaxCmdSN;
  UINT32  ExpDataSN;
  UINT32  BiReadResidualCount;
  UINT32  ResidualCount;
} SCSI_RESPONSE;

typedef struct _ISCSI_SENSE_DATA {
  UINT16  Length;
  UINT8   Data[2];
} ISCSI_SENSE_DATA;

///
/// iSCSI Task Managment Function Request.
///
typedef struct _ISCSI_TMF_REQUEST {
  UINT8   OpCode;
  UINT8   Fuction;
  UINT16  Reserved1;
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Lun[8];
  UINT32  InitiatorTaskTag;
  UINT32  ReferencedTaskTag;
  UINT32  CmdSN;
  UINT32  ExpStatSN;
  UINT32  RefCmdSN;
  UINT32  ExpDataSN;
  UINT32  Reserved2[2];
} ISCSI_TMF_REQUEST;

///
/// iSCSI Task Management Function Response.
///
typedef struct _ISCSI_TMF_RESPONSE {
  UINT8   OpCode;
  UINT8   Reserved1;
  UINT8   Response;
  UINT8   Reserved2;
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT32  Reserver3[2];
  UINT32  InitiatorTaskTag;
  UINT32  Reserved4;
  UINT32  StatSN;
  UINT32  ExpCmdSN;
  UINT32  MaxCmdSN;
  UINT32  Reserved[3];
} ISCSI_TMF_RESPONSE;

///
/// SCSI Data-Out
///
typedef struct _ISCSI_SCSI_DATA_OUT {
  UINT8   OpCode;
  UINT8   Reserved1[3];
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Lun[8];
  UINT32  InitiatorTaskTag;
  UINT32  TargetTransferTag;
  UINT32  Reserved2;
  UINT32  ExpStatSN;
  UINT32  Reserved3;
  UINT32  DataSN;
  UINT32  BufferOffset;
  UINT32  Reserved4;
} ISCSI_SCSI_DATA_OUT;

///
/// SCSI Data-In
///
typedef struct _ISCSI_SCSI_DATA_IN {
  UINT8   OpCode;
  UINT8   Flags;
  UINT8   Reserved1;
  UINT8   Status;
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Lun[8];
  UINT32  InitiatorTaskTag;
  UINT32  TargetTransferTag;
  UINT32  StatSN;
  UINT32  ExpCmdSN;
  UINT32  MaxCmdSN;
  UINT32  DataSN;
  UINT32  BufferOffset;
  UINT32  ResidualCount;
} ISCSI_SCSI_DATA_IN;

///
/// Ready To Transfer.
///
typedef struct _ISCSI_READY_TO_TRANSFER {
  UINT8   OpCode;
  UINT8   Reserved1[3];
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Lun[8];
  UINT32  InitiatorTaskTag;
  UINT32  TargetTransferTag;
  UINT32  StatSN;
  UINT32  ExpCmdSN;
  UINT32  MaxCmdSN;
  UINT32  R2TSeqNum;
  UINT32  BufferOffset;
  UINT32  DesiredDataTransferLength;
} ISCSI_READY_TO_TRANSFER;

typedef struct _ISCSI_ASYNC_MESSAGE {
  UINT8   OpCode;
  UINT8   Reserved1[8];
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Lun[8];
  UINT32  InitiatorTaskTag;
  UINT32  Reserved2;
  UINT32  StatSN;
  UINT32  ExpCmdSN;
  UINT32  MaxCmdSN;
  UINT8   AsyncEvent;
  UINT8   AsyncVCode;
  UINT16  Parameter1;
  UINT16  Parameter2;
  UINT16  Parameter3;
  UINT32  Reserved3;
} ISCSI_ASYNC_MESSAGE;

///
/// Login Request.
///
typedef struct _ISCSI_LOGIN_REQUEST {
  UINT8   OpCode;
  UINT8   Flags;
  UINT8   VersionMax;
  UINT8   VersionMin;
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Isid[6];
  UINT16  Tsih;
  UINT32  InitiatorTaskTag;
  UINT16  Cid;
  UINT16  Reserved1;
  UINT32  CmdSN;
  UINT32  ExpStatSN;
  UINT32  Reserved2[4];
} ISCSI_LOGIN_REQUEST;

///
/// Login Response.
///
typedef struct _ISCSI_LOGIN_RESPONSE {
  UINT8   OpCode;
  UINT8   Flags;
  UINT8   VersionMax;
  UINT8   VersionActive;
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Isid[6];
  UINT16  Tsih;
  UINT32  InitiatorTaskTag;
  UINT32  Reserved1;
  UINT32  StatSN;
  UINT32  ExpCmdSN;
  UINT32  MaxCmdSN;
  UINT8   StatusClass;
  UINT8   StatusDetail;
  UINT8   Reserved2[10];
} ISCSI_LOGIN_RESPONSE;

///
/// Logout Request.
///
typedef struct _ISCSI_LOGOUT_REQUEST {
  UINT8   OpCode;
  UINT8   ReasonCode;
  UINT16  Reserved1;
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT32  Reserved2[2];
  UINT32  InitiatorTaskTag;
  UINT16  Cid;
  UINT16  Reserved3;
  UINT32  CmdSN;
  UINT32  ExpStatSN;
  UINT32  Reserved4[4];
} ISCSI_LOGOUT_REQUEST;

///
/// Logout Response.
///
typedef struct _ISCSI_LOGOUT_RESPONSE {
  UINT8   OpCode;
  UINT8   Reserved1;
  UINT8   Response;
  UINT8   Reserved2;
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT32  Reserved3[2];
  UINT32  InitiatorTaskTag;
  UINT32  Reserved4;
  UINT32  StatSN;
  UINT32  ExpCmdSN;
  UINT32  MaxCmdSN;
  UINT32  Reserved5;
  UINT16  Time2Wait;
  UINT16  Time2Retain;
  UINT32  Reserved6;
} ISCSI_LOGOUT_RESPONSE;

///
/// SNACK Request.
///
typedef struct _ISCSI_SNACK_REQUEST {
  UINT8   OpCode;
  UINT8   Type;
  UINT16  Reserved1;
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Lun[8];
  UINT32  InitiatorTaskTag;
  UINT32  TargetTransferTag;
  UINT32  Reserved2;
  UINT32  ExpStatSN;
  UINT32  Reserved[2];
  UINT32  BegRun;
  UINT32  RunLength;
} ISCSI_SNACK_REQUEST;

///
/// Reject.
///
typedef struct _ISCSI_REJECT {
  UINT8   OpCode;
  UINT8   Reserved1;
  UINT8   Reason;
  UINT8   Reserved2;
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT32  Reserved3[2];
  UINT32  InitiatorTaskTag;
  UINT32  Reserved4;
  UINT32  StatSN;
  UINT32  ExpCmdSN;
  UINT32  MaxCmdSN;
  UINT32  DataSN;
  UINT32  Reserved5[2];
} ISCSI_REJECT;

///
/// NOP-Out.
///
typedef struct _ISCSI_NOP_OUT {
  UINT8   OpCode;
  UINT8   Reserved1[3];
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Lun[8];
  UINT32  InitiatorTaskTag;
  UINT32  TargetTransferTag;
  UINT32  CmdSN;
  UINT32  ExpStatSN;
  UINT32  Reserved2[4];
} ISCSI_NOP_OUT;

///
/// NOP-In.
///
typedef struct _ISCSI_NOP_IN {
  UINT8   OpCode;
  UINT8   Reserved1[3];
  UINT8   TotalAHSLength;
  UINT8   DataSegmentLength[3];
  UINT8   Lun[8];
  UINT32  InitiatorTaskTag;
  UINT32  TargetTransferTag;
  UINT32  StatSN;
  UINT32  ExpCmdSN;
  UINT32  MaxCmdSN;
  UINT32  Reserved2[3];
} ISCSI_NOP_IN;

typedef enum {
  IScsiDigestNone,
  IScsiDigestCRC32
} ISCSI_DIGEST_TYPE;

typedef struct _ISCSI_XFER_CONTEXT {
  UINT32  TargetTransferTag;
  UINT32  Offset;
  UINT32  DesiredLength;
  UINT32  ExpDataSN;
} ISCSI_XFER_CONTEXT;

typedef struct _ISCSI_IN_BUFFER_CONTEXT {
  UINT8   *InData;
  UINT32  InDataLen;
} ISCSI_IN_BUFFER_CONTEXT;

typedef struct _ISCSI_TCB {
  LIST_ENTRY          Link;

  BOOLEAN             SoFarInOrder;
  UINT32              ExpDataSN;
  BOOLEAN             FbitReceived;
  BOOLEAN             StatusXferd;
  UINT32              ActiveR2Ts;
  UINT32              Response;
  CHAR8               *Reason;
  UINT32              InitiatorTaskTag;
  UINT32              CmdSN;
  UINT32              SNACKTag;

  ISCSI_XFER_CONTEXT  XferContext;

  ISCSI_CONNECTION    *Conn;
} ISCSI_TCB;

typedef struct _ISCSI_KEY_VALUE_PAIR {
  LIST_ENTRY      List;

  CHAR8           *Key;
  CHAR8           *Value;
} ISCSI_KEY_VALUE_PAIR;

/**
  Attach the iSCSI connection to the iSCSI session.

  @param[in, out]  Session The iSCSI session.
  @param[in, out]  Conn    The iSCSI connection.

**/
VOID
IScsiAttatchConnection (
  IN OUT ISCSI_SESSION     *Session,
  IN OUT ISCSI_CONNECTION  *Conn
  );

/**
  Detach the iSCSI connection from the session it belongs to.

  @param[in, out]  Conn The iSCSI connection.

**/
VOID
IScsiDetatchConnection (
  IN OUT ISCSI_CONNECTION  *Conn
  );

/**
  This function performs the iSCSI connection login.

  @param[in, out]  Conn      The iSCSI connection to login.
  @param           Timeout   The timeout value in milliseconds.

  @retval EFI_SUCCESS        The iSCSI connection is logged into the iSCSI target.
  @retval EFI_TIMEOUT        Timeout occurred during the login procedure.
  @retval Others             Other errors as indicated.

**/
EFI_STATUS
IScsiConnLogin (
  IN OUT ISCSI_CONNECTION    *Conn,
  IN     UINT16              Timeout
  );

/**
  Create a TCP connection for the iSCSI session.

  @param[in]  Session Points to the iSCSI session.

  @return The newly created iSCSI connection.

**/
ISCSI_CONNECTION *
IScsiCreateConnection (
  IN ISCSI_SESSION      *Session
  );

/**
  Destroy an iSCSI connection.

  @param[in]  Conn The connection to destroy.

**/
VOID
IScsiDestroyConnection (
  IN ISCSI_CONNECTION  *Conn
  );

/**
  Login the iSCSI session.

  @param[in]  Session           The iSCSI session

  @retval EFI_SUCCESS           The iSCSI session login procedure finished.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
  @retval EFI_NO_MEDIA          There was a media error.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
IScsiSessionLogin (
  IN ISCSI_SESSION  *Session
  );

/**
  Wait for IPsec negotiation, then try to login the iSCSI session again.

  @param[in]  Session           The iSCSI session

  @retval EFI_SUCCESS           The iSCSI session login procedure finished.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.
  @retval EFI_PROTOCOL_ERROR    Some kind of iSCSI protocol error happened.

**/
EFI_STATUS
IScsiSessionReLogin (
  IN ISCSI_SESSION  *Session
  );

/**
  Build and send the iSCSI login request to the iSCSI target according to
  the current login stage.

  @param[in]  Conn             The connection in the iSCSI login phase.

  @retval EFI_SUCCESS          The iSCSI login request PDU is built and sent on this
                               connection.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.
  @retval EFI_DEVICE_ERROR     Some kind of device error happened.

**/
EFI_STATUS
IScsiSendLoginReq (
  IN ISCSI_CONNECTION  *Conn
  );

/**
  Receive and process the iSCSI login response.

  @param[in]  Conn             The connection in the iSCSI login phase.

  @retval EFI_SUCCESS          The iSCSI login response PDU is received and processed.
  @retval Others               Other errors as indicated.

**/
EFI_STATUS
IScsiReceiveLoginRsp (
  IN ISCSI_CONNECTION  *Conn
  );

/**
  Add an iSCSI key-value pair as a string into the data segment of the Login Request PDU.
  The DataSegmentLength and the actual size of the net buffer containing this PDU will be
  updated.

  @param[in, out]  Pdu         The iSCSI PDU whose data segment the key-value pair will
                               be added to.
  @param[in]       Key         The key name string.
  @param[in]       Value       The value string.

  @retval EFI_SUCCESS          The key-valu pair is added to the PDU's datasegment and
                               the correspondence length fields are updated.
  @retval EFI_OUT_OF_RESOURCES There is not enough space in the PDU to add the key-value
                               pair.
**/
EFI_STATUS
IScsiAddKeyValuePair (
  IN OUT NET_BUF      *Pdu,
  IN CHAR8            *Key,
  IN CHAR8            *Value
  );

/**
  Prepare the iSCSI login request to be sent according to the current login status.

  @param[in, out]  Conn The connection in the iSCSI login phase.

  @return The pointer to the net buffer containing the iSCSI login request built.
  @retval NULL     Other errors as indicated.

**/
NET_BUF *
IScsiPrepareLoginReq (
  IN OUT ISCSI_CONNECTION  *Conn
  );

/**
  Process the iSCSI Login Response.

  @param[in, out]  Conn The connection on which the iSCSI login response is received.
  @param[in, out]  Pdu  The iSCSI login response PDU.

  @retval EFI_SUCCESS        The iSCSI login response PDU is processed and all check are passed.
  @retval EFI_PROTOCOL_ERROR Some kind of iSCSI protocol error happened.
  @retval EFI_MEDIA_CHANGED  Target is redirected.
  @retval Others             Other errors as indicated.

**/
EFI_STATUS
IScsiProcessLoginRsp (
  IN OUT ISCSI_CONNECTION  *Conn,
  IN OUT NET_BUF           *Pdu
  );

/**
  Updated the target information according the data received in the iSCSI
  login response with an target redirection status.

  @param[in, out] Session      The iSCSI session.
  @param[in]      Data         The data segment which should contain the
                               TargetAddress key-value list.
  @param[in]      Len          Length of the data.

  @retval EFI_SUCCESS          The target address is updated.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.
  @retval EFI_NOT_FOUND        The TargetAddress key is not found.
  @retval Others               Other errors as indicated.

**/
EFI_STATUS
IScsiUpdateTargetAddress (
  IN OUT ISCSI_SESSION         *Session,
  IN     CHAR8                 *Data,
  IN     UINT32                Len
  );

/**
  The callback function to free the net buffer list.

  @param[in]  Arg The opaque parameter.

**/
VOID
EFIAPI
IScsiFreeNbufList (
  VOID *Arg
  );

/**
  Receive an iSCSI response PDU. An iSCSI response PDU contains an iSCSI PDU header and
  an optional data segment. The two parts will be put into two blocks of buffers in the
  net buffer. The digest check will be conducted in this function if needed and the digests
  will be trimmed from the PDU buffer.

  @param[in]  Conn         The iSCSI connection to receive data from.
  @param[out] Pdu          The received iSCSI pdu.
  @param[in]  Context      The context used to describe information on the caller provided
                           buffer to receive data segment of the iSCSI pdu, it's optional.
  @param[in]  HeaderDigest Whether there will be header digest received.
  @param[in]  DataDigest   Whether there will be data digest.
  @param[in]  TimeoutEvent The timeout event, it's optional.

  @retval EFI_SUCCESS          An iSCSI pdu is received.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.
  @retval EFI_PROTOCOL_ERROR   Some kind of iSCSI protocol error occurred.
  @retval Others               Other errors as indicated.

**/
EFI_STATUS
IScsiReceivePdu (
  IN ISCSI_CONNECTION                      *Conn,
  OUT NET_BUF                              **Pdu,
  IN ISCSI_IN_BUFFER_CONTEXT               *Context, OPTIONAL
  IN BOOLEAN                               HeaderDigest,
  IN BOOLEAN                               DataDigest,
  IN EFI_EVENT                             TimeoutEvent OPTIONAL
  );

/**
  Check and get the result of the parameter negotiation.

  @param[in, out]  Conn          The connection in iSCSI login.

  @retval EFI_SUCCESS          The parmeter check is passed and negotiation is finished.
  @retval EFI_PROTOCOL_ERROR   Some kind of iSCSI protocol error occurred.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.

**/
EFI_STATUS
IScsiCheckOpParams (
  IN OUT ISCSI_CONNECTION  *Conn
  );

/**
  Fill the operational parameters.

  @param[in]       Conn    The connection in iSCSI login.
  @param[in, out]  Pdu     The iSCSI login request PDU to fill the parameters.

**/
VOID
IScsiFillOpParams (
  IN     ISCSI_CONNECTION  *Conn,
  IN OUT NET_BUF           *Pdu
  );

/**
  Pad the iSCSI AHS or data segment to an integer number of 4 byte words.

  @param[in, out]  Pdu         The iSCSI pdu which contains segments to pad.
  @param[in]       Len         The length of the last semgnet in the PDU.

  @retval EFI_SUCCESS          The segment is padded or no need to pad it.
  @retval EFI_OUT_OF_RESOURCES There is not enough remaining free space to add the
                               padding bytes.
**/
EFI_STATUS
IScsiPadSegment (
  IN OUT NET_BUF      *Pdu,
  IN     UINT32       Len
  );

/**
  Build a key-value list from the data segment.

  @param[in]  Data The data segment containing the key-value pairs.
  @param[in]  Len  Length of the data segment.

  @return The key-value list.
  @retval NULL Other errors as indicated.

**/
LIST_ENTRY *
IScsiBuildKeyValueList (
  IN CHAR8  *Data,
  IN UINT32 Len
  );

/**
  Get the value string by the key name from the key-value list. If found,
  the key-value entry will be removed from the list.

  @param[in, out]  KeyValueList  The key-value list.
  @param[in]       Key           The key name to find.

  @return The value string.
  @retval NULL The key value pair can not be found.

**/
CHAR8 *
IScsiGetValueByKeyFromList (
  IN OUT LIST_ENTRY     *KeyValueList,
  IN     CHAR8          *Key
  );

/**
  Free the key-value list.

  @param[in]  KeyValueList The key-value list.

**/
VOID
IScsiFreeKeyValueList (
  IN LIST_ENTRY      *KeyValueList
  );

/**
  Normalize the iSCSI name according to RFC.

  @param[in, out]  Name       The iSCSI name.
  @param[in]       Len        length of the iSCSI name.

  @retval EFI_SUCCESS        The iSCSI name is valid and normalized.
  @retval EFI_PROTOCOL_ERROR The iSCSI name is mal-formatted or not in the IQN format.

**/
EFI_STATUS
IScsiNormalizeName (
  IN OUT CHAR8      *Name,
  IN     UINTN      Len
  );

/**
  Execute the SCSI command issued through the EXT SCSI PASS THRU protocol.

  @param[in]       PassThru  The EXT SCSI PASS THRU protocol.
  @param[in]       Target    The target ID.
  @param[in]       Lun       The LUN.
  @param[in, out]  Packet    The request packet containing IO request, SCSI command
                             buffer and buffers to read/write.

  @retval EFI_SUCCES           The SCSI command is executed and the result is updated to
                               the Packet.
  @retval EFI_DEVICE_ERROR     Session state was not as required.
  @retval EFI_OUT_OF_RESOURCES Failed to allocate memory.
  @retval EFI_NOT_READY        The target can not accept new commands.
  @retval Others               Other errors as indicated.

**/
EFI_STATUS
IScsiExecuteScsiCommand (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                 *PassThru,
  IN UINT8                                           *Target,
  IN UINT64                                          Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet
  );

/**
  Reinstate the session on some error.

  @param[in]  Session           The iSCSI session

  @retval EFI_SUCCES            The session is reinstated from some error.
  @retval Other                 Reinstatement failed.

**/
EFI_STATUS
IScsiSessionReinstatement (
  IN ISCSI_SESSION  *Session
  );

/**
  Initialize some session parameters before login.

  @param[in, out]  Session  The iSCSI session.
  @param[in]       Recovery Whether the request is from a fresh new start or recovery.

**/
VOID
IScsiSessionInit (
  IN OUT ISCSI_SESSION  *Session,
  IN BOOLEAN            Recovery
  );

/**
  Abort the iSCSI session, that is, reset all the connection and free the
  resources.

  @param[in, out]  Session The iSCSI session.

**/
VOID
IScsiSessionAbort (
  IN OUT ISCSI_SESSION  *Session
  );

#endif
