/*++ @file

  Common definitions for Universal Flash Storage (UFS)

  Copyright (c) Microsoft Corporation. All rights reserved.
  Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  JESD220 - Universal Flash Storage (UFS)
  Version 2.0
  https://www.jedec.org/system/files/docs/JESD220C-2_1.pdf
--*/

#ifndef __UFS_H__
#define __UFS_H__

#include <Base.h>

#define UFS_LUN_0             0x00
#define UFS_LUN_1             0x01
#define UFS_LUN_2             0x02
#define UFS_LUN_3             0x03
#define UFS_LUN_4             0x04
#define UFS_LUN_5             0x05
#define UFS_LUN_6             0x06
#define UFS_LUN_7             0x07
#define UFS_WLUN_REPORT_LUNS  0x81
#define UFS_WLUN_UFS_DEV      0xD0
#define UFS_WLUN_BOOT         0xB0
#define UFS_WLUN_RPMB         0xC4

#pragma pack(1)

//
// UFS 2.0 Spec Section 10.5.3 - UTP Command UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8     TransCode : 6;    /* Transaction Type - 0x01*/
  UINT8     Dd        : 1;
  UINT8     Hd        : 1;
  UINT8     Flags;
  UINT8     Lun;
  UINT8     TaskTag;          /* Task Tag */

  //
  // DW1
  //
  UINT8     CmdSet    : 4;    /* Command Set Type */
  UINT8     Rsvd1     : 4;
  UINT8     Rsvd2;
  UINT8     Rsvd3;
  UINT8     Rsvd4;

  //
  // DW2
  //
  UINT8     EhsLen;           /* Total EHS Length - 0x00 */
  UINT8     Rsvd5;
  UINT16    DataSegLen;       /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3
  //
  UINT32    ExpDataTranLen;   /* Expected Data Transfer Length - Big Endian */

  //
  // DW4 - DW7
  //
  UINT8     Cdb[16];
} UTP_COMMAND_UPIU;

//
// UFS 2.0 Spec Section 10.5.4 - UTP Response UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8     TransCode : 6;    /* Transaction Type - 0x21*/
  UINT8     Dd        : 1;
  UINT8     Hd        : 1;
  UINT8     Flags;
  UINT8     Lun;
  UINT8     TaskTag;          /* Task Tag */

  //
  // DW1
  //
  UINT8     CmdSet    : 4;    /* Command Set Type */
  UINT8     Rsvd1     : 4;
  UINT8     Rsvd2;
  UINT8     Response;         /* Response */
  UINT8     Status;           /* Status */

  //
  // DW2
  //
  UINT8     EhsLen;           /* Total EHS Length - 0x00 */
  UINT8     DevInfo;          /* Device Information */
  UINT16    DataSegLen;       /* Data Segment Length - Big Endian */

  //
  // DW3
  //
  UINT32    ResTranCount;     /* Residual Transfer Count - Big Endian */

  //
  // DW4 - DW7
  //
  UINT8     Rsvd3[16];

  //
  // Data Segment - Sense Data
  //
  UINT16    SenseDataLen;     /* Sense Data Length - Big Endian */
  UINT8     SenseData[18];    /* Sense Data */
} UTP_RESPONSE_UPIU;

//
// UFS 2.0 Spec Section 10.5.5 - UTP Data-Out UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8     TransCode : 6;    /* Transaction Type - 0x02*/
  UINT8     Dd        : 1;
  UINT8     Hd        : 1;
  UINT8     Flags;
  UINT8     Lun;
  UINT8     TaskTag;          /* Task Tag */

  //
  // DW1
  //
  UINT8     Rsvd1[4];

  //
  // DW2
  //
  UINT8     EhsLen;           /* Total EHS Length - 0x00 */
  UINT8     Rsvd2;
  UINT16    DataSegLen;       /* Data Segment Length - Big Endian */

  //
  // DW3
  //
  UINT32    DataBufOffset;    /* Data Buffer Offset - Big Endian */

  //
  // DW4
  //
  UINT32    DataTranCount;    /* Data Transfer Count - Big Endian */

  //
  // DW5 - DW7
  //
  UINT8     Rsvd3[12];

  //
  // Data Segment - Data to be sent out
  //
  // UINT8  Data[];            /* Data to be sent out, maximum is 65535 bytes */
} UTP_DATA_OUT_UPIU;

//
// UFS 2.0 Spec Section 10.5.6 - UTP Data-In UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8     TransCode : 6;    /* Transaction Type - 0x22*/
  UINT8     Dd        : 1;
  UINT8     Hd        : 1;
  UINT8     Flags;
  UINT8     Lun;
  UINT8     TaskTag;          /* Task Tag */

  //
  // DW1
  //
  UINT8     Rsvd1[4];

  //
  // DW2
  //
  UINT8     EhsLen;           /* Total EHS Length - 0x00 */
  UINT8     Rsvd2;
  UINT16    DataSegLen;       /* Data Segment Length - Big Endian */

  //
  // DW3
  //
  UINT32    DataBufOffset;    /* Data Buffer Offset - Big Endian */

  //
  // DW4
  //
  UINT32    DataTranCount;    /* Data Transfer Count - Big Endian */

  //
  // DW5 - DW7
  //
  UINT8     Rsvd3[12];

  //
  // Data Segment - Data to be read
  //
  // UINT8  Data[];            /* Data to be read, maximum is 65535 bytes */
} UTP_DATA_IN_UPIU;

//
// UFS 2.0 Spec Section 10.5.7 - UTP Ready-To-Transfer UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8     TransCode : 6;    /* Transaction Type - 0x31*/
  UINT8     Dd        : 1;
  UINT8     Hd        : 1;
  UINT8     Flags;
  UINT8     Lun;
  UINT8     TaskTag;          /* Task Tag */

  //
  // DW1
  //
  UINT8     Rsvd1[4];

  //
  // DW2
  //
  UINT8     EhsLen;           /* Total EHS Length - 0x00 */
  UINT8     Rsvd2;
  UINT16    DataSegLen;       /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3
  //
  UINT32    DataBufOffset;    /* Data Buffer Offset - Big Endian */

  //
  // DW4
  //
  UINT32    DataTranCount;    /* Data Transfer Count - Big Endian */

  //
  // DW5 - DW7
  //
  UINT8     Rsvd3[12];

  //
  // Data Segment - Data to be read
  //
  // UINT8  Data[];            /* Data to be read, maximum is 65535 bytes */
} UTP_RDY_TO_TRAN_UPIU;

//
// UFS 2.0 Spec Section 10.5.8 - UTP Task Management Request UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8     TransCode : 6;    /* Transaction Type - 0x04*/
  UINT8     Dd        : 1;
  UINT8     Hd        : 1;
  UINT8     Flags;
  UINT8     Lun;
  UINT8     TaskTag;          /* Task Tag */

  //
  // DW1
  //
  UINT8     Rsvd1;
  UINT8     TskManFunc;       /* Task Management Function */
  UINT8     Rsvd2[2];

  //
  // DW2
  //
  UINT8     EhsLen;           /* Total EHS Length - 0x00 */
  UINT8     Rsvd3;
  UINT16    DataSegLen;       /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3
  //
  UINT32    InputParam1;      /* Input Parameter 1 - Big Endian */

  //
  // DW4
  //
  UINT32    InputParam2;      /* Input Parameter 2 - Big Endian */

  //
  // DW5
  //
  UINT32    InputParam3;      /* Input Parameter 3 - Big Endian */

  //
  // DW6 - DW7
  //
  UINT8     Rsvd4[8];
} UTP_TM_REQ_UPIU;

//
// UFS 2.0 Spec Section 10.5.9 - UTP Task Management Response UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8     TransCode : 6;    /* Transaction Type - 0x24*/
  UINT8     Dd        : 1;
  UINT8     Hd        : 1;
  UINT8     Flags;
  UINT8     Lun;
  UINT8     TaskTag;          /* Task Tag */

  //
  // DW1
  //
  UINT8     Rsvd1[2];
  UINT8     Resp;             /* Response */
  UINT8     Rsvd2;

  //
  // DW2
  //
  UINT8     EhsLen;           /* Total EHS Length - 0x00 */
  UINT8     Rsvd3;
  UINT16    DataSegLen;       /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3
  //
  UINT32    OutputParam1;     /* Output Parameter 1 - Big Endian */

  //
  // DW4
  //
  UINT32    OutputParam2;     /* Output Parameter 2 - Big Endian */

  //
  // DW5 - DW7
  //
  UINT8     Rsvd4[12];
} UTP_TM_RESP_UPIU;

typedef struct {
  UINT8     Opcode;
  UINT8     DescId;
  UINT8     Index;
  UINT8     Selector;
  UINT16    Rsvd1;
  UINT16    Length;
  UINT32    Value;
  UINT32    Rsvd2;
} UTP_UPIU_TSF;

//
// UFS 2.0 Spec Section 10.5.10 - UTP Query Request UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8           TransCode : 6; /* Transaction Type - 0x16*/
  UINT8           Dd        : 1;
  UINT8           Hd        : 1;
  UINT8           Flags;
  UINT8           Rsvd1;
  UINT8           TaskTag;    /* Task Tag */

  //
  // DW1
  //
  UINT8           Rsvd2;
  UINT8           QueryFunc;  /* Query Function */
  UINT8           Rsvd3[2];

  //
  // DW2
  //
  UINT8           EhsLen;     /* Total EHS Length - 0x00 */
  UINT8           Rsvd4;
  UINT16          DataSegLen; /* Data Segment Length - Big Endian */

  //
  // DW3 - 6
  //
  UTP_UPIU_TSF    Tsf;        /* Transaction Specific Fields */

  //
  // DW7
  //
  UINT8           Rsvd5[4];

  //
  // Data Segment - Data to be transferred
  //
  // UINT8  Data[];            /* Data to be transferred, maximum is 65535 bytes */
} UTP_QUERY_REQ_UPIU;

#define QUERY_FUNC_STD_READ_REQ   0x01
#define QUERY_FUNC_STD_WRITE_REQ  0x81

typedef enum {
  UtpQueryFuncOpcodeNop     = 0x00,
  UtpQueryFuncOpcodeRdDesc  = 0x01,
  UtpQueryFuncOpcodeWrDesc  = 0x02,
  UtpQueryFuncOpcodeRdAttr  = 0x03,
  UtpQueryFuncOpcodeWrAttr  = 0x04,
  UtpQueryFuncOpcodeRdFlag  = 0x05,
  UtpQueryFuncOpcodeSetFlag = 0x06,
  UtpQueryFuncOpcodeClrFlag = 0x07,
  UtpQueryFuncOpcodeTogFlag = 0x08
} UTP_QUERY_FUNC_OPCODE;

//
// UFS 2.0 Spec Section 10.5.11 - UTP Query Response UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8           TransCode : 6; /* Transaction Type - 0x36*/
  UINT8           Dd        : 1;
  UINT8           Hd        : 1;
  UINT8           Flags;
  UINT8           Rsvd1;
  UINT8           TaskTag;    /* Task Tag */

  //
  // DW1
  //
  UINT8           Rsvd2;
  UINT8           QueryFunc;  /* Query Function */
  UINT8           QueryResp;  /* Query Response */
  UINT8           Rsvd3;

  //
  // DW2
  //
  UINT8           EhsLen;     /* Total EHS Length - 0x00 */
  UINT8           DevInfo;    /* Device Information */
  UINT16          DataSegLen; /* Data Segment Length - Big Endian */

  //
  // DW3 - 6
  //
  UTP_UPIU_TSF    Tsf;        /* Transaction Specific Fields */

  //
  // DW7
  //
  UINT8           Rsvd4[4];

  //
  // Data Segment - Data to be transferred
  //
  // UINT8      Data[];        /* Data to be transferred, maximum is 65535 bytes */
} UTP_QUERY_RESP_UPIU;

typedef enum {
  UfsUtpQueryResponseSuccess             = 0x00,
  UfsUtpQueryResponseParamNotReadable    = 0xF6,
  UfsUtpQueryResponseParamNotWriteable   = 0xF7,
  UfsUtpQueryResponseParamAlreadyWritten = 0xF8,
  UfsUtpQueryResponseInvalidLen          = 0xF9,
  UfsUtpQueryResponseInvalidVal          = 0xFA,
  UfsUtpQueryResponseInvalidSelector     = 0xFB,
  UfsUtpQueryResponseInvalidIndex        = 0xFC,
  UfsUtpQueryResponseInvalidIdn          = 0xFD,
  UfsUtpQueryResponseInvalidOpc          = 0xFE,
  UfsUtpQueryResponseGeneralFailure      = 0xFF
} UTP_QUERY_RESP_CODE;

//
// UFS 2.0 Spec Section 10.5.12 - UTP Reject UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8     TransCode : 6;    /* Transaction Type - 0x3F*/
  UINT8     Dd        : 1;
  UINT8     Hd        : 1;
  UINT8     Flags;
  UINT8     Lun;
  UINT8     TaskTag;          /* Task Tag */

  //
  // DW1
  //
  UINT8     Rsvd1[2];
  UINT8     Response;         /* Response - 0x01 */
  UINT8     Rsvd2;

  //
  // DW2
  //
  UINT8     EhsLen;           /* Total EHS Length - 0x00 */
  UINT8     DevInfo;          /* Device Information - 0x00 */
  UINT16    DataSegLen;       /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3
  //
  UINT8     HdrSts;           /* Basic Header Status */
  UINT8     Rsvd3;
  UINT8     E2ESts;           /* End-to-End Status */
  UINT8     Rsvd4;

  //
  // DW4 - DW7
  //
  UINT8     Rsvd5[16];
} UTP_REJ_UPIU;

//
// UFS 2.0 Spec Section 10.5.13 - UTP NOP OUT UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8     TransCode : 6;    /* Transaction Type - 0x00*/
  UINT8     Dd        : 1;
  UINT8     Hd        : 1;
  UINT8     Flags;
  UINT8     Rsvd1;
  UINT8     TaskTag;          /* Task Tag */

  //
  // DW1
  //
  UINT8     Rsvd2[4];

  //
  // DW2
  //
  UINT8     EhsLen;           /* Total EHS Length - 0x00 */
  UINT8     Rsvd3;
  UINT16    DataSegLen;       /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3 - DW7
  //
  UINT8     Rsvd4[20];
} UTP_NOP_OUT_UPIU;

//
// UFS 2.0 Spec Section 10.5.14 - UTP NOP IN UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8     TransCode : 6;    /* Transaction Type - 0x20*/
  UINT8     Dd        : 1;
  UINT8     Hd        : 1;
  UINT8     Flags;
  UINT8     Rsvd1;
  UINT8     TaskTag;          /* Task Tag */

  //
  // DW1
  //
  UINT8     Rsvd2[2];
  UINT8     Resp;             /* Response - 0x00 */
  UINT8     Rsvd3;

  //
  // DW2
  //
  UINT8     EhsLen;           /* Total EHS Length - 0x00 */
  UINT8     DevInfo;          /* Device Information - 0x00 */
  UINT16    DataSegLen;       /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3 - DW7
  //
  UINT8     Rsvd4[20];
} UTP_NOP_IN_UPIU;

//
// UFS Descriptors
//
typedef enum {
  UfsDeviceDesc    = 0x00,
  UfsConfigDesc    = 0x01,
  UfsUnitDesc      = 0x02,
  UfsInterConnDesc = 0x04,
  UfsStringDesc    = 0x05,
  UfsGeometryDesc  = 0x07,
  UfsPowerDesc     = 0x08
} UFS_DESC_IDN;

//
// UFS 2.0 Spec Section 14.1.6.2 - Device Descriptor
//
typedef struct {
  UINT8     Length;
  UINT8     DescType;
  UINT8     Device;
  UINT8     DevClass;
  UINT8     DevSubClass;
  UINT8     Protocol;
  UINT8     NumLun;
  UINT8     NumWLun;
  UINT8     BootEn;
  UINT8     DescAccessEn;
  UINT8     InitPowerMode;
  UINT8     HighPriorityLun;
  UINT8     SecureRemovalType;
  UINT8     SecurityLun;
  UINT8     BgOpsTermLat;
  UINT8     InitActiveIccLevel;
  UINT16    SpecVersion;
  UINT16    ManufactureDate;
  UINT8     ManufacturerName;
  UINT8     ProductName;
  UINT8     SerialName;
  UINT8     OemId;
  UINT16    ManufacturerId;
  UINT8     Ud0BaseOffset;
  UINT8     Ud0ConfParamLen;
  UINT8     DevRttCap;
  UINT16    PeriodicRtcUpdate;
  UINT8     Rsvd1[17];
  UINT8     Rsvd2[16];
} UFS_DEV_DESC;

typedef struct {
  UINT8     Length;
  UINT8     DescType;
  UINT8     Rsvd1;
  UINT8     BootEn;
  UINT8     DescAccessEn;
  UINT8     InitPowerMode;
  UINT8     HighPriorityLun;
  UINT8     SecureRemovalType;
  UINT8     InitActiveIccLevel;
  UINT16    PeriodicRtcUpdate;
  UINT8     Rsvd2[5];
} UFS_CONFIG_DESC_GEN_HEADER;

typedef struct {
  UINT8     LunEn;
  UINT8     BootLunId;
  UINT8     LunWriteProt;
  UINT8     MemType;
  UINT32    NumAllocUnits;
  UINT8     DataReliability;
  UINT8     LogicBlkSize;
  UINT8     ProvisionType;
  UINT16    CtxCap;
  UINT8     Rsvd1[3];
} UFS_UNIT_DESC_CONFIG_PARAMS;

//
// UFS 2.0 Spec Section 14.1.6.3 - Configuration Descriptor
//
typedef struct {
  UFS_CONFIG_DESC_GEN_HEADER     Header;
  UFS_UNIT_DESC_CONFIG_PARAMS    UnitDescConfParams[8];
} UFS_CONFIG_DESC;

//
// UFS 2.0 Spec Section 14.1.6.4 - Geometry Descriptor
//
typedef struct {
  UINT8     Length;
  UINT8     DescType;
  UINT8     MediaTech;
  UINT8     Rsvd1;
  UINT64    TotalRawDevCapacity;
  UINT8     Rsvd2;
  UINT32    SegSize;
  UINT8     AllocUnitSize;
  UINT8     MinAddrBlkSize;
  UINT8     OptReadBlkSize;
  UINT8     OptWriteBlkSize;
  UINT8     MaxInBufSize;
  UINT8     MaxOutBufSize;
  UINT8     RpmbRwSize;
  UINT8     Rsvd3;
  UINT8     DataOrder;
  UINT8     MaxCtxIdNum;
  UINT8     SysDataTagUnitSize;
  UINT8     SysDataResUnitSize;
  UINT8     SupSecRemovalTypes;
  UINT16    SupMemTypes;
  UINT32    SysCodeMaxNumAllocUnits;
  UINT16    SupCodeCapAdjFac;
  UINT32    NonPersMaxNumAllocUnits;
  UINT16    NonPersCapAdjFac;
  UINT32    Enhance1MaxNumAllocUnits;
  UINT16    Enhance1CapAdjFac;
  UINT32    Enhance2MaxNumAllocUnits;
  UINT16    Enhance2CapAdjFac;
  UINT32    Enhance3MaxNumAllocUnits;
  UINT16    Enhance3CapAdjFac;
  UINT32    Enhance4MaxNumAllocUnits;
  UINT16    Enhance4CapAdjFac;
} UFS_GEOMETRY_DESC;

//
// UFS 2.0 Spec Section 14.1.6.5 - Unit Descriptor
//
typedef struct {
  UINT8     Length;
  UINT8     DescType;
  UINT8     UnitIdx;
  UINT8     LunEn;
  UINT8     BootLunId;
  UINT8     LunWriteProt;
  UINT8     LunQueueDep;
  UINT8     Rsvd1;
  UINT8     MemType;
  UINT8     DataReliability;
  UINT8     LogicBlkSize;
  UINT64    LogicBlkCount;
  UINT32    EraseBlkSize;
  UINT8     ProvisionType;
  UINT64    PhyMemResCount;
  UINT16    CtxCap;
  UINT8     LargeUnitGranularity;
} UFS_UNIT_DESC;

//
// UFS 2.0 Spec Section 14.1.6.6 - RPMB Unit Descriptor
//
typedef struct {
  UINT8     Length;
  UINT8     DescType;
  UINT8     UnitIdx;
  UINT8     LunEn;
  UINT8     BootLunId;
  UINT8     LunWriteProt;
  UINT8     LunQueueDep;
  UINT8     Rsvd1;
  UINT8     MemType;
  UINT8     Rsvd2;
  UINT8     LogicBlkSize;
  UINT64    LogicBlkCount;
  UINT32    EraseBlkSize;
  UINT8     ProvisionType;
  UINT64    PhyMemResCount;
  UINT8     Rsvd3[3];
} UFS_RPMB_UNIT_DESC;

typedef struct {
  UINT16    Value : 10;
  UINT16    Rsvd1 : 4;
  UINT16    Unit  : 2;
} UFS_POWER_PARAM_ELEMENT;

//
// UFS 2.0 Spec Section 14.1.6.7 - Power Parameter Descriptor
//
typedef struct {
  UINT8                      Length;
  UINT8                      DescType;
  UFS_POWER_PARAM_ELEMENT    ActiveIccLevelVcc[16];
  UFS_POWER_PARAM_ELEMENT    ActiveIccLevelVccQ[16];
  UFS_POWER_PARAM_ELEMENT    ActiveIccLevelVccQ2[16];
} UFS_POWER_DESC;

//
// UFS 2.0 Spec Section 14.1.6.8 - InterConnect Descriptor
//
typedef struct {
  UINT8     Length;
  UINT8     DescType;
  UINT16    UniProVer;
  UINT16    MphyVer;
} UFS_INTER_CONNECT_DESC;

//
// UFS 2.0 Spec Section 14.1.6.9 - 14.1.6.12 - String Descriptor
//
typedef struct {
  UINT8     Length;
  UINT8     DescType;
  CHAR16    Unicode[126];
} UFS_STRING_DESC;

//
// UFS 2.0 Spec Section 14.2 - Flags
//
typedef enum {
  UfsFlagDevInit         = 0x01,
  UfsFlagPermWpEn        = 0x02,
  UfsFlagPowerOnWpEn     = 0x03,
  UfsFlagBgOpsEn         = 0x04,
  UfsFlagPurgeEn         = 0x06,
  UfsFlagPhyResRemoval   = 0x08,
  UfsFlagBusyRtc         = 0x09,
  UfsFlagPermDisFwUpdate = 0x0B
} UFS_FLAGS_IDN;

//
// UFS 2.0 Spec Section 14.2 - Attributes
//
typedef enum {
  UfsAttrBootLunEn        = 0x00,
  UfsAttrCurPowerMode     = 0x02,
  UfsAttrActiveIccLevel   = 0x03,
  UfsAttrOutOfOrderDataEn = 0x04,
  UfsAttrBgOpStatus       = 0x05,
  UfsAttrPurgeStatus      = 0x06,
  UfsAttrMaxDataInSize    = 0x07,
  UfsAttrMaxDataOutSize   = 0x08,
  UfsAttrDynCapNeeded     = 0x09,
  UfsAttrRefClkFreq       = 0x0a,
  UfsAttrConfigDescLock   = 0x0b,
  UfsAttrMaxNumOfRtt      = 0x0c,
  UfsAttrExceptionEvtCtrl = 0x0d,
  UfsAttrExceptionEvtSts  = 0x0e,
  UfsAttrSecondsPassed    = 0x0f,
  UfsAttrContextConf      = 0x10,
  UfsAttrCorrPrgBlkNum    = 0x11
} UFS_ATTR_IDN;

#pragma pack()

#endif
