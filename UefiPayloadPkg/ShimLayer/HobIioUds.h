/** @file

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HOB_IIO_UDS_H__
#define __HOB_IIO_UDS_H__

#include <PiPei.h>

#define IIO_UNIVERSAL_DATA_GUID { 0x7FF396A1, 0xEE7D, 0x431E, { 0xBA, 0x53, 0x8F, 0xCA, 0x12, 0x7C, 0x44, 0xC0 } }

typedef enum {
  TYPE_UBOX = 0,              // Stack/Tile with only Ubox device
  TYPE_UBOX_IIO,              // DMI/PCIe Stack (Stack0) that hosts both IIO and Ubox devices (this is used in 14nm SOC)
  TYPE_IIO,                   // PCIe/CXL/DMI stack
  TYPE_MCP,
  TYPE_FPGA,
  TYPE_HFI,
  TYPE_NAC,
  TYPE_GRAPHICS,
  TYPE_DINO,
  TYPE_TSNO_DICH,
  TYPE_DICH,
  TYPE_TIP,
  TYPE_RESERVED,
  TYPE_DISABLED,              // This item must be prior to stack specific disable types
  TYPE_UBOX_IIO_DIS,
  TYPE_IIO_DIS,
  TYPE_MCP_DIS,
  TYPE_FPGA_DIS,
  TYPE_HFI_DIS,
  TYPE_NAC_DIS,
  TYPE_GRAPHICS_DIS,
  TYPE_DINO_DIS,
  TYPE_RESERVED_DIS,
  TYPE_TSNO_DICH_DIS,
  TYEP_DICH_DIS,
  TYPE_TIP_DIS,
  TYPE_NONE
} STACK_TYPE;


#ifndef MAX_SOCKET
#define MAX_SOCKET              2
#endif

#if (MAX_SOCKET == 1)
  #define MAX_FW_KTI_PORTS     4    // Maximum KTI PORTS to be used in structure definition.
#else
  #define MAX_FW_KTI_PORTS     6    // Maximum KTI PORTS to be used in structure definition
#endif //(MAX_SOCKET == 1)

#define MAX_LOGIC_IIO_STACK          18

#ifndef MAX_IIO_PCIROOTS_PER_STACK
#define MAX_IIO_PCIROOTS_PER_STACK   3   // PCI roots that can be created for a stack
#endif

#define MAX_COMPUTE_DIE            3
#define MAX_CHA_MAP                (2 * MAX_COMPUTE_DIE)  //for GNR & SRF only, each compute die has its own CAPID6 & CAPID7 (i.e. 2 CAPID registers)

#ifndef MAX_MESSAGE_LENGTH
#define MAX_MESSAGE_LENGTH  500
#endif

#pragma pack(1)

typedef enum {
  TYPE_SCF_BAR = 0,
  TYPE_PCU_BAR,
  TYPE_MEM_BAR0,
  TYPE_MEM_BAR1,
  TYPE_MEM_BAR2,
  TYPE_MEM_BAR3,
  TYPE_MEM_BAR4,
  TYPE_MEM_BAR5,
  TYPE_MEM_BAR6,
  TYPE_MEM_BAR7,
  TYPE_SBREG_BAR,
  TYPE_MAX_MMIO_BAR
} MMIO_BARS;

typedef struct {
  UINT8   Major;
  UINT8   Minor;
  UINT8   Revision;
  UINT16  BuildNumber;
} RC_VERSION;

//--------------------------------------------------------------------------------------//
// Structure definitions for Universal Data Store (UDS)
//--------------------------------------------------------------------------------------//
typedef struct {
  UINT8                     Valid;         // TRUE, if the link is valid (i.e reached normal operation)
  UINT8                     PeerSocId;     // Socket ID
  UINT8                     PeerSocType;   // Socket Type (0 - CPU; 1 - IIO)
  UINT8                     PeerPort;      // Port of the peer socket
} QPI_PEER_DATA;

typedef struct {
  UINT8                     Valid;
  UINT8                     PcieSegment;
  UINT64                    SegMmcfgBase;
  UINT32                    StackPresentBitmap;
  UINT16                    Cxl1p1PresentBitmap; // Bitmap of stacks where CXL 1p1 is connected
  UINT16                    CxlCapableBitmap;    // Bitmap of stacks capable of CXL
  UINT8                     TotM3Kti;
  UINT8                     TotCha;
  UINT32                    ChaList[MAX_CHA_MAP];
  UINT32                    SocId;
  QPI_PEER_DATA             PeerInfo[MAX_FW_KTI_PORTS];    // QPI LEP info
} QPI_CPU_DATA;

typedef struct {
  UINT8                     Valid;
  UINT8                     SocId;
  QPI_PEER_DATA             PeerInfo[MAX_SOCKET];    // QPI LEP info
} QPI_IIO_DATA;

/**
 * PCI resources that establish one PCI hierarchy for PCI Enumerator.
 */
typedef struct {
  UINT16                  UidType;               // Type of UID for this root bridge.
  UINT8                   BusBase;               // Base of PCI bus numbers available for PCI devices
  UINT8                   BusLimit;              // Limit of PCI bus numbers available for PCI devices
  UINT16                  IoBase;                // Base of IO resources available for PCI devices
  UINT16                  IoLimit;               // Limit of IO resources available for PCI devices
  UINT32                  Mmio32Base;            // Base of low MMIO resources available for PCI devices
  UINT32                  Mmio32Limit;           // Limit of low MMIO resources available for PCI devices
  UINT64                  Mmio64Base;            // Base of high MMIO resources available for PCI devices
  UINT64                  Mmio64Limit;           // Limit of high MMIO resources available for PCI devices
} UDS_PCIROOT_RES;

/**
 * This structore keeps resources configured in Host I/O Processor (HIOP) for one stack.
 * One HIOP may produce more than one PCI hierarchy, these are in PciRoot[] table.
 */
typedef struct {
  UINT8                   Personality;
  UINT8                   PciRootBridgeNum;      // Number of valid entries in PciRoot[] table
  UINT8                   BusBase;               // Base of Bus configured for this stack
  UINT8                   BusLimit;              // Limit of Bus configured for this stack
  UINT16                  IoBase;                // Base of IO configured for this stack
  UINT16                  IoLimit;               // Limit of IO configured for this stack
  UINT32                  IoApicBase;
  UINT32                  IoApicLimit;
  UINT32                  Mmio32Base;            // Base of low MMIO configured for this stack in memory map
  UINT32                  Mmio32Limit;           // Limit of low MMIO configured for this stack in memory map
  UINT64                  Mmio64Base;            // Base of high MMIO configured for this stack in memory map
  UINT64                  Mmio64Limit;           // Limit of high MMIO configured for this stack in memory map
  UDS_PCIROOT_RES         PciRoot[MAX_IIO_PCIROOTS_PER_STACK];
} UDS_STACK_RES;

typedef struct {
  UINT8                   Valid;
  UINT8                   SocketID;            // Socket ID of the IIO (0..3)
  UINT8                   BusBase;
  UINT8                   BusLimit;
  UINT16                  IoBase;
  UINT16                  IoLimit;
  UINT32                  IoApicBase;
  UINT32                  IoApicLimit;
  UINT32                  Mmio32Base;          // Base of low MMIO configured for this socket in memory map
  UINT32                  Mmio32Limit;         // Limit of low MMIO configured for this socket in memory map
  UINT64                  Mmio64Base;          // Base of high MMIO configured for this socket in memory map
  UINT64                  Mmio64Limit;         // Limit of high MMIO configured for this socket in memory map
  UDS_STACK_RES           StackRes[MAX_LOGIC_IIO_STACK];
} UDS_SOCKET_RES;

typedef struct {
  UINT16                  PlatGlobalIoBase;       // Global IO Base
  UINT16                  PlatGlobalIoLimit;      // Global IO Limit
  UINT32                  PlatGlobalMmio32Base;   // Global Mmiol base
  UINT32                  PlatGlobalMmio32Limit;  // Global Mmiol limit
  UINT64                  PlatGlobalMmio64Base;   // Global Mmioh Base [43:0]
  UINT64                  PlatGlobalMmio64Limit;  // Global Mmioh Limit [43:0]
  QPI_CPU_DATA            CpuQpiInfo[MAX_SOCKET]; // QPI related info per CPU
  QPI_IIO_DATA            IioQpiInfo[MAX_SOCKET]; // QPI related info per IIO
  UINT32                  MemTsegSize;
  UINT32                  MemIedSize;
  UINT64                  PciExpressBase;         // PCI Config Space base address
  UINT32                  PciExpressSize;         // PCI Config Space size
  UINT32                  MemTolm;
  UDS_SOCKET_RES          IIO_resource[MAX_SOCKET];
  UINT8                   numofIIO;
  UINT8                   MaxBusNumber;
  UINT32                  packageBspApicID[MAX_SOCKET]; // This data array is valid only for SBSP, not for non-SBSP CPUs. <AS> for CpuSv
  UINT16                  IoGranularity;
  UINT32                  MmiolGranularity;
  UINT64                  MmiohGranularity;
  UINT8                   RemoteRequestThreshold;  //5370389
  UINT32                  UboxMmioSize;
  UINT32                  MaxAddressBits;
} PLATFORM_DATA;

typedef struct {
    BOOLEAN                 FailFlag;
    CHAR16                  Message[MAX_MESSAGE_LENGTH];
} REBALANCE_FAIL_INFO;

typedef struct {
    UINT8                   CurrentUpiiLinkSpeed;    // Current programmed UPI Link speed (Slow/Full speed mode)
    UINT8                   CurrentUpiLinkFrequency; // Current requested UPI Link frequency (in GT)
    UINT8                   OutKtiCpuSktHotPlugEn;            // 0 - Disabled, 1 - Enabled for PM X2APIC
    UINT32                  OutKtiPerLinkL1En[MAX_SOCKET];    // output kti link enabled status for PM
    UINT8                   IsocEnable;
    UINT32                  meRequestedSize;        // Size of the memory range requested by ME FW, in MB
    UINT8                   DmiVc1;
    UINT8                   DmiVcm;
    UINT32                  CpuPCPSInfo;
    UINT8                   cpuSubType;
    UINT8                   SystemRasType;
    UINT8                   numCpus;           // 1,..4. Total number of CPU packages installed and detected (1..4)by QPI RC
    UINT16                  tolmLimit;
    RC_VERSION              RcVersion;
    UINT8                   bootMode;
    UINT8                   OutClusterOnDieEn; // Whether RC enabled COD support
    UINT8                   OutSncEn;
    UINT8                   OutNumOfCluster;
    UINT16                  LlcSizeReg;
    UINT8                   IoDcMode;
    UINT8                   BitsUsed;    //For 5 Level Paging
    REBALANCE_FAIL_INFO     RebalanceFailInfo;
} SYSTEM_STATUS;

typedef struct {
    PLATFORM_DATA           PlatformData;
    SYSTEM_STATUS           SystemStatus;
    UINT32                  OemValue;
} IIO_UDS;

#pragma pack()

#endif  // _IIO_UNIVERSAL_DATA_HOB_H_
