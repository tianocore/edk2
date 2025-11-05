/** @file
  Header file for MADT table parser

  Copyright (c) 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - Arm Generic Interrupt Controller Architecture Specification,
      GIC architecture version 3 and version 4, issue E
    - Arm Server Base System Architecture 5.0
**/

#ifndef MADT_PARSER_H_
#define MADT_PARSER_H_

///
/// Level 3 base server system Private Peripheral Inerrupt (PPI) ID assignments
///
#define ARM_PPI_ID_OVERFLOW_INTERRUPT_FROM_CNTP    30
#define ARM_PPI_ID_OVERFLOW_INTERRUPT_FROM_CNTPS   29
#define ARM_PPI_ID_OVERFLOW_INTERRUPT_FROM_CNTHV   28
#define ARM_PPI_ID_OVERFLOW_INTERRUPT_FROM_CNTV    27
#define ARM_PPI_ID_OVERFLOW_INTERRUPT_FROM_CNTHP   26
#define ARM_PPI_ID_GIC_MAINTENANCE_INTERRUPT       25
#define ARM_PPI_ID_CTIIRQ                          24
#define ARM_PPI_ID_PERFORMANCE_MONITORS_INTERRUPT  23
#define ARM_PPI_ID_COMMIRQ                         22
#define ARM_PPI_ID_PMBIRQ                          21
#define ARM_PPI_ID_CNTHPS                          20
#define ARM_PPI_ID_CNTHVS                          19

///
/// PPI ID allowed ranges
///
#define ARM_PPI_ID_MAX           31
#define ARM_PPI_ID_MIN           16
#define ARM_PPI_ID_EXTENDED_MAX  1119
#define ARM_PPI_ID_EXTENDED_MIN  1056

#endif // MADT_PARSER_H_
