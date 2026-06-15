/** @file SmmuV3Registers.h

    This file is the SmmuV3Registers header file for SMMU driver.

    Contains all relevant register definitions for SMMUv3 found per the spec:
    <https://developer.arm.com/documentation/ihi0070/latest/>

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMMUV3_REGISTERS_H_
#define SMMUV3_REGISTERS_H_

typedef enum _SMMUV3_REVISION {
  SmmuV3Rev0,
  SmmuV3Rev1,
  SmmuV3Rev2,
  SmmuV3Rev3,
} SMMUV3_REVISION;

//
// ------------------------------------------------------ Data Type Definitions
//

//
// SMMUv3 ID registers (IDR0 - IDR5, IIDR, AIDR).
//

typedef union _SMMUV3_IDR0 {
  struct {
    UINT32    S2p        : 1;
    UINT32    S1p        : 1;
    UINT32    Ttf        : 2;
    UINT32    Cohacc     : 1;
    UINT32    Btm        : 1;
    UINT32    Httu       : 2;
    UINT32    DormHint   : 1;
    UINT32    Hyp        : 1;
    UINT32    Ats        : 1;
    UINT32    Ns1Ats     : 1;
    UINT32    Asid16     : 1;
    UINT32    Msi        : 1;
    UINT32    Sev        : 1;
    UINT32    Atos       : 1;
    UINT32    Pri        : 1;
    UINT32    Vmw        : 1;
    UINT32    Vmid16     : 1;
    UINT32    Cd2L       : 1;
    UINT32    Vatos      : 1;
    UINT32    Ttendian   : 2;
    UINT32    Reserved0  : 1;
    UINT32    StallModel : 2;
    UINT32    TermModel  : 1;
    UINT32    StLevel    : 2;
    UINT32    Reserved1  : 3;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_IDR0;

typedef union _SMMUV3_IDR1 {
  struct {
    UINT32    SidSize      : 6;
    UINT32    SSidSize     : 5;
    UINT32    PriQs        : 5;
    UINT32    EventQs      : 5;
    UINT32    CmdQs        : 5;
    UINT32    AttrPermsOvr : 1;
    UINT32    AttrTypesOvr : 1;
    UINT32    Rel          : 1;
    UINT32    QueuesPreset : 1;
    UINT32    TablesPreset : 1;
    UINT32    Ecmdq        : 1;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_IDR1;

#define SMMUV3_VATOS_REGION_OFFSET      0x20000
#define SMMUV3_VATOS_REGION_UNIT_SIZE   0x10000
#define SMMUV3_VATOS_REGION_TOTAL_SIZE  0x10000

typedef union _SMMUV3_IDR2 {
  struct {
    UINT32    BaVatos  : 10;
    UINT32    Reserved : 22;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_IDR2;

typedef union _SMMUV3_IDR3 {
  struct {
    UINT32    Reserved0 : 2;
    UINT32    Had       : 1;
    UINT32    Pbha      : 1;
    UINT32    Xnx       : 1;
    UINT32    Pps       : 1;
    UINT32    Reserved1 : 1;
    UINT32    Mpam      : 1;
    UINT32    Fwb       : 1;
    UINT32    Stt       : 1;
    UINT32    Ril       : 1;
    UINT32    Bbml      : 2;
    UINT32    Reserved2 : 19;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_IDR3;

typedef union _SMMUV3_IDR4 {
  struct {
    UINT32    Impl;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_IDR4;

typedef union _SMMUV3_IDR5 {
  struct {
    UINT32    Oas       : 3;
    UINT32    Reserved0 : 1;
    UINT32    Gran4k    : 1;
    UINT32    Gran16k   : 1;
    UINT32    Gran64k   : 1;
    UINT32    Reserved1 : 3;
    UINT32    Vax       : 2;
    UINT32    Reserved2 : 4;
    UINT32    StallMax  : 16;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_IDR5;

typedef union _SMMUV3_IIDR {
  struct {
    UINT32    Implementer : 12;
    UINT32    Revision    : 4;
    UINT32    Variant     : 4;
    UINT32    ProductId   : 12;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_IIDR;

typedef union _SMMUV3_AIDR {
  struct {
    UINT32    ArchMinorRev : 4;
    UINT32    ArchMajorRev : 4;
    UINT32    Reserved     : 24;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_AIDR;

//
// SMMUv3 control registers (CR0 - CR2).
//

#define SMMUV3_CR0_VALID_MASK                   (0x5FUL)
#define SMMUV3_CR0_SMMU_CMDQ_EVTQ_PRIQ_EN_MASK  (0xFUL)
#define SMMUV3_CR0_SMMU_EN_MASK                 (0x1UL)

typedef union _SMMUV3_CR0 {
  struct {
    UINT32    SmmuEn    : 1;
    UINT32    PriQEn    : 1;
    UINT32    EventQEn  : 1;
    UINT32    CmdQEn    : 1;
    UINT32    AtsChk    : 1;
    UINT32    Reserved0 : 1;
    UINT32    Vmw       : 3;
    UINT32    Reserved  : 23;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_CR0;

//
// The CR0ACK register has the same format as CR0.
//

typedef SMMUV3_CR0      _SMMUV3_CR0ACK;
typedef _SMMUV3_CR0ACK  SMMUV3_CR0ACK;

#define SMMUV3_CR1_VALID_MASK  (0x3FUL)

typedef union _SMMUV3_CR1 {
  struct {
    UINT32    QueueIc  : 2;
    UINT32    QueueOc  : 2;
    UINT32    QueueSh  : 2;
    UINT32    TableIc  : 2;
    UINT32    TableOc  : 2;
    UINT32    TableSh  : 2;
    UINT32    Reserved : 20;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_CR1;

#define SMMUV3_CR2_VALID_MASK  (0x7UL)

typedef union _SMMUV3_CR2 {
  struct {
    UINT32    E2h       : 1;
    UINT32    RecInvSid : 1;
    UINT32    Ptm       : 1;
    UINT32    Reserved  : 29;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_CR2;

typedef union _SMMUV3_GBPA {
  struct {
    UINT32    MemAttr   : 4;
    UINT32    Mtcfg     : 1;
    UINT32    Reserved0 : 3;
    UINT32    AllocCfg  : 4;
    UINT32    ShCfg     : 2;
    UINT32    Reserved1 : 2;
    UINT32    PrivCfg   : 2;
    UINT32    InstCfg   : 2;
    UINT32    Abort     : 1;
    UINT32    Reserved2 : 10;
    UINT32    Update    : 1;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_GBPA;

typedef struct _SMMUV3_AGBPA {
  UINT32    Impl;
} SMMUV3_AGBPA;

typedef union _SMMUV3_STATUSR {
  struct {
    UINT32    Dormant  : 1;
    UINT32    Reserved : 31;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_STATUSR;

//
// Global error control and IRQ configuration registers.
//

#define SMMUV3_IRQ_CTRL_GLOBAL_PRIQ_EVTQ_EN_MASK  (0x7UL)

typedef union _SMMUV3_IRQ_CTRL {
  struct {
    UINT32    GlobalErrorIrqEn : 1;
    UINT32    PriqIrqEn        : 1;
    UINT32    EventqIrqEn      : 1;
    UINT32    Reserved         : 29;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_IRQ_CTRL;

typedef SMMUV3_IRQ_CTRL      _SMMUV3_IRQ_CTRLACK;
typedef _SMMUV3_IRQ_CTRLACK  SMMUV3_IRQ_CTRLACK;

//
// Define a mask of the valid bits within the GERROR register.
//

#define SMMUV3_GERROR_VALID_MASK      (0x1FDUL)
#define SMMUV3_GERROR_SFM_ERROR_MASK  (0x100UL)

typedef union _SMMUV3_GERROR {
  struct {
    UINT32    CmdqErr         : 1;
    UINT32    Reserved0       : 1;
    UINT32    EventqAbtErr    : 1;
    UINT32    PriqAbtErr      : 1;
    UINT32    MsiCmdqAbtErr   : 1;
    UINT32    MsiEventqAbtErr : 1;
    UINT32    MsiPriqAbtErr   : 1;
    UINT32    MsiGerrorAbtErr : 1;
    UINT32    SfmErr          : 1;
    UINT32    Reserved        : 23;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_GERROR;

typedef SMMUV3_GERROR    _SMMUV3_GERRORN;
typedef _SMMUV3_GERRORN  SMMUV3_GERRORN;

typedef union _SMMUV3_GERROR_IRQ_CFG0 {
  struct {
    UINT64    Reserved  : 2;
    UINT64    Addr      : 50;
    UINT64    Reserved1 : 12;
  } Bits;
  UINT64    AsUINT64;
} SMMUV3_GERROR_IRQ_CFG0;

typedef struct _SMMUV3_GERROR_IRQ_CFG1 {
  UINT32    Data;
} SMMUV3_GERROR_IRQ_CFG1;

typedef union _SMMUV3_GERROR_IRQ_CFG2 {
  struct {
    UINT32    MemAttr  : 4;
    UINT32    Sh       : 2;
    UINT32    Reserved : 26;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_GERROR_IRQ_CFG2;

//
// Stream table base and configuration registers.
//

typedef union _SMMUV3_STRTAB_BASE {
  struct {
    UINT64    Reserved0 : 6;
    UINT64    Addr      : 46;
    UINT64    Reserved1 : 10;
    UINT64    Ra        : 1;
    UINT64    Reserved2 : 1;
  } Bits;
  UINT64    AsUINT64;
} SMMUV3_STRTAB_BASE;

typedef union _SMMUV3_STRTAB_BASE_CFG {
  struct {
    UINT32    Log2Size  : 6;
    UINT32    Split     : 5;
    UINT32    Reserved0 : 5;
    UINT32    Fmt       : 2;
    UINT32    Reserved2 : 14;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_STRTAB_BASE_CFG;

//
// Command queue base, producer and consumer index registers.
//

typedef union _SMMUV3_CMDQ_BASE {
  struct {
    UINT64    Log2Size  : 5;
    UINT64    Addr      : 47;
    UINT64    Reserved0 : 10;
    UINT64    Ra        : 1;
    UINT64    Reserved1 : 1;
  } Bits;
  UINT64    AsUINT64;
} SMMUV3_CMDQ_BASE;

typedef union _SMMUV3_CMDQ_CONS {
  struct {
    UINT32    ReadIndex : 20;
    UINT32    Reserved0 : 4;
    UINT32    Err       : 7;
    UINT32    Reserved1 : 1;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_CMDQ_CONS;

typedef union _SMMUV3_CMDQ_PROD {
  struct {
    UINT32    WriteIndex : 20;
    UINT32    Reserved   : 12;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_CMDQ_PROD;

//
// Event queue base, producer/consumer, and IRQ configuration registers.
//

typedef union SMMUV3_EVENTQ_BASE {
  struct {
    UINT64    Log2Size  : 5;
    UINT64    Addr      : 47;
    UINT64    Reserved0 : 10;
    UINT64    Wa        : 1;
    UINT64    Reserved1 : 1;
  } Bits;
  UINT64    AsUINT64;
} SMMUV3_EVENTQ_BASE;

typedef union SMMUV3_EVENTQ_CONS {
  struct {
    UINT32    ReadIndex : 20;
    UINT32    Reserved  : 11;
    UINT32    OvAckFlag : 1;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_EVENTQ_CONS;

typedef union SMMUV3_EVENTQ_PROD {
  struct {
    UINT32    WriteIndex : 20;
    UINT32    Reserved   : 11;
    UINT32    OvFlag     : 1;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_EVENTQ_PROD;

typedef union _SMMUV3_EVENTQ_IRQ_CFG0 {
  struct {
    UINT64    Reserved  : 2;
    UINT64    Addr      : 50;
    UINT64    Reserved1 : 12;
  } Bits;
  UINT64    AsUINT64;
} SMMUV3_EVENTQ_IRQ_CFG0;

typedef struct _SMMUV3_EVENTQ_IRQ_CFG1 {
  UINT32    Data;
} SMMUV3_EVENTQ_IRQ_CFG1;

typedef union _SMMUV3_EVENTQ_IRQ_CFG2 {
  struct {
    UINT32    MemAttr  : 4;
    UINT32    Sh       : 2;
    UINT32    Reserved : 26;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_EVENTQ_IRQ_CFG2;

//
// PRI queue base, producer/consumer, and IRQ configuration registers.
//

typedef union _SMMUV3_PRIQ_BASE {
  struct {
    UINT64    Log2Size  : 5;
    UINT64    Addr      : 47;
    UINT64    Reserved0 : 10;
    UINT64    Wa        : 1;
    UINT64    Reserved1 : 1;
  } Bits;
  UINT64    AsUINT64;
} SMMUV3_PRIQ_BASE;

typedef union _SMMUV3_PRIQ_CONS {
  struct {
    UINT32    ReadIndex : 20;
    UINT32    Reserved  : 11;
    UINT32    OvAckFlg  : 1;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_PRIQ_CONS;

typedef union _SMMUV3_PRIQ_PROD {
  struct {
    UINT32    WriteIndex : 20;
    UINT32    Reserved   : 11;
    UINT32    OvFlg      : 1;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_PRIQ_PROD;

typedef union _SMMUV3_PRIQ_IRQ_CFG0 {
  struct {
    UINT64    Reserved  : 2;
    UINT64    Addr      : 50;
    UINT64    Reserved1 : 12;
  } Bits;
  UINT64    AsUINT64;
} SMMUV3_PRIQ_IRQ_CFG0;

typedef struct _SMMUV3_PRIQ_IRQ_CFG1 {
  UINT32    Data;
} SMMUV3_PRIQ_IRQ_CFG1;

typedef union _SMMUV3_PRIQ_IRQ_CFG2 {
  struct {
    UINT32    MemAttr  : 4;
    UINT32    Sh       : 2;
    UINT32    Reserved : 26;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_PRIQ_IRQ_CFG2;

//
// ATOS (Address Translation) control and configuration registers.
//

typedef union _SMMUV3_GATOS_CTRL {
  struct {
    UINT32    Run      : 1;
    UINT32    Reserved : 31;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_GATOS_CTRL;

typedef union _SMMUV3_GATOS_SID {
  struct {
    UINT64    StreamId    : 32;
    UINT64    SubstreamId : 20;
    UINT64    SsidValid   : 1;
    UINT64    Reserved    : 11;
  } Bits;
  UINT64    AsUINT64;
} SMMUV3_GATOS_SID;

typedef union _SMMUV3_GATOS_ADDR {
  struct {
    UINT64    Reserved                  : 6;
    UINT64    HttuI                     : 1;
    UINT64    InstructionNotData        : 1;
    UINT64    ReadNotWrite              : 1;
    UINT64    PrivilegedNotUnprivileged : 1;
    UINT64    Type                      : 2;
    UINT64    Addr                      : 52;
  } Bits;
  UINT64    AsUINT64;
} SMMUV3_GATOS_ADDR;

typedef union _SMMUV3_GATOS_PAR {
  struct {
    UINT64    Fault     : 1;
    UINT64    Reserved0 : 7;
    UINT64    Sh        : 2;
    UINT64    Reserved1 : 1;
    UINT64    Size      : 1;
    UINT64    Addr      : 40;
    UINT64    Reserved2 : 4;
    UINT64    Attr      : 8;
  } NoFault;

  struct {
    UINT64    Fault     : 1;
    UINT64    Reason    : 2;
    UINT64    Reserved0 : 1;
    UINT64    FaultCode : 8;
    UINT64    Faddr     : 40;
    UINT64    Reserved1 : 8;
    UINT64    ImpDef    : 4;
  } Fault;

  UINT64    AsUINT64;
} SMMUV3_GATOS_PAR;

typedef union _SMMUV3_MPAMIDR {
  struct {
    UINT32    PartIdMax : 16;
    UINT32    PmgMax    : 8;
    UINT32    Reserved  : 8;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_MPAMIDR;

typedef union _SMMUV3_GMPAM {
  struct {
    UINT32    SoPartId : 16;
    UINT32    SoPmg    : 8;
    UINT32    Reserved : 7;
    UINT32    Update   : 1;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_GMPAM;

typedef union _SMMUV3_GBPMPAM {
  struct {
    UINT32    GbpPartId : 16;
    UINT32    GbpPmg    : 8;
    UINT32    Reserved  : 7;
    UINT32    Update    : 1;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_GBPMPAM;

typedef union _SMMUV3_VATOS_SEL {
  struct {
    UINT32    Vmid     : 16;
    UINT32    Reserved : 16;
  } Bits;
  UINT32    AsUINT32;
} SMMUV3_VATOS_SEL;

//
// SMMUv3 (component and peripheral) ID registers. These are laid out as follows
// in Page 0:
// Offsets 0xFF0 - 0xFFC: CIDR0 to CIDR3
// Offsets 0xFD0 - 0xFDC - PIDR4 to PIDR7
// Offsets 0xFE0 - 0xFEC - PIDR0 to PIDR3
//

typedef struct _SMMUV3_CIDRS {
  UINT32    Cidr0;
  UINT32    Cidr1;
  UINT32    Cidr2;
  UINT32    Cidr3;
} SMMUV3_CIDRS;

typedef union _SMMUV3_PIDR_4_5 {
  struct {
    UINT64    Des2      : 4;
    UINT64    Size      : 4;
    UINT64    Reserved0 : 24;
    UINT64    Reserved1 : 32;
  } Bits;

  UINT64    AsUINT64;
} SMMUV3_PIDR_4_5;

typedef union _SMMUV3_PIDR_0_1 {
  struct {
    UINT64    Part0     : 8;
    UINT64    Reserved0 : 24;
    UINT64    Part1     : 4;
    UINT64    Des0      : 4;
    UINT64    Reserved1 : 24;
  } Bits;

  UINT64    AsUINT64;
} SMMUV3_PIDR_0_1;

typedef union _SMMUV3_PIDR_2_3 {
  struct {
    UINT64    Des1      : 3;
    UINT64    JedecId   : 1;
    UINT64    Revision  : 4;
    UINT64    Reserved0 : 24;
    UINT64    Cmod      : 4;
    UINT64    Revand    : 4;
    UINT64    Reserved1 : 24;
  } Bits;

  UINT64    AsUINT64;
} SMMUV3_PIDR_2_3;

typedef struct _SMMUV3_PIDRS {
  SMMUV3_PIDR_4_5    Pidr4_5;
  UINT64             Pidr6_7;
  SMMUV3_PIDR_0_1    Pidr0_1;
  SMMUV3_PIDR_2_3    Pidr2_3;
} SMMUV3_PIDRS;

typedef struct _SMMUV3_ID_REGS {
  SMMUV3_PIDRS    Pidrs;
  SMMUV3_CIDRS    Cidrs;
} SMMUV3_ID_REGS;

//
// Page 0 SMMUv3 register layout.
//

typedef struct _SMMUV3_REGISTER_LAYOUT_PAGE0 {
  SMMUV3_IDR0               Idr0;
  SMMUV3_IDR1               Idr1;
  SMMUV3_IDR2               Idr2;
  SMMUV3_IDR3               Idr3;
  SMMUV3_IDR4               Idr4;
  SMMUV3_IDR5               Idr5;
  SMMUV3_IIDR               Iidr;
  SMMUV3_AIDR               Aidr;
  SMMUV3_CR0                Cr0;
  SMMUV3_CR0ACK             Cr0Ack;
  SMMUV3_CR1                Cr1;
  SMMUV3_CR2                Cr2;
  UINT32                    Reserved0[4];
  SMMUV3_STATUSR            StatusR;
  SMMUV3_GBPA               Gbpa;
  SMMUV3_AGBPA              Agbpa;
  UINT32                    Reserved1[1];
  SMMUV3_IRQ_CTRL           IrqCtrl;
  SMMUV3_IRQ_CTRLACK        IrqCtrlAck;
  UINT32                    Reserved2[2];
  SMMUV3_GERROR             GError;
  SMMUV3_GERRORN            GErrorN;
  SMMUV3_GERROR_IRQ_CFG0    GErrorIrqCfg0;
  SMMUV3_GERROR_IRQ_CFG1    GErrorIrqCfg1;
  SMMUV3_GERROR_IRQ_CFG2    GErrorIrqCfg2;
  UINT32                    Reserved3[2];
  SMMUV3_STRTAB_BASE        StrTabBase;
  SMMUV3_STRTAB_BASE_CFG    StrTabBaseCfg;
  UINT32                    Reserved4[1];
  SMMUV3_CMDQ_BASE          CmdQBase;
  SMMUV3_CMDQ_PROD          CmdQProd;
  SMMUV3_CMDQ_CONS          CmdQCons;
  SMMUV3_EVENTQ_BASE        EventQBase;
  UINT32                    Reserved5[2]; // Aliases SMMU_EVENTQ_PROD + SMMU_EVENTQ_CONS
  SMMUV3_EVENTQ_IRQ_CFG0    EventQIrqCfg0;
  SMMUV3_EVENTQ_IRQ_CFG1    EventQIrqCfg1;
  SMMUV3_EVENTQ_IRQ_CFG2    EventQIrqCfg2;
  SMMUV3_PRIQ_BASE          PriQBase;
  UINT32                    Reserved6[2]; // Aliases SMMU_PRIQ_PROD + SMMU_PRIQ_CONS
  SMMUV3_PRIQ_IRQ_CFG0      PriQIrqCfg0;
  SMMUV3_PRIQ_IRQ_CFG1      PriQIrqCfg1;
  SMMUV3_PRIQ_IRQ_CFG2      PriQIrqCfg2;
  UINT32                    Reserved7[8];
  SMMUV3_GATOS_CTRL         GatosCtrl;
  UINT32                    Reserved8[1];
  SMMUV3_GATOS_SID          GatosSid;
  SMMUV3_GATOS_ADDR         GatosAddr;
  SMMUV3_GATOS_PAR          GatosPar;
  UINT32                    Reserved9[4];
  SMMUV3_MPAMIDR            Mpamidr;
  SMMUV3_GMPAM              Gmpam;
  SMMUV3_GBPMPAM            Gbpmpam;
  UINT32                    Reserved10[17];
  SMMUV3_VATOS_SEL          VatosSel;
  UINT32                    Reserved11[799];
  UINT32                    Impl0[64];
  UINT32                    Reserved12[52];
  SMMUV3_ID_REGS            IdRegs;

  //
  // Rest are implementation defined registers and registers for secure
  // state management. Left undefined unless needed.
  //
} SMMUV3_REGISTER_LAYOUT_PAGE0;

//
// Page 1 SMMUv3 register layout.
//

typedef struct _SMMUV3_REGISTER_LAYOUT_PAGE1 {
  UINT32                Reserved0[42];
  SMMUV3_EVENTQ_PROD    EventQProd;
  SMMUV3_EVENTQ_CONS    EventQCons;
  UINT32                Reserved1[6];
  SMMUV3_PRIQ_PROD      PriQProd;
  SMMUV3_PRIQ_CONS      PriQCons;
} SMMUV3_REGISTER_LAYOUT_PAGE1;

//
// Level 1 Stream table descriptor
//

typedef union _SMMUV3_L1_STREAM_TABLE_DESCRIPTOR {
  struct {
    UINT64    Span      : 5;
    UINT64    Reserved0 : 1;
    UINT64    L2Ptr     : 46;
    UINT64    Reserved1 : 12;
  } Bits;
  UINT64    AsUINT64;
} SMMUV3_L1_STREAM_TABLE_DESCRIPTOR;

typedef union  _SMMUV3_STREAM_TABLE_ENTRY {
  struct {
    UINT64    Valid        : 1;
    UINT64    Config       : 3;
    UINT64    S1Fmt        : 2;
    UINT64    S1ContextPtr : 46;
    UINT64    Reserved0    : 7;
    UINT64    S1CdMax      : 5;
    UINT64    S1Dss        : 2;
    UINT64    S1Cir        : 2;
    UINT64    S1Cor        : 2;
    UINT64    S1Csh        : 2;
    UINT64    S2Hwu59      : 1;
    UINT64    S2Hwu60      : 1;
    UINT64    S2Hwu61      : 1;
    UINT64    S2Hwu62      : 1;
    UINT64    Dre          : 1;
    UINT64    Cont         : 4;
    UINT64    Dcp          : 1;
    UINT64    Ppar         : 1;
    UINT64    Mev          : 1;
    UINT64    ResSw        : 4;
    UINT64    Reserved1    : 1;
    UINT64    S2Fwb        : 1;
    UINT64    S1Mpam       : 1;
    UINT64    S1StallD     : 1;
    UINT64    Eats         : 2;
    UINT64    Strw         : 2;
    UINT64    MemAttr      : 4;
    UINT64    Mtcfg        : 1;
    UINT64    AllocCfg     : 4;
    UINT64    Reserved2    : 3;
    UINT64    ShCfg        : 2;
    UINT64    NsCfg        : 2;
    UINT64    PrivCfg      : 2;
    UINT64    InstCfg      : 2;
    UINT64    Impl0        : 12;
    UINT64    S2Vmid       : 16;
    UINT64    Impl1        : 16;
    UINT64    S2T0Sz       : 6;
    UINT64    S2Sl0        : 2;
    UINT64    S2Ir0        : 2;
    UINT64    S2Or0        : 2;
    UINT64    S2Sh0        : 2;
    UINT64    S2Tg         : 2;
    UINT64    S2Ps         : 3;
    UINT64    S2Aa64       : 1;
    UINT64    S2Endi       : 1;
    UINT64    S2Affd       : 1;
    UINT64    S2Ptw        : 1;
    UINT64    S2Had        : 2;
    UINT64    S2Rs         : 2;
    UINT64    Reserved3    : 5;
    UINT64    S2Nsw        : 1;
    UINT64    S2Nsa        : 1;
    UINT64    Reserved4    : 2;
    UINT64    S2Ttb        : 48;
    UINT64    Reserved5    : 12;
    UINT64    Impl2        : 16;
    UINT64    PartId       : 16;
    UINT64    Reserved6    : 32;
    UINT64    Pmg          : 8;
    UINT64    Reserved7    : 4;
    UINT64    VmsPtr       : 40;
    UINT64    Reserved8    : 12;
    UINT64    Reserved9;
    UINT64    Reserved10;
  } Bits;
  UINT64    AsUINT64[8];
} SMMUV3_STREAM_TABLE_ENTRY;

STATIC_ASSERT (sizeof (SMMUV3_STREAM_TABLE_ENTRY) == 64, "Invalid size for SMMUV3_STREAM_TABLE_ENTRY");

//
// Define an enumeration of valid values for stream entry config field
// (SMMUV3_STREAM_TABLE_ENTRY.Config).
//

typedef enum _SMMUV3_STREAM_ENTRY_CONFIG_TYPE {
  StreamEntryConfigS1BlockedS2Blocked = 0,
  StreamEntryConfigS1BypassS2Bypass   = 4,
  StreamEntryConfigS1TranslateS2Bypass,
  StreamEntryConfigS1BypassS2Translate,
  StreamEntryConfigS1TranslateS2Translate
} SMMUV3_STREAM_ENTRY_CONFIG_TYPE;

//
// Level 1 Context descriptor
//

typedef union _SMMUV3_L1_CONTEXT_DESCRIPTOR {
  struct {
    UINT64    Valid     : 1;
    UINT64    Reserved0 : 11;
    UINT64    L2Ptr     : 40;
    UINT64    Reserved1 : 12;
  } Bits;
  UINT64    AsUINT64;
} SMMUV3_L1_CONTEXT_DESCRIPTOR;

//
// Context descriptor
//

typedef union  _SMMUV3_CONTEXT_DESCRIPTOR {
  struct {
    UINT64    T0Sz      : 6;
    UINT64    Tg0       : 2;
    UINT64    Ir0       : 2;
    UINT64    Or0       : 2;
    UINT64    Sh0       : 2;
    UINT64    Epd0      : 1;
    UINT64    Endi      : 1;
    UINT64    T1Sz      : 6;
    UINT64    Tg1       : 2;
    UINT64    Ir1       : 2;
    UINT64    Or1       : 2;
    UINT64    Sh1       : 2;
    UINT64    Epd1      : 1;
    UINT64    Valid     : 1;
    UINT64    Ips       : 3;
    UINT64    Affd      : 1;
    UINT64    Wxn       : 1;
    UINT64    UWxn      : 1;
    UINT64    Tbi0      : 1;
    UINT64    Tbi1      : 1;
    UINT64    Pan       : 1;
    UINT64    Aa64      : 1;
    UINT64    Had       : 2;
    UINT64    Ars       : 3;
    UINT64    Aset      : 1;
    UINT64    Asid      : 16;
    UINT64    NsCfg0    : 1;
    UINT64    Had0      : 1;
    UINT64    Reserved0 : 2;
    UINT64    Ttb0      : 48;
    UINT64    Reserved1 : 8;
    UINT64    Hwu059    : 1;
    UINT64    Hwu060    : 1;
    UINT64    Hwu061    : 1;
    UINT64    Hwu062    : 1;
    UINT64    NsCfg1    : 1;
    UINT64    Had1      : 1;
    UINT64    Reserved2 : 2;
    UINT64    Ttb1      : 48;
    UINT64    Reserved3 : 8;
    UINT64    Hwu159    : 1;
    UINT64    Hwu160    : 1;
    UINT64    Hwu161    : 1;
    UINT64    Hwu162    : 1;
    UINT64    Mair0     : 32;
    UINT64    Mair1     : 32;
    UINT64    AMair0    : 32;
    UINT64    AMair1    : 32;
    UINT64    Impl      : 32;
    UINT64    PartId    : 16;
    UINT64    Pmg       : 8;
    UINT64    Reserved4 : 8;
    UINT64    Reserved5[2];
  } Bits;
  UINT64    AsUINT64[8];
} SMMUV3_CONTEXT_DESCRIPTOR;

//
// Command opcodes and their formats.
//

typedef enum _SMMUV3_COMMAND_OPCODE {
  CmdPrefetchConfig = 0x1,
  CmdPrefetchAddr,
  CmdCfgiSte,
  CmdCfgiSteRange,
  CmdCfgiCd,
  CmdCfgiCdAll,
  CmdCfgiVmsPidm,
  CmdTlbiNhAll = 0x10,
  CmdTlbiNhAsid,
  CmdTlbiNhVa,
  CmdTlbiNhVaa,
  CmdTlbiEl3All = 0x18,
  CmdTlbiEl3Va  = 0x1A,
  CmdTlbiEl2All = 0x20,
  CmdTlbiEl2Asid,
  CmdTlbiEl2Va,
  CmdTlbiEl2Vaa,
  CmdTlbiS12VmAll = 0x28,
  CmdTlbiS2Ipa    = 0x2A,
  CmdTlbiNsnhAll  = 0x30,
  CmdAtcInv       = 0x40,
  CmdPriResp      = 0x41,
  CmdResume       = 0x44,
  CmdSync         = 0x46
} SMMUV3_COMMAND_OPCODE;

typedef struct _SMMUV3_CMD_CFGI_STE {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 2;
  UINT32    SSec      : 1;
  UINT32    Reserved1 : 21;
  UINT32    StreamId;
  UINT64    Leaf      : 1;
  UINT64    Reserved2 : 63;
} SMMUV3_CMD_CFGI_STE;

typedef struct _SMMUV3_CMD_CFGI_STE_RANGE {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 2;
  UINT32    SSec      : 1;
  UINT32    Reserved1 : 21;
  UINT32    StreamId;
  UINT64    Range     : 5;
  UINT64    Reserved2 : 59;
} SMMUV3_CMD_CFGI_STE_RANGE;

typedef struct _SMMUV3_CMD_CFGI_CD {
  UINT32    Opcode      : 8;
  UINT32    Reserved0   : 2;
  UINT32    SSec        : 1;
  UINT32    Reserved1   : 1;
  UINT32    SubStreamId : 20;
  UINT32    StreamId;
  UINT64    Leaf        : 1;
  UINT64    Reserved2   : 63;
} SMMUV3_CMD_CFGI_CD;

typedef struct _SMMUV3_CMD_CFGI_CD_ALL {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 2;
  UINT32    SSec      : 1;
  UINT32    Reserved1 : 21;
  UINT32    StreamId;
  UINT64    Reserved2;
} SMMUV3_CMD_CFGI_CD_ALL;

typedef struct _SMMUV3_CMD_CFGI_VMS_PIDM {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 2;
  UINT32    SSec      : 1;
  UINT32    Reserved1 : 21;
  UINT32    Vmid      : 16;
  UINT32    Reserved2 : 16;
  UINT64    Reserved3;
} SMMUV3_CMD_CFGI_VMS_PIDM;

typedef struct _SMMUV3_CMD_CFGI_ALL {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 2;
  UINT32    SSec      : 1;
  UINT32    Reserved1 : 21;
  UINT32    Ignored;
  UINT64    Range     : 5;
  UINT64    Reserved2 : 59;
} SMMUV3_CMD_CFGI_ALL;

typedef struct _SMMUV3_CMD_TLBI_NH_ALL {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 24;
  UINT32    Vmid      : 16;
  UINT32    Reserved1 : 16;
  UINT64    Reserved2;
} SMMUV3_CMD_TLBI_NH_ALL;

typedef struct _SMMUV3_CMD_TLBI_NH_ASID {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 24;
  UINT32    Vmid      : 16;
  UINT32    Asid      : 16;
  UINT64    Reserved1;
} SMMUV3_CMD_TLBI_NH_ASID;

typedef struct _SMMUV3_CMD_TLBI_NH_VAA {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 4;
  UINT32    Num       : 5;
  UINT32    Reserved1 : 3;
  UINT32    Scale     : 5;
  UINT32    Reserved2 : 7;
  UINT32    Vmid      : 16;
  UINT32    Reserved3 : 16;
  UINT64    Leaf      : 1;
  UINT64    Reserved4 : 7;
  UINT64    Ttl       : 2;
  UINT64    Tg        : 2;
  UINT64    Address   : 52;
} SMMUV3_CMD_TLBI_NH_VAA;

typedef struct _SMMUV3_CMD_TLBI_NH_VA {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 4;
  UINT32    Num       : 5;
  UINT32    Reserved1 : 3;
  UINT32    Scale     : 5;
  UINT32    Reserved2 : 7;
  UINT32    Vmid      : 16;
  UINT32    Asid      : 16;
  UINT64    Leaf      : 1;
  UINT64    Reserved3 : 7;
  UINT64    Ttl       : 2;
  UINT64    Tg        : 2;
  UINT64    Address   : 52;
} SMMUV3_CMD_TLBI_NH_VA;

typedef struct _SMMUV3_CMD_TLBI_EL2_ALL {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 24;
  UINT32    Reserved1;
  UINT64    Reserved2;
} SMMUV3_CMD_TLBI_EL2_ALL;

typedef struct _SMMUV3_CMD_TLBI_EL2_VA {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 4;
  UINT32    Num       : 5;
  UINT32    Reserved1 : 3;
  UINT32    Scale     : 5;
  UINT32    Reserved2 : 7;
  UINT32    Reserved3 : 16;
  UINT32    Asid      : 16;
  UINT64    Leaf      : 1;
  UINT64    Reserved4 : 7;
  UINT64    Ttl       : 2;
  UINT64    Tg        : 2;
  UINT64    Address   : 52;
} SMMUV3_CMD_TLBI_EL2_VA;

typedef struct _SMMUV3_CMD_TLBI_EL2_VAA {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 4;
  UINT32    Num       : 5;
  UINT32    Reserved1 : 3;
  UINT32    Scale     : 5;
  UINT32    Reserved2 : 7;
  UINT32    Reserved3;
  UINT64    Leaf      : 1;
  UINT64    Reserved4 : 7;
  UINT64    Ttl       : 2;
  UINT64    Tg        : 2;
  UINT64    Address   : 52;
} SMMUV3_CMD_TLBI_EL2_VAA;

typedef struct _SMMUV3_CMD_TLBI_EL2_ASID {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 24;
  UINT32    Reserved1 : 16;
  UINT32    Asid      : 16;
  UINT64    Reserved2;
} SMMUV3_CMD_TLBI_EL2_ASID;

//
// Stage 2 TLB invalidation commands.
//

typedef struct _SMMUV3_CMD_TLBI_S2_IPA {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 4;
  UINT32    Num       : 5;
  UINT32    Reserved1 : 3;
  UINT32    Scale     : 5;
  UINT32    Reserved2 : 7;
  UINT32    Vmid      : 16;
  UINT32    Reserved3 : 16;
  UINT64    Leaf      : 1;
  UINT64    Reserved4 : 7;
  UINT64    Ttl       : 2;
  UINT64    Tg        : 2;
  UINT64    Address   : 40;
  UINT64    Reserved5 : 12;
} SMMUV3_CMD_TLBI_S2_IPA;

typedef struct _SMMUV3_CMD_TLBI_S12_VMALL {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 24;
  UINT32    Vmid      : 16;
  UINT32    Reserved1 : 16;
  UINT64    Reserved2;
} SMMUV3_CMD_TLBI_S12_VMALL;

//
// Common TLB invalidation commands.
//

typedef struct _SMMUV3_CMD_TLBI_NSNH_ALL {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 24;
  UINT32    Reserved1;
  UINT64    Reserved2;
} SMMUV3_CMD_TLBI_NSNH_ALL;

//
// Fault response and synchronization commands.
//

typedef struct _SMMUV3_CMD_RESUME {
  UINT32    Opcode    : 8;
  UINT32    Reserved0 : 2;
  UINT32    SSec      : 1;
  UINT32    Reserved1 : 1;
  UINT32    Ac        : 1;
  UINT32    Ab        : 1;
  UINT32    Reserved2 : 18;
  UINT32    StreamId;
  UINT64    Stag      : 16;
  UINT64    Reserved3 : 48;
} SMMUV3_CMD_RESUME;

typedef struct _SMMUV3_CMD_SYNC {
  UINT32    Opcode     : 8;
  UINT32    Reserved0  : 4;
  UINT32    Cs         : 2;
  UINT32    Reserved1  : 8;
  UINT32    Msh        : 2;
  UINT32    MsiAttr    : 4;
  UINT32    Reserved2  : 4;
  UINT32    MsiData;
  UINT64    Reserved3  : 2;
  UINT64    MsiAddress : 50;
  UINT64    Reserved4  : 12;
} SMMUV3_CMD_SYNC;

typedef union _SMMUV3_CMD_GENERIC {
  SMMUV3_CMD_CFGI_STE          CfgiSte;
  SMMUV3_CMD_CFGI_STE_RANGE    CfgiSteRange;
  SMMUV3_CMD_CFGI_CD           CfgiCd;
  SMMUV3_CMD_CFGI_CD_ALL       CfgiCdAll;
  SMMUV3_CMD_CFGI_VMS_PIDM     CfgiVmsPidm;
  SMMUV3_CMD_CFGI_ALL          CfgiAll;
  SMMUV3_CMD_TLBI_NH_ALL       TlbiNhAll;
  SMMUV3_CMD_TLBI_NH_ASID      TlbiNhAsid;
  SMMUV3_CMD_TLBI_NH_VAA       TlbiNhVaa;
  SMMUV3_CMD_TLBI_NH_VA        TlbiNhVa;
  SMMUV3_CMD_TLBI_EL2_ALL      TlbiEl2All;
  SMMUV3_CMD_TLBI_EL2_VA       TlbiEl2Va;
  SMMUV3_CMD_TLBI_EL2_VAA      TlbiEl2Vaa;
  SMMUV3_CMD_TLBI_EL2_ASID     TlbiEl2Asid;
  SMMUV3_CMD_TLBI_S2_IPA       TlbiS2Ipa;
  SMMUV3_CMD_TLBI_S12_VMALL    TlbiS12VmAll;
  SMMUV3_CMD_TLBI_NSNH_ALL     TlbiNsnhAll;
  SMMUV3_CMD_RESUME            Resume;
  SMMUV3_CMD_SYNC              Sync;

  struct {
    UINT64    CmdLow;
    UINT64    CmdHigh;
  } Raw;
} SMMUV3_CMD_GENERIC;

//
// Event/Fault types and their formats.
//

typedef enum _SMMUV3_FAULT_TYPE {
  FaultTypeUnsupportedUpstreamTransaction = 0x1,
  FaultTypeStartingFault                  = FaultTypeUnsupportedUpstreamTransaction,
  FaultTypeBadStreamId,
  FaultTypeStreamEntryFetchAbort,
  FaultTypeBadStreamEntry,
  FaultTypeBadAtsTranslationRequest,
  FaultTypeStreamDisabled,
  FaultTypeTranslationForbidden,
  FaultTypeBadSubstreamId,
  FaultTypeContextDescriptorFetchAbort,
  FaultTypeBadContextDescriptor,
  FaultTypeTranslationWalkExternalAbort,
  FaultTypeTranslation = 0x10,
  FaultTypeAddressSize,
  FaultTypeAccessFlag,
  FaultTypePermission,
  FaultTypeTlbConflict = 0x20,
  FaultTypeConfigurationCacheConflict,
  FaultTypePageRequest = 0x24,
  FaultTypeVmsFetchAbort,
  FaultTypeImplDefinedFaultStart = 0xE0,
  FaultTypeImplDefinedFaultEnd   = 0xEF,
  FaultTypeEndingFault           = FaultTypeImplDefinedFaultEnd,
} SMMUV3_FAULT_TYPE;

typedef struct _SMMUV3_UNSUPPORTED_UPSTREAM_TRANSACTION_FAULT {
  UINT32    Type                      : 8;
  UINT32    Reserved0                 : 3;
  UINT32    Ssv                       : 1;
  UINT32    SubstreamId               : 20;
  UINT32    StreamId;
  UINT32    Reason                    : 16;
  UINT32    Reserved1                 : 16;
  UINT32    Reserved2                 : 1;
  UINT32    PrivilegedNotUnprivileged : 1;
  UINT32    InstructionNotData        : 1;
  UINT32    ReadNotWrite              : 1;
  UINT32    Reserved3                 : 28;
  UINT64    InputAddress;
  UINT64    Reserved4;
} SMMUV3_UNSUPPORTED_UPSTREAM_TRANSACTION_FAULT;

typedef struct _SMMUV3_BAD_STREAM_ID_FAULT {
  UINT32    Type        : 8;
  UINT32    Reserved0   : 3;
  UINT32    Ssv         : 1;
  UINT32    SubstreamId : 20;
  UINT32    StreamId;
  UINT64    Reserved1;
  UINT64    Reserved2;
  UINT64    Reserved3;
} SMMUV3_BAD_STREAM_ID_FAULT;

typedef struct _SMMUV3_STREAM_ENTRY_FETCH_ABORT_FAULT {
  UINT32    Type         : 8;
  UINT32    Reserved0    : 3;
  UINT32    Ssv          : 1;
  UINT32    SubstreamId  : 20;
  UINT32    StreamId;
  UINT32    Reason       : 16;
  UINT32    Reserved1    : 16;
  UINT32    Reserved2;
  UINT64    Reserved3;
  UINT64    Reserved4    : 3;
  UINT64    FetchAddress : 49;
  UINT64    Reserved5    : 12;
} SMMUV3_STREAM_ENTRY_FETCH_ABORT_FAULT;

typedef struct _SMMUV3_BAD_STREAM_ENTRY_FAULT {
  UINT32    Type        : 8;
  UINT32    Reserved0   : 3;
  UINT32    Ssv         : 1;
  UINT32    SubstreamId : 20;
  UINT32    StreamId;
  UINT64    Reserved1;
  UINT64    Reserved2;
  UINT64    Reserved3;
} SMMUV3_BAD_STREAM_ENTRY_FAULT;

typedef struct _SMMUV3_BAD_ATS_TRANSLATION_REQUEST {
  UINT32    Type         : 8;
  UINT32    Reserved0    : 3;
  UINT32    Ssv          : 1;
  UINT32    SubstreamId  : 20;
  UINT32    StreamId;
  UINT32    Span         : 4;
  UINT32    Reserved1    : 24;
  UINT32    Privilege    : 1;
  UINT32    Execute      : 1;
  UINT32    Write        : 1;
  UINT32    Read         : 1;
  UINT32    Reserved2;
  UINT64    Reserved3    : 12;
  UINT64    InputAddress : 52;
  UINT64    Reserved4;
} SMMUV3_BAD_ATS_TRANSLATION_REQUEST;

typedef struct _SMMUV3_STREAM_DISABLED_FAULT {
  UINT32    Type      : 8;
  UINT32    Reserved0 : 24;
  UINT32    StreamId;
  UINT64    Reserved1;
  UINT64    Reserved2;
  UINT64    Reserved3;
} SMMUV3_STREAM_DISABLED_FAULT;

typedef struct _SMMUV3_TRANSLATION_FORBIDDEN_FAULT {
  UINT32    Type         : 8;
  UINT32    Reserved0    : 24;
  UINT32    StreamId;
  UINT32    Reserved1;
  UINT32    Reserved2    : 3;
  UINT32    ReadNotWrite : 1;
  UINT32    Reserved3    : 28;
  UINT64    InputAddress;
  UINT64    Reserved4;
} SMMUV3_TRANSLATION_FORBIDDEN_FAULT;

typedef struct _SMMUV3_BAD_SUBSTREAM_ID_FAULT {
  UINT32    Type        : 8;
  UINT32    Reserved0   : 4;
  UINT32    SubstreamId : 20;
  UINT32    StreamId;
  UINT64    Reserved1;
  UINT64    Reserved2;
  UINT64    Reserved3;
} SMMUV3_BAD_SUBSTREAM_ID_FAULT;

typedef struct _SMMUV3_CONTEXT_DESCRIPTOR_FETCH_ABORT_FAULT {
  UINT32    Type         : 8;
  UINT32    Reserved0    : 3;
  UINT32    Ssv          : 1;
  UINT32    SubstreamId  : 20;
  UINT32    StreamId;
  UINT32    Reason       : 16;
  UINT32    Reserved1    : 16;
  UINT32    Reserved2;
  UINT64    Reserved3;
  UINT64    Reserved4    : 3;
  UINT64    FetchAddress : 49;
  UINT64    Reserved5    : 12;
} SMMUV3_CONTEXT_DESCRIPTOR_FETCH_ABORT_FAULT;

typedef struct _SMMUV3_BAD_CONTEXT_DESCRIPTOR_FAULT {
  UINT32    Type        : 8;
  UINT32    Reserved0   : 3;
  UINT32    Ssv         : 1;
  UINT32    SubstreamId : 20;
  UINT32    StreamId;
  UINT64    Reserved1;
  UINT64    Reserved2;
  UINT64    Reserved3;
} SMMUV3_BAD_CONTEXT_DESCRIPTOR_FAULT;

typedef struct _SMMUV3_TRANSLATION_WALK_EXTERNAL_ABORT_FAULT {
  UINT32    Type                      : 8;
  UINT32    Reserved0                 : 3;
  UINT32    Ssv                       : 1;
  UINT32    SubstreamId               : 20;
  UINT32    StreamId;
  UINT32    Reason                    : 16;
  UINT32    Reserved1                 : 16;
  UINT32    Reserved2                 : 1;
  UINT32    PrivilegedNotUnprivileged : 1;
  UINT32    InstructionNotData        : 1;
  UINT32    ReadNotWrite              : 1;
  UINT32    Reserved3                 : 2;
  UINT32    NonSecureIpa              : 1;
  UINT32    Stage2                    : 1;
  UINT32    Class                     : 2;
  UINT32    Reserved4                 : 22;
  UINT64    InputAddress;
  UINT64    Reserved5                 : 3;
  UINT64    FetchAddress              : 49;
  UINT64    Reserved6                 : 12;
} SMMUV3_TRANSLATION_WALK_EXTERNAL_ABORT_FAULT;

typedef struct _SMMUV3_TRANSLATION_FAULT {
  UINT32    Type                      : 8;
  UINT32    Reserved0                 : 3;
  UINT32    Ssv                       : 1;
  UINT32    SubstreamId               : 20;
  UINT32    StreamId;
  UINT32    Stag                      : 16;
  UINT32    Reserved1                 : 15;
  UINT32    Stall                     : 1;
  UINT32    Reserved2                 : 1;
  UINT32    PrivilegedNotUnprivileged : 1;
  UINT32    InstructionNotData        : 1;
  UINT32    ReadNotWrite              : 1;
  UINT32    Reserved3                 : 2;
  UINT32    NonSecureIpa              : 1;
  UINT32    Stage2                    : 1;
  UINT32    Class                     : 2;
  UINT32    Reserved4                 : 6;
  UINT32    ImplDefined               : 16;
  UINT64    InputAddress;
  UINT64    Reserved5                 : 12;
  UINT64    Ipa                       : 40;
  UINT64    Reserved6                 : 12;
} SMMUV3_TRANSLATION_FAULT;

typedef struct _SMMUV3_ADDRESS_SIZE_FAULT {
  UINT32    Type                      : 8;
  UINT32    Reserved0                 : 3;
  UINT32    Ssv                       : 1;
  UINT32    SubstreamId               : 20;
  UINT32    StreamId;
  UINT32    Stag                      : 16;
  UINT32    Reserved1                 : 15;
  UINT32    Stall                     : 1;
  UINT32    Reserved2                 : 1;
  UINT32    PrivilegedNotUnprivileged : 1;
  UINT32    InstructionNotData        : 1;
  UINT32    ReadNotWrite              : 1;
  UINT32    Reserved3                 : 2;
  UINT32    NonSecureIpa              : 1;
  UINT32    Stage2                    : 1;
  UINT32    Class                     : 2;
  UINT32    Reserved4                 : 6;
  UINT32    ImplDefined               : 16;
  UINT64    InputAddress;
  UINT64    Reserved5                 : 12;
  UINT64    Ipa                       : 40;
  UINT64    Reserved6                 : 12;
} SMMUV3_ADDRESS_SIZE_FAULT;

typedef struct _SMMUV3_ACCESS_FLAG_FAULT {
  UINT32    Type                      : 8;
  UINT32    Reserved0                 : 3;
  UINT32    Ssv                       : 1;
  UINT32    SubstreamId               : 20;
  UINT32    StreamId;
  UINT32    Stag                      : 16;
  UINT32    Reserved1                 : 15;
  UINT32    Stall                     : 1;
  UINT32    Reserved2                 : 1;
  UINT32    PrivilegedNotUnprivileged : 1;
  UINT32    InstructionNotData        : 1;
  UINT32    ReadNotWrite              : 1;
  UINT32    Reserved3                 : 2;
  UINT32    NonSecureIpa              : 1;
  UINT32    Stage2                    : 1;
  UINT32    Class                     : 2;
  UINT32    Reserved4                 : 6;
  UINT32    ImplDefined               : 16;
  UINT64    InputAddress;
  UINT64    Reserved5                 : 12;
  UINT64    Ipa                       : 40;
  UINT64    Reserved6                 : 12;
} SMMUV3_ACCESS_FLAG_FAULT;

typedef struct _SMMUV3_PERMISSION_FAULT {
  UINT32    Type                      : 8;
  UINT32    Reserved0                 : 3;
  UINT32    Ssv                       : 1;
  UINT32    SubstreamId               : 20;
  UINT32    StreamId;
  UINT32    Stag                      : 16;
  UINT32    Reserved1                 : 15;
  UINT32    Stall                     : 1;
  UINT32    Reserved2                 : 1;
  UINT32    PrivilegedNotUnprivileged : 1;
  UINT32    InstructionNotData        : 1;
  UINT32    ReadNotWrite              : 1;
  UINT32    Reserved3                 : 2;
  UINT32    NonSecureIpa              : 1;
  UINT32    Stage2                    : 1;
  UINT32    Class                     : 2;
  UINT32    Reserved4                 : 6;
  UINT32    ImplDefined               : 16;
  UINT64    InputAddress;
  UINT64    Reserved5                 : 12;
  UINT64    Ipa                       : 40;
  UINT64    Reserved6                 : 12;
} SMMUV3_PERMISSION_FAULT;

typedef struct _SMMUV3_TLB_CONFLICT {
  UINT32    Type                      : 8;
  UINT32    Reserved0                 : 3;
  UINT32    Ssv                       : 1;
  UINT32    SubstreamId               : 20;
  UINT32    StreamId;
  UINT32    Reason;
  UINT32    Reserved1                 : 1;
  UINT32    PrivilegedNotUnprivileged : 1;
  UINT32    InstructionNotData        : 1;
  UINT32    ReadNotWrite              : 1;
  UINT32    Reserved2                 : 2;
  UINT32    NonSecureIpa              : 1;
  UINT32    Stage2                    : 1;
  UINT32    Reserved3                 : 24;
  UINT64    InputAddress;
  UINT64    Reserved5                 : 12;
  UINT64    Ipa                       : 40;
  UINT64    Reserved6                 : 12;
} SMMUV3_TLB_CONFLICT;

typedef struct _SMMUV3_CONFIGURATION_CACHE_CONFLICT {
  UINT32    Type        : 8;
  UINT32    Reserved0   : 3;
  UINT32    Ssv         : 1;
  UINT32    SubstreamId : 20;
  UINT32    StreamId;
  UINT32    Reason;
  UINT32    Reserved1;
  UINT64    Reserved2;
  UINT64    Reserved3;
} SMMUV3_CONFIGURATION_CACHE_CONFLICT;

typedef struct _SMMUV3_PAGE_REQUEST_FAULT {
  UINT32    Type              : 8;
  UINT32    Reserved0         : 3;
  UINT32    Ssv               : 1;
  UINT32    SubstreamId       : 20;
  UINT32    StreamId;
  UINT32    Reserved1;
  UINT32    Reserved2         : 1;
  UINT32    UserExecute       : 1;
  UINT32    UserWrite         : 1;
  UINT32    UserRead          : 1;
  UINT32    Reserved3         : 1;
  UINT32    PrivilegedExecute : 1;
  UINT32    PrivilegedWrite   : 1;
  UINT32    PrivilegedRead    : 1;
  UINT32    Reserved4         : 4;
  UINT32    Span              : 8;
  UINT32    Reserved5         : 12;
  UINT64    Reserved6         : 12;
  UINT64    InputAddress      : 52;
  UINT64    Reserved7;
} SMMUV3_PAGE_REQUEST_FAULT;

typedef struct _SMMUV3_VMS_FETCH_ABORT_FAULT {
  UINT32    Type         : 8;
  UINT32    Reserved0    : 3;
  UINT32    Ssv          : 1;
  UINT32    SubstreamId  : 20;
  UINT32    StreamId;
  UINT32    Reason       : 16;
  UINT32    Reserved1    : 16;
  UINT32    Reserved2;
  UINT64    Reserved3;
  UINT64    Reserved4    : 3;
  UINT64    FetchAddress : 49;
  UINT64    Reserved5    : 12;
} SMMUV3_VMS_FETCH_ABORT_FAULT;

typedef struct _SMMUV3_FAULT_GENERIC {
  UINT32    Type                      : 8;
  UINT32    Reserved0                 : 3;
  UINT32    Ssv                       : 1;
  UINT32    SubstreamId               : 20;
  UINT32    StreamId;
  UINT32    Reserved1;
  UINT32    Reserved2                 : 1;
  UINT32    PrivilegedNotUnprivileged : 1;
  UINT32    InstructionNotData        : 1;
  UINT32    ReadNotWrite              : 1;
  UINT32    Reserved3                 : 3;
  UINT32    Stage2                    : 1;
  UINT32    Class                     : 2;
  UINT32    Reserved4                 : 22;
  UINT64    InputAddress;
  UINT64    Reserved5;
} SMMUV3_FAULT_GENERIC;

typedef union _SMMUV3_FAULT_RECORD {
  SMMUV3_UNSUPPORTED_UPSTREAM_TRANSACTION_FAULT    UnsupportedUpstream;
  SMMUV3_BAD_STREAM_ID_FAULT                       BadStreamdId;
  SMMUV3_STREAM_ENTRY_FETCH_ABORT_FAULT            StreamEntryFetchAbort;
  SMMUV3_BAD_STREAM_ENTRY_FAULT                    BadStreamEntry;
  SMMUV3_BAD_ATS_TRANSLATION_REQUEST               BadAtsRequest;
  SMMUV3_STREAM_DISABLED_FAULT                     StreamDisabled;
  SMMUV3_TRANSLATION_FORBIDDEN_FAULT               TranslationForbidden;
  SMMUV3_BAD_SUBSTREAM_ID_FAULT                    BadSubstreamId;
  SMMUV3_CONTEXT_DESCRIPTOR_FETCH_ABORT_FAULT      CdFetchAbort;
  SMMUV3_BAD_CONTEXT_DESCRIPTOR_FAULT              CdFault;
  SMMUV3_TRANSLATION_WALK_EXTERNAL_ABORT_FAULT     WalkAbort;
  SMMUV3_TRANSLATION_FAULT                         Translation;
  SMMUV3_ADDRESS_SIZE_FAULT                        AddressSize;
  SMMUV3_ACCESS_FLAG_FAULT                         AccessFlag;
  SMMUV3_PERMISSION_FAULT                          Permission;
  SMMUV3_TLB_CONFLICT                              TlbConflict;
  SMMUV3_CONFIGURATION_CACHE_CONFLICT              ConfigCache;
  SMMUV3_PAGE_REQUEST_FAULT                        PageRequest;
  SMMUV3_VMS_FETCH_ABORT_FAULT                     VmsFetchAbort;
  SMMUV3_FAULT_GENERIC                             Generic;
  UINT64                                           Fault[4];
} SMMUV3_FAULT_RECORD;

//
// --------------------------------------------------------------------- Macros
//

//
// Define mask for command queue opcodes.
//

#define SMMUV3_COMMAND_OPCODE_MASK        (0xFF)
#define SMMUV3_LAST_VALID_COMMAND_OPCODE  (CmdSync)

//
// Define macros to build various cache, TLB and fault response commands.
//

#define SMMUV3_BUILD_CMD_CFGI_STE(Command, InputStreamId, InputLeaf) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->CfgiSte.Opcode = CmdCfgiSte; \
    (Command)->CfgiSte.StreamId = InputStreamId; \
    (Command)->CfgiSte.Leaf = InputLeaf; \
}

#define SMMUV3_BUILD_CMD_CFGI_STE_RANGE(Command, InputStreamId, InputRange) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->CfgiSteRange.Opcode = CmdCfgiSteRange; \
    (Command)->CfgiSteRange.StreamId = InputStreamId; \
    (Command)->CfgiSteRange.Range = InputRange; \
}

#define SMMUV3_BUILD_CMD_CFGI_CD(Command, InputStreamId, InputLeaf) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->CfgiCd.Opcode = CmdCfgiCd; \
    (Command)->CfgiCd.StreamId = InputStreamId; \
    (Command)->CfgiCd.Leaf = InputLeaf; \
}

#define SMMUV3_BUILD_CMD_CFGI_CD_ALL(Command, InputStreamId) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->CfgiCdAll.Opcode = CmdCfgiCdAll; \
    (Command)->CfgiCdAll.StreamId = InputStreamId; \
}

#define SMMUV3_BUILD_CMD_CFGI_ALL(Command) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->CfgiSteRange.Opcode = CmdCfgiSteRange; \
    (Command)->CfgiSteRange.Range = 31; \
}

#define SMMUV3_BUILD_CMD_TLBI_EL2_ALL(Command) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->TlbiEl2All.Opcode = CmdTlbiEl2All; \
}

#define SMMUV3_BUILD_CMD_TLBI_NSNH_ALL(Command) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->TlbiNsnhAll.Opcode = CmdTlbiNsnhAll; \
}

#define SMMUV3_BUILD_CMD_SYNC_NO_INTERRUPT(Command) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->Sync.Opcode = CmdSync; \
}

#define SMMUV3_BUILD_CMD_TLBI_NH_ALL(Command, InputVmid) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->TlbiNhAll.Opcode = CmdTlbiNhAll; \
    (Command)->TlbiNhAsid.Vmid = InputVmid; \
}

#define SMMUV3_BUILD_CMD_TLBI_NH_ASID(Command, InputVmid, InputAsid) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->TlbiNhAsid.Opcode = CmdTlbiNhAsid; \
    (Command)->TlbiNhAsid.Vmid = InputVmid; \
    (Command)->TlbiNhAsid.Asid = InputAsid; \
}

#define SMMUV3_BUILD_CMD_TLBI_NH_VA(Command, \
                                    InputVmid, \
                                    InputAsid, \
                                    InputAddress) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->TlbiNhVa.Opcode = CmdTlbiNhVa; \
    (Command)->TlbiNhVa.Vmid = InputVmid; \
    (Command)->TlbiNhVa.Asid = InputAsid; \
    (Command)->TlbiNhVa.Address = ((InputAddress) >> 12); \
}

#define SMMUV3_BUILD_CMD_TLBI_NH_VAA(Command, \
                                     InputVmid, \
                                     InputAddress) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->TlbiNhVaa.Opcode = CmdTlbiNhVaa; \
    (Command)->TlbiNhVaa.Vmid = InputVmid; \
    (Command)->TlbiNhVaa.Address = ((InputAddress) >> 12); \
}

//
// Stage-2 TLB invalidation macros.
//

#define SMMUV3_BUILD_CMD_TLBI_S12_VMALL(Command, InputVmid) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->TlbiS12VmAll.Opcode = CmdTlbiS12VmAll; \
    (Command)->TlbiS12VmAll.Vmid = InputVmid; \
}

#define SMMUV3_BUILD_CMD_TLBI_S2_IPA(Command, InputVmid, InputAddress) \
{ \
    (Command)->Raw.CmdLow = 0; \
    (Command)->Raw.CmdHigh = 0; \
    (Command)->TlbiS2Ipa.Opcode = CmdTlbiS2Ipa; \
    (Command)->TlbiS2Ipa.Vmid = InputVmid; \
    (Command)->TlbiS2Ipa.Address = ((InputAddress) >> 12); \
}

#define SMMUV3_LINEAR_STREAM_TABLE_SIZE_FROM_LOG2(Log2Size) \
    (UINT32)((UINT32)(1UL << (Log2Size)) * \
                (UINT16)sizeof(SMMUV3_STREAM_TABLE_ENTRY))

#define SMMUV3_L1_STREAM_TABLE_SIZE_FROM_LOG2(Log2Size) \
    (UINT32)((UINT32)(1UL << (Log2Size)) * \
                (UINT32)sizeof(UINT64))

//
// Register offsets for Page0.
//
#define SMMU_IDR0             0x0000
#define SMMU_IDR1             0x0004
#define SMMU_IDR2             0x0008
#define SMMU_IDR3             0x000C
#define SMMU_IDR4             0x0010
#define SMMU_IDR5             0x0014
#define SMMU_IIDR             0x0018
#define SMMU_AIDR             0x001C
#define SMMU_CR0              0x0020
#define SMMU_CR0ACK           0x0024
#define SMMU_CR1              0x0028
#define SMMU_CR2              0x002C
#define SMMU_STATUSR          0x0040
#define SMMU_GBPA             0x0044
#define SMMU_AGBPA            0x0048
#define SMMU_IRQ_CTRL         0x0050
#define SMMU_IRQ_CTRLACK      0x0054
#define SMMU_GERROR           0x0060
#define SMMU_GERRORN          0x0064
#define SMMU_GERROR_IRQ_CFG0  0x0068
#define SMMU_GERROR_IRQ_CFG1  0x0070
#define SMMU_GERROR_IRQ_CFG2  0x0074
#define SMMU_STRTAB_BASE      0x0080
#define SMMU_STRTAB_BASE_CFG  0x0088
#define SMMU_CMDQ_BASE        0x0090
#define SMMU_CMDQ_PROD        0x0098
#define SMMU_CMDQ_CONS        0x009C
#define SMMU_EVENTQ_BASE      0x00A0
#define SMMU_EVENTQ_PROD      0x00A8
#define SMMU_EVENTQ_CONS      0x00AC
#define SMMU_EVENTQ_IRQ_CFG0  0x00B0
#define SMMU_EVENTQ_IRQ_CFG1  0x00B8
#define SMMU_EVENTQ_IRQ_CFG2  0x00BC
#define SMMU_PRIQ_BASE        0x00C0
#define SMMU_PRIQ_PROD        0x00C8
#define SMMU_PRIQ_CONS        0x00CC
#define SMMU_PRIQ_IRQ_CFG0    0x00D0
#define SMMU_PRIQ_IRQ_CFG1    0x00D8
#define SMMU_PRIQ_IRQ_CFG2    0x00DC
#define SMMU_GATOS_CTRL       0x0100
#define SMMU_GATOS_SID        0x0108
#define SMMU_GATOS_ADDR       0x0110
#define SMMU_GATOS_PAR        0x0118
#define SMMU_MPAMIDR          0x0130
#define SMMU_GMPAM            0x0138
#define SMMU_GBPMPAM          0x013C
#define SMMU_VATOS_SEL        0x0180
#define SMMU_IDR6             0x0190
#define SMMU_DPT_BASE         0x0200
#define SMMU_DPT_BASE_CFG     0x0208
#define SMMU_DPT_CFG_FAR      0x0210

// Implementation Defined Registers
#define SMMU_IMPL_DEF_START   0x0E00
#define SMMU_IMPL_DEF_END     0x0EFF
#define SMMU_ID_REGS_START    0x0FD0
#define SMMU_ID_REGS_END      0x0FFC
#define SMMU_IMPL_DEF2_START  0x1000
#define SMMU_IMPL_DEF2_END    0x3FFF

// Command Queue Control Page Registers
#define SMMU_CMDQ_CONTROL_PAGE_BASE(n)    (0x4000 + 32 * (n))
#define SMMU_CMDQ_CONTROL_PAGE_CFG(n)     (0x4008 + 32 * (n))
#define SMMU_CMDQ_CONTROL_PAGE_STATUS(n)  (0x400C + 32 * (n))

// Secure Registers
#define SMMU_S_IDR0             0x8000
#define SMMU_S_IDR1             0x8004
#define SMMU_S_IDR2             0x8008
#define SMMU_S_IDR3             0x800C
#define SMMU_S_IDR4             0x8010
#define SMMU_S_CR0              0x8020
#define SMMU_S_CR0ACK           0x8024
#define SMMU_S_CR1              0x8028
#define SMMU_S_CR2              0x802C
#define SMMU_S_INIT             0x803C
#define SMMU_S_GBPA             0x8044
#define SMMU_S_AGBPA            0x8048
#define SMMU_S_IRQ_CTRL         0x8050
#define SMMU_S_IRQ_CTRLACK      0x8054
#define SMMU_S_GERROR           0x8060
#define SMMU_S_GERRORN          0x8064
#define SMMU_S_GERROR_IRQ_CFG0  0x8068
#define SMMU_S_GERROR_IRQ_CFG1  0x8070
#define SMMU_S_GERROR_IRQ_CFG2  0x8074
#define SMMU_S_STRTAB_BASE      0x8080
#define SMMU_S_STRTAB_BASE_CFG  0x8088
#define SMMU_S_CMDQ_BASE        0x8090
#define SMMU_S_CMDQ_PROD        0x8098
#define SMMU_S_CMDQ_CONS        0x809C
#define SMMU_S_EVENTQ_BASE      0x80A0
#define SMMU_S_EVENTQ_PROD      0x80A8
#define SMMU_S_EVENTQ_CONS      0x80AC
#define SMMU_S_EVENTQ_IRQ_CFG0  0x80B0
#define SMMU_S_EVENTQ_IRQ_CFG1  0x80B8
#define SMMU_S_EVENTQ_IRQ_CFG2  0x80BC
#define SMMU_S_GATOS_CTRL       0x8100
#define SMMU_S_GATOS_SID        0x8108
#define SMMU_S_GATOS_ADDR       0x8110
#define SMMU_S_GATOS_PAR        0x8118
#define SMMU_S_MPAMIDR          0x8130
#define SMMU_S_GMPAM            0x8138
#define SMMU_S_GBPMPAM          0x813C
#define SMMU_S_VATOS_SEL        0x8180
#define SMMU_S_IDR6             0x8190

// Secure Implementation Defined Registers
#define SMMU_S_IMPL_DEF_START   0x8E00
#define SMMU_S_IMPL_DEF_END     0x8EFF
#define SMMU_S_IMPL_DEF2_START  0x9000
#define SMMU_S_IMPL_DEF2_END    0xBFFF

// Secure Command Queue Control Page Registers
#define SMMU_S_CMDQ_CONTROL_PAGE_BASE(n)    (0xC000 + 32 * (n))
#define SMMU_S_CMDQ_CONTROL_PAGE_CFG(n)     (0xC008 + 32 * (n))
#define SMMU_S_CMDQ_CONTROL_PAGE_STATUS(n)  (0xC00C + 32 * (n))

// SMMU_GBPA register fields.
#define SMMU_GBPA_UPDATE  BIT31
#define SMMU_GBPA_ABORT   BIT20

#endif
