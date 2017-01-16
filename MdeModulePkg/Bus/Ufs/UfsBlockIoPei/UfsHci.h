/** @file
  UfsPassThruDxe driver is used to produce EFI_EXT_SCSI_PASS_THRU protocol interface
  for upper layer application to execute UFS-supported SCSI cmds.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _UFS_PASS_THRU_HCI_H_
#define _UFS_PASS_THRU_HCI_H_

//
// Host Capabilities Register Offsets
//
#define UFS_HC_CAP_OFFSET          0x0000  // Controller Capabilities
#define UFS_HC_VER_OFFSET          0x0008  // Version
#define UFS_HC_DDID_OFFSET         0x0010  // Device ID and Device Class
#define UFS_HC_PMID_OFFSET         0x0014  // Product ID and Manufacturer ID
#define UFS_HC_AHIT_OFFSET         0x0018  // Auto-Hibernate Idle Timer
//
// Operation and Runtime Register Offsets
//
#define UFS_HC_IS_OFFSET           0x0020  // Interrupt Status
#define UFS_HC_IE_OFFSET           0x0024  // Interrupt Enable
#define UFS_HC_STATUS_OFFSET       0x0030  // Host Controller Status
#define UFS_HC_ENABLE_OFFSET       0x0034  // Host Controller Enable
#define UFS_HC_UECPA_OFFSET        0x0038  // Host UIC Error Code PHY Adapter Layer
#define UFS_HC_UECDL_OFFSET        0x003c  // Host UIC Error Code Data Link Layer
#define UFS_HC_UECN_OFFSET         0x0040  // Host UIC Error Code Network Layer
#define UFS_HC_UECT_OFFSET         0x0044  // Host UIC Error Code Transport Layer
#define UFS_HC_UECDME_OFFSET       0x0048  // Host UIC Error Code DME
#define UFS_HC_UTRIACR_OFFSET      0x004c  // UTP Transfer Request Interrupt Aggregation Control Register
//
// UTP Transfer Register Offsets
//
#define UFS_HC_UTRLBA_OFFSET       0x0050  // UTP Transfer Request List Base Address
#define UFS_HC_UTRLBAU_OFFSET      0x0054  // UTP Transfer Request List Base Address Upper 32-Bits
#define UFS_HC_UTRLDBR_OFFSET      0x0058  // UTP Transfer Request List Door Bell Register
#define UFS_HC_UTRLCLR_OFFSET      0x005c  // UTP Transfer Request List CLear Register
#define UFS_HC_UTRLRSR_OFFSET      0x0060  // UTP Transfer Request Run-Stop Register
//
// UTP Task Management Register Offsets
//
#define UFS_HC_UTMRLBA_OFFSET      0x0070  // UTP Task Management Request List Base Address
#define UFS_HC_UTMRLBAU_OFFSET     0x0074  // UTP Task Management Request List Base Address Upper 32-Bits
#define UFS_HC_UTMRLDBR_OFFSET     0x0078  // UTP Task Management Request List Door Bell Register
#define UFS_HC_UTMRLCLR_OFFSET     0x007c  // UTP Task Management Request List CLear Register
#define UFS_HC_UTMRLRSR_OFFSET     0x0080  // UTP Task Management Run-Stop Register
//
// UIC Command Register Offsets
//
#define UFS_HC_UIC_CMD_OFFSET      0x0090  // UIC Command Register
#define UFS_HC_UCMD_ARG1_OFFSET    0x0094  // UIC Command Argument 1
#define UFS_HC_UCMD_ARG2_OFFSET    0x0098  // UIC Command Argument 2
#define UFS_HC_UCMD_ARG3_OFFSET    0x009c  // UIC Command Argument 3
//
// UMA Register Offsets
//
#define UFS_HC_UMA_OFFSET          0x00b0  // Reserved for Unified Memory Extension

#define UFS_HC_HCE_EN              BIT0
#define UFS_HC_HCS_DP              BIT0
#define UFS_HC_HCS_UCRDY           BIT3
#define UFS_HC_IS_ULSS             BIT8
#define UFS_HC_IS_UCCS             BIT10
#define UFS_HC_CAP_64ADDR          BIT24
#define UFS_HC_CAP_NUTMRS          (BIT16 | BIT17 | BIT18)
#define UFS_HC_CAP_NUTRS           (BIT0 | BIT1 | BIT2 | BIT3 | BIT4)
#define UFS_HC_UTMRLRSR            BIT0
#define UFS_HC_UTRLRSR             BIT0

//
// The initial value of the OCS field of UTP TRD or TMRD descriptor
// defined in JEDEC JESD223 specification
//
#define UFS_HC_TRD_OCS_INIT_VALUE  0x0F

//
// A maximum of length of 256KB is supported by PRDT entry
//
#define UFS_MAX_DATA_LEN_PER_PRD   0x40000

#define UFS_STORAGE_COMMAND_TYPE   0x01

#define UFS_REGULAR_COMMAND        0x00
#define UFS_INTERRUPT_COMMAND      0x01

#define UFS_LUN_0                  0x00
#define UFS_LUN_1                  0x01
#define UFS_LUN_2                  0x02
#define UFS_LUN_3                  0x03
#define UFS_LUN_4                  0x04
#define UFS_LUN_5                  0x05
#define UFS_LUN_6                  0x06
#define UFS_LUN_7                  0x07
#define UFS_WLUN_REPORT_LUNS       0x81
#define UFS_WLUN_UFS_DEV           0xD0
#define UFS_WLUN_BOOT              0xB0
#define UFS_WLUN_RPMB              0xC4

#pragma pack(1)

//
// UFSHCI 2.0 Spec Section 5.2.1 Offset 00h: CAP - Controller Capabilities
//
typedef struct {
  UINT8  Nutrs:4;      // Number of UTP Transfer Request Slots
  UINT8  Rsvd1:4;

  UINT8  NoRtt;        // Number of outstanding READY TO TRANSFER (RTT) requests supported

  UINT8  Nutmrs:3;     // Number of UTP Task Management Request Slots
  UINT8  Rsvd2:4;
  UINT8  AutoHs:1;     // Auto-Hibernation Support

  UINT8  As64:1;       // 64-bit addressing supported
  UINT8  Oodds:1;      // Out of order data delivery supported
  UINT8  UicDmetms:1;  // UIC DME_TEST_MODE command supported
  UINT8  Ume:1;        // Reserved for Unified Memory Extension
  UINT8  Rsvd4:4;
} UFS_HC_CAP;

//
// UFSHCI 2.0 Spec Section 5.2.2 Offset 08h: VER - UFS Version
//
typedef struct {
  UINT8  Vs:4;         // Version Suffix
  UINT8  Mnr:4;        // Minor version number

  UINT8  Mjr;          // Major version number

  UINT16 Rsvd1;
} UFS_HC_VER;

//
// UFSHCI 2.0 Spec Section 5.2.3 Offset 10h: HCPID - Host Controller Product ID
//
#define UFS_HC_PID     UINT32

//
// UFSHCI 2.0 Spec Section 5.2.4 Offset 14h: HCMID - Host Controller Manufacturer ID
//
#define UFS_HC_MID     UINT32

//
// UFSHCI 2.0 Spec Section 5.2.5 Offset 18h: AHIT - Auto-Hibernate Idle Timer
//
typedef struct {
  UINT32 Ahitv:10;     // Auto-Hibernate Idle Timer Value 
  UINT32 Ts:3;         // Timer scale
  UINT32 Rsvd1:19;
} UFS_HC_AHIT;

//
// UFSHCI 2.0 Spec Section 5.3.1 Offset 20h: IS - Interrupt Status
//
typedef struct {
  UINT16 Utrcs:1;      // UTP Transfer Request Completion Status
  UINT16 Udepri:1;     // UIC DME_ENDPOINT_RESET Indication
  UINT16 Ue:1;         // UIC Error
  UINT16 Utms:1;       // UIC Test Mode Status 

  UINT16 Upms:1;       // UIC Power Mode Status 
  UINT16 Uhxs:1;       // UIC Hibernate Exit Status 
  UINT16 Uhes:1;       // UIC Hibernate Enter Status 
  UINT16 Ulls:1;       // UIC Link Lost Status 

  UINT16 Ulss:1;       // UIC Link Startup Status 
  UINT16 Utmrcs:1;     // UTP Task  Management Request Completion Status 
  UINT16 Uccs:1;       // UIC Command Completion Status 
  UINT16 Dfes:1;       // Device Fatal Error Status  

  UINT16 Utpes:1;      // UTP Error Status  
  UINT16 Rsvd1:3;

  UINT16 Hcfes:1;      // Host Controller Fatal Error Status
  UINT16 Sbfes:1;      // System Bus Fatal Error Status
  UINT16 Rsvd2:14;
} UFS_HC_IS;

//
// UFSHCI 2.0 Spec Section 5.3.2 Offset 24h: IE - Interrupt Enable
//
typedef struct {
  UINT16 Utrce:1;      // UTP Transfer Request Completion Enable
  UINT16 Udeprie:1;    // UIC DME_ENDPOINT_RESET Enable
  UINT16 Uee:1;        // UIC Error Enable
  UINT16 Utmse:1;      // UIC Test Mode Status Enable

  UINT16 Upmse:1;      // UIC Power Mode Status Enable 
  UINT16 Uhxse:1;      // UIC Hibernate Exit Status Enable
  UINT16 Uhese:1;      // UIC Hibernate Enter Status Enable 
  UINT16 Ullse:1;      // UIC Link Lost Status Enable

  UINT16 Ulsse:1;      // UIC Link Startup Status Enable
  UINT16 Utmrce:1;     // UTP Task  Management Request Completion Enable
  UINT16 Ucce:1;       // UIC Command Completion Enable
  UINT16 Dfee:1;       // Device Fatal Error Enable

  UINT16 Utpee:1;      // UTP Error Enable
  UINT16 Rsvd1:3;

  UINT16 Hcfee:1;      // Host Controller Fatal Error Enable
  UINT16 Sbfee:1;      // System Bus Fatal Error Enable
  UINT16 Rsvd2:14;
} UFS_HC_IE;

//
// UFSHCI 2.0 Spec Section 5.3.3 Offset 30h: HCS - Host Controller Status
//
typedef struct {
  UINT8  Dp:1;         // Device Present
  UINT8  UtrlRdy:1;    // UTP Transfer Request List Ready
  UINT8  UtmrlRdy:1;   // UTP Task Management Request List Ready
  UINT8  UcRdy:1;      // UIC COMMAND Ready
  UINT8  Rsvd1:4;

  UINT8  Upmcrs:3;     // UIC Power Mode Change Request Status
  UINT8  Rsvd2:1;      // UIC Hibernate Exit Status Enable
  UINT8  Utpec:4;      // UTP Error Code

  UINT8  TtagUtpE;     // Task Tag of UTP error
  UINT8  TlunUtpE;     // Target LUN of UTP error
} UFS_HC_STATUS;

//
// UFSHCI 2.0 Spec Section 5.3.4 Offset 34h: HCE - Host Controller Enable
//
typedef struct {
  UINT32 Hce:1;        // Host Controller Enable
  UINT32 Rsvd1:31;
} UFS_HC_ENABLE;

//
// UFSHCI 2.0 Spec Section 5.3.5 Offset 38h: UECPA - Host UIC Error Code PHY Adapter Layer
//
typedef struct {
  UINT32 Ec:5;         // UIC PHY Adapter Layer Error Code
  UINT32 Rsvd1:26;
  UINT32 Err:1;        // UIC PHY Adapter Layer Error
} UFS_HC_UECPA;

//
// UFSHCI 2.0 Spec Section 5.3.6 Offset 3ch: UECDL - Host UIC Error Code Data Link Layer
//
typedef struct {
  UINT32 Ec:15;        // UIC Data Link Layer Error Code
  UINT32 Rsvd1:16;
  UINT32 Err:1;        // UIC Data Link Layer Error
} UFS_HC_UECDL;

//
// UFSHCI 2.0 Spec Section 5.3.7 Offset 40h: UECN - Host UIC Error Code Network Layer
//
typedef struct {
  UINT32 Ec:3;         // UIC Network Layer Error Code
  UINT32 Rsvd1:28;
  UINT32 Err:1;        // UIC Network Layer Error
} UFS_HC_UECN;

//
// UFSHCI 2.0 Spec Section 5.3.8 Offset 44h: UECT - Host UIC Error Code Transport Layer
//
typedef struct {
  UINT32 Ec:7;         // UIC Transport Layer Error Code
  UINT32 Rsvd1:24;
  UINT32 Err:1;        // UIC Transport Layer Error
} UFS_HC_UECT;

//
// UFSHCI 2.0 Spec Section 5.3.9 Offset 48h: UECDME - Host UIC Error Code
//
typedef struct {
  UINT32 Ec:1;         // UIC DME Error Code
  UINT32 Rsvd1:30;
  UINT32 Err:1;        // UIC DME Error
} UFS_HC_UECDME;

//
// UFSHCI 2.0 Spec Section 5.3.10 Offset 4Ch: UTRIACR - UTP Transfer Request Interrupt Aggregation Control Register
//
typedef struct {
  UINT8  IaToVal;      // Interrupt aggregation timeout value

  UINT8  IacTh:5;      // Interrupt aggregation counter threshold
  UINT8  Rsvd1:3;

  UINT8  Ctr:1;        // Counter and Timer Reset
  UINT8  Rsvd2:3;
  UINT8  Iasb:1;       // Interrupt aggregation status bit
  UINT8  Rsvd3:3;

  UINT8  IapwEn:1;     // Interrupt aggregation parameter write enable
  UINT8  Rsvd4:6;
  UINT8  IaEn:1;       // Interrupt Aggregation Enable/Disable
} UFS_HC_UTRIACR;

//
// UFSHCI 2.0 Spec Section 5.4.1 Offset 50h: UTRLBA - UTP Transfer Request List Base Address
//
typedef struct {
  UINT32 Rsvd1:10;
  UINT32 UtrlBa:22;    // UTP Transfer Request List Base Address
} UFS_HC_UTRLBA;

//
// UFSHCI 2.0 Spec Section 5.4.2 Offset 54h: UTRLBAU - UTP Transfer Request List Base Address Upper 32-bits
//
#define UFS_HC_UTRLBAU UINT32

//
// UFSHCI 2.0 Spec Section 5.4.3 Offset 58h: UTRLDBR - UTP Transfer Request List Door Bell Register
//
#define UFS_HC_UTRLDBR UINT32

//
// UFSHCI 2.0 Spec Section 5.4.4 Offset 5Ch: UTRLCLR - UTP Transfer Request List CLear Register
//
#define UFS_HC_UTRLCLR UINT32

#if 0
//
// UFSHCI 2.0 Spec Section 5.4.5 Offset 60h: UTRLRSR - UTP Transfer Request List Run Stop Register
//
typedef struct {
  UINT32 UtrlRsr:1;    // UTP Transfer Request List Run-Stop Register
  UINT32 Rsvd1:31;
} UFS_HC_UTRLRSR;
#endif

//
// UFSHCI 2.0 Spec Section 5.5.1 Offset 70h: UTMRLBA - UTP Task Management Request List Base Address
//
typedef struct {
  UINT32 Rsvd1:10;
  UINT32 UtmrlBa:22;   // UTP Task Management Request List Base Address
} UFS_HC_UTMRLBA;

//
// UFSHCI 2.0 Spec Section 5.5.2 Offset 74h: UTMRLBAU - UTP Task Management Request List Base Address Upper 32-bits
//
#define UFS_HC_UTMRLBAU UINT32

//
// UFSHCI 2.0 Spec Section 5.5.3 Offset 78h: UTMRLDBR - UTP Task Management Request List Door Bell Register
//
typedef struct {
  UINT32 UtmrlDbr:8;   // UTP Task Management Request List Door bell Register
  UINT32 Rsvd1:24;
} UFS_HC_UTMRLDBR;

//
// UFSHCI 2.0 Spec Section 5.5.4 Offset 7Ch: UTMRLCLR - UTP Task Management Request List CLear Register
//
typedef struct {
  UINT32 UtmrlClr:8;   // UTP Task Management List Clear Register
  UINT32 Rsvd1:24;
} UFS_HC_UTMRLCLR;

#if 0
//
// UFSHCI 2.0 Spec Section 5.5.5 Offset 80h: UTMRLRSR - UTP Task Management Request List Run Stop Register
//
typedef struct {
  UINT32 UtmrlRsr:1;   // UTP Task Management Request List Run-Stop Register
  UINT32 Rsvd1:31;
} UFS_HC_UTMRLRSR;
#endif

//
// UFSHCI 2.0 Spec Section 5.6.1 Offset 90h: UICCMD - UIC Command
//
typedef struct {
  UINT32 CmdOp:8;      // Command Opcode
  UINT32 Rsvd1:24;
} UFS_HC_UICCMD;

//
// UFSHCI 2.0 Spec Section 5.6.2 Offset 94h: UICCMDARG1 - UIC Command Argument 1
//
#define UFS_HC_UICCMD_ARG1 UINT32

//
// UFSHCI 2.0 Spec Section 5.6.2 Offset 98h: UICCMDARG2 - UIC Command Argument 2
//
#define UFS_HC_UICCMD_ARG2 UINT32

//
// UFSHCI 2.0 Spec Section 5.6.2 Offset 9ch: UICCMDARG3 - UIC Command Argument 3
//
#define UFS_HC_UICCMD_ARG3 UINT32

//
// UIC command opcodes
//
typedef enum {
  UfsUicDmeGet            = 0x01,
  UfsUicDmeSet            = 0x02,
  UfsUicDmePeerGet        = 0x03,
  UfsUicDmePeerSet        = 0x04,
  UfsUicDmePwrOn          = 0x10,
  UfsUicDmePwrOff         = 0x11,
  UfsUicDmeEnable         = 0x12,
  UfsUicDmeReset          = 0x14,
  UfsUicDmeEndpointReset  = 0x15,
  UfsUicDmeLinkStartup    = 0x16,
  UfsUicDmeHibernateEnter = 0x17,
  UfsUicDmeHibernateExit  = 0x18,
  UfsUicDmeTestMode       = 0x1A
} UFS_UIC_OPCODE;

//
// UTP Transfer Request Descriptor
//
typedef struct {
  //
  // DW0
  //
  UINT32 Rsvd1:24;
  UINT32 Int:1;               /* Interrupt */
  UINT32 Dd:2;                /* Data Direction */
  UINT32 Rsvd2:1;
  UINT32 Ct:4;                /* Command Type */

  //
  // DW1
  //
  UINT32 Rsvd3;

  //
  // DW2
  //
  UINT32 Ocs:8;               /* Overall Command Status */
  UINT32 Rsvd4:24;

  //
  // DW3
  //
  UINT32 Rsvd5;

  //
  // DW4
  //
  UINT32 Rsvd6:7;
  UINT32 UcdBa:25;            /* UTP Command Descriptor Base Address */
  
  //
  // DW5
  //
  UINT32 UcdBaU;              /* UTP Command Descriptor Base Address Upper 32-bits */
 
  //
  // DW6
  //
  UINT16 RuL;                 /* Response UPIU Length */  
  UINT16 RuO;                 /* Response UPIU Offset */

  //
  // DW7
  //
  UINT16 PrdtL;               /* PRDT Length */  
  UINT16 PrdtO;               /* PRDT Offset */
} UTP_TRD;

typedef struct {
  //
  // DW0
  //
  UINT32 Rsvd1:2;
  UINT32 DbAddr:30;           /* Data Base Address */
  
  //
  // DW1
  //
  UINT32 DbAddrU;             /* Data Base Address Upper 32-bits */
 
  //
  // DW2
  //
  UINT32 Rsvd2;

  //
  // DW3
  //
  UINT32 DbCount:18;          /* Data Byte Count */
  UINT32 Rsvd3:14;
} UTP_TR_PRD;

//
// UFS 2.0 Spec Section 10.5.3 - UTP Command UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8  TransCode:6;         /* Transaction Type - 0x01*/
  UINT8  Dd:1;
  UINT8  Hd:1;
  UINT8  Flags;
  UINT8  Lun;
  UINT8  TaskTag;             /* Task Tag */

  //
  // DW1
  //
  UINT8  CmdSet:4;            /* Command Set Type */
  UINT8  Rsvd1:4;
  UINT8  Rsvd2;
  UINT8  Rsvd3;
  UINT8  Rsvd4;

  //
  // DW2
  //
  UINT8  EhsLen;              /* Total EHS Length - 0x00 */
  UINT8  Rsvd5;
  UINT16 DataSegLen;          /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3
  //
  UINT32 ExpDataTranLen;      /* Expected Data Transfer Length - Big Endian */

  //
  // DW4 - DW7
  //
  UINT8  Cdb[16];
} UTP_COMMAND_UPIU;

//
// UFS 2.0 Spec Section 10.5.4 - UTP Response UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8  TransCode:6;         /* Transaction Type - 0x21*/
  UINT8  Dd:1;
  UINT8  Hd:1;
  UINT8  Flags;
  UINT8  Lun;
  UINT8  TaskTag;             /* Task Tag */

  //
  // DW1
  //
  UINT8  CmdSet:4;            /* Command Set Type */
  UINT8  Rsvd1:4;
  UINT8  Rsvd2;
  UINT8  Response;            /* Response */
  UINT8  Status;              /* Status */

  //
  // DW2
  //
  UINT8  EhsLen;              /* Total EHS Length - 0x00 */
  UINT8  DevInfo;             /* Device Information */
  UINT16 DataSegLen;          /* Data Segment Length - Big Endian */

  //
  // DW3
  //
  UINT32 ResTranCount;        /* Residual Transfer Count - Big Endian */

  //
  // DW4 - DW7
  //
  UINT8  Rsvd3[16];

  //
  // Data Segment - Sense Data
  //
  UINT16 SenseDataLen;        /* Sense Data Length - Big Endian */
  UINT8  SenseData[18];       /* Sense Data */
} UTP_RESPONSE_UPIU;

//
// UFS 2.0 Spec Section 10.5.5 - UTP Data-Out UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8  TransCode:6;         /* Transaction Type - 0x02*/
  UINT8  Dd:1;
  UINT8  Hd:1;
  UINT8  Flags;
  UINT8  Lun;
  UINT8  TaskTag;             /* Task Tag */

  //
  // DW1
  //
  UINT8  Rsvd1[4];

  //
  // DW2
  //
  UINT8  EhsLen;              /* Total EHS Length - 0x00 */
  UINT8  Rsvd2;
  UINT16 DataSegLen;          /* Data Segment Length - Big Endian */

  //
  // DW3
  //
  UINT32 DataBufOffset;       /* Data Buffer Offset - Big Endian */

  //
  // DW4
  //
  UINT32 DataTranCount;       /* Data Transfer Count - Big Endian */

  //
  // DW5 - DW7
  //
  UINT8  Rsvd3[12];

  //
  // Data Segment - Data to be sent out
  //
  //UINT8  Data[];            /* Data to be sent out, maximum is 65535 bytes */
} UTP_DATA_OUT_UPIU;

//
// UFS 2.0 Spec Section 10.5.6 - UTP Data-In UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8  TransCode:6;         /* Transaction Type - 0x22*/
  UINT8  Dd:1;
  UINT8  Hd:1;
  UINT8  Flags;
  UINT8  Lun;
  UINT8  TaskTag;             /* Task Tag */

  //
  // DW1
  //
  UINT8  Rsvd1[4];

  //
  // DW2
  //
  UINT8  EhsLen;              /* Total EHS Length - 0x00 */
  UINT8  Rsvd2;
  UINT16 DataSegLen;          /* Data Segment Length - Big Endian */

  //
  // DW3
  //
  UINT32 DataBufOffset;       /* Data Buffer Offset - Big Endian */

  //
  // DW4
  //
  UINT32 DataTranCount;       /* Data Transfer Count - Big Endian */

  //
  // DW5 - DW7
  //
  UINT8  Rsvd3[12];

  //
  // Data Segment - Data to be read
  //
  //UINT8  Data[];            /* Data to be read, maximum is 65535 bytes */
} UTP_DATA_IN_UPIU;

//
// UFS 2.0 Spec Section 10.5.7 - UTP Ready-To-Transfer UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8  TransCode:6;         /* Transaction Type - 0x31*/
  UINT8  Dd:1;
  UINT8  Hd:1;
  UINT8  Flags;
  UINT8  Lun;
  UINT8  TaskTag;             /* Task Tag */

  //
  // DW1
  //
  UINT8  Rsvd1[4];

  //
  // DW2
  //
  UINT8  EhsLen;              /* Total EHS Length - 0x00 */
  UINT8  Rsvd2;
  UINT16 DataSegLen;          /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3
  //
  UINT32 DataBufOffset;       /* Data Buffer Offset - Big Endian */

  //
  // DW4
  //
  UINT32 DataTranCount;       /* Data Transfer Count - Big Endian */

  //
  // DW5 - DW7
  //
  UINT8  Rsvd3[12];

  //
  // Data Segment - Data to be read
  //
  //UINT8  Data[];            /* Data to be read, maximum is 65535 bytes */
} UTP_RDY_TO_TRAN_UPIU;

//
// UFS 2.0 Spec Section 10.5.8 - UTP Task Management Request UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8  TransCode:6;         /* Transaction Type - 0x04*/
  UINT8  Dd:1;
  UINT8  Hd:1;
  UINT8  Flags;
  UINT8  Lun;
  UINT8  TaskTag;             /* Task Tag */

  //
  // DW1
  //
  UINT8  Rsvd1;
  UINT8  TskManFunc;          /* Task Management Function */
  UINT8  Rsvd2[2];

  //
  // DW2
  //
  UINT8  EhsLen;              /* Total EHS Length - 0x00 */
  UINT8  Rsvd3;
  UINT16 DataSegLen;          /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3
  //
  UINT32 InputParam1;         /* Input Parameter 1 - Big Endian */

  //
  // DW4
  //
  UINT32 InputParam2;         /* Input Parameter 2 - Big Endian */

  //
  // DW5
  //
  UINT32 InputParam3;         /* Input Parameter 3 - Big Endian */

  //
  // DW6 - DW7
  //
  UINT8  Rsvd4[8];
} UTP_TM_REQ_UPIU;

//
// UFS 2.0 Spec Section 10.5.9 - UTP Task Management Response UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8  TransCode:6;         /* Transaction Type - 0x24*/
  UINT8  Dd:1;
  UINT8  Hd:1;
  UINT8  Flags;
  UINT8  Lun;
  UINT8  TaskTag;             /* Task Tag */

  //
  // DW1
  //
  UINT8  Rsvd1[2];
  UINT8  Resp;                /* Response */
  UINT8  Rsvd2;

  //
  // DW2
  //
  UINT8  EhsLen;              /* Total EHS Length - 0x00 */
  UINT8  Rsvd3;
  UINT16 DataSegLen;          /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3
  //
  UINT32 OutputParam1;        /* Output Parameter 1 - Big Endian */

  //
  // DW4
  //
  UINT32 OutputParam2;        /* Output Parameter 2 - Big Endian */

  //
  // DW5 - DW7
  //
  UINT8  Rsvd4[12];
} UTP_TM_RESP_UPIU;

//
// UTP Task Management Request Descriptor
//
typedef struct {
  //
  // DW0
  //
  UINT32 Rsvd1:24;
  UINT32 Int:1;               /* Interrupt */
  UINT32 Rsvd2:7;

  //
  // DW1
  //
  UINT32 Rsvd3;

  //
  // DW2
  //
  UINT32 Ocs:8;               /* Overall Command Status */
  UINT32 Rsvd4:24;

  //
  // DW3
  //
  UINT32 Rsvd5;

  //
  // DW4 - DW11
  //
  UTP_TM_REQ_UPIU TmReq;      /* Task Management Request UPIU */
  
  //
  // DW12 - DW19
  //
  UTP_TM_RESP_UPIU TmResp;    /* Task Management Response UPIU */
} UTP_TMRD;


typedef struct {
  UINT8  Opcode;
  UINT8  DescId;
  UINT8  Index;
  UINT8  Selector;
  UINT16 Rsvd1;
  UINT16 Length;
  UINT32 Value;
  UINT32 Rsvd2;
} UTP_UPIU_TSF;

//
// UFS 2.0 Spec Section 10.5.10 - UTP Query Request UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8        TransCode:6;   /* Transaction Type - 0x16*/
  UINT8        Dd:1;
  UINT8        Hd:1;
  UINT8        Flags;
  UINT8        Rsvd1;
  UINT8        TaskTag;       /* Task Tag */

  //
  // DW1
  //
  UINT8        Rsvd2;
  UINT8        QueryFunc;     /* Query Function */
  UINT8        Rsvd3[2];

  //
  // DW2
  //
  UINT8        EhsLen;        /* Total EHS Length - 0x00 */
  UINT8        Rsvd4;
  UINT16       DataSegLen;    /* Data Segment Length - Big Endian */

  //
  // DW3 - 6
  //
  UTP_UPIU_TSF Tsf;           /* Transaction Specific Fields */

  //
  // DW7
  //
  UINT8        Rsvd5[4];

  //
  // Data Segment - Data to be transferred
  //
  //UINT8  Data[];            /* Data to be transferred, maximum is 65535 bytes */
} UTP_QUERY_REQ_UPIU;

#define QUERY_FUNC_STD_READ_REQ     0x01
#define QUERY_FUNC_STD_WRITE_REQ    0x81

typedef enum {
  UtpQueryFuncOpcodeNop      = 0x00,
  UtpQueryFuncOpcodeRdDesc   = 0x01,
  UtpQueryFuncOpcodeWrDesc   = 0x02,
  UtpQueryFuncOpcodeRdAttr   = 0x03,
  UtpQueryFuncOpcodeWrAttr   = 0x04,
  UtpQueryFuncOpcodeRdFlag   = 0x05,
  UtpQueryFuncOpcodeSetFlag  = 0x06,
  UtpQueryFuncOpcodeClrFlag  = 0x07,
  UtpQueryFuncOpcodeTogFlag  = 0x08
} UTP_QUERY_FUNC_OPCODE;

//
// UFS 2.0 Spec Section 10.5.11 - UTP Query Response UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8        TransCode:6;   /* Transaction Type - 0x36*/
  UINT8        Dd:1;
  UINT8        Hd:1;
  UINT8        Flags;
  UINT8        Rsvd1;
  UINT8        TaskTag;       /* Task Tag */

  //
  // DW1
  //
  UINT8        Rsvd2;
  UINT8        QueryFunc;     /* Query Function */
  UINT8        QueryResp;     /* Query Response */
  UINT8        Rsvd3;

  //
  // DW2
  //
  UINT8        EhsLen;        /* Total EHS Length - 0x00 */
  UINT8        DevInfo;       /* Device Information */
  UINT16       DataSegLen;    /* Data Segment Length - Big Endian */

  //
  // DW3 - 6
  //
  UTP_UPIU_TSF Tsf;           /* Transaction Specific Fields */

  //
  // DW7
  //
  UINT8        Rsvd4[4];

  //
  // Data Segment - Data to be transferred
  //
  //UINT8      Data[];        /* Data to be transferred, maximum is 65535 bytes */
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
  UINT8  TransCode:6;         /* Transaction Type - 0x3F*/
  UINT8  Dd:1;
  UINT8  Hd:1;
  UINT8  Flags;
  UINT8  Lun;
  UINT8  TaskTag;             /* Task Tag */

  //
  // DW1
  //
  UINT8  Rsvd1[2];
  UINT8  Response;            /* Response - 0x01 */
  UINT8  Rsvd2;

  //
  // DW2
  //
  UINT8  EhsLen;              /* Total EHS Length - 0x00 */
  UINT8  DevInfo;             /* Device Information - 0x00 */
  UINT16 DataSegLen;          /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3
  //
  UINT8  HdrSts;              /* Basic Header Status */
  UINT8  Rsvd3;
  UINT8  E2ESts;              /* End-to-End Status */
  UINT8  Rsvd4;

  //
  // DW4 - DW7
  //
  UINT8  Rsvd5[16];
} UTP_REJ_UPIU;

//
// UFS 2.0 Spec Section 10.5.13 - UTP NOP OUT UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8  TransCode:6;         /* Transaction Type - 0x00*/
  UINT8  Dd:1;
  UINT8  Hd:1;
  UINT8  Flags;
  UINT8  Rsvd1;
  UINT8  TaskTag;             /* Task Tag */

  //
  // DW1
  //
  UINT8  Rsvd2[4];

  //
  // DW2
  //
  UINT8  EhsLen;              /* Total EHS Length - 0x00 */
  UINT8  Rsvd3;
  UINT16 DataSegLen;          /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3 - DW7
  //
  UINT8  Rsvd4[20];
} UTP_NOP_OUT_UPIU;

//
// UFS 2.0 Spec Section 10.5.14 - UTP NOP IN UPIU
//
typedef struct {
  //
  // DW0
  //
  UINT8  TransCode:6;         /* Transaction Type - 0x20*/
  UINT8  Dd:1;
  UINT8  Hd:1;
  UINT8  Flags;
  UINT8  Rsvd1;
  UINT8  TaskTag;             /* Task Tag */

  //
  // DW1
  //
  UINT8  Rsvd2[2];
  UINT8  Resp;                /* Response - 0x00 */
  UINT8  Rsvd3;

  //
  // DW2
  //
  UINT8  EhsLen;              /* Total EHS Length - 0x00 */
  UINT8  DevInfo;             /* Device Information - 0x00 */
  UINT16 DataSegLen;          /* Data Segment Length - Big Endian - 0x0000 */

  //
  // DW3 - DW7
  //
  UINT8  Rsvd4[20];
} UTP_NOP_IN_UPIU;

//
// UFS Descriptors
//
typedef enum {
  UfsDeviceDesc     = 0x00,
  UfsConfigDesc     = 0x01,
  UfsUnitDesc       = 0x02,
  UfsInterConnDesc  = 0x04,
  UfsStringDesc     = 0x05,
  UfsGeometryDesc   = 0x07,
  UfsPowerDesc      = 0x08
} UFS_DESC_IDN;

//
// UFS 2.0 Spec Section 14.1.6.2 - Device Descriptor
//
typedef struct {
  UINT8  Length;
  UINT8  DescType;
  UINT8  Device;
  UINT8  DevClass;
  UINT8  DevSubClass;
  UINT8  Protocol;
  UINT8  NumLun;
  UINT8  NumWLun;
  UINT8  BootEn;
  UINT8  DescAccessEn;
  UINT8  InitPowerMode;
  UINT8  HighPriorityLun;
  UINT8  SecureRemovalType;
  UINT8  SecurityLun;
  UINT8  BgOpsTermLat;
  UINT8  InitActiveIccLevel;
  UINT16 SpecVersion;
  UINT16 ManufactureDate;
  UINT8  ManufacturerName;
  UINT8  ProductName;
  UINT8  SerialName;
  UINT8  OemId;
  UINT16 ManufacturerId;
  UINT8  Ud0BaseOffset;
  UINT8  Ud0ConfParamLen;
  UINT8  DevRttCap;
  UINT16 PeriodicRtcUpdate;
  UINT8  Rsvd1[17];
  UINT8  Rsvd2[16];
} UFS_DEV_DESC;

typedef struct {
  UINT8  Length;
  UINT8  DescType;
  UINT8  Rsvd1;
  UINT8  BootEn;
  UINT8  DescAccessEn;
  UINT8  InitPowerMode;
  UINT8  HighPriorityLun;
  UINT8  SecureRemovalType;
  UINT8  InitActiveIccLevel;
  UINT16 PeriodicRtcUpdate;
  UINT8  Rsvd2[5];
} UFS_CONFIG_DESC_GEN_HEADER;

typedef struct {
  UINT8  LunEn;
  UINT8  BootLunId;
  UINT8  LunWriteProt;
  UINT8  MemType;
  UINT32 NumAllocUnits;
  UINT8  DataReliability;
  UINT8  LogicBlkSize;
  UINT8  ProvisionType;
  UINT16 CtxCap;
  UINT8  Rsvd1[3];
} UFS_UNIT_DESC_CONFIG_PARAMS;

//
// UFS 2.0 Spec Section 14.1.6.3 - Configuration Descriptor
//
typedef struct {
  UFS_CONFIG_DESC_GEN_HEADER  Header;
  UFS_UNIT_DESC_CONFIG_PARAMS UnitDescConfParams[8];
} UFS_CONFIG_DESC;

//
// UFS 2.0 Spec Section 14.1.6.4 - Geometry Descriptor
//
typedef struct {
  UINT8  Length;
  UINT8  DescType;
  UINT8  MediaTech;
  UINT8  Rsvd1;
  UINT64 TotalRawDevCapacity;
  UINT8  Rsvd2;
  UINT32 SegSize;
  UINT8  AllocUnitSize;
  UINT8  MinAddrBlkSize;
  UINT8  OptReadBlkSize;
  UINT8  OptWriteBlkSize;
  UINT8  MaxInBufSize;
  UINT8  MaxOutBufSize;
  UINT8  RpmbRwSize;
  UINT8  Rsvd3;
  UINT8  DataOrder;
  UINT8  MaxCtxIdNum;
  UINT8  SysDataTagUnitSize;
  UINT8  SysDataResUnitSize;
  UINT8  SupSecRemovalTypes;
  UINT16 SupMemTypes;
  UINT32 SysCodeMaxNumAllocUnits;
  UINT16 SupCodeCapAdjFac;
  UINT32 NonPersMaxNumAllocUnits;
  UINT16 NonPersCapAdjFac;
  UINT32 Enhance1MaxNumAllocUnits;
  UINT16 Enhance1CapAdjFac;
  UINT32 Enhance2MaxNumAllocUnits;
  UINT16 Enhance2CapAdjFac;
  UINT32 Enhance3MaxNumAllocUnits;
  UINT16 Enhance3CapAdjFac;
  UINT32 Enhance4MaxNumAllocUnits;
  UINT16 Enhance4CapAdjFac;
} UFS_GEOMETRY_DESC;

//
// UFS 2.0 Spec Section 14.1.6.5 - Unit Descriptor
//
typedef struct {
  UINT8  Length;
  UINT8  DescType;
  UINT8  UnitIdx;
  UINT8  LunEn;
  UINT8  BootLunId;
  UINT8  LunWriteProt;
  UINT8  LunQueueDep;
  UINT8  Rsvd1;
  UINT8  MemType;
  UINT8  DataReliability;
  UINT8  LogicBlkSize;
  UINT64 LogicBlkCount;
  UINT32 EraseBlkSize;
  UINT8  ProvisionType;
  UINT64 PhyMemResCount;
  UINT16 CtxCap;
  UINT8  LargeUnitGranularity;
} UFS_UNIT_DESC;

//
// UFS 2.0 Spec Section 14.1.6.6 - RPMB Unit Descriptor
//
typedef struct {
  UINT8  Length;
  UINT8  DescType;
  UINT8  UnitIdx;
  UINT8  LunEn;
  UINT8  BootLunId;
  UINT8  LunWriteProt;
  UINT8  LunQueueDep;
  UINT8  Rsvd1;
  UINT8  MemType;
  UINT8  Rsvd2;
  UINT8  LogicBlkSize;
  UINT64 LogicBlkCount;
  UINT32 EraseBlkSize;
  UINT8  ProvisionType;
  UINT64 PhyMemResCount;
  UINT8  Rsvd3[3];
} UFS_RPMB_UNIT_DESC;

typedef struct {
  UINT16 Value:10;
  UINT16 Rsvd1:4;
  UINT16 Unit:2;
} UFS_POWER_PARAM_ELEMENT;

//
// UFS 2.0 Spec Section 14.1.6.7 - Power Parameter Descriptor
//
typedef struct {
  UINT8                    Length;
  UINT8                    DescType;
  UFS_POWER_PARAM_ELEMENT  ActiveIccLevelVcc[16];
  UFS_POWER_PARAM_ELEMENT  ActiveIccLevelVccQ[16];
  UFS_POWER_PARAM_ELEMENT  ActiveIccLevelVccQ2[16];
} UFS_POWER_DESC;

//
// UFS 2.0 Spec Section 14.1.6.8 - InterConnect Descriptor
//
typedef struct {
  UINT8  Length;
  UINT8  DescType;
  UINT16 UniProVer;
  UINT16 MphyVer;
} UFS_INTER_CONNECT_DESC;

//
// UFS 2.0 Spec Section 14.1.6.9 - 14.1.6.12 - String Descriptor
//
typedef struct {
  UINT8  Length;
  UINT8  DescType;
  CHAR16 Unicode[126];
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
  UfsAttrBootLunEn         = 0x00,
  UfsAttrCurPowerMode      = 0x02,
  UfsAttrActiveIccLevel    = 0x03,
  UfsAttrOutOfOrderDataEn  = 0x04,
  UfsAttrBgOpStatus        = 0x05,
  UfsAttrPurgeStatus       = 0x06,
  UfsAttrMaxDataInSize     = 0x07,
  UfsAttrMaxDataOutSize    = 0x08,
  UfsAttrDynCapNeeded      = 0x09,
  UfsAttrRefClkFreq        = 0x0a,
  UfsAttrConfigDescLock    = 0x0b,
  UfsAttrMaxNumOfRtt       = 0x0c,
  UfsAttrExceptionEvtCtrl  = 0x0d,
  UfsAttrExceptionEvtSts   = 0x0e,
  UfsAttrSecondsPassed     = 0x0f,
  UfsAttrContextConf       = 0x10,
  UfsAttrCorrPrgBlkNum     = 0x11
} UFS_ATTR_IDN;

typedef enum {
  UfsNoData                = 0,
  UfsDataOut               = 1,
  UfsDataIn                = 2,
  UfsDdReserved
} UFS_DATA_DIRECTION;


#pragma pack()

#endif

