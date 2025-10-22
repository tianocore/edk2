/** @file

  Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EmmcBlockIoPei.h"

/**
  Read/Write specified EMMC host controller mmio register.

  @param[in]      Address      The address of the mmio register to be read/written.
  @param[in]      Read         A boolean to indicate it's read or write operation.
  @param[in]      Count        The width of the mmio register in bytes.
                               Must be 1, 2 , 4 or 8 bytes.
  @param[in, out] Data         For read operations, the destination buffer to store
                               the results. For write operations, the source buffer
                               to write data from. The caller is responsible for
                               having ownership of the data buffer and ensuring its
                               size not less than Count bytes.

  @retval EFI_INVALID_PARAMETER The Address or the Data or the Count is not valid.
  @retval EFI_SUCCESS           The read/write operation succeeds.
  @retval Others                The read/write operation fails.

**/
EFI_STATUS
EFIAPI
EmmcPeimHcRwMmio (
  IN     UINTN    Address,
  IN     BOOLEAN  Read,
  IN     UINT8    Count,
  IN OUT VOID     *Data
  )
{
  if ((Address == 0) || (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Count != 1) && (Count != 2) && (Count != 4) && (Count != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Count) {
    case 1:
      if (Read) {
        *(UINT8 *)Data = MmioRead8 (Address);
      } else {
        MmioWrite8 (Address, *(UINT8 *)Data);
      }

      break;
    case 2:
      if (Read) {
        *(UINT16 *)Data = MmioRead16 (Address);
      } else {
        MmioWrite16 (Address, *(UINT16 *)Data);
      }

      break;
    case 4:
      if (Read) {
        *(UINT32 *)Data = MmioRead32 (Address);
      } else {
        MmioWrite32 (Address, *(UINT32 *)Data);
      }

      break;
    case 8:
      if (Read) {
        *(UINT64 *)Data = MmioRead64 (Address);
      } else {
        MmioWrite64 (Address, *(UINT64 *)Data);
      }

      break;
    default:
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Do OR operation with the value of the specified EMMC host controller mmio register.

  @param[in] Address           The address of the mmio register to be read/written.
  @param[in] Count             The width of the mmio register in bytes.
                               Must be 1, 2 , 4 or 8 bytes.
  @param[in] OrData            The pointer to the data used to do OR operation.
                               The caller is responsible for having ownership of
                               the data buffer and ensuring its size not less than
                               Count bytes.

  @retval EFI_INVALID_PARAMETER The Address or the OrData or the Count is not valid.
  @retval EFI_SUCCESS           The OR operation succeeds.
  @retval Others                The OR operation fails.

**/
EFI_STATUS
EFIAPI
EmmcPeimHcOrMmio (
  IN  UINTN  Address,
  IN  UINT8  Count,
  IN  VOID   *OrData
  )
{
  EFI_STATUS  Status;
  UINT64      Data;
  UINT64      Or;

  Status = EmmcPeimHcRwMmio (Address, TRUE, Count, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Count == 1) {
    Or = *(UINT8 *)OrData;
  } else if (Count == 2) {
    Or = *(UINT16 *)OrData;
  } else if (Count == 4) {
    Or = *(UINT32 *)OrData;
  } else if (Count == 8) {
    Or = *(UINT64 *)OrData;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  Data  |= Or;
  Status = EmmcPeimHcRwMmio (Address, FALSE, Count, &Data);

  return Status;
}

/**
  Do AND operation with the value of the specified EMMC host controller mmio register.

  @param[in] Address           The address of the mmio register to be read/written.
  @param[in] Count             The width of the mmio register in bytes.
                               Must be 1, 2 , 4 or 8 bytes.
  @param[in] AndData           The pointer to the data used to do AND operation.
                               The caller is responsible for having ownership of
                               the data buffer and ensuring its size not less than
                               Count bytes.

  @retval EFI_INVALID_PARAMETER The Address or the AndData or the Count is not valid.
  @retval EFI_SUCCESS           The AND operation succeeds.
  @retval Others                The AND operation fails.

**/
EFI_STATUS
EFIAPI
EmmcPeimHcAndMmio (
  IN  UINTN  Address,
  IN  UINT8  Count,
  IN  VOID   *AndData
  )
{
  EFI_STATUS  Status;
  UINT64      Data;
  UINT64      And;

  Status = EmmcPeimHcRwMmio (Address, TRUE, Count, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Count == 1) {
    And = *(UINT8 *)AndData;
  } else if (Count == 2) {
    And = *(UINT16 *)AndData;
  } else if (Count == 4) {
    And = *(UINT32 *)AndData;
  } else if (Count == 8) {
    And = *(UINT64 *)AndData;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  Data  &= And;
  Status = EmmcPeimHcRwMmio (Address, FALSE, Count, &Data);

  return Status;
}

/**
  Wait for the value of the specified MMIO register set to the test value.

  @param[in]  Address       The address of the mmio register to be checked.
  @param[in]  Count         The width of the mmio register in bytes.
                            Must be 1, 2, 4 or 8 bytes.
  @param[in]  MaskValue     The mask value of memory.
  @param[in]  TestValue     The test value of memory.

  @retval EFI_NOT_READY     The MMIO register hasn't set to the expected value.
  @retval EFI_SUCCESS       The MMIO register has expected value.
  @retval Others            The MMIO operation fails.

**/
EFI_STATUS
EFIAPI
EmmcPeimHcCheckMmioSet (
  IN  UINTN   Address,
  IN  UINT8   Count,
  IN  UINT64  MaskValue,
  IN  UINT64  TestValue
  )
{
  EFI_STATUS  Status;
  UINT64      Value;

  //
  // Access PCI MMIO space to see if the value is the tested one.
  //
  Value  = 0;
  Status = EmmcPeimHcRwMmio (Address, TRUE, Count, &Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Value &= MaskValue;

  if (Value == TestValue) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_READY;
}

/**
  Wait for the value of the specified MMIO register set to the test value.

  @param[in]  Address       The address of the mmio register to wait.
  @param[in]  Count         The width of the mmio register in bytes.
                            Must be 1, 2, 4 or 8 bytes.
  @param[in]  MaskValue     The mask value of memory.
  @param[in]  TestValue     The test value of memory.
  @param[in]  Timeout       The time out value for wait memory set, uses 1
                            microsecond as a unit.

  @retval EFI_TIMEOUT       The MMIO register hasn't expected value in timeout
                            range.
  @retval EFI_SUCCESS       The MMIO register has expected value.
  @retval Others            The MMIO operation fails.

**/
EFI_STATUS
EFIAPI
EmmcPeimHcWaitMmioSet (
  IN  UINTN   Address,
  IN  UINT8   Count,
  IN  UINT64  MaskValue,
  IN  UINT64  TestValue,
  IN  UINT64  Timeout
  )
{
  EFI_STATUS  Status;
  BOOLEAN     InfiniteWait;

  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  while (InfiniteWait || (Timeout > 0)) {
    Status = EmmcPeimHcCheckMmioSet (
               Address,
               Count,
               MaskValue,
               TestValue
               );
    if (Status != EFI_NOT_READY) {
      return Status;
    }

    //
    // Stall for 1 microsecond.
    //
    MicroSecondDelay (1);

    Timeout--;
  }

  return EFI_TIMEOUT;
}

/**
  Software reset the specified EMMC host controller and enable all interrupts.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The software reset executes successfully.
  @retval Others            The software reset fails.

**/
EFI_STATUS
EmmcPeimHcReset (
  IN UINTN  Bar
  )
{
  EFI_STATUS  Status;
  UINT8       SwReset;

  SwReset = 0xFF;
  Status  = EmmcPeimHcRwMmio (Bar + EMMC_HC_SW_RST, FALSE, sizeof (SwReset), &SwReset);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcPeimHcReset: write full 1 fails: %r\n", Status));
    return Status;
  }

  Status = EmmcPeimHcWaitMmioSet (
             Bar + EMMC_HC_SW_RST,
             sizeof (SwReset),
             0xFF,
             0x00,
             EMMC_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "EmmcPeimHcReset: reset done with %r\n", Status));
    return Status;
  }

  //
  // Enable all interrupt after reset all.
  //
  Status = EmmcPeimHcEnableInterrupt (Bar);

  return Status;
}

/**
  Set all interrupt status bits in Normal and Error Interrupt Status Enable
  register.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The operation executes successfully.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimHcEnableInterrupt (
  IN UINTN  Bar
  )
{
  EFI_STATUS  Status;
  UINT16      IntStatus;

  //
  // Enable all bits in Error Interrupt Status Enable Register
  //
  IntStatus = 0xFFFF;
  Status    = EmmcPeimHcRwMmio (Bar + EMMC_HC_ERR_INT_STS_EN, FALSE, sizeof (IntStatus), &IntStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Enable all bits in Normal Interrupt Status Enable Register
  //
  IntStatus = 0xFFFF;
  Status    = EmmcPeimHcRwMmio (Bar + EMMC_HC_NOR_INT_STS_EN, FALSE, sizeof (IntStatus), &IntStatus);

  return Status;
}

/**
  Get the capability data from the specified slot.

  @param[in]  Bar             The mmio base address of the slot to be accessed.
  @param[out] Capability      The buffer to store the capability data.

  @retval EFI_SUCCESS         The operation executes successfully.
  @retval Others              The operation fails.

**/
EFI_STATUS
EmmcPeimHcGetCapability (
  IN     UINTN          Bar,
  OUT EMMC_HC_SLOT_CAP  *Capability
  )
{
  EFI_STATUS  Status;
  UINT64      Cap;

  Status = EmmcPeimHcRwMmio (Bar + EMMC_HC_CAP, TRUE, sizeof (Cap), &Cap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (Capability, &Cap, sizeof (Cap));

  return EFI_SUCCESS;
}

/**
  Detect whether there is a EMMC card attached at the specified EMMC host controller
  slot.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.1 for details.

  @param[in]  Bar           The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       There is a EMMC card attached.
  @retval EFI_NO_MEDIA      There is not a EMMC card attached.
  @retval Others            The detection fails.

**/
EFI_STATUS
EmmcPeimHcCardDetect (
  IN UINTN  Bar
  )
{
  EFI_STATUS  Status;
  UINT16      Data;
  UINT32      PresentState;

  //
  // Check Normal Interrupt Status Register
  //
  Status = EmmcPeimHcRwMmio (Bar + EMMC_HC_NOR_INT_STS, TRUE, sizeof (Data), &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Data & (BIT6 | BIT7)) != 0) {
    //
    // Clear BIT6 and BIT7 by writing 1 to these two bits if set.
    //
    Data  &= BIT6 | BIT7;
    Status = EmmcPeimHcRwMmio (Bar + EMMC_HC_NOR_INT_STS, FALSE, sizeof (Data), &Data);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Check Present State Register to see if there is a card presented.
  //
  Status = EmmcPeimHcRwMmio (Bar + EMMC_HC_PRESENT_STATE, TRUE, sizeof (PresentState), &PresentState);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((PresentState & BIT16) != 0) {
    return EFI_SUCCESS;
  } else {
    return EFI_NO_MEDIA;
  }
}

/**
  Stop EMMC card clock.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.2.2 for details.

  @param[in]  Bar           The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       Succeed to stop EMMC clock.
  @retval Others            Fail to stop EMMC clock.

**/
EFI_STATUS
EmmcPeimHcStopClock (
  IN UINTN  Bar
  )
{
  EFI_STATUS  Status;
  UINT32      PresentState;
  UINT16      ClockCtrl;

  //
  // Ensure no SD transactions are occurring on the SD Bus by
  // waiting for Command Inhibit (DAT) and Command Inhibit (CMD)
  // in the Present State register to be 0.
  //
  Status = EmmcPeimHcWaitMmioSet (
             Bar + EMMC_HC_PRESENT_STATE,
             sizeof (PresentState),
             BIT0 | BIT1,
             0,
             EMMC_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set SD Clock Enable in the Clock Control register to 0
  //
  ClockCtrl = (UINT16) ~BIT2;
  Status    = EmmcPeimHcAndMmio (Bar + EMMC_HC_CLOCK_CTRL, sizeof (ClockCtrl), &ClockCtrl);

  return Status;
}

/**
  EMMC card clock supply.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.2.1 for details.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] ClockFreq      The max clock frequency to be set. The unit is KHz.

  @retval EFI_SUCCESS       The clock is supplied successfully.
  @retval Others            The clock isn't supplied successfully.

**/
EFI_STATUS
EmmcPeimHcClockSupply (
  IN UINTN   Bar,
  IN UINT64  ClockFreq
  )
{
  EFI_STATUS        Status;
  EMMC_HC_SLOT_CAP  Capability;
  UINT32            BaseClkFreq;
  UINT32            SettingFreq;
  UINT32            Divisor;
  UINT32            Remainder;
  UINT16            ControllerVer;
  UINT16            ClockCtrl;

  //
  // Calculate a divisor for SD clock frequency
  //
  Status = EmmcPeimHcGetCapability (Bar, &Capability);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Capability.BaseClkFreq != 0);

  BaseClkFreq = Capability.BaseClkFreq;

  if (ClockFreq == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (ClockFreq > (BaseClkFreq * 1000)) {
    ClockFreq = BaseClkFreq * 1000;
  }

  //
  // Calculate the divisor of base frequency.
  //
  Divisor     = 0;
  SettingFreq = BaseClkFreq * 1000;
  while (ClockFreq < SettingFreq) {
    Divisor++;

    SettingFreq = (BaseClkFreq * 1000) / (2 * Divisor);
    Remainder   = (BaseClkFreq * 1000) % (2 * Divisor);
    if ((ClockFreq == SettingFreq) && (Remainder == 0)) {
      break;
    }

    if ((ClockFreq == SettingFreq) && (Remainder != 0)) {
      SettingFreq++;
    }
  }

  DEBUG ((DEBUG_INFO, "BaseClkFreq %dMHz Divisor %d ClockFreq %dKhz\n", BaseClkFreq, Divisor, ClockFreq));

  Status = EmmcPeimHcRwMmio (Bar + EMMC_HC_CTRL_VER, TRUE, sizeof (ControllerVer), &ControllerVer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set SDCLK Frequency Select and Internal Clock Enable fields in Clock Control register.
  //
  if ((ControllerVer & 0xFF) == 2) {
    ASSERT (Divisor <= 0x3FF);
    ClockCtrl = ((Divisor & 0xFF) << 8) | ((Divisor & 0x300) >> 2);
  } else if (((ControllerVer & 0xFF) == 0) || ((ControllerVer & 0xFF) == 1)) {
    //
    // Only the most significant bit can be used as divisor.
    //
    if (((Divisor - 1) & Divisor) != 0) {
      Divisor = 1 << (HighBitSet32 (Divisor) + 1);
    }

    ASSERT (Divisor <= 0x80);
    ClockCtrl = (Divisor & 0xFF) << 8;
  } else {
    DEBUG ((DEBUG_ERROR, "Unknown SD Host Controller Spec version [0x%x]!!!\n", ControllerVer));
    return EFI_UNSUPPORTED;
  }

  //
  // Stop bus clock at first
  //
  Status = EmmcPeimHcStopClock (Bar);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Supply clock frequency with specified divisor
  //
  ClockCtrl |= BIT0;
  Status     = EmmcPeimHcRwMmio (Bar + EMMC_HC_CLOCK_CTRL, FALSE, sizeof (ClockCtrl), &ClockCtrl);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Set SDCLK Frequency Select and Internal Clock Enable fields fails\n"));
    return Status;
  }

  //
  // Wait Internal Clock Stable in the Clock Control register to be 1
  //
  Status = EmmcPeimHcWaitMmioSet (
             Bar + EMMC_HC_CLOCK_CTRL,
             sizeof (ClockCtrl),
             BIT1,
             BIT1,
             EMMC_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set SD Clock Enable in the Clock Control register to 1
  //
  ClockCtrl = BIT2;
  Status    = EmmcPeimHcOrMmio (Bar + EMMC_HC_CLOCK_CTRL, sizeof (ClockCtrl), &ClockCtrl);

  return Status;
}

/**
  EMMC bus power control.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.3 for details.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] PowerCtrl      The value setting to the power control register.

  @retval TRUE              There is a EMMC card attached.
  @retval FALSE             There is no a EMMC card attached.

**/
EFI_STATUS
EmmcPeimHcPowerControl (
  IN UINTN  Bar,
  IN UINT8  PowerCtrl
  )
{
  EFI_STATUS  Status;

  //
  // Clr SD Bus Power
  //
  PowerCtrl &= (UINT8) ~BIT0;
  Status     = EmmcPeimHcRwMmio (Bar + EMMC_HC_POWER_CTRL, FALSE, sizeof (PowerCtrl), &PowerCtrl);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set SD Bus Voltage Select and SD Bus Power fields in Power Control Register
  //
  PowerCtrl |= BIT0;
  Status     = EmmcPeimHcRwMmio (Bar + EMMC_HC_POWER_CTRL, FALSE, sizeof (PowerCtrl), &PowerCtrl);

  return Status;
}

/**
  Set the EMMC bus width.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.4 for details.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] BusWidth       The bus width used by the EMMC device, it must be 1, 4 or 8.

  @retval EFI_SUCCESS       The bus width is set successfully.
  @retval Others            The bus width isn't set successfully.

**/
EFI_STATUS
EmmcPeimHcSetBusWidth (
  IN UINTN   Bar,
  IN UINT16  BusWidth
  )
{
  EFI_STATUS  Status;
  UINT8       HostCtrl1;

  if (BusWidth == 1) {
    HostCtrl1 = (UINT8) ~(BIT5 | BIT1);
    Status    = EmmcPeimHcAndMmio (Bar + EMMC_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
  } else if (BusWidth == 4) {
    Status = EmmcPeimHcRwMmio (Bar + EMMC_HC_HOST_CTRL1, TRUE, sizeof (HostCtrl1), &HostCtrl1);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    HostCtrl1 |= BIT1;
    HostCtrl1 &= (UINT8) ~BIT5;
    Status     = EmmcPeimHcRwMmio (Bar + EMMC_HC_HOST_CTRL1, FALSE, sizeof (HostCtrl1), &HostCtrl1);
  } else if (BusWidth == 8) {
    Status = EmmcPeimHcRwMmio (Bar + EMMC_HC_HOST_CTRL1, TRUE, sizeof (HostCtrl1), &HostCtrl1);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    HostCtrl1 &= (UINT8) ~BIT1;
    HostCtrl1 |= BIT5;
    Status     = EmmcPeimHcRwMmio (Bar + EMMC_HC_HOST_CTRL1, FALSE, sizeof (HostCtrl1), &HostCtrl1);
  } else {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  return Status;
}

/**
  Supply EMMC card with lowest clock frequency at initialization.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The clock is supplied successfully.
  @retval Others            The clock isn't supplied successfully.

**/
EFI_STATUS
EmmcPeimHcInitClockFreq (
  IN UINTN  Bar
  )
{
  EFI_STATUS        Status;
  EMMC_HC_SLOT_CAP  Capability;
  UINT32            InitFreq;

  //
  // Calculate a divisor for SD clock frequency
  //
  Status = EmmcPeimHcGetCapability (Bar, &Capability);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Capability.BaseClkFreq == 0) {
    //
    // Don't support get Base Clock Frequency information via another method
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Supply 400KHz clock frequency at initialization phase.
  //
  InitFreq = 400;
  Status   = EmmcPeimHcClockSupply (Bar, InitFreq);
  return Status;
}

/**
  Supply EMMC card with maximum voltage at initialization.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.3 for details.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The voltage is supplied successfully.
  @retval Others            The voltage isn't supplied successfully.

**/
EFI_STATUS
EmmcPeimHcInitPowerVoltage (
  IN UINTN  Bar
  )
{
  EFI_STATUS        Status;
  EMMC_HC_SLOT_CAP  Capability;
  UINT8             MaxVoltage;
  UINT8             HostCtrl2;

  //
  // Get the support voltage of the Host Controller
  //
  Status = EmmcPeimHcGetCapability (Bar, &Capability);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Calculate supported maximum voltage according to SD Bus Voltage Select
  //
  if (Capability.Voltage33 != 0) {
    //
    // Support 3.3V
    //
    MaxVoltage = 0x0E;
  } else if (Capability.Voltage30 != 0) {
    //
    // Support 3.0V
    //
    MaxVoltage = 0x0C;
  } else if (Capability.Voltage18 != 0) {
    //
    // Support 1.8V
    //
    MaxVoltage = 0x0A;
    HostCtrl2  = BIT3;
    Status     = EmmcPeimHcOrMmio (Bar + EMMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    MicroSecondDelay (5000);
  } else {
    ASSERT (FALSE);
    return EFI_DEVICE_ERROR;
  }

  //
  // Set SD Bus Voltage Select and SD Bus Power fields in Power Control Register
  //
  Status = EmmcPeimHcPowerControl (Bar, MaxVoltage);

  return Status;
}

/**
  Initialize the Timeout Control register with most conservative value at initialization.

  Refer to SD Host Controller Simplified spec 3.0 Section 2.2.15 for details.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The timeout control register is configured successfully.
  @retval Others            The timeout control register isn't configured successfully.

**/
EFI_STATUS
EmmcPeimHcInitTimeoutCtrl (
  IN UINTN  Bar
  )
{
  EFI_STATUS  Status;
  UINT8       Timeout;

  Timeout = 0x0E;
  Status  = EmmcPeimHcRwMmio (Bar + EMMC_HC_TIMEOUT_CTRL, FALSE, sizeof (Timeout), &Timeout);

  return Status;
}

/**
  Initial EMMC host controller with lowest clock frequency, max power and max timeout value
  at initialization.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The host controller is initialized successfully.
  @retval Others            The host controller isn't initialized successfully.

**/
EFI_STATUS
EmmcPeimHcInitHost (
  IN UINTN  Bar
  )
{
  EFI_STATUS  Status;

  Status = EmmcPeimHcInitClockFreq (Bar);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcPeimHcInitPowerVoltage (Bar);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcPeimHcInitTimeoutCtrl (Bar);
  return Status;
}

/**
  Turn on/off LED.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] On             The boolean to turn on/off LED.

  @retval EFI_SUCCESS       The LED is turned on/off successfully.
  @retval Others            The LED isn't turned on/off successfully.

**/
EFI_STATUS
EmmcPeimHcLedOnOff (
  IN UINTN    Bar,
  IN BOOLEAN  On
  )
{
  EFI_STATUS  Status;
  UINT8       HostCtrl1;

  if (On) {
    HostCtrl1 = BIT0;
    Status    = EmmcPeimHcOrMmio (Bar + EMMC_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
  } else {
    HostCtrl1 = (UINT8) ~BIT0;
    Status    = EmmcPeimHcAndMmio (Bar + EMMC_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
  }

  return Status;
}

/**
  Build ADMA descriptor table for transfer.

  Refer to SD Host Controller Simplified spec 3.0 Section 1.13 for details.

  @param[in] Trb            The pointer to the EMMC_TRB instance.

  @retval EFI_SUCCESS       The ADMA descriptor table is created successfully.
  @retval Others            The ADMA descriptor table isn't created successfully.

**/
EFI_STATUS
BuildAdmaDescTable (
  IN EMMC_TRB  *Trb
  )
{
  EFI_PHYSICAL_ADDRESS  Data;
  UINT64                DataLen;
  UINT64                Entries;
  UINT64                Index;
  UINT64                Remaining;
  UINT32                Address;

  Data    = Trb->DataPhy;
  DataLen = Trb->DataLen;
  //
  // Only support 32bit ADMA Descriptor Table
  //
  if ((Data >= 0x100000000ul) || ((Data + DataLen) > 0x100000000ul)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Address field shall be set on 32-bit boundary (Lower 2-bit is always set to 0)
  // for 32-bit address descriptor table.
  //
  if ((Data & (BIT0 | BIT1)) != 0) {
    DEBUG ((DEBUG_INFO, "The buffer [0x%x] to construct ADMA desc is not aligned to 4 bytes boundary!\n", Data));
  }

  Entries = DivU64x32 ((DataLen + ADMA_MAX_DATA_PER_LINE - 1), ADMA_MAX_DATA_PER_LINE);

  Trb->AdmaDescSize = (UINTN)MultU64x32 (Entries, sizeof (EMMC_HC_ADMA_DESC_LINE));
  Trb->AdmaDesc     = EmmcPeimAllocateMem (Trb->Slot->Private->Pool, Trb->AdmaDescSize);
  if (Trb->AdmaDesc == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Remaining = DataLen;
  Address   = (UINT32)Data;
  for (Index = 0; Index < Entries; Index++) {
    if (Remaining <= ADMA_MAX_DATA_PER_LINE) {
      Trb->AdmaDesc[Index].Valid   = 1;
      Trb->AdmaDesc[Index].Act     = 2;
      Trb->AdmaDesc[Index].Length  = (UINT16)Remaining;
      Trb->AdmaDesc[Index].Address = Address;
      break;
    } else {
      Trb->AdmaDesc[Index].Valid   = 1;
      Trb->AdmaDesc[Index].Act     = 2;
      Trb->AdmaDesc[Index].Length  = 0;
      Trb->AdmaDesc[Index].Address = Address;
    }

    Remaining -= ADMA_MAX_DATA_PER_LINE;
    Address   += ADMA_MAX_DATA_PER_LINE;
  }

  //
  // Set the last descriptor line as end of descriptor table
  //
  Trb->AdmaDesc[Index].End = 1;
  return EFI_SUCCESS;
}

/**
  Create a new TRB for the EMMC cmd request.

  @param[in] Slot           The slot number of the EMMC card to send the command to.
  @param[in] Packet         A pointer to the SD command data structure.

  @return Created Trb or NULL.

**/
EMMC_TRB *
EmmcPeimCreateTrb (
  IN EMMC_PEIM_HC_SLOT    *Slot,
  IN EMMC_COMMAND_PACKET  *Packet
  )
{
  EMMC_TRB               *Trb;
  EFI_STATUS             Status;
  EMMC_HC_SLOT_CAP       Capability;
  EDKII_IOMMU_OPERATION  MapOp;
  UINTN                  MapLength;

  //
  // Calculate a divisor for SD clock frequency
  //
  Status = EmmcPeimHcGetCapability (Slot->EmmcHcBase, &Capability);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Trb = AllocateZeroPool (sizeof (EMMC_TRB));
  if (Trb == NULL) {
    return NULL;
  }

  Trb->Slot      = Slot;
  Trb->BlockSize = 0x200;
  Trb->Packet    = Packet;
  Trb->Timeout   = Packet->Timeout;

  if ((Packet->InTransferLength != 0) && (Packet->InDataBuffer != NULL)) {
    Trb->Data    = Packet->InDataBuffer;
    Trb->DataLen = Packet->InTransferLength;
    Trb->Read    = TRUE;
  } else if ((Packet->OutTransferLength != 0) && (Packet->OutDataBuffer != NULL)) {
    Trb->Data    = Packet->OutDataBuffer;
    Trb->DataLen = Packet->OutTransferLength;
    Trb->Read    = FALSE;
  } else if ((Packet->InTransferLength == 0) && (Packet->OutTransferLength == 0)) {
    Trb->Data    = NULL;
    Trb->DataLen = 0;
  } else {
    goto Error;
  }

  if ((Trb->DataLen != 0) && (Trb->DataLen < Trb->BlockSize)) {
    Trb->BlockSize = (UINT16)Trb->DataLen;
  }

  if (Packet->EmmcCmdBlk->CommandIndex == EMMC_SEND_TUNING_BLOCK) {
    Trb->Mode = EmmcPioMode;
  } else {
    if (Trb->Read) {
      MapOp = EdkiiIoMmuOperationBusMasterWrite;
    } else {
      MapOp = EdkiiIoMmuOperationBusMasterRead;
    }

    if (Trb->DataLen != 0) {
      MapLength = Trb->DataLen;
      Status    = IoMmuMap (MapOp, Trb->Data, &MapLength, &Trb->DataPhy, &Trb->DataMap);

      if (EFI_ERROR (Status) || (MapLength != Trb->DataLen)) {
        DEBUG ((DEBUG_ERROR, "EmmcPeimCreateTrb: Fail to map data buffer.\n"));
        goto Error;
      }
    }

    if (Trb->DataLen == 0) {
      Trb->Mode = EmmcNoData;
    } else if (Capability.Adma2 != 0) {
      Trb->Mode = EmmcAdmaMode;
      Status    = BuildAdmaDescTable (Trb);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
    } else if (Capability.Sdma != 0) {
      Trb->Mode = EmmcSdmaMode;
    } else {
      Trb->Mode = EmmcPioMode;
    }
  }

  return Trb;

Error:
  EmmcPeimFreeTrb (Trb);
  return NULL;
}

/**
  Free the resource used by the TRB.

  @param[in] Trb        The pointer to the EMMC_TRB instance.

**/
VOID
EmmcPeimFreeTrb (
  IN EMMC_TRB  *Trb
  )
{
  if ((Trb != NULL) && (Trb->DataMap != NULL)) {
    IoMmuUnmap (Trb->DataMap);
  }

  if ((Trb != NULL) && (Trb->AdmaDesc != NULL)) {
    EmmcPeimFreeMem (Trb->Slot->Private->Pool, Trb->AdmaDesc, Trb->AdmaDescSize);
  }

  if (Trb != NULL) {
    FreePool (Trb);
  }

  return;
}

/**
  Check if the env is ready for execute specified TRB.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] Trb            The pointer to the EMMC_TRB instance.

  @retval EFI_SUCCESS       The env is ready for TRB execution.
  @retval EFI_NOT_READY     The env is not ready for TRB execution.
  @retval Others            Some erros happen.

**/
EFI_STATUS
EmmcPeimCheckTrbEnv (
  IN UINTN     Bar,
  IN EMMC_TRB  *Trb
  )
{
  EFI_STATUS           Status;
  EMMC_COMMAND_PACKET  *Packet;
  UINT32               PresentState;

  Packet = Trb->Packet;

  if ((Packet->EmmcCmdBlk->CommandType == EmmcCommandTypeAdtc) ||
      (Packet->EmmcCmdBlk->ResponseType == EmmcResponceTypeR1b) ||
      (Packet->EmmcCmdBlk->ResponseType == EmmcResponceTypeR5b))
  {
    //
    // Wait Command Inhibit (CMD) and Command Inhibit (DAT) in
    // the Present State register to be 0
    //
    PresentState = BIT0 | BIT1;
  } else {
    //
    // Wait Command Inhibit (CMD) in the Present State register
    // to be 0
    //
    PresentState = BIT0;
  }

  Status = EmmcPeimHcCheckMmioSet (
             Bar + EMMC_HC_PRESENT_STATE,
             sizeof (PresentState),
             PresentState,
             0
             );

  return Status;
}

/**
  Wait for the env to be ready for execute specified TRB.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] Trb            The pointer to the EMMC_TRB instance.

  @retval EFI_SUCCESS       The env is ready for TRB execution.
  @retval EFI_TIMEOUT       The env is not ready for TRB execution in time.
  @retval Others            Some erros happen.

**/
EFI_STATUS
EmmcPeimWaitTrbEnv (
  IN UINTN     Bar,
  IN EMMC_TRB  *Trb
  )
{
  EFI_STATUS           Status;
  EMMC_COMMAND_PACKET  *Packet;
  UINT64               Timeout;
  BOOLEAN              InfiniteWait;

  //
  // Wait Command Complete Interrupt Status bit in Normal Interrupt Status Register
  //
  Packet  = Trb->Packet;
  Timeout = Packet->Timeout;
  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  while (InfiniteWait || (Timeout > 0)) {
    //
    // Check Trb execution result by reading Normal Interrupt Status register.
    //
    Status = EmmcPeimCheckTrbEnv (Bar, Trb);
    if (Status != EFI_NOT_READY) {
      return Status;
    }

    //
    // Stall for 1 microsecond.
    //
    MicroSecondDelay (1);

    Timeout--;
  }

  return EFI_TIMEOUT;
}

/**
  Execute the specified TRB.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] Trb            The pointer to the EMMC_TRB instance.

  @retval EFI_SUCCESS       The TRB is sent to host controller successfully.
  @retval Others            Some erros happen when sending this request to the host controller.

**/
EFI_STATUS
EmmcPeimExecTrb (
  IN UINTN     Bar,
  IN EMMC_TRB  *Trb
  )
{
  EFI_STATUS           Status;
  EMMC_COMMAND_PACKET  *Packet;
  UINT16               Cmd;
  UINT16               IntStatus;
  UINT32               Argument;
  UINT16               BlkCount;
  UINT16               BlkSize;
  UINT16               TransMode;
  UINT8                HostCtrl1;
  UINT32               SdmaAddr;
  UINT64               AdmaAddr;

  Packet = Trb->Packet;
  //
  // Clear all bits in Error Interrupt Status Register
  //
  IntStatus = 0xFFFF;
  Status    = EmmcPeimHcRwMmio (Bar + EMMC_HC_ERR_INT_STS, FALSE, sizeof (IntStatus), &IntStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Clear all bits in Normal Interrupt Status Register
  //
  IntStatus = 0xFFFF;
  Status    = EmmcPeimHcRwMmio (Bar + EMMC_HC_NOR_INT_STS, FALSE, sizeof (IntStatus), &IntStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set Host Control 1 register DMA Select field
  //
  if (Trb->Mode == EmmcAdmaMode) {
    HostCtrl1 = BIT4;
    Status    = EmmcPeimHcOrMmio (Bar + EMMC_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  EmmcPeimHcLedOnOff (Bar, TRUE);

  if (Trb->Mode == EmmcSdmaMode) {
    if ((UINT64)(UINTN)Trb->DataPhy >= 0x100000000ul) {
      return EFI_INVALID_PARAMETER;
    }

    SdmaAddr = (UINT32)(UINTN)Trb->DataPhy;
    Status   = EmmcPeimHcRwMmio (Bar + EMMC_HC_SDMA_ADDR, FALSE, sizeof (SdmaAddr), &SdmaAddr);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else if (Trb->Mode == EmmcAdmaMode) {
    AdmaAddr = (UINT64)(UINTN)Trb->AdmaDesc;
    Status   = EmmcPeimHcRwMmio (Bar + EMMC_HC_ADMA_SYS_ADDR, FALSE, sizeof (AdmaAddr), &AdmaAddr);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  BlkSize = Trb->BlockSize;
  if (Trb->Mode == EmmcSdmaMode) {
    //
    // Set SDMA boundary to be 512K bytes.
    //
    BlkSize |= 0x7000;
  }

  Status = EmmcPeimHcRwMmio (Bar + EMMC_HC_BLK_SIZE, FALSE, sizeof (BlkSize), &BlkSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BlkCount = 0;
  if (Trb->Mode != EmmcNoData) {
    //
    // Calculate Block Count.
    //
    BlkCount = (UINT16)(Trb->DataLen / Trb->BlockSize);
  }

  Status = EmmcPeimHcRwMmio (Bar + EMMC_HC_BLK_COUNT, FALSE, sizeof (BlkCount), &BlkCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Argument = Packet->EmmcCmdBlk->CommandArgument;
  Status   = EmmcPeimHcRwMmio (Bar + EMMC_HC_ARG1, FALSE, sizeof (Argument), &Argument);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TransMode = 0;
  if (Trb->Mode != EmmcNoData) {
    if (Trb->Mode != EmmcPioMode) {
      TransMode |= BIT0;
    }

    if (Trb->Read) {
      TransMode |= BIT4;
    }

    if (BlkCount > 1) {
      TransMode |= BIT5 | BIT1;
    }
  }

  Status = EmmcPeimHcRwMmio (Bar + EMMC_HC_TRANS_MOD, FALSE, sizeof (TransMode), &TransMode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Cmd = (UINT16)LShiftU64 (Packet->EmmcCmdBlk->CommandIndex, 8);
  if (Packet->EmmcCmdBlk->CommandType == EmmcCommandTypeAdtc) {
    Cmd |= BIT5;
  }

  //
  // Convert ResponseType to value
  //
  if (Packet->EmmcCmdBlk->CommandType != EmmcCommandTypeBc) {
    switch (Packet->EmmcCmdBlk->ResponseType) {
      case EmmcResponceTypeR1:
      case EmmcResponceTypeR5:
      case EmmcResponceTypeR6:
      case EmmcResponceTypeR7:
        Cmd |= (BIT1 | BIT3 | BIT4);
        break;
      case EmmcResponceTypeR2:
        Cmd |= (BIT0 | BIT3);
        break;
      case EmmcResponceTypeR3:
      case EmmcResponceTypeR4:
        Cmd |= BIT1;
        break;
      case EmmcResponceTypeR1b:
      case EmmcResponceTypeR5b:
        Cmd |= (BIT0 | BIT1 | BIT3 | BIT4);
        break;
      default:
        ASSERT (FALSE);
        break;
    }
  }

  //
  // Execute cmd
  //
  Status = EmmcPeimHcRwMmio (Bar + EMMC_HC_COMMAND, FALSE, sizeof (Cmd), &Cmd);
  return Status;
}

/**
  Check the TRB execution result.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] Trb            The pointer to the EMMC_TRB instance.

  @retval EFI_SUCCESS       The TRB is executed successfully.
  @retval EFI_NOT_READY     The TRB is not completed for execution.
  @retval Others            Some erros happen when executing this request.

**/
EFI_STATUS
EmmcPeimCheckTrbResult (
  IN UINTN     Bar,
  IN EMMC_TRB  *Trb
  )
{
  EFI_STATUS           Status;
  EMMC_COMMAND_PACKET  *Packet;
  UINT16               IntStatus;
  UINT32               Response[4];
  UINT32               SdmaAddr;
  UINT8                Index;
  UINT8                SwReset;
  UINT32               PioLength;

  SwReset = 0;
  Packet  = Trb->Packet;
  //
  // Check Trb execution result by reading Normal Interrupt Status register.
  //
  Status = EmmcPeimHcRwMmio (
             Bar + EMMC_HC_NOR_INT_STS,
             TRUE,
             sizeof (IntStatus),
             &IntStatus
             );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Check Transfer Complete bit is set or not.
  //
  if ((IntStatus & BIT1) == BIT1) {
    if ((IntStatus & BIT15) == BIT15) {
      //
      // Read Error Interrupt Status register to check if the error is
      // Data Timeout Error.
      // If yes, treat it as success as Transfer Complete has higher
      // priority than Data Timeout Error.
      //
      Status = EmmcPeimHcRwMmio (
                 Bar + EMMC_HC_ERR_INT_STS,
                 TRUE,
                 sizeof (IntStatus),
                 &IntStatus
                 );
      if (!EFI_ERROR (Status)) {
        if ((IntStatus & BIT4) == BIT4) {
          Status = EFI_SUCCESS;
        } else {
          Status = EFI_DEVICE_ERROR;
        }
      }
    }

    goto Done;
  }

  //
  // Check if there is a error happened during cmd execution.
  // If yes, then do error recovery procedure to follow SD Host Controller
  // Simplified Spec 3.0 section 3.10.1.
  //
  if ((IntStatus & BIT15) == BIT15) {
    Status = EmmcPeimHcRwMmio (
               Bar + EMMC_HC_ERR_INT_STS,
               TRUE,
               sizeof (IntStatus),
               &IntStatus
               );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    if ((IntStatus & 0x0F) != 0) {
      SwReset |= BIT1;
    }

    if ((IntStatus & 0xF0) != 0) {
      SwReset |= BIT2;
    }

    Status = EmmcPeimHcRwMmio (
               Bar + EMMC_HC_SW_RST,
               FALSE,
               sizeof (SwReset),
               &SwReset
               );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Status = EmmcPeimHcWaitMmioSet (
               Bar + EMMC_HC_SW_RST,
               sizeof (SwReset),
               0xFF,
               0,
               EMMC_TIMEOUT
               );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  //
  // Check if DMA interrupt is signalled for the SDMA transfer.
  //
  if ((Trb->Mode == EmmcSdmaMode) && ((IntStatus & BIT3) == BIT3)) {
    //
    // Clear DMA interrupt bit.
    //
    IntStatus = BIT3;
    Status    = EmmcPeimHcRwMmio (
                  Bar + EMMC_HC_NOR_INT_STS,
                  FALSE,
                  sizeof (IntStatus),
                  &IntStatus
                  );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Update SDMA Address register.
    //
    SdmaAddr = EMMC_SDMA_ROUND_UP ((UINT32)(UINTN)Trb->DataPhy, EMMC_SDMA_BOUNDARY);
    Status   = EmmcPeimHcRwMmio (
                 Bar + EMMC_HC_SDMA_ADDR,
                 FALSE,
                 sizeof (UINT32),
                 &SdmaAddr
                 );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    Trb->DataPhy = (UINT32)(UINTN)SdmaAddr;
  }

  if ((Packet->EmmcCmdBlk->CommandType != EmmcCommandTypeAdtc) &&
      (Packet->EmmcCmdBlk->ResponseType != EmmcResponceTypeR1b) &&
      (Packet->EmmcCmdBlk->ResponseType != EmmcResponceTypeR5b))
  {
    if ((IntStatus & BIT0) == BIT0) {
      Status = EFI_SUCCESS;
      goto Done;
    }
  }

  if (Packet->EmmcCmdBlk->CommandIndex == EMMC_SEND_TUNING_BLOCK) {
    //
    // When performing tuning procedure (Execute Tuning is set to 1) through PIO mode,
    // wait Buffer Read Ready bit of Normal Interrupt Status Register to be 1.
    // Refer to SD Host Controller Simplified Specification 3.0 figure 2-29 for details.
    //
    if ((IntStatus & BIT5) == BIT5) {
      //
      // Clear Buffer Read Ready interrupt at first.
      //
      IntStatus = BIT5;
      EmmcPeimHcRwMmio (Bar + EMMC_HC_NOR_INT_STS, FALSE, sizeof (IntStatus), &IntStatus);
      //
      // Read data out from Buffer Port register
      //
      for (PioLength = 0; PioLength < Trb->DataLen; PioLength += 4) {
        EmmcPeimHcRwMmio (Bar + EMMC_HC_BUF_DAT_PORT, TRUE, 4, (UINT8 *)Trb->Data + PioLength);
      }

      Status = EFI_SUCCESS;
      goto Done;
    }
  }

  Status = EFI_NOT_READY;
Done:
  //
  // Get response data when the cmd is executed successfully.
  //
  if (!EFI_ERROR (Status)) {
    if (Packet->EmmcCmdBlk->CommandType != EmmcCommandTypeBc) {
      for (Index = 0; Index < 4; Index++) {
        Status = EmmcPeimHcRwMmio (
                   Bar + EMMC_HC_RESPONSE + Index * 4,
                   TRUE,
                   sizeof (UINT32),
                   &Response[Index]
                   );
        if (EFI_ERROR (Status)) {
          EmmcPeimHcLedOnOff (Bar, FALSE);
          return Status;
        }
      }

      CopyMem (Packet->EmmcStatusBlk, Response, sizeof (Response));
    }
  }

  if (Status != EFI_NOT_READY) {
    EmmcPeimHcLedOnOff (Bar, FALSE);
  }

  return Status;
}

/**
  Wait for the TRB execution result.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] Trb            The pointer to the EMMC_TRB instance.

  @retval EFI_SUCCESS       The TRB is executed successfully.
  @retval Others            Some erros happen when executing this request.

**/
EFI_STATUS
EmmcPeimWaitTrbResult (
  IN UINTN     Bar,
  IN EMMC_TRB  *Trb
  )
{
  EFI_STATUS           Status;
  EMMC_COMMAND_PACKET  *Packet;
  UINT64               Timeout;
  BOOLEAN              InfiniteWait;

  Packet = Trb->Packet;
  //
  // Wait Command Complete Interrupt Status bit in Normal Interrupt Status Register
  //
  Timeout = Packet->Timeout;
  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  while (InfiniteWait || (Timeout > 0)) {
    //
    // Check Trb execution result by reading Normal Interrupt Status register.
    //
    Status = EmmcPeimCheckTrbResult (Bar, Trb);
    if (Status != EFI_NOT_READY) {
      return Status;
    }

    //
    // Stall for 1 microsecond.
    //
    MicroSecondDelay (1);

    Timeout--;
  }

  return EFI_TIMEOUT;
}

/**
  Sends EMMC command to an EMMC card that is attached to the EMMC controller.

  If Packet is successfully sent to the EMMC card, then EFI_SUCCESS is returned.

  If a device error occurs while sending the Packet, then EFI_DEVICE_ERROR is returned.

  If Slot is not in a valid range for the EMMC controller, then EFI_INVALID_PARAMETER
  is returned.

  If Packet defines a data command but both InDataBuffer and OutDataBuffer are NULL,
  EFI_INVALID_PARAMETER is returned.

  @param[in]     Slot           The slot number of the Emmc card to send the command to.
  @param[in,out] Packet         A pointer to the EMMC command data structure.

  @retval EFI_SUCCESS           The EMMC Command Packet was sent by the host.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send the SD
                                command Packet.
  @retval EFI_INVALID_PARAMETER Packet, Slot, or the contents of the Packet is invalid.
  @retval EFI_INVALID_PARAMETER Packet defines a data command but both InDataBuffer and
                                OutDataBuffer are NULL.
  @retval EFI_NO_MEDIA          SD Device not present in the Slot.
  @retval EFI_UNSUPPORTED       The command described by the EMMC Command Packet is not
                                supported by the host controller.
  @retval EFI_BAD_BUFFER_SIZE   The InTransferLength or OutTransferLength exceeds the
                                limit supported by EMMC card ( i.e. if the number of bytes
                                exceed the Last LBA).

**/
EFI_STATUS
EFIAPI
EmmcPeimExecCmd (
  IN     EMMC_PEIM_HC_SLOT    *Slot,
  IN OUT EMMC_COMMAND_PACKET  *Packet
  )
{
  EFI_STATUS  Status;
  EMMC_TRB    *Trb;

  if (Packet == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->EmmcCmdBlk == NULL) || (Packet->EmmcStatusBlk == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->OutDataBuffer == NULL) && (Packet->OutTransferLength != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->InDataBuffer == NULL) && (Packet->InTransferLength != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Trb = EmmcPeimCreateTrb (Slot, Packet);
  if (Trb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EmmcPeimWaitTrbEnv (Slot->EmmcHcBase, Trb);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = EmmcPeimExecTrb (Slot->EmmcHcBase, Trb);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = EmmcPeimWaitTrbResult (Slot->EmmcHcBase, Trb);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

Done:
  EmmcPeimFreeTrb (Trb);

  return Status;
}

/**
  Send command GO_IDLE_STATE (CMD0 with argument of 0x00000000) to the device to
  make it go to Idle State.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.4 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.

  @retval EFI_SUCCESS       The EMMC device is reset correctly.
  @retval Others            The device reset fails.

**/
EFI_STATUS
EmmcPeimReset (
  IN EMMC_PEIM_HC_SLOT  *Slot
  )
{
  EMMC_COMMAND_BLOCK   EmmcCmdBlk;
  EMMC_STATUS_BLOCK    EmmcStatusBlk;
  EMMC_COMMAND_PACKET  Packet;
  EFI_STATUS           Status;

  ZeroMem (&EmmcCmdBlk, sizeof (EmmcCmdBlk));
  ZeroMem (&EmmcStatusBlk, sizeof (EmmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.EmmcCmdBlk    = &EmmcCmdBlk;
  Packet.EmmcStatusBlk = &EmmcStatusBlk;
  Packet.Timeout       = EMMC_TIMEOUT;

  EmmcCmdBlk.CommandIndex    = EMMC_GO_IDLE_STATE;
  EmmcCmdBlk.CommandType     = EmmcCommandTypeBc;
  EmmcCmdBlk.ResponseType    = 0;
  EmmcCmdBlk.CommandArgument = 0;

  Status = EmmcPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SEND_OP_COND to the EMMC device to get the data of the OCR register.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.4 for details.

  @param[in]      Slot      The slot number of the Emmc card to send the command to.
  @param[in, out] Argument  On input, the argument of SEND_OP_COND is to send to the device.
                            On output, the argument is the value of OCR register.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimGetOcr (
  IN     EMMC_PEIM_HC_SLOT  *Slot,
  IN OUT UINT32             *Argument
  )
{
  EMMC_COMMAND_BLOCK   EmmcCmdBlk;
  EMMC_STATUS_BLOCK    EmmcStatusBlk;
  EMMC_COMMAND_PACKET  Packet;
  EFI_STATUS           Status;

  ZeroMem (&EmmcCmdBlk, sizeof (EmmcCmdBlk));
  ZeroMem (&EmmcStatusBlk, sizeof (EmmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.EmmcCmdBlk    = &EmmcCmdBlk;
  Packet.EmmcStatusBlk = &EmmcStatusBlk;
  Packet.Timeout       = EMMC_TIMEOUT;

  EmmcCmdBlk.CommandIndex    = EMMC_SEND_OP_COND;
  EmmcCmdBlk.CommandType     = EmmcCommandTypeBcr;
  EmmcCmdBlk.ResponseType    = EmmcResponceTypeR3;
  EmmcCmdBlk.CommandArgument = *Argument;

  Status = EmmcPeimExecCmd (Slot, &Packet);
  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    *Argument = EmmcStatusBlk.Resp0;
  }

  return Status;
}

/**
  Broadcast command ALL_SEND_CID to the bus to ask all the EMMC devices to send the
  data of their CID registers.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.4 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimGetAllCid (
  IN EMMC_PEIM_HC_SLOT  *Slot
  )
{
  EMMC_COMMAND_BLOCK   EmmcCmdBlk;
  EMMC_STATUS_BLOCK    EmmcStatusBlk;
  EMMC_COMMAND_PACKET  Packet;
  EFI_STATUS           Status;

  ZeroMem (&EmmcCmdBlk, sizeof (EmmcCmdBlk));
  ZeroMem (&EmmcStatusBlk, sizeof (EmmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.EmmcCmdBlk    = &EmmcCmdBlk;
  Packet.EmmcStatusBlk = &EmmcStatusBlk;
  Packet.Timeout       = EMMC_TIMEOUT;

  EmmcCmdBlk.CommandIndex    = EMMC_ALL_SEND_CID;
  EmmcCmdBlk.CommandType     = EmmcCommandTypeBcr;
  EmmcCmdBlk.ResponseType    = EmmcResponceTypeR2;
  EmmcCmdBlk.CommandArgument = 0;

  Status = EmmcPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SET_RELATIVE_ADDR to the EMMC device to assign a Relative device
  Address (RCA).

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.4 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] Rca            The relative device address to be assigned.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimSetRca (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN UINT32             Rca
  )
{
  EMMC_COMMAND_BLOCK   EmmcCmdBlk;
  EMMC_STATUS_BLOCK    EmmcStatusBlk;
  EMMC_COMMAND_PACKET  Packet;
  EFI_STATUS           Status;

  ZeroMem (&EmmcCmdBlk, sizeof (EmmcCmdBlk));
  ZeroMem (&EmmcStatusBlk, sizeof (EmmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.EmmcCmdBlk    = &EmmcCmdBlk;
  Packet.EmmcStatusBlk = &EmmcStatusBlk;
  Packet.Timeout       = EMMC_TIMEOUT;

  EmmcCmdBlk.CommandIndex    = EMMC_SET_RELATIVE_ADDR;
  EmmcCmdBlk.CommandType     = EmmcCommandTypeAc;
  EmmcCmdBlk.ResponseType    = EmmcResponceTypeR1;
  EmmcCmdBlk.CommandArgument = Rca << 16;

  Status = EmmcPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SEND_CSD to the EMMC device to get the data of the CSD register.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in]  Slot          The slot number of the Emmc card to send the command to.
  @param[in]  Rca           The relative device address of selected device.
  @param[out] Csd           The buffer to store the content of the CSD register.
                            Note the caller should ignore the lowest byte of this
                            buffer as the content of this byte is meaningless even
                            if the operation succeeds.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimGetCsd (
  IN     EMMC_PEIM_HC_SLOT  *Slot,
  IN     UINT32             Rca,
  OUT EMMC_CSD              *Csd
  )
{
  EMMC_COMMAND_BLOCK   EmmcCmdBlk;
  EMMC_STATUS_BLOCK    EmmcStatusBlk;
  EMMC_COMMAND_PACKET  Packet;
  EFI_STATUS           Status;

  ZeroMem (&EmmcCmdBlk, sizeof (EmmcCmdBlk));
  ZeroMem (&EmmcStatusBlk, sizeof (EmmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.EmmcCmdBlk    = &EmmcCmdBlk;
  Packet.EmmcStatusBlk = &EmmcStatusBlk;
  Packet.Timeout       = EMMC_TIMEOUT;

  EmmcCmdBlk.CommandIndex    = EMMC_SEND_CSD;
  EmmcCmdBlk.CommandType     = EmmcCommandTypeAc;
  EmmcCmdBlk.ResponseType    = EmmcResponceTypeR2;
  EmmcCmdBlk.CommandArgument = Rca << 16;

  Status = EmmcPeimExecCmd (Slot, &Packet);
  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    CopyMem (((UINT8 *)Csd) + 1, &EmmcStatusBlk.Resp0, sizeof (EMMC_CSD) - 1);
  }

  return Status;
}

/**
  Send command SELECT_DESELECT_CARD to the EMMC device to select/deselect it.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] Rca            The relative device address of selected device.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimSelect (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN UINT32             Rca
  )
{
  EMMC_COMMAND_BLOCK   EmmcCmdBlk;
  EMMC_STATUS_BLOCK    EmmcStatusBlk;
  EMMC_COMMAND_PACKET  Packet;
  EFI_STATUS           Status;

  ZeroMem (&EmmcCmdBlk, sizeof (EmmcCmdBlk));
  ZeroMem (&EmmcStatusBlk, sizeof (EmmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.EmmcCmdBlk    = &EmmcCmdBlk;
  Packet.EmmcStatusBlk = &EmmcStatusBlk;
  Packet.Timeout       = EMMC_TIMEOUT;

  EmmcCmdBlk.CommandIndex    = EMMC_SELECT_DESELECT_CARD;
  EmmcCmdBlk.CommandType     = EmmcCommandTypeAc;
  EmmcCmdBlk.ResponseType    = EmmcResponceTypeR1;
  EmmcCmdBlk.CommandArgument = Rca << 16;

  Status = EmmcPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SEND_EXT_CSD to the EMMC device to get the data of the EXT_CSD register.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in]  Slot          The slot number of the Emmc card to send the command to.
  @param[out] ExtCsd        The buffer to store the content of the EXT_CSD register.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimGetExtCsd (
  IN     EMMC_PEIM_HC_SLOT  *Slot,
  OUT EMMC_EXT_CSD          *ExtCsd
  )
{
  EMMC_COMMAND_BLOCK   EmmcCmdBlk;
  EMMC_STATUS_BLOCK    EmmcStatusBlk;
  EMMC_COMMAND_PACKET  Packet;
  EFI_STATUS           Status;

  ZeroMem (&EmmcCmdBlk, sizeof (EmmcCmdBlk));
  ZeroMem (&EmmcStatusBlk, sizeof (EmmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.EmmcCmdBlk    = &EmmcCmdBlk;
  Packet.EmmcStatusBlk = &EmmcStatusBlk;
  Packet.Timeout       = EMMC_TIMEOUT;

  EmmcCmdBlk.CommandIndex    = EMMC_SEND_EXT_CSD;
  EmmcCmdBlk.CommandType     = EmmcCommandTypeAdtc;
  EmmcCmdBlk.ResponseType    = EmmcResponceTypeR1;
  EmmcCmdBlk.CommandArgument = 0x00000000;

  Packet.InDataBuffer     = ExtCsd;
  Packet.InTransferLength = sizeof (EMMC_EXT_CSD);

  Status = EmmcPeimExecCmd (Slot, &Packet);
  return Status;
}

/**
  Send command SWITCH to the EMMC device to switch the mode of operation of the
  selected Device or modifies the EXT_CSD registers.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] Access         The access mode of SWITCH command.
  @param[in] Index          The offset of the field to be access.
  @param[in] Value          The value to be set to the specified field of EXT_CSD register.
  @param[in] CmdSet         The value of CmdSet field of EXT_CSD register.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimSwitch (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN UINT8              Access,
  IN UINT8              Index,
  IN UINT8              Value,
  IN UINT8              CmdSet
  )
{
  EMMC_COMMAND_BLOCK   EmmcCmdBlk;
  EMMC_STATUS_BLOCK    EmmcStatusBlk;
  EMMC_COMMAND_PACKET  Packet;
  EFI_STATUS           Status;

  ZeroMem (&EmmcCmdBlk, sizeof (EmmcCmdBlk));
  ZeroMem (&EmmcStatusBlk, sizeof (EmmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.EmmcCmdBlk    = &EmmcCmdBlk;
  Packet.EmmcStatusBlk = &EmmcStatusBlk;
  Packet.Timeout       = EMMC_TIMEOUT;

  EmmcCmdBlk.CommandIndex    = EMMC_SWITCH;
  EmmcCmdBlk.CommandType     = EmmcCommandTypeAc;
  EmmcCmdBlk.ResponseType    = EmmcResponceTypeR1b;
  EmmcCmdBlk.CommandArgument = (Access << 24) | (Index << 16) | (Value << 8) | CmdSet;

  Status = EmmcPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SEND_STATUS to the addressed EMMC device to get its status register.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in]  Slot          The slot number of the Emmc card to send the command to.
  @param[in]  Rca           The relative device address of addressed device.
  @param[out] DevStatus     The returned device status.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimSendStatus (
  IN     EMMC_PEIM_HC_SLOT  *Slot,
  IN     UINT32             Rca,
  OUT UINT32                *DevStatus
  )
{
  EMMC_COMMAND_BLOCK   EmmcCmdBlk;
  EMMC_STATUS_BLOCK    EmmcStatusBlk;
  EMMC_COMMAND_PACKET  Packet;
  EFI_STATUS           Status;

  ZeroMem (&EmmcCmdBlk, sizeof (EmmcCmdBlk));
  ZeroMem (&EmmcStatusBlk, sizeof (EmmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.EmmcCmdBlk    = &EmmcCmdBlk;
  Packet.EmmcStatusBlk = &EmmcStatusBlk;
  Packet.Timeout       = EMMC_TIMEOUT;

  EmmcCmdBlk.CommandIndex    = EMMC_SEND_STATUS;
  EmmcCmdBlk.CommandType     = EmmcCommandTypeAc;
  EmmcCmdBlk.ResponseType    = EmmcResponceTypeR1;
  EmmcCmdBlk.CommandArgument = Rca << 16;

  Status = EmmcPeimExecCmd (Slot, &Packet);
  if (!EFI_ERROR (Status)) {
    *DevStatus = EmmcStatusBlk.Resp0;
  }

  return Status;
}

/**
  Send command SET_BLOCK_COUNT to the addressed EMMC device to set the number of
  blocks for the following block read/write cmd.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] BlockCount     The number of the logical block to access.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimSetBlkCount (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN UINT16             BlockCount
  )
{
  EMMC_COMMAND_BLOCK   EmmcCmdBlk;
  EMMC_STATUS_BLOCK    EmmcStatusBlk;
  EMMC_COMMAND_PACKET  Packet;
  EFI_STATUS           Status;

  ZeroMem (&EmmcCmdBlk, sizeof (EmmcCmdBlk));
  ZeroMem (&EmmcStatusBlk, sizeof (EmmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.EmmcCmdBlk    = &EmmcCmdBlk;
  Packet.EmmcStatusBlk = &EmmcStatusBlk;
  Packet.Timeout       = EMMC_TIMEOUT;

  EmmcCmdBlk.CommandIndex    = EMMC_SET_BLOCK_COUNT;
  EmmcCmdBlk.CommandType     = EmmcCommandTypeAc;
  EmmcCmdBlk.ResponseType    = EmmcResponceTypeR1;
  EmmcCmdBlk.CommandArgument = BlockCount;

  Status = EmmcPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command READ_MULTIPLE_BLOCK/WRITE_MULTIPLE_BLOCK to the addressed EMMC device
  to read/write the specified number of blocks.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.10.4 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] Lba            The logical block address of starting access.
  @param[in] BlockSize      The block size of specified EMMC device partition.
  @param[in] Buffer         The pointer to the transfer buffer.
  @param[in] BufferSize     The size of transfer buffer.
  @param[in] IsRead         Boolean to show the operation direction.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimRwMultiBlocks (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN EFI_LBA            Lba,
  IN UINT32             BlockSize,
  IN VOID               *Buffer,
  IN UINTN              BufferSize,
  IN BOOLEAN            IsRead
  )
{
  EMMC_COMMAND_BLOCK   EmmcCmdBlk;
  EMMC_STATUS_BLOCK    EmmcStatusBlk;
  EMMC_COMMAND_PACKET  Packet;
  EFI_STATUS           Status;

  ZeroMem (&EmmcCmdBlk, sizeof (EmmcCmdBlk));
  ZeroMem (&EmmcStatusBlk, sizeof (EmmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.EmmcCmdBlk    = &EmmcCmdBlk;
  Packet.EmmcStatusBlk = &EmmcStatusBlk;
  //
  // Calculate timeout value through the below formula.
  // Timeout = (transfer size) / (2MB/s).
  // Taking 2MB/s as divisor is because it's nearest to the eMMC lowest
  // transfer speed (2.4MB/s).
  // Refer to eMMC 5.0 spec section 6.9.1 for details.
  //
  Packet.Timeout = (BufferSize / (2 * 1024 * 1024) + 1) * 1000 * 1000;

  if (IsRead) {
    Packet.InDataBuffer     = Buffer;
    Packet.InTransferLength = (UINT32)BufferSize;

    EmmcCmdBlk.CommandIndex = EMMC_READ_MULTIPLE_BLOCK;
    EmmcCmdBlk.CommandType  = EmmcCommandTypeAdtc;
    EmmcCmdBlk.ResponseType = EmmcResponceTypeR1;
  } else {
    Packet.OutDataBuffer     = Buffer;
    Packet.OutTransferLength = (UINT32)BufferSize;

    EmmcCmdBlk.CommandIndex = EMMC_WRITE_MULTIPLE_BLOCK;
    EmmcCmdBlk.CommandType  = EmmcCommandTypeAdtc;
    EmmcCmdBlk.ResponseType = EmmcResponceTypeR1;
  }

  if (Slot->SectorAddressing) {
    EmmcCmdBlk.CommandArgument = (UINT32)Lba;
  } else {
    EmmcCmdBlk.CommandArgument = (UINT32)MultU64x32 (Lba, BlockSize);
  }

  Status = EmmcPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SEND_TUNING_BLOCK to the EMMC device for HS200 optimal sampling point
  detection.

  It may be sent up to 40 times until the host finishes the tuning procedure.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] BusWidth       The bus width to work.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimSendTuningBlk (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN UINT8              BusWidth
  )
{
  EMMC_COMMAND_BLOCK   EmmcCmdBlk;
  EMMC_STATUS_BLOCK    EmmcStatusBlk;
  EMMC_COMMAND_PACKET  Packet;
  EFI_STATUS           Status;
  UINT8                TuningBlock[128];

  ZeroMem (&EmmcCmdBlk, sizeof (EmmcCmdBlk));
  ZeroMem (&EmmcStatusBlk, sizeof (EmmcStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.EmmcCmdBlk    = &EmmcCmdBlk;
  Packet.EmmcStatusBlk = &EmmcStatusBlk;
  Packet.Timeout       = EMMC_TIMEOUT;

  EmmcCmdBlk.CommandIndex    = EMMC_SEND_TUNING_BLOCK;
  EmmcCmdBlk.CommandType     = EmmcCommandTypeAdtc;
  EmmcCmdBlk.ResponseType    = EmmcResponceTypeR1;
  EmmcCmdBlk.CommandArgument = 0;

  Packet.InDataBuffer = TuningBlock;
  if (BusWidth == 8) {
    Packet.InTransferLength = sizeof (TuningBlock);
  } else {
    Packet.InTransferLength = 64;
  }

  Status = EmmcPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Tuning the clock to get HS200 optimal sampling point.

  Command SEND_TUNING_BLOCK may be sent up to 40 times until the host finishes the
  tuning procedure.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 and SD Host Controller
  Simplified Spec 3.0 section Figure 2-29 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] BusWidth       The bus width to work.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimTuningClkForHs200 (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN UINT8              BusWidth
  )
{
  EFI_STATUS  Status;
  UINT8       HostCtrl2;
  UINT8       Retry;

  //
  // Notify the host that the sampling clock tuning procedure starts.
  //
  HostCtrl2 = BIT6;
  Status    = EmmcPeimHcOrMmio (Slot->EmmcHcBase + EMMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Ask the device to send a sequence of tuning blocks till the tuning procedure is done.
  //
  Retry = 0;
  do {
    Status = EmmcPeimSendTuningBlk (Slot, BusWidth);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = EmmcPeimHcRwMmio (Slot->EmmcHcBase + EMMC_HC_HOST_CTRL2, TRUE, sizeof (HostCtrl2), &HostCtrl2);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((HostCtrl2 & (BIT6 | BIT7)) == 0) {
      break;
    }

    if ((HostCtrl2 & (BIT6 | BIT7)) == BIT7) {
      return EFI_SUCCESS;
    }
  } while (++Retry < 40);

  DEBUG ((DEBUG_ERROR, "EmmcPeimTuningClkForHs200: Send tuning block fails at %d times with HostCtrl2 %02x\n", Retry, HostCtrl2));
  //
  // Abort the tuning procedure and reset the tuning circuit.
  //
  HostCtrl2 = (UINT8) ~(BIT6 | BIT7);
  Status    = EmmcPeimHcAndMmio (Slot->EmmcHcBase + EMMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_DEVICE_ERROR;
}

/**
  Switch the bus width to specified width.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.9 and SD Host Controller
  Simplified Spec 3.0 section Figure 3-7 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] IsDdr          If TRUE, use dual data rate data simpling method. Otherwise
                            use single data rate data simpling method.
  @param[in] BusWidth       The bus width to be set, it could be 4 or 8.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimSwitchBusWidth (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN UINT32             Rca,
  IN BOOLEAN            IsDdr,
  IN UINT8              BusWidth
  )
{
  EFI_STATUS  Status;
  UINT8       Access;
  UINT8       Index;
  UINT8       Value;
  UINT8       CmdSet;
  UINT32      DevStatus;

  //
  // Write Byte, the Value field is written into the byte pointed by Index.
  //
  Access = 0x03;
  Index  = OFFSET_OF (EMMC_EXT_CSD, BusWidth);
  if (BusWidth == 4) {
    Value = 1;
  } else if (BusWidth == 8) {
    Value = 2;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  if (IsDdr) {
    Value += 4;
  }

  CmdSet = 0;
  Status = EmmcPeimSwitch (Slot, Access, Index, Value, CmdSet);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcPeimSendStatus (Slot, Rca, &DevStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check the switch operation is really successful or not.
  //
  if ((DevStatus & BIT7) != 0) {
    return EFI_DEVICE_ERROR;
  }

  Status = EmmcPeimHcSetBusWidth (Slot->EmmcHcBase, BusWidth);

  return Status;
}

/**
  Switch the clock frequency to the specified value.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6 and SD Host Controller
  Simplified Spec 3.0 section Figure 3-3 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] HsTiming       The value to be written to HS_TIMING field of EXT_CSD register.
  @param[in] ClockFreq      The max clock frequency to be set, the unit is MHz.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimSwitchClockFreq (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN UINT32             Rca,
  IN UINT8              HsTiming,
  IN UINT32             ClockFreq
  )
{
  EFI_STATUS  Status;
  UINT8       Access;
  UINT8       Index;
  UINT8       Value;
  UINT8       CmdSet;
  UINT32      DevStatus;

  //
  // Write Byte, the Value field is written into the byte pointed by Index.
  //
  Access = 0x03;
  Index  = OFFSET_OF (EMMC_EXT_CSD, HsTiming);
  Value  = HsTiming;
  CmdSet = 0;

  Status = EmmcPeimSwitch (Slot, Access, Index, Value, CmdSet);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcPeimSendStatus (Slot, Rca, &DevStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check the switch operation is really successful or not.
  //
  if ((DevStatus & BIT7) != 0) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Convert the clock freq unit from MHz to KHz.
  //
  Status = EmmcPeimHcClockSupply (Slot->EmmcHcBase, ClockFreq * 1000);

  return Status;
}

/**
  Switch to the High Speed timing according to request.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 and SD Host Controller
  Simplified Spec 3.0 section Figure 2-29 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] ClockFreq      The max clock frequency to be set.
  @param[in] IsDdr          If TRUE, use dual data rate data simpling method. Otherwise
                            use single data rate data simpling method.
  @param[in] BusWidth       The bus width to be set, it could be 4 or 8.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimSwitchToHighSpeed (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN UINT32             Rca,
  IN UINT32             ClockFreq,
  IN BOOLEAN            IsDdr,
  IN UINT8              BusWidth
  )
{
  EFI_STATUS  Status;
  UINT8       HsTiming;
  UINT8       HostCtrl1;
  UINT8       HostCtrl2;

  Status = EmmcPeimSwitchBusWidth (Slot, Rca, IsDdr, BusWidth);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set to High Speed timing
  //
  HostCtrl1 = BIT2;
  Status    = EmmcPeimHcOrMmio (Slot->EmmcHcBase + EMMC_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HostCtrl2 = (UINT8) ~0x7;
  Status    = EmmcPeimHcAndMmio (Slot->EmmcHcBase + EMMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (IsDdr) {
    HostCtrl2 = BIT2;
  } else if (ClockFreq == 52) {
    HostCtrl2 = BIT0;
  } else {
    HostCtrl2 = 0;
  }

  Status = EmmcPeimHcOrMmio (Slot->EmmcHcBase + EMMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HsTiming = 1;
  Status   = EmmcPeimSwitchClockFreq (Slot, Rca, HsTiming, ClockFreq);

  return Status;
}

/**
  Switch to the HS200 timing according to request.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 and SD Host Controller
  Simplified Spec 3.0 section Figure 2-29 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] ClockFreq      The max clock frequency to be set.
  @param[in] BusWidth       The bus width to be set, it could be 4 or 8.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimSwitchToHS200 (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN UINT32             Rca,
  IN UINT32             ClockFreq,
  IN UINT8              BusWidth
  )
{
  EFI_STATUS  Status;
  UINT8       HsTiming;
  UINT8       HostCtrl2;
  UINT16      ClockCtrl;

  if ((BusWidth != 4) && (BusWidth != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EmmcPeimSwitchBusWidth (Slot, Rca, FALSE, BusWidth);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set to HS200/SDR104 timing
  //
  //
  // Stop bus clock at first
  //
  Status = EmmcPeimHcStopClock (Slot->EmmcHcBase);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HostCtrl2 = (UINT8) ~0x7;
  Status    = EmmcPeimHcAndMmio (Slot->EmmcHcBase + EMMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HostCtrl2 = BIT0 | BIT1;
  Status    = EmmcPeimHcOrMmio (Slot->EmmcHcBase + EMMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Wait Internal Clock Stable in the Clock Control register to be 1 before set SD Clock Enable bit
  //
  Status = EmmcPeimHcWaitMmioSet (
             Slot->EmmcHcBase + EMMC_HC_CLOCK_CTRL,
             sizeof (ClockCtrl),
             BIT1,
             BIT1,
             EMMC_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set SD Clock Enable in the Clock Control register to 1
  //
  ClockCtrl = BIT2;
  Status    = EmmcPeimHcOrMmio (Slot->EmmcHcBase + EMMC_HC_CLOCK_CTRL, sizeof (ClockCtrl), &ClockCtrl);

  HsTiming = 2;
  Status   = EmmcPeimSwitchClockFreq (Slot, Rca, HsTiming, ClockFreq);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EmmcPeimTuningClkForHs200 (Slot, BusWidth);

  return Status;
}

/**
  Switch to the HS400 timing according to request.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 and SD Host Controller
  Simplified Spec 3.0 section Figure 2-29 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] ClockFreq      The max clock frequency to be set.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimSwitchToHS400 (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN UINT32             Rca,
  IN UINT32             ClockFreq
  )
{
  EFI_STATUS  Status;
  UINT8       HsTiming;
  UINT8       HostCtrl2;

  Status = EmmcPeimSwitchToHS200 (Slot, Rca, ClockFreq, 8);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set to High Speed timing and set the clock frequency to a value less than 52MHz.
  //
  HsTiming = 1;
  Status   = EmmcPeimSwitchClockFreq (Slot, Rca, HsTiming, 52);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // HS400 mode must use 8 data lines.
  //
  Status = EmmcPeimSwitchBusWidth (Slot, Rca, TRUE, 8);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set to HS400 timing
  //
  HostCtrl2 = (UINT8) ~0x7;
  Status    = EmmcPeimHcAndMmio (Slot->EmmcHcBase + EMMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HostCtrl2 = BIT0 | BIT2;
  Status    = EmmcPeimHcOrMmio (Slot->EmmcHcBase + EMMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HsTiming = 3;
  Status   = EmmcPeimSwitchClockFreq (Slot, Rca, HsTiming, ClockFreq);

  return Status;
}

/**
  Switch the high speed timing according to request.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.6.8 and SD Host Controller
  Simplified Spec 3.0 section Figure 2-29 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.
  @param[in] Rca            The relative device address to be assigned.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
EmmcPeimSetBusMode (
  IN EMMC_PEIM_HC_SLOT  *Slot,
  IN UINT32             Rca
  )
{
  EFI_STATUS        Status;
  EMMC_HC_SLOT_CAP  Capability;
  UINT8             HsTiming;
  BOOLEAN           IsDdr;
  UINT32            ClockFreq;
  UINT8             BusWidth;

  Status = EmmcPeimGetCsd (Slot, Rca, &Slot->Csd);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcPeimSetBusMode: EmmcPeimGetCsd fails with %r\n", Status));
    return Status;
  }

  if ((Slot->Csd.CSizeLow | Slot->Csd.CSizeHigh << 2) == 0xFFF) {
    Slot->SectorAddressing = TRUE;
  } else {
    Slot->SectorAddressing = FALSE;
  }

  Status = EmmcPeimSelect (Slot, Rca);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcPeimSetBusMode: EmmcPeimSelect fails with %r\n", Status));
    return Status;
  }

  Status = EmmcPeimHcGetCapability (Slot->EmmcHcBase, &Capability);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcPeimSetBusMode: EmmcPeimHcGetCapability fails with %r\n", Status));
    return Status;
  }

  ASSERT (Capability.BaseClkFreq != 0);
  //
  // Check if the Host Controller support 8bits bus width.
  //
  if (Capability.BusWidth8 != 0) {
    BusWidth = 8;
  } else {
    BusWidth = 4;
  }

  //
  // Get Device_Type from EXT_CSD register.
  //
  Status = EmmcPeimGetExtCsd (Slot, &Slot->ExtCsd);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcPeimSetBusMode: EmmcPeimGetExtCsd fails with %r\n", Status));
    return Status;
  }

  //
  // Calculate supported bus speed/bus width/clock frequency.
  //
  HsTiming  = 0;
  IsDdr     = FALSE;
  ClockFreq = 0;
  if (((Slot->ExtCsd.DeviceType & (BIT4 | BIT5))  != 0) && (Capability.Sdr104 != 0)) {
    HsTiming  = 2;
    IsDdr     = FALSE;
    ClockFreq = 200;
  } else if (((Slot->ExtCsd.DeviceType & (BIT2 | BIT3))  != 0) && (Capability.Ddr50 != 0)) {
    HsTiming  = 1;
    IsDdr     = TRUE;
    ClockFreq = 52;
  } else if (((Slot->ExtCsd.DeviceType & BIT1)  != 0) && (Capability.HighSpeed != 0)) {
    HsTiming  = 1;
    IsDdr     = FALSE;
    ClockFreq = 52;
  } else if (((Slot->ExtCsd.DeviceType & BIT0)  != 0) && (Capability.HighSpeed != 0)) {
    HsTiming  = 1;
    IsDdr     = FALSE;
    ClockFreq = 26;
  }

  //
  // Check if both of the device and the host controller support HS400 DDR mode.
  //
  if (((Slot->ExtCsd.DeviceType & (BIT6 | BIT7))  != 0) && (Capability.Hs400 != 0)) {
    //
    // The host controller supports 8bits bus.
    //
    ASSERT (BusWidth == 8);
    HsTiming  = 3;
    IsDdr     = TRUE;
    ClockFreq = 200;
  }

  if ((ClockFreq == 0) || (HsTiming == 0)) {
    //
    // Continue using default setting.
    //
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "HsTiming %d ClockFreq %d BusWidth %d Ddr %a\n", HsTiming, ClockFreq, BusWidth, IsDdr ? "TRUE" : "FALSE"));

  if (HsTiming == 3) {
    //
    // Execute HS400 timing switch procedure
    //
    Status = EmmcPeimSwitchToHS400 (Slot, Rca, ClockFreq);
  } else if (HsTiming == 2) {
    //
    // Execute HS200 timing switch procedure
    //
    Status = EmmcPeimSwitchToHS200 (Slot, Rca, ClockFreq, BusWidth);
  } else {
    //
    // Execute High Speed timing switch procedure
    //
    Status = EmmcPeimSwitchToHighSpeed (Slot, Rca, ClockFreq, IsDdr, BusWidth);
  }

  return Status;
}

/**
  Execute EMMC device identification procedure.

  Refer to EMMC Electrical Standard Spec 5.1 Section 6.4 for details.

  @param[in] Slot           The slot number of the Emmc card to send the command to.

  @retval EFI_SUCCESS       There is a EMMC card.
  @retval Others            There is not a EMMC card.

**/
EFI_STATUS
EmmcPeimIdentification (
  IN EMMC_PEIM_HC_SLOT  *Slot
  )
{
  EFI_STATUS  Status;
  UINT32      Ocr;
  UINT32      Rca;
  UINTN       Retry;

  Status = EmmcPeimReset (Slot);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcPeimIdentification: EmmcPeimReset fails with %r\n", Status));
    return Status;
  }

  Ocr   = 0;
  Retry = 0;
  do {
    Status = EmmcPeimGetOcr (Slot, &Ocr);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "EmmcPeimIdentification: EmmcPeimGetOcr fails with %r\n", Status));
      return Status;
    }

    if (Retry++ == 100) {
      DEBUG ((DEBUG_ERROR, "EmmcPeimIdentification: EmmcPeimGetOcr fails too many times\n"));
      return EFI_DEVICE_ERROR;
    }

    MicroSecondDelay (10 * 1000);
  } while ((Ocr & BIT31) == 0);

  Status = EmmcPeimGetAllCid (Slot);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcPeimIdentification: EmmcPeimGetAllCid fails with %r\n", Status));
    return Status;
  }

  //
  // Don't support multiple devices on the slot, that is
  // shared bus slot feature.
  //
  Rca    = 1;
  Status = EmmcPeimSetRca (Slot, Rca);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EmmcPeimIdentification: EmmcPeimSetRca fails with %r\n", Status));
    return Status;
  }

  //
  // Enter Data Tranfer Mode.
  //
  DEBUG ((DEBUG_INFO, "Found a EMMC device at slot [%d], RCA [%d]\n", Slot, Rca));

  Status = EmmcPeimSetBusMode (Slot, Rca);

  return Status;
}
