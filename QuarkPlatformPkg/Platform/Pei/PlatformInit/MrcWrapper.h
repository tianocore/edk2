/** @file
Framework PEIM to initialize memory on an DDR2 SDRAM Memory Controller.

Copyright (c) 2013 - 2016 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MRC_WRAPPER_H
#define _MRC_WRAPPER_H

#include <Ppi/QNCMemoryInit.h>
#include "PlatformEarlyInit.h"

//
// Define the default memory areas required
//
#define EDKII_RESERVED_SIZE_PAGES         0x20
#define ACPI_NVS_SIZE_PAGES               0x60
#define RUNTIME_SERVICES_DATA_SIZE_PAGES  0x20
#define RUNTIME_SERVICES_CODE_SIZE_PAGES  0x80
#define ACPI_RECLAIM_SIZE_PAGES           0x20
#define EDKII_DXE_MEM_SIZE_PAGES          0x20

#define AP_STARTUP_VECTOR                 0x00097000

//
// Maximum number of "Socket Sets", where a "Socket Set is a set of matching
// DIMM's from the various channels
//
#define MAX_SOCKET_SETS      2

//
// Maximum number of memory ranges supported by the memory controller
//
#define MAX_RANGES (MAX_ROWS + 5)

//
// Min. of 48MB PEI phase
//
#define  PEI_MIN_MEMORY_SIZE               (6 * 0x800000)
#define  PEI_RECOVERY_MIN_MEMORY_SIZE      (6 * 0x800000)

#define PEI_MEMORY_RANGE_OPTION_ROM UINT32
#define PEI_MR_OPTION_ROM_NONE      0x00000000

//
// SMRAM Memory Range
//
#define PEI_MEMORY_RANGE_SMRAM      UINT32
#define PEI_MR_SMRAM_ALL            0xFFFFFFFF
#define PEI_MR_SMRAM_NONE           0x00000000
#define PEI_MR_SMRAM_CACHEABLE_MASK 0x80000000
#define PEI_MR_SMRAM_SEGTYPE_MASK   0x00FF0000
#define PEI_MR_SMRAM_ABSEG_MASK     0x00010000
#define PEI_MR_SMRAM_HSEG_MASK      0x00020000
#define PEI_MR_SMRAM_TSEG_MASK      0x00040000
//
// SMRAM Size is a multiple of 128KB.
//
#define PEI_MR_SMRAM_SIZE_MASK          0x0000FFFF

//
// Pci Memory Hole
//
#define PEI_MEMORY_RANGE_PCI_MEMORY       UINT32

typedef enum {
  Ignore,
  Quick,
  Sparse,
  Extensive
} PEI_MEMORY_TEST_OP;

//
// MRC Params Variable structure.
//

typedef struct {
  MrcTimings_t timings;              // Actual MRC config values saved in variable store.
  UINT8        VariableStorePad[8];  // Allow for data stored in variable is required to be multiple of 8bytes.
} PLATFORM_VARIABLE_MEMORY_CONFIG_DATA;

///
/// MRC Params Platform Data Flags bits
///
#define PDAT_MRC_FLAG_ECC_EN            BIT0
#define PDAT_MRC_FLAG_SCRAMBLE_EN       BIT1
#define PDAT_MRC_FLAG_MEMTEST_EN        BIT2
#define PDAT_MRC_FLAG_TOP_TREE_EN       BIT3  ///< 0b DDR "fly-by" topology else 1b DDR "tree" topology.
#define PDAT_MRC_FLAG_WR_ODT_EN         BIT4  ///< If set ODR signal is asserted to DRAM devices on writes.

///
/// MRC Params Platform Data.
///
typedef struct {
  UINT32       Flags;                   ///< Bitmap of PDAT_MRC_FLAG_XXX defs above.
  UINT8        DramWidth;               ///< 0=x8, 1=x16, others=RESERVED.
  UINT8        DramSpeed;               ///< 0=DDRFREQ_800, 1=DDRFREQ_1066, others=RESERVED. Only 533MHz SKU support 1066 memory.
  UINT8        DramType;                ///< 0=DDR3,1=DDR3L, others=RESERVED.
  UINT8        RankMask;                ///< bit[0] RANK0_EN, bit[1] RANK1_EN, others=RESERVED.
  UINT8        ChanMask;                ///< bit[0] CHAN0_EN, others=RESERVED.
  UINT8        ChanWidth;               ///< 1=x16, others=RESERVED.
  UINT8        AddrMode;                ///< 0, 1, 2 (mode 2 forced if ecc enabled), others=RESERVED.
  UINT8        SrInt;                   ///< 1=1.95us, 2=3.9us, 3=7.8us, others=RESERVED. REFRESH_RATE.
  UINT8        SrTemp;                  ///< 0=normal, 1=extended, others=RESERVED.
  UINT8        DramRonVal;              ///< 0=34ohm, 1=40ohm, others=RESERVED. RON_VALUE Select MRS1.DIC driver impedance control.
  UINT8        DramRttNomVal;           ///< 0=40ohm, 1=60ohm, 2=120ohm, others=RESERVED.
  UINT8        DramRttWrVal;            ///< 0=off others=RESERVED.
  UINT8        SocRdOdtVal;             ///< 0=off, 1=60ohm, 2=120ohm, 3=180ohm, others=RESERVED.
  UINT8        SocWrRonVal;             ///< 0=27ohm, 1=32ohm, 2=40ohm, others=RESERVED.
  UINT8        SocWrSlewRate;           ///< 0=2.5V/ns, 1=4V/ns, others=RESERVED.
  UINT8        DramDensity;             ///< 0=512Mb, 1=1Gb, 2=2Gb, 3=4Gb, others=RESERVED.
  UINT32       tRAS;                    ///< ACT to PRE command period in picoseconds.
  UINT32       tWTR;                    ///< Delay from start of internal write transaction to internal read command in picoseconds.
  UINT32       tRRD;                    ///< ACT to ACT command period (JESD79 specific to page size 1K/2K) in picoseconds.
  UINT32       tFAW;                    ///< Four activate window (JESD79 specific to page size 1K/2K) in picoseconds.
  UINT8        tCL;                     ///< DRAM CAS Latency in clocks.
} PDAT_MRC_ITEM;

//
// Memory range types
//
typedef enum {
  DualChannelDdrMainMemory,
  DualChannelDdrSmramCacheable,
  DualChannelDdrSmramNonCacheable,
  DualChannelDdrGraphicsMemoryCacheable,
  DualChannelDdrGraphicsMemoryNonCacheable,
  DualChannelDdrReservedMemory,
  DualChannelDdrMaxMemoryRangeType
} PEI_DUAL_CHANNEL_DDR_MEMORY_RANGE_TYPE;

//
// Memory map range information
//
typedef struct {
  EFI_PHYSICAL_ADDRESS                          PhysicalAddress;
  EFI_PHYSICAL_ADDRESS                          CpuAddress;
  EFI_PHYSICAL_ADDRESS                          RangeLength;
  PEI_DUAL_CHANNEL_DDR_MEMORY_RANGE_TYPE        Type;
} PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE;

//
// Function prototypes.
//

EFI_STATUS
InstallEfiMemory (
  IN      EFI_PEI_SERVICES                           **PeiServices,
  IN      EFI_PEI_READ_ONLY_VARIABLE2_PPI            *VariableServices,
  IN      EFI_BOOT_MODE                              BootMode,
  IN      UINT32                                     TotalMemorySize
  );

EFI_STATUS
InstallS3Memory (
  IN      EFI_PEI_SERVICES                      **PeiServices,
  IN      EFI_PEI_READ_ONLY_VARIABLE2_PPI       *VariableServices,
  IN      UINT32                                TotalMemorySize
  );

EFI_STATUS
MemoryInit (
  IN EFI_PEI_SERVICES                       **PeiServices
  );


EFI_STATUS
LoadConfig (
  IN      EFI_PEI_SERVICES                        **PeiServices,
  IN      EFI_PEI_READ_ONLY_VARIABLE2_PPI         *VariableServices,
  IN OUT  MRCParams_t                             *MrcData
  );

EFI_STATUS
SaveConfig (
  IN      MRCParams_t                      *MrcData
  );

VOID
RetriveRequiredMemorySize (
  IN      EFI_PEI_SERVICES                  **PeiServices,
  OUT     UINTN                             *Size
  );

EFI_STATUS
GetMemoryMap (
  IN     EFI_PEI_SERVICES                                    **PeiServices,
  IN     UINT32                                              TotalMemorySize,
  IN OUT PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE               *MemoryMap,
  IN OUT UINT8                                               *NumRanges
  );

EFI_STATUS
ChooseRanges (
  IN OUT   PEI_MEMORY_RANGE_OPTION_ROM      *OptionRomMask,
  IN OUT   PEI_MEMORY_RANGE_SMRAM           *SmramMask,
  IN OUT   PEI_MEMORY_RANGE_PCI_MEMORY      *PciMemoryMask
  );

EFI_STATUS
GetPlatformMemorySize (
  IN      EFI_PEI_SERVICES                       **PeiServices,
  IN      EFI_BOOT_MODE                          BootMode,
  IN OUT  UINT64                                 *MemorySize
  );

EFI_STATUS
BaseMemoryTest (
  IN  EFI_PEI_SERVICES                   **PeiServices,
  IN  EFI_PHYSICAL_ADDRESS               BeginAddress,
  IN  UINT64                             MemoryLength,
  IN  PEI_MEMORY_TEST_OP                 Operation,
  OUT EFI_PHYSICAL_ADDRESS               *ErrorAddress
  );

EFI_STATUS
SetPlatformImrPolicy (
  IN      EFI_PHYSICAL_ADDRESS    PeiMemoryBaseAddress,
  IN      UINT64                  PeiMemoryLength,
  IN      UINTN                   RequiredMemSize
  );

VOID
EFIAPI
InfoPostInstallMemory (
  OUT     UINT32                  *RmuBaseAddressPtr OPTIONAL,
  OUT     EFI_SMRAM_DESCRIPTOR    **SmramDescriptorPtr OPTIONAL,
  OUT     UINTN                   *NumSmramRegionsPtr OPTIONAL
  );

#endif
