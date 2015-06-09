/** @file
  Various register numbers and value bits based on the following publications:
  - Intel(R) datasheet 316966-002
  - Intel(R) datasheet 316972-004

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.   The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __Q35_MCH_ICH9_H__
#define __Q35_MCH_ICH9_H__

#include <Library/PciLib.h>

//
// Host Bridge Device ID (DID) value for Q35/MCH
//
#define INTEL_Q35_MCH_DEVICE_ID 0x29C0

//
// B/D/F/Type: 0/0/0/PCI
//
#define DRAMC_REGISTER_Q35(Offset) PCI_LIB_ADDRESS (0, 0, 0, (Offset))

#define MCH_GGC               0x52
#define MCH_GGC_IVD             BIT1

#define MCH_SMRAM             0x9D
#define MCH_SMRAM_D_LCK         BIT4
#define MCH_SMRAM_G_SMRAME      BIT3

#define MCH_ESMRAMC           0x9E
#define MCH_ESMRAMC_H_SMRAME    BIT7
#define MCH_ESMRAMC_E_SMERR     BIT6
#define MCH_ESMRAMC_SM_CACHE    BIT5
#define MCH_ESMRAMC_SM_L1       BIT4
#define MCH_ESMRAMC_SM_L2       BIT3
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
