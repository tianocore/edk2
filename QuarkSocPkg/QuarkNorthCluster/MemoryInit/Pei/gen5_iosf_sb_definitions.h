/************************************************************************
 *
 * Copyright (c) 2013-2015 Intel Corporation.
 *
* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * MCU register definition
 *
 ************************************************************************/
#ifndef __IOSF_DEFINITIONS_H
#define __IOSF_DEFINITIONS_H

// Define each of the IOSF-SB register offsets used by MRC.


// MCU registers (DUNIT):
// ====
#define DRP                 0x0000
#define DTR0                0x0001
#define DTR1                0x0002
#define DTR2                0x0003
#define DTR3                0x0004
#define DTR4                0x0005
#define DPMC0               0x0006
#define DPMC1               0x0007
#define DRFC                0x0008
#define DSCH                0x0009
#define DCAL                0x000A
#define DRMC                0x000B
#define PMSTS               0x000C
#define DCO                 0x000F
#define DSTAT               0x0020
#define DECCCTRL            0x0060
#define DFUSESTAT           0x0070
#define SCRMSEED            0x0080
#define SCRMLO              0x0081
#define SCRMHI              0x0082

#define MCU_CH_OFFSET       0x0040
#define MCU_RK_OFFSET       0x0020

////
//
// BEGIN DUnit register definition
//
#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t rank0Enabled       :1;             /**< BIT [0]   Rank 0 Enable */
        uint32_t rank1Enabled       :1;             /**< BIT [1]   Rank 1 Enable */
        uint32_t reserved0          :2;
        uint32_t dimm0DevWidth      :2;             /**< BIT [5:4] DIMM 0 Device Width (Rank0&1)  */
        uint32_t dimm0DevDensity    :2;             /**< BIT [7:6] DIMM 0 Device Density          */
        uint32_t reserved1          :1;
        uint32_t dimm1DevWidth      :2;             /**< BIT [10:9]  DIMM 1 Device Width (Rank2&3)  */
        uint32_t dimm1DevDensity    :2;             /**< BIT [12:11] DIMM 1 Device Density          */
        uint32_t split64            :1;             /**< BIT [13] split 64B transactions */
        uint32_t addressMap         :2;             /**< BIT [15:14] Address Map select */
        uint32_t reserved3          :14;
        uint32_t mode32             :1;             /**< BIT [30] Select 32bit data interface*/
        uint32_t reserved4          :1;
    } field;
} RegDRP;                                           /**< DRAM Rank Population and Interface Register */
#pragma pack()


#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t dramFrequency      :2;             /**< DRAM Frequency (000=800,001=1033,010=1333) */
        uint32_t reserved1          :2;
        uint32_t tRP                :4;             /**< bit [7:4]   Precharge to Activate Delay  */
        uint32_t tRCD               :4;             /**< bit [11:8]  Activate to CAS Delay  */
        uint32_t tCL                :3;             /**< bit [14:12] CAS Latency  */
        uint32_t reserved4          :1;
        uint32_t tXS                :1;             /**< SRX Delay  */
        uint32_t reserved5          :1;
        uint32_t tXSDLL             :1;             /**< SRX To DLL Delay  */
        uint32_t reserved6          :1;
        uint32_t tZQCS              :1;             /**< bit [20] ZQTS recovery Latncy  */
        uint32_t reserved7          :1;
        uint32_t tZQCL              :1;             /**< bit [22] ZQCL recovery Latncy  */
        uint32_t reserved8          :1;
        uint32_t pmeDelay           :2;             /**< bit [25:24] Power mode entry delay  */
        uint32_t reserved9          :2;
        uint32_t CKEDLY             :4;               /**< bit [31:28]  */
    } field;
} RegDTR0;                                          /**< DRAM Timing Register 0 */
#pragma pack()

#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t tWCL               :3;             /**< bit [2:0] CAS Write Latency */
        uint32_t reserved1          :1;
        uint32_t tCMD               :2;             /**< bit [5:4] Command transport duration */
        uint32_t reserved2          :2;
        uint32_t tWTP               :4;             /**< Write to Precharge */
        uint32_t tCCD               :2;             /**< CAS to CAS delay */
        uint32_t reserved4          :2;
        uint32_t tFAW               :4;             /**< Four bank Activation Window*/
        uint32_t tRAS               :4;             /**< Row Activation Period: */
        uint32_t tRRD               :2;             /**<Row activation to Row activation Delay */
        uint32_t reserved5          :2;
        uint32_t tRTP               :3;             /**<Read to Precharge Delay */
        uint32_t reserved6          :1;
    } field;
} RegDTR1;                                          /**< DRAM Timing Register 1 */
#pragma pack()

#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t tRRDR              :3;             /**< RD to RD from different ranks, same DIMM */
        uint32_t reserved1          :5;
        uint32_t tWWDR              :3;             /**< WR to WR from different ranks, same DIMM. */
        uint32_t reserved3          :5;
        uint32_t tRWDR              :4;             /**< bit [19:16] RD to WR from different ranks, same DIMM. */
        uint32_t reserved5          :12;
    } field;
} RegDTR2;                                          /**< DRAM Timing Register 2 */
#pragma pack()

#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t tWRDR              :3;             /**< WR to RD from different ranks, same DIMM. */
        uint32_t reserved1          :1;
        uint32_t tWRDD              :3;             /**< WR to RD from different DIMM. */
        uint32_t reserved2          :1;
        uint32_t tRWSR              :4;             /**< RD to WR Same Rank. */
        uint32_t reserved3          :1;
        uint32_t tWRSR              :4;             /**< WR to RD Same Rank. */
        uint32_t reserved4          :5;
        uint32_t tXP                :2;             /**< Time from CKE set on to any command. */
        uint32_t PWD_DLY            :4;             /**< Extended Power-Down Delay. */
        uint32_t EnDeRate           :1;
        uint32_t DeRateOvr          :1;
        uint32_t DeRateStat         :1;
        uint32_t reserved5          :1;
    } field;
} RegDTR3;                                          /**< DRAM Timing Register 3 */
#pragma pack()


#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t WRODTSTRT          :2;             /**< WR command to ODT assert delay */
        uint32_t reserved1          :2;
        uint32_t WRODTSTOP          :3;             /**< Write command to ODT de-assert delay. */
        uint32_t reserved2          :1;
        uint32_t RDODTSTRT          :3;             /**< Read command to ODT assert delay */
        uint32_t reserved3          :1;
        uint32_t RDODTSTOP          :3;             /**< Read command to ODT de-assert delay */
        uint32_t ODTDIS             :1;             /**< ODT disable */
        uint32_t TRGSTRDIS          :1;             /**< Write target rank is not stretched */
        uint32_t RDODTDIS           :1;             /**< Disable Read ODT */
        uint32_t WRBODTDIS          :1;             /**< Disable Write ODT */
        uint32_t reserved5          :13;
    } field;
} RegDTR4;                                          /**< DRAM Timing Register 3 */
#pragma pack()

#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t SREntryDelay       :8;             /**< Self-Refresh Entry Delay: */
        uint32_t powerModeOpCode    :5;             /**< SPID Power Mode Opcode */
        uint32_t reserved1          :3;
        uint32_t PCLSTO             :3;             /**< Page Close Timeout Period */
        uint32_t reserved2          :1;
        uint32_t PCLSWKOK           :1;             /**< Wake Allowed For Page Close Timeout */
        uint32_t PREAPWDEN          :1;             /**< Send Precharge All to rank before entering Power-Down mode. */
        uint32_t reserved3          :1;
        uint32_t DYNSREN            :1;             /**< Dynamic Self-Refresh */
        uint32_t CLKGTDIS           :1;             /**< Clock Gating Disabled*/
        uint32_t DISPWRDN           :1;             /**< Disable Power Down*/
        uint32_t reserved4          :2;
        uint32_t REUTCLKGTDIS       :1;
        uint32_t ENPHYCLKGATE       :1;
        uint32_t reserved5          :2;
    } field;
} RegDPMC0;                                           /**< DRAM Power Management Control Register 0 */
#pragma pack()

#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t REFWMLO            :4;             /**< Refresh Opportunistic Watermark */
        uint32_t REFWMHI            :4;             /**< Refresh High Watermark*/
        uint32_t REFWMPNC           :4;             /**< Refresh Panic Watermark */
        uint32_t tREFI              :3;             /**< bit [14:12] Refresh Period */
        uint32_t reserved1          :1;
        uint32_t REFCNTMAX          :2;             /**< Refresh Max tREFI Interval */
        uint32_t reserved2          :2;
        uint32_t REFSKEWDIS         :1;             /**< tREFI counters */
        uint32_t REFDBTCLR          :1;
        uint32_t reserved3          :2;
        uint32_t CuRefRate          :3;
        uint32_t DisRefBW           :1;
        uint32_t reserved4          :4;
    } field;
} RegDRCF;                                           /**< DRAM Refresh Control Register*/
#pragma pack()

#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t reserved1          :8;
        uint32_t ZQCINT             :3;             /**< ZQ Calibration Short Interval: */
        uint32_t reserved2          :1;
        uint32_t SRXZQCL            :2;             /** < ZQ Calibration Length */
        uint32_t ZQCalType          :1;
        uint32_t ZQCalStart         :1;
        uint32_t TQPollStart        :1;
        uint32_t TQPollRS           :2;
        uint32_t reserved3          :5;
        uint32_t MRRData            :8;             /**< bit[31:24] */
    } field;
} RegDCAL;                                          /**< DRAM Calibration Control*/
#pragma pack()

#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t OOOAGETRH          :5;             /**< Out-of-Order Aging Threshold */
        uint32_t reserved1          :3;
        uint32_t OOODIS             :1;             /**< Out-of-Order Disable */
        uint32_t OOOST3DIS          :1;             /**< Out-of-Order Disabled when RequestBD_Status is 3. */
        uint32_t reserved2          :2;
        uint32_t NEWBYPDIS          :1;
        uint32_t reserved3          :3;
        uint32_t IPREQMAX           :3;             /** < Max In-Progress Requests stored in MC */
        uint32_t reserved4          :13;
    } field;
} RegDSCH;                                           /**< DRAM Scheduler Control Register */
#pragma pack()

#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t DRPLOCK            :1;             /**< DRP lock bit */
        uint32_t reserved1          :7;
        uint32_t REUTLOCK           :1;             /**< REUT lock bit */
        uint32_t reserved2          :19;
        uint32_t PMICTL             :1;             /**< PRI Control Select: 0-memory_manager, 1-hte */
        uint32_t PMIDIS             :1;             /**< PMIDIS Should be set is using IOSF-SB RW */
        uint32_t DIOIC              :1;             /**< DDRIO initialization is complete */
        uint32_t IC                 :1;             /**< D-unit Initialization Complete */
    } field;
} RegDCO;                                           /**< DRAM Controller Operation Register*/
#pragma pack()

#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t SBEEN       :1;             /**< Enable Single Bit Error Detection and Correction */
        uint32_t DBEEN       :1;             /**< Enable Double Bit Error Detection */
        uint32_t CBOEN       :3;             /**< Enable ECC Check Bits Override */
        uint32_t SYNSEL      :2;             /**< ECC Syndrome Bits Select for Observation */
    uint32_t CLRSBECNT   :1;             /**< Clear ECC Single Bit Error Count */
        uint32_t CBOV        :8;             /**< ECC Check Bits Override Value */
        uint32_t reserved1   :1;             /**<  */
        uint32_t ENCBGEN     :1;             /**< Enable Generation of ECC Check Bits */
        uint32_t ENCBGESWIZ  :1;             /**< Enable Same Chip ECC Byte Lane Swizzle */

    } field;
} RegDECCCTRL;                                      /**< DRAM ECC Control Register */
#pragma pack()


#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t FUS_DUN_ECC_DIS              :1;
    uint32_t FUS_DUN_MAX_SUPPORTED_MEMORY :3;
    uint32_t FUS_DUN_MAX_DEVDEN           :2;
    uint32_t RESERVED1                    :1;
    uint32_t FUS_DUN_RANK2_DIS            :1;
    uint32_t FUS_DUN_OOO_DIS              :1;
    uint32_t FUS_DUN_MEMX8_DIS            :1;
    uint32_t FUS_DUN_MEMX16_DIS           :1;
    uint32_t RESERVED2                    :1;
    uint32_t FUS_DUN_1N_DIS               :1;
    uint32_t FUS_DUN_DQ_SCRAMBLER_DIS     :1;
    uint32_t RESERVED3                    :1;
    uint32_t FUS_DUN_32BIT_DRAM_IFC       :1;
    } field;
} RegDFUSESTAT;
#pragma pack()

//
// END DUnit register definition
//
////



////
//
// DRAM Initialization Structures used in JEDEC Message Bus Commands
//

#pragma pack(1)
typedef union {
        uint32_t      raw;
    struct {
        unsigned    command         :3;             /**< Command: 000-MRS,001-Refresh,010-Pre-charge,011-Activate,110-ZQ,111-NOP */
        unsigned    bankAddress     :3;             /**< Bank Address (BA[2:0]) */
        unsigned    BL              :2;             /**< Burst Length, CDV:1*/
        unsigned    CL              :1;             /**< CL Reserved CDV:0 */
        unsigned    RBT             :1;             /**< Read Burst Type */
        unsigned    casLatency      :3;             /**< cas Latency */
        unsigned    TM              :1;             /**< Test mode */
        unsigned    dllReset        :1;             /**< DLL Reset */
        unsigned    writeRecovery   :3;             /**< Write Recovery for Auto Pre-Charge: 001=2,010=3,011=4,100=5,101=6 */
        unsigned    PPD             :1;             /**< DLL Control for Precharge Power-Down CDV:1 */
        unsigned    reserved1       :3;
        unsigned    rankSelect      :4;             /**< Rank Select */
        unsigned    reserved2       :6;
    } field;
} DramInitDDR3MRS0;                                 /**< DDR3 Mode Register Set (MRS) Command */
#pragma pack()

#pragma pack(1)
typedef union {
        uint32_t      raw;
    struct {
        unsigned    command         :3;             /**< Command: 000-MRS,001-Refresh,010-Pre-charge,011-Activate,110-ZQ,111-NOP */
        unsigned    bankAddress     :3;             /**< Bank Address (BA[2:0]) */
        unsigned    dllEnabled      :1;             /**< CDV=0 */
        unsigned    DIC0            :1;             /**< Output Driver Impedance Control */
        unsigned    rttNom0         :1;             /**< RTT_nom[0] */
        unsigned    MRC_AL          :2;             /**< Additive Latency = 0 */
        unsigned    DIC1            :1;             /**< Reserved */
        unsigned    rttNom1         :1;             /**< RTT_nom[1] */
        unsigned    wlEnabled       :1;             /**< Write Leveling Enable */
        unsigned    reserved1       :1;
        unsigned    rttNom2         :1;             /** < RTT_nom[2] */
        unsigned    reserved2       :1;
        unsigned    TDQS            :1;             /**< TDQS Enable */
        unsigned    Qoff            :1;             /**< Output Buffers Disabled */
        unsigned    reserved3       :3;
        unsigned    rankSelect      :4;             /**< Rank Select */
        unsigned    reserved4       :6;
    } field;
} DramInitDDR3EMR1;                                 /**< DDR3 Extended Mode Register 1 Set (EMRS1) Command */
#pragma pack()

#pragma pack(1)
typedef union {
        uint32_t      raw;
    struct {
        uint32_t    command         :3;             /**< Command: 000-MRS,001-Refresh,010-Pre-charge,011-Activate,110-ZQ,111-NOP */
        uint32_t    bankAddress     :3;             /**< Bank Address (BA[2:0]) */
        uint32_t    PASR            :3;             /**< Partial Array Self-Refresh */
        uint32_t    CWL             :3;             /**< CAS Write Latency */
        uint32_t    ASR             :1;             /**< Auto Self-Refresh */
        uint32_t    SRT             :1;             /**< SR Temperature Range = 0*/
        uint32_t    reserved1       :1;
        uint32_t    rtt_WR          :2;             /**< Rtt_WR */
        uint32_t    reserved2       :5;
        uint32_t    rankSelect      :4;             /**< Rank Select */
        uint32_t    reserved3       :6;
    } field;
} DramInitDDR3EMR2;                                 /**< DDR3 Extended Mode Register 2 Set (EMRS2) Command */
#pragma pack()

#pragma pack(1)
typedef union {
        uint32_t      raw;
    struct {
        uint32_t    command         :3;             /**< Command: 000-MRS,001-Refresh,010-Pre-charge,011-Activate,110-ZQ,111-NOP */
        uint32_t    bankAddress     :3;             /**< Bank Address (BA[2:0]) */
        uint32_t    MPR_Location    :2;             /**< MPR Location */
        uint32_t    MPR             :1;             /**< MPR: Multi Purpose Register */
        uint32_t    reserved1       :13;
        uint32_t    rankSelect      :4;             /**< Rank Select */
        uint32_t    reserved2       :6;
    } field;
} DramInitDDR3EMR3;                                 /**< DDR3 Extended Mode Register 2 Set (EMRS2) Command */
#pragma pack()

#pragma pack(1)
typedef union {
    uint32_t    raw;
    struct {
        uint32_t    command         :3;             /**< Command: 000-MRS,001-Refresh,010-Pre-charge,011-Activate,110 - ZQ Calibration,111-NOP */
        uint32_t    bankAddress     :3;             /**< Bank Address (BA[2:0]) */
        uint32_t    multAddress     :16;            /**< Multiplexed Address (MA[14:0]) */
        uint32_t    rankSelect      :2;             /**< Rank Select */
        uint32_t    reserved3       :8;
    } field;
} DramInitMisc;                                     /**< Miscellaneous DDRx Initialization Command */
#pragma pack()

//
// Construct DRAM init command using DramInitXxxx pattern
//
#define DCMD_MRS1(rnk,dat) (0 | ((rnk)<<22) | (1<<3) | ((dat)<<6))
#define DCMD_REF(rnk)      (1 | ((rnk)<<22))
#define DCMD_PRE(rnk)      (2 | ((rnk)<<22))
#define DCMD_PREA(rnk)     (2 | ((rnk)<<22) | (BIT10<<6))
#define DCMD_ACT(rnk,row)  (3 | ((rnk)<<22) | ((row)<<6))
#define DCMD_WR(rnk,col)   (4 | ((rnk)<<22) | ((col)<<6))
#define DCMD_RD(rnk,col)   (5 | ((rnk)<<22) | ((col)<<6))
#define DCMD_ZQCS(rnk)     (6 | ((rnk)<<22))
#define DCMD_ZQCL(rnk)     (6 | ((rnk)<<22) | (BIT10<<6))
#define DCMD_NOP(rnk)      (7 | ((rnk)<<22))




#define DDR3_EMRS1_DIC_40       (0)
#define DDR3_EMRS1_DIC_34       (1)

#define DDR3_EMRS2_RTTWR_60     (BIT9)
#define DDR3_EMRS2_RTTWR_120    (BIT10)

#define DDR3_EMRS1_RTTNOM_0     (0)
#define DDR3_EMRS1_RTTNOM_60    (BIT2)
#define DDR3_EMRS1_RTTNOM_120   (BIT6)
#define DDR3_EMRS1_RTTNOM_40    (BIT6|BIT2)
#define DDR3_EMRS1_RTTNOM_20    (BIT9)
#define DDR3_EMRS1_RTTNOM_30    (BIT9|BIT2)


//
// END DRAM Init...
//
////


// HOST_BRIDGE registers:
#define HMBOUND             0x0020  //ok

// MEMORY_MANAGER registers:
#define BCTRL               0x0004
#define BWFLUSH             0x0008
#define BDEBUG1             0x00C4

////
//
// BEGIN DDRIO registers
//

// DDR IOs & COMPs:
#define DDRIODQ_BL_OFFSET   0x0800
#define DDRIODQ_CH_OFFSET   ((NUM_BYTE_LANES/2) * DDRIODQ_BL_OFFSET)
#define DDRIOCCC_CH_OFFSET  0x0800
#define DDRCOMP_CH_OFFSET   0x0100

// CH0-BL01-DQ
#define DQOBSCKEBBCTL       0x0000
#define DQDLLTXCTL          0x0004
#define DQDLLRXCTL          0x0008
#define DQMDLLCTL           0x000C
#define B0RXIOBUFCTL        0x0010
#define B0VREFCTL           0x0014
#define B0RXOFFSET1         0x0018
#define B0RXOFFSET0         0x001C
#define B1RXIOBUFCTL        0x0020
#define B1VREFCTL           0x0024
#define B1RXOFFSET1         0x0028
#define B1RXOFFSET0         0x002C
#define DQDFTCTL            0x0030
#define DQTRAINSTS          0x0034
#define B1DLLPICODER0       0x0038
#define B0DLLPICODER0       0x003C
#define B1DLLPICODER1       0x0040
#define B0DLLPICODER1       0x0044
#define B1DLLPICODER2       0x0048
#define B0DLLPICODER2       0x004C
#define B1DLLPICODER3       0x0050
#define B0DLLPICODER3       0x0054
#define B1RXDQSPICODE       0x0058
#define B0RXDQSPICODE       0x005C
#define B1RXDQPICODER32     0x0060
#define B1RXDQPICODER10     0x0064
#define B0RXDQPICODER32     0x0068
#define B0RXDQPICODER10     0x006C
#define B01PTRCTL0          0x0070
#define B01PTRCTL1          0x0074
#define B01DBCTL0           0x0078
#define B01DBCTL1           0x007C
#define B0LATCTL0           0x0080
#define B1LATCTL0           0x0084
#define B01LATCTL1          0x0088
#define B0ONDURCTL          0x008C
#define B1ONDURCTL          0x0090
#define B0OVRCTL            0x0094
#define B1OVRCTL            0x0098
#define DQCTL               0x009C
#define B0RK2RKCHGPTRCTRL   0x00A0
#define B1RK2RKCHGPTRCTRL   0x00A4
#define DQRK2RKCTL          0x00A8
#define DQRK2RKPTRCTL       0x00AC
#define B0RK2RKLAT          0x00B0
#define B1RK2RKLAT          0x00B4
#define DQCLKALIGNREG0      0x00B8
#define DQCLKALIGNREG1      0x00BC
#define DQCLKALIGNREG2      0x00C0
#define DQCLKALIGNSTS0      0x00C4
#define DQCLKALIGNSTS1      0x00C8
#define DQCLKGATE           0x00CC
#define B0COMPSLV1          0x00D0
#define B1COMPSLV1          0x00D4
#define B0COMPSLV2          0x00D8
#define B1COMPSLV2          0x00DC
#define B0COMPSLV3          0x00E0
#define B1COMPSLV3          0x00E4
#define DQVISALANECR0TOP    0x00E8
#define DQVISALANECR1TOP    0x00EC
#define DQVISACONTROLCRTOP  0x00F0
#define DQVISALANECR0BL     0x00F4
#define DQVISALANECR1BL     0x00F8
#define DQVISACONTROLCRBL   0x00FC
#define DQTIMINGCTRL        0x010C
// CH0-ECC
#define ECCDLLTXCTL         0x2004
#define ECCDLLRXCTL         0x2008
#define ECCMDLLCTL          0x200C
#define ECCB1DLLPICODER0    0x2038
#define ECCB1DLLPICODER1    0x2040
#define ECCB1DLLPICODER2    0x2048
#define ECCB1DLLPICODER3    0x2050
#define ECCB01DBCTL0        0x2078
#define ECCB01DBCTL1        0x207C
#define ECCCLKALIGNREG0     0x20B8
#define ECCCLKALIGNREG1     0x20BC
#define ECCCLKALIGNREG2     0x20C0
// CH0-CMD
#define CMDOBSCKEBBCTL      0x4800
#define CMDDLLTXCTL         0x4808
#define CMDDLLRXCTL         0x480C
#define CMDMDLLCTL          0x4810
#define CMDRCOMPODT         0x4814
#define CMDDLLPICODER0      0x4820
#define CMDDLLPICODER1      0x4824
#define CMDCFGREG0          0x4840
#define CMDPTRREG           0x4844
#define CMDCLKALIGNREG0     0x4850
#define CMDCLKALIGNREG1     0x4854
#define CMDCLKALIGNREG2     0x4858
#define CMDPMCONFIG0        0x485C
#define CMDPMDLYREG0        0x4860
#define CMDPMDLYREG1        0x4864
#define CMDPMDLYREG2        0x4868
#define CMDPMDLYREG3        0x486C
#define CMDPMDLYREG4        0x4870
#define CMDCLKALIGNSTS0     0x4874
#define CMDCLKALIGNSTS1     0x4878
#define CMDPMSTS0           0x487C
#define CMDPMSTS1           0x4880
#define CMDCOMPSLV          0x4884
#define CMDBONUS0           0x488C
#define CMDBONUS1           0x4890
#define CMDVISALANECR0      0x4894
#define CMDVISALANECR1      0x4898
#define CMDVISACONTROLCR    0x489C
#define CMDCLKGATE          0x48A0
#define CMDTIMINGCTRL       0x48A4
// CH0-CLK-CTL
#define CCOBSCKEBBCTL       0x5800
#define CCRCOMPIO           0x5804
#define CCDLLTXCTL          0x5808
#define CCDLLRXCTL          0x580C
#define CCMDLLCTL           0x5810
#define CCRCOMPODT          0x5814
#define CCDLLPICODER0       0x5820
#define CCDLLPICODER1       0x5824
#define CCDDR3RESETCTL      0x5830
#define CCCFGREG0           0x5838
#define CCCFGREG1           0x5840
#define CCPTRREG            0x5844
#define CCCLKALIGNREG0      0x5850
#define CCCLKALIGNREG1      0x5854
#define CCCLKALIGNREG2      0x5858
#define CCPMCONFIG0         0x585C
#define CCPMDLYREG0         0x5860
#define CCPMDLYREG1         0x5864
#define CCPMDLYREG2         0x5868
#define CCPMDLYREG3         0x586C
#define CCPMDLYREG4         0x5870
#define CCCLKALIGNSTS0      0x5874
#define CCCLKALIGNSTS1      0x5878
#define CCPMSTS0            0x587C
#define CCPMSTS1            0x5880
#define CCCOMPSLV1          0x5884
#define CCCOMPSLV2          0x5888
#define CCCOMPSLV3          0x588C
#define CCBONUS0            0x5894
#define CCBONUS1            0x5898
#define CCVISALANECR0       0x589C
#define CCVISALANECR1       0x58A0
#define CCVISACONTROLCR     0x58A4
#define CCCLKGATE           0x58A8
#define CCTIMINGCTL         0x58AC
// COMP
#define CMPCTRL             0x6800
#define SOFTRSTCNTL         0x6804
#define MSCNTR              0x6808
#define NMSCNTRL            0x680C
#define LATCH1CTL           0x6814
#define COMPVISALANECR0     0x681C
#define COMPVISALANECR1     0x6820
#define COMPVISACONTROLCR   0x6824
#define COMPBONUS0          0x6830
#define TCOCNTCTRL          0x683C
#define DQANAODTPUCTL       0x6840
#define DQANAODTPDCTL       0x6844
#define DQANADRVPUCTL       0x6848
#define DQANADRVPDCTL       0x684C
#define DQANADLYPUCTL       0x6850
#define DQANADLYPDCTL       0x6854
#define DQANATCOPUCTL       0x6858
#define DQANATCOPDCTL       0x685C
#define CMDANADRVPUCTL      0x6868
#define CMDANADRVPDCTL      0x686C
#define CMDANADLYPUCTL      0x6870
#define CMDANADLYPDCTL      0x6874
#define CLKANAODTPUCTL      0x6880
#define CLKANAODTPDCTL      0x6884
#define CLKANADRVPUCTL      0x6888
#define CLKANADRVPDCTL      0x688C
#define CLKANADLYPUCTL      0x6890
#define CLKANADLYPDCTL      0x6894
#define CLKANATCOPUCTL      0x6898
#define CLKANATCOPDCTL      0x689C
#define DQSANAODTPUCTL      0x68A0
#define DQSANAODTPDCTL      0x68A4
#define DQSANADRVPUCTL      0x68A8
#define DQSANADRVPDCTL      0x68AC
#define DQSANADLYPUCTL      0x68B0
#define DQSANADLYPDCTL      0x68B4
#define DQSANATCOPUCTL      0x68B8
#define DQSANATCOPDCTL      0x68BC
#define CTLANADRVPUCTL      0x68C8
#define CTLANADRVPDCTL      0x68CC
#define CTLANADLYPUCTL      0x68D0
#define CTLANADLYPDCTL      0x68D4
#define CHNLBUFSTATIC       0x68F0
#define COMPOBSCNTRL        0x68F4
#define COMPBUFFDBG0        0x68F8
#define COMPBUFFDBG1        0x68FC
#define CFGMISCCH0          0x6900
#define COMPEN0CH0          0x6904
#define COMPEN1CH0          0x6908
#define COMPEN2CH0          0x690C
#define STATLEGEN0CH0       0x6910
#define STATLEGEN1CH0       0x6914
#define DQVREFCH0           0x6918
#define CMDVREFCH0          0x691C
#define CLKVREFCH0          0x6920
#define DQSVREFCH0          0x6924
#define CTLVREFCH0          0x6928
#define TCOVREFCH0          0x692C
#define DLYSELCH0           0x6930
#define TCODRAMBUFODTCH0    0x6934
#define CCBUFODTCH0         0x6938
#define RXOFFSETCH0         0x693C
#define DQODTPUCTLCH0       0x6940
#define DQODTPDCTLCH0       0x6944
#define DQDRVPUCTLCH0       0x6948
#define DQDRVPDCTLCH0       0x694C
#define DQDLYPUCTLCH0       0x6950
#define DQDLYPDCTLCH0       0x6954
#define DQTCOPUCTLCH0       0x6958
#define DQTCOPDCTLCH0       0x695C
#define CMDDRVPUCTLCH0      0x6968
#define CMDDRVPDCTLCH0      0x696C
#define CMDDLYPUCTLCH0      0x6970
#define CMDDLYPDCTLCH0      0x6974
#define CLKODTPUCTLCH0      0x6980
#define CLKODTPDCTLCH0      0x6984
#define CLKDRVPUCTLCH0      0x6988
#define CLKDRVPDCTLCH0      0x698C
#define CLKDLYPUCTLCH0      0x6990
#define CLKDLYPDCTLCH0      0x6994
#define CLKTCOPUCTLCH0      0x6998
#define CLKTCOPDCTLCH0      0x699C
#define DQSODTPUCTLCH0      0x69A0
#define DQSODTPDCTLCH0      0x69A4
#define DQSDRVPUCTLCH0      0x69A8
#define DQSDRVPDCTLCH0      0x69AC
#define DQSDLYPUCTLCH0      0x69B0
#define DQSDLYPDCTLCH0      0x69B4
#define DQSTCOPUCTLCH0      0x69B8
#define DQSTCOPDCTLCH0      0x69BC
#define CTLDRVPUCTLCH0      0x69C8
#define CTLDRVPDCTLCH0      0x69CC
#define CTLDLYPUCTLCH0      0x69D0
#define CTLDLYPDCTLCH0      0x69D4
#define FNLUPDTCTLCH0       0x69F0
// PLL
#define MPLLCTRL0           0x7800
#define MPLLCTRL1           0x7808
#define MPLLCSR0            0x7810
#define MPLLCSR1            0x7814
#define MPLLCSR2            0x7820
#define MPLLDFT             0x7828
#define MPLLMON0CTL         0x7830
#define MPLLMON1CTL         0x7838
#define MPLLMON2CTL         0x783C
#define SFRTRIM             0x7850
#define MPLLDFTOUT0         0x7858
#define MPLLDFTOUT1         0x785C
#define MASTERRSTN          0x7880
#define PLLLOCKDEL          0x7884
#define SFRDEL              0x7888
#define CRUVISALANECR0      0x78F0
#define CRUVISALANECR1      0x78F4
#define CRUVISACONTROLCR    0x78F8
#define IOSFVISALANECR0     0x78FC
#define IOSFVISALANECR1     0x7900
#define IOSFVISACONTROLCR   0x7904

//
// END DDRIO registers
//
////


#endif
