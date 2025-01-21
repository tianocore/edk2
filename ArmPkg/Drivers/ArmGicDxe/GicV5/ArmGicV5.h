/** @file
*
*  Copyright (c) 2025, Arm Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef ARMGICV5_H_
#define ARMGICV5_H_

#define GICV5_IRS_CONFIG_FRAME_SIZE  SIZE_64KB

#define GICV5_IRS_IDR5         0x14
#define GICV5_IRS_IDR6         0x18
#define GICV5_IRS_IDR7         0x1c
#define GICV5_IRS_SPI_SELR     0x108
#define GICV5_IRS_SPI_CFGR     0x114
#define GICV5_IRS_SPI_STATUSR  0x118

#define GICV5_IRS_SPI_STATUSR_IDLE  (1 << 0)
#define GICV5_IRS_SPI_STATUSR_V     (1 << 1)

#define GICV5_PPI_EDGE_TRIGGER   0
#define GICV5_PPI_LEVEL_TRIGGER  1

#define GICV5_INTERRUPT_ID_MASK    0xFFFFFF
#define GICV5_INTERRUPT_TYPE_MASK  (7 << 29)

#define GICV5_INTERRUPT_ID_VALID  (1ull << 32)

#define GICV5_INTERRUPT_TYPE_PPI  (1 << 29)
#define GICV5_INTERRUPT_TYPE_SPI  (3 << 29)

#define GICV5_NUM_PPI_INTERRUPTS  128

#define GICV5_IRS_IDLE_TIMEOUT_MS  10

/**
  Read GICv5 ICC_PPI_ENABLER0_EL1 system register. This provides the interrupt
  enable mask for PPIs 0-63.

  @return Value of ICC_PPI_ENABLER0_EL1
**/
UINT64
EFIAPI
ArmGicV5GetPpiEnabler0 (
  VOID
  );

/**
  Read GICv5 ICC_PPI_ENABLER1_EL1 system register.. This provides the interrupt
  enable mask for PPIs 64-127.

  @return Value of ICC_PPI_ENABLER1_EL1
**/
UINT64
EFIAPI
ArmGicV5GetPpiEnabler1 (
  VOID
  );

/**
  Write to GICv5 ICC_PPI_ENABLER0_EL1 system register. This provides the
  interrupt enable mask for PPIs 0-63.

  @param  InterruptMask   New interrupt mask to write
**/
VOID
EFIAPI
ArmGicV5SetPpiEnabler0 (
  IN UINT64  InterruptMask
  );

/**
  Write to GICv5 ICC_PPI_ENABLER1_EL1 system register. This provides the
  interrupt enable mask for PPIs 64-127.

  @param  InterruptMask   New interrupt mask to write
**/
VOID
EFIAPI
ArmGicV5SetPpiEnabler1 (
  IN UINT64  InterruptMask
  );

/**
  Read GICv5 ICC_PPI_HMR0_EL1 system register. This provides the interrupt
  trigger type for PPIs 0-63.

  @return Value of ICC_PPI_HMR0_EL1
**/
UINT64
EFIAPI
ArmGicV5GetPPIHMR0 (
  VOID
  );

/**
  Read GICv5 ICC_PPI_HMR1_EL1 system register. This provides the interrupt
  trigger type for PPIs 64-127.

  @return Value of ICC_PPI_HMR1_EL1
**/
UINT64
EFIAPI
ArmGicV5GetPPIHMR1 (
  VOID
  );

/**

  @param  SpiId  ID of SPI to enable
**/
VOID
EFIAPI
ArmGicV5SpiEnable (
  IN UINT32  SpiId
  );

/**

  @param  SpiId  ID of SPI to disable
**/
VOID
EFIAPI
ArmGicV5SpiDisable (
  IN UINT32  SpiId
  );

UINT64
EFIAPI
ArmGicV5ReadInterruptConfig (
  IN UINT32  InterruptId
  );

/**
  Enable GICv5 interrupt interface.
**/
VOID
EFIAPI
ArmGicV5EnableInterruptInterface (
  UINT64  IrsConfigFrameBase
  );

/**
  Disable GICv5 interrupt interface.
**/
VOID
EFIAPI
ArmGicV5DisableInterruptInterface (
  UINT64  IrsConfigFrameBase
  );

/**
  Clear the active state of the given GICv5 interrupt.

  @param   InterruptId   The INTID of the interrupt to deactivate
**/
VOID
EFIAPI
ArmGicV5DeactivateInterrupt (
  IN UINTN  InterruptId
  );

/**
  Signal End of Interrupt, causing a priority drop of the running priority at
  the CPU interface in the current interrupt domain.
**/
VOID
EFIAPI
ArmGicV5EndOfInterrupt (
  VOID
  );

/**
  Acknowledge the highest priority pending interrupt (HPPI) in the current
  interrupt domain.
**/
UINTN
EFIAPI
ArmGicV5AcknowledgeInterrupt (
  VOID
  );

#endif // ARMGICV5_H_
