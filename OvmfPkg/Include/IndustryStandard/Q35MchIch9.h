/** @file
  Various register numbers and value bits based on the following publications:
  - Intel(R) datasheet 316966-002
  - Intel(R) datasheet 316972-004

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __Q35_MCH_ICH9_H__
#define __Q35_MCH_ICH9_H__

#include <Library/PciLib.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Protocol/PciRootBridgeIo.h>

//
// Host Bridge Device ID (DID) value for Q35/MCH
//
#define INTEL_Q35_MCH_DEVICE_ID 0x29C0

//
// B/D/F/Type: 0/0/0/PCI
//
#define DRAMC_REGISTER_Q35(Offset) PCI_LIB_ADDRESS (0, 0, 0, (Offset))

#define MCH_EXT_TSEG_MB       0x50
#define MCH_EXT_TSEG_MB_QUERY   0xFFFF

#define MCH_GGC               0x52
#define MCH_GGC_IVD             BIT1

#define MCH_PCIEXBAR_LOW      0x60
#define MCH_PCIEXBAR_LOWMASK    0x0FFFFFFF
#define MCH_PCIEXBAR_BUS_FF     0
#define MCH_PCIEXBAR_EN         BIT0

#define MCH_PCIEXBAR_HIGH     0x64
#define MCH_PCIEXBAR_HIGHMASK   0xFFFFFFF0

#define MCH_PAM0              0x90
#define MCH_PAM1              0x91
#define MCH_PAM2              0x92
#define MCH_PAM3              0x93
#define MCH_PAM4              0x94
#define MCH_PAM5              0x95
#define MCH_PAM6              0x96

#define MCH_SMRAM             0x9D
#define MCH_SMRAM_D_LCK         BIT4
#define MCH_SMRAM_G_SMRAME      BIT3

#define MCH_ESMRAMC           0x9E
#define MCH_ESMRAMC_H_SMRAME    BIT7
#define MCH_ESMRAMC_E_SMERR     BIT6
#define MCH_ESMRAMC_SM_CACHE    BIT5
#define MCH_ESMRAMC_SM_L1       BIT4
#define MCH_ESMRAMC_SM_L2       BIT3
#define MCH_ESMRAMC_TSEG_EXT    (BIT2 | BIT1)
#define MCH_ESMRAMC_TSEG_8MB    BIT2
#define MCH_ESMRAMC_TSEG_2MB    BIT1
#define MCH_ESMRAMC_TSEG_1MB    0
#define MCH_ESMRAMC_TSEG_MASK   (BIT2 | BIT1)
#define MCH_ESMRAMC_T_EN        BIT0

#define MCH_GBSM              0xA4
#define MCH_GBSM_MB_SHIFT       20

#define MCH_BGSM              0xA8
#define MCH_BGSM_MB_SHIFT       20

#define MCH_TSEGMB            0xAC
#define MCH_TSEGMB_MB_SHIFT     20

#define MCH_TOLUD             0xB0
#define MCH_TOLUD_MB_SHIFT      4

//
// B/D/F/Type: 0/0x1f/0/PCI
//
#define POWER_MGMT_REGISTER_Q35(Offset) \
  PCI_LIB_ADDRESS (0, 0x1f, 0, (Offset))

#define POWER_MGMT_REGISTER_Q35_EFI_PCI_ADDRESS(Offset) \
  EFI_PCI_ADDRESS (0, 0x1f, 0, (Offset))

#define ICH9_PMBASE               0x40
#define ICH9_PMBASE_MASK            (BIT15 | BIT14 | BIT13 | BIT12 | BIT11 | \
                                     BIT10 | BIT9  | BIT8  | BIT7)

#define ICH9_ACPI_CNTL            0x44
#define ICH9_ACPI_CNTL_ACPI_EN      BIT7

#define ICH9_GEN_PMCON_1          0xA0
#define ICH9_GEN_PMCON_1_SMI_LOCK   BIT4

#define ICH9_RCBA                 0xF0
#define ICH9_RCBA_EN                BIT0

//
// IO ports
//
#define ICH9_APM_CNT 0xB2
#define ICH9_APM_STS 0xB3

//
// IO ports relative to PMBASE
//
#define ICH9_PMBASE_OFS_SMI_EN 0x30
#define ICH9_SMI_EN_APMC_EN      BIT5
#define ICH9_SMI_EN_GBL_SMI_EN   BIT0

#define ICH9_ROOT_COMPLEX_BASE 0xFED1C000

#endif
