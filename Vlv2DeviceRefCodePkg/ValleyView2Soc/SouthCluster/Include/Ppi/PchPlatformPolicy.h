/**
**/
/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



  @file
  PchPlatformPolicy.h

  @brief
  PCH policy PPI produced by a platform driver specifying various
  expected PCH settings. This PPI is consumed by the PCH PEI modules.

**/
#ifndef PCH_PLATFORM_POLICY_H_
#define PCH_PLATFORM_POLICY_H_
//
// External include files do NOT need to be explicitly specified in real EDKII
// environment
//


#include "PchRegs.h"

//
#define PCH_PLATFORM_POLICY_PPI_GUID \
  { \
    0x15344673, 0xd365, 0x4be2, 0x85, 0x13, 0x14, 0x97, 0xcc, 0x7, 0x61, 0x1d \
  }

extern EFI_GUID                         gPchPlatformPolicyPpiGuid;

///
/// Forward reference for ANSI C compatibility
///
typedef struct _PCH_PLATFORM_POLICY_PPI PCH_PLATFORM_POLICY_PPI;

///
/// PPI revision number
/// Any backwards compatible changes to this PPI will result in an update in the revision number
/// Major changes will require publication of a new PPI
///
/// Revision 1:    Original version
///
#define PCH_PLATFORM_POLICY_PPI_REVISION_1  1
#define PCH_PLATFORM_POLICY_PPI_REVISION_2  2
#define PCH_PLATFORM_POLICY_PPI_REVISION_3  3
#define PCH_PLATFORM_POLICY_PPI_REVISION_4  4
#define PCH_PLATFORM_POLICY_PPI_REVISION_5  5
//
// Generic definitions for device enabling/disabling used by PCH code.
//
#define PCH_DEVICE_ENABLE   1
#define PCH_DEVICE_DISABLE  0

typedef struct {
  UINT8  ThermalDataReportEnable  : 1;   // OBSOLETE from Revision 5 !!! DO NOT USE !!!
  UINT8  MchTempReadEnable        : 1;
  UINT8  PchTempReadEnable        : 1;
  UINT8  CpuEnergyReadEnable      : 1;
  UINT8  CpuTempReadEnable        : 1;
  UINT8  Cpu2TempReadEnable       : 1;
  UINT8  TsOnDimmEnable           : 1;
  UINT8  Dimm1TempReadEnable      : 1;

  UINT8  Dimm2TempReadEnable      : 1;
  UINT8  Dimm3TempReadEnable      : 1;
  UINT8  Dimm4TempReadEnable      : 1;
  UINT8  Rsvdbits                 : 5;
} PCH_THERMAL_REPORT_CONTROL;
//
// ---------------------------- HPET Config -----------------------------
//
typedef struct {
  BOOLEAN Enable; /// Determines if enable HPET function
  UINT32  Base;   /// The HPET base address
} PCH_HPET_CONFIG;


///
/// ---------------------------- SATA Config -----------------------------
///
typedef enum {
  PchSataModeIde,
  PchSataModeAhci,
  PchSataModeRaid,
  PchSataModeMax
} PCH_SATA_MODE;

///
/// ---------------------------- PCI Express Config -----------------------------
///
typedef enum {
  PchPcieAuto,
  PchPcieGen1,
  PchPcieGen2
} PCH_PCIE_SPEED;

typedef struct {
  PCH_PCIE_SPEED  PcieSpeed[PCH_PCIE_MAX_ROOT_PORTS];
} PCH_PCIE_CONFIG;

///
/// ---------------------------- IO APIC Config -----------------------------
///
typedef struct {
  UINT8 IoApicId;
} PCH_IOAPIC_CONFIG;

///
/// --------------------- Low Power Input Output Config ------------------------
///
typedef struct {
  UINT8                   LpssPciModeEnabled    : 1;    /// Determines if LPSS PCI Mode enabled
  UINT8                   Dma0Enabled           : 1;     /// Determines if LPSS DMA1 enabled
  UINT8                   Dma1Enabled           : 1;     /// Determines if LPSS DMA2 enabled
  UINT8                   I2C0Enabled           : 1;     /// Determines if LPSS I2C #1 enabled
  UINT8                   I2C1Enabled           : 1;     /// Determines if LPSS I2C #2 enabled
  UINT8                   I2C2Enabled           : 1;     /// Determines if LPSS I2C #3 enabled
  UINT8                   I2C3Enabled           : 1;     /// Determines if LPSS I2C #4 enabled
  UINT8                   I2C4Enabled           : 1;     /// Determines if LPSS I2C #5 enabled
  UINT8                   I2C5Enabled           : 1;     /// Determines if LPSS I2C #6 enabled
  UINT8                   I2C6Enabled           : 1;     /// Determines if LPSS I2C #7 enabled
  UINT8                   Pwm0Enabled           : 1;     /// Determines if LPSS PWM #1 enabled
  UINT8                   Pwm1Enabled           : 1;     /// Determines if LPSS PWM #2 enabled
  UINT8                   Hsuart0Enabled        : 1;     /// Determines if LPSS HSUART #1 enabled
  UINT8                   Hsuart1Enabled        : 1;     /// Determines if LPSS HSUART #2 enabled
  UINT8                   SpiEnabled            : 1;     /// Determines if LPSS SPI enabled
  UINT8                   Rsvdbits              : 2;
} PEI_PCH_LPSS_CONFIG;

///
/// ------------ General PCH Platform Policy PPI definition ------------
///
struct _PCH_PLATFORM_POLICY_PPI {
  UINT8                         Revision;
  UINT8                         BusNumber;  // Bus Number of the PCH device
  UINT32                        SpiBase;    // SPI Base Address.
  UINT32                        PmcBase;    // PMC Base Address.
  UINT32                        SmbmBase;   // SMB Memory Base Address.
  UINT32                        IoBase;     // IO Base Address.
  UINT32                        IlbBase;    // Intel Legacy Block Base Address.
  UINT32                        PUnitBase;  // PUnit Base Address.
  UINT32                        Rcba;       // Root Complex Base Address.
  UINT32                        MphyBase;   // MPHY Base Address.
  UINT16                        AcpiBase;   // ACPI I/O Base address.
  UINT16                        GpioBase;   // GPIO Base address
  PCH_HPET_CONFIG               *HpetConfig;
  PCH_SATA_MODE                 SataMode;
  PCH_PCIE_CONFIG               *PcieConfig;
  PCH_IOAPIC_CONFIG             *IoApicConfig;
  PEI_PCH_LPSS_CONFIG           *LpssConfig;
  BOOLEAN                       EnableRmh;      // Determines if enable USB RMH function
  BOOLEAN                       EhciPllCfgEnable;
};

#endif
