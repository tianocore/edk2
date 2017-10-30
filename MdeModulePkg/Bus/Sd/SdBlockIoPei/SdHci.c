/** @file

  Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SdBlockIoPei.h"

/**
  Read/Write specified SD host controller mmio register.

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
SdPeimHcRwMmio (
  IN     UINTN                 Address,
  IN     BOOLEAN               Read,
  IN     UINT8                 Count,
  IN OUT VOID                  *Data
  )
{
  if ((Address == 0) || (Data == NULL))  {
    return EFI_INVALID_PARAMETER;
  }

  if ((Count != 1) && (Count != 2) && (Count != 4) && (Count != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Count) {
    case 1:
      if (Read) {
        *(UINT8*)Data = MmioRead8 (Address);
      } else {
        MmioWrite8 (Address, *(UINT8*)Data);
      }
      break;
    case 2:
      if (Read) {
        *(UINT16*)Data = MmioRead16 (Address);
      } else {
        MmioWrite16 (Address, *(UINT16*)Data);
      }
      break;
    case 4:
      if (Read) {
        *(UINT32*)Data = MmioRead32 (Address);
      } else {
        MmioWrite32 (Address, *(UINT32*)Data);
      }
      break;
    case 8:
      if (Read) {
        *(UINT64*)Data = MmioRead64 (Address);
      } else {
        MmioWrite64 (Address, *(UINT64*)Data);
      }
      break;
    default:
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Do OR operation with the value of the specified SD host controller mmio register.

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
SdPeimHcOrMmio (
  IN  UINTN                    Address,
  IN  UINT8                    Count,
  IN  VOID                     *OrData
  )
{
  EFI_STATUS                   Status;
  UINT64                       Data;
  UINT64                       Or;

  Status = SdPeimHcRwMmio (Address, TRUE, Count, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Count == 1) {
    Or = *(UINT8*) OrData;
  } else if (Count == 2) {
    Or = *(UINT16*) OrData;
  } else if (Count == 4) {
    Or = *(UINT32*) OrData;
  } else if (Count == 8) {
    Or = *(UINT64*) OrData;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  Data  |= Or;
  Status = SdPeimHcRwMmio (Address, FALSE, Count, &Data);

  return Status;
}

/**
  Do AND operation with the value of the specified SD host controller mmio register.

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
SdPeimHcAndMmio (
  IN  UINTN                    Address,
  IN  UINT8                    Count,
  IN  VOID                     *AndData
  )
{
  EFI_STATUS                   Status;
  UINT64                       Data;
  UINT64                       And;

  Status = SdPeimHcRwMmio (Address, TRUE, Count, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Count == 1) {
    And = *(UINT8*) AndData;
  } else if (Count == 2) {
    And = *(UINT16*) AndData;
  } else if (Count == 4) {
    And = *(UINT32*) AndData;
  } else if (Count == 8) {
    And = *(UINT64*) AndData;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  Data  &= And;
  Status = SdPeimHcRwMmio (Address, FALSE, Count, &Data);

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
SdPeimHcCheckMmioSet (
  IN  UINTN                     Address,
  IN  UINT8                     Count,
  IN  UINT64                    MaskValue,
  IN  UINT64                    TestValue
  )
{
  EFI_STATUS            Status;
  UINT64                Value;

  //
  // Access PCI MMIO space to see if the value is the tested one.
  //
  Value  = 0;
  Status = SdPeimHcRwMmio (Address, TRUE, Count, &Value);
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
SdPeimHcWaitMmioSet (
  IN  UINTN                     Address,
  IN  UINT8                     Count,
  IN  UINT64                    MaskValue,
  IN  UINT64                    TestValue,
  IN  UINT64                    Timeout
  )
{
  EFI_STATUS            Status;
  BOOLEAN               InfiniteWait;

  if (Timeout == 0) {
    InfiniteWait = TRUE;
  } else {
    InfiniteWait = FALSE;
  }

  while (InfiniteWait || (Timeout > 0)) {
    Status = SdPeimHcCheckMmioSet (
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
  Software reset the specified SD host controller and enable all interrupts.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The software reset executes successfully.
  @retval Others            The software reset fails.

**/
EFI_STATUS
SdPeimHcReset (
  IN UINTN                  Bar
  )
{
  EFI_STATUS                Status;
  UINT8                     SwReset;

  SwReset = 0xFF;
  Status  = SdPeimHcRwMmio (Bar + SD_HC_SW_RST, FALSE, sizeof (SwReset), &SwReset);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SdPeimHcReset: write full 1 fails: %r\n", Status));
    return Status;
  }

  Status = SdPeimHcWaitMmioSet (
             Bar + SD_HC_SW_RST,
             sizeof (SwReset),
             0xFF,
             0x00,
             SD_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "SdPeimHcReset: reset done with %r\n", Status));
    return Status;
  }
  //
  // Enable all interrupt after reset all.
  //
  Status = SdPeimHcEnableInterrupt (Bar);

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
SdPeimHcEnableInterrupt (
  IN UINTN                  Bar
  )
{
  EFI_STATUS                Status;
  UINT16                    IntStatus;

  //
  // Enable all bits in Error Interrupt Status Enable Register
  //
  IntStatus = 0xFFFF;
  Status = SdPeimHcRwMmio (Bar + SD_HC_ERR_INT_STS_EN, FALSE, sizeof (IntStatus), &IntStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Enable all bits in Normal Interrupt Status Enable Register
  //
  IntStatus = 0xFFFF;
  Status = SdPeimHcRwMmio (Bar + SD_HC_NOR_INT_STS_EN, FALSE, sizeof (IntStatus), &IntStatus);

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
SdPeimHcGetCapability (
  IN     UINTN              Bar,
     OUT SD_HC_SLOT_CAP     *Capability
  )
{
  EFI_STATUS                Status;
  UINT64                    Cap;

  Status = SdPeimHcRwMmio (Bar + SD_HC_CAP, TRUE, sizeof (Cap), &Cap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (Capability, &Cap, sizeof (Cap));

  return EFI_SUCCESS;
}

/**
  Detect whether there is a SD card attached at the specified SD host controller
  slot.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.1 for details.

  @param[in]  Bar           The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       There is a SD card attached.
  @retval EFI_NO_MEDIA      There is not a SD card attached.
  @retval Others            The detection fails.

**/
EFI_STATUS
SdPeimHcCardDetect (
  IN UINTN                  Bar
  )
{
  EFI_STATUS                Status;
  UINT16                    Data;
  UINT32                    PresentState;

  //
  // Check Normal Interrupt Status Register
  //
  Status = SdPeimHcRwMmio (Bar + SD_HC_NOR_INT_STS, TRUE, sizeof (Data), &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Data & (BIT6 | BIT7)) != 0) {
    //
    // Clear BIT6 and BIT7 by writing 1 to these two bits if set.
    //
    Data  &= BIT6 | BIT7;
    Status = SdPeimHcRwMmio (Bar + SD_HC_NOR_INT_STS, FALSE, sizeof (Data), &Data);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Check Present State Register to see if there is a card presented.
  //
  Status = SdPeimHcRwMmio (Bar + SD_HC_PRESENT_STATE, TRUE, sizeof (PresentState), &PresentState);
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
  Stop SD card clock.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.2.2 for details.

  @param[in]  Bar           The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       Succeed to stop SD clock.
  @retval Others            Fail to stop SD clock.

**/
EFI_STATUS
SdPeimHcStopClock (
  IN UINTN                  Bar
  )
{
  EFI_STATUS                Status;
  UINT32                    PresentState;
  UINT16                    ClockCtrl;

  //
  // Ensure no SD transactions are occurring on the SD Bus by
  // waiting for Command Inhibit (DAT) and Command Inhibit (CMD)
  // in the Present State register to be 0.
  //
  Status = SdPeimHcWaitMmioSet (
             Bar + SD_HC_PRESENT_STATE,
             sizeof (PresentState),
             BIT0 | BIT1,
             0,
             SD_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set SD Clock Enable in the Clock Control register to 0
  //
  ClockCtrl = (UINT16)~BIT2;
  Status = SdPeimHcAndMmio (Bar + SD_HC_CLOCK_CTRL, sizeof (ClockCtrl), &ClockCtrl);

  return Status;
}

/**
  SD card clock supply.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.2.1 for details.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] ClockFreq      The max clock frequency to be set. The unit is KHz.

  @retval EFI_SUCCESS       The clock is supplied successfully.
  @retval Others            The clock isn't supplied successfully.

**/
EFI_STATUS
SdPeimHcClockSupply (
  IN UINTN                  Bar,
  IN UINT64                 ClockFreq
  )
{
  EFI_STATUS                Status;
  SD_HC_SLOT_CAP            Capability;
  UINT32                    BaseClkFreq;
  UINT32                    SettingFreq;
  UINT32                    Divisor;
  UINT32                    Remainder;
  UINT16                    ControllerVer;
  UINT16                    ClockCtrl;

  //
  // Calculate a divisor for SD clock frequency
  //
  Status = SdPeimHcGetCapability (Bar, &Capability);
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
      SettingFreq ++;
    }
  }

  DEBUG ((EFI_D_INFO, "BaseClkFreq %dMHz Divisor %d ClockFreq %dKhz\n", BaseClkFreq, Divisor, ClockFreq));

  Status = SdPeimHcRwMmio (Bar + SD_HC_CTRL_VER, TRUE, sizeof (ControllerVer), &ControllerVer);
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
    DEBUG ((EFI_D_ERROR, "Unknown SD Host Controller Spec version [0x%x]!!!\n", ControllerVer));
    return EFI_UNSUPPORTED;
  }

  //
  // Stop bus clock at first
  //
  Status = SdPeimHcStopClock (Bar);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Supply clock frequency with specified divisor
  //
  ClockCtrl |= BIT0;
  Status = SdPeimHcRwMmio (Bar + SD_HC_CLOCK_CTRL, FALSE, sizeof (ClockCtrl), &ClockCtrl);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Set SDCLK Frequency Select and Internal Clock Enable fields fails\n"));
    return Status;
  }

  //
  // Wait Internal Clock Stable in the Clock Control register to be 1
  //
  Status = SdPeimHcWaitMmioSet (
             Bar + SD_HC_CLOCK_CTRL,
             sizeof (ClockCtrl),
             BIT1,
             BIT1,
             SD_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set SD Clock Enable in the Clock Control register to 1
  //
  ClockCtrl = BIT2;
  Status = SdPeimHcOrMmio (Bar + SD_HC_CLOCK_CTRL, sizeof (ClockCtrl), &ClockCtrl);

  return Status;
}

/**
  SD bus power control.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.3 for details.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] PowerCtrl      The value setting to the power control register.

  @retval TRUE              There is a SD card attached.
  @retval FALSE             There is no a SD card attached.

**/
EFI_STATUS
SdPeimHcPowerControl (
  IN UINTN                  Bar,
  IN UINT8                  PowerCtrl
  )
{
  EFI_STATUS                Status;

  //
  // Clr SD Bus Power
  //
  PowerCtrl &= (UINT8)~BIT0;
  Status = SdPeimHcRwMmio (Bar + SD_HC_POWER_CTRL, FALSE, sizeof (PowerCtrl), &PowerCtrl);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set SD Bus Voltage Select and SD Bus Power fields in Power Control Register
  //
  PowerCtrl |= BIT0;
  Status = SdPeimHcRwMmio (Bar + SD_HC_POWER_CTRL, FALSE, sizeof (PowerCtrl), &PowerCtrl);

  return Status;
}

/**
  Set the SD bus width.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.4 for details.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] BusWidth       The bus width used by the SD device, it must be 1, 4 or 8.

  @retval EFI_SUCCESS       The bus width is set successfully.
  @retval Others            The bus width isn't set successfully.

**/
EFI_STATUS
SdPeimHcSetBusWidth (
  IN UINTN                  Bar,
  IN UINT16                 BusWidth
  )
{
  EFI_STATUS                Status;
  UINT8                     HostCtrl1;

  if (BusWidth == 1) {
    HostCtrl1 = (UINT8)~(BIT5 | BIT1);
    Status = SdPeimHcAndMmio (Bar + SD_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
  } else if (BusWidth == 4) {
    Status = SdPeimHcRwMmio (Bar + SD_HC_HOST_CTRL1, TRUE, sizeof (HostCtrl1), &HostCtrl1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    HostCtrl1 |= BIT1;
    HostCtrl1 &= (UINT8)~BIT5;
    Status = SdPeimHcRwMmio (Bar + SD_HC_HOST_CTRL1, FALSE, sizeof (HostCtrl1), &HostCtrl1);
  } else if (BusWidth == 8) {
    Status = SdPeimHcRwMmio (Bar + SD_HC_HOST_CTRL1, TRUE, sizeof (HostCtrl1), &HostCtrl1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    HostCtrl1 &= (UINT8)~BIT1;
    HostCtrl1 |= BIT5;
    Status = SdPeimHcRwMmio (Bar + SD_HC_HOST_CTRL1, FALSE, sizeof (HostCtrl1), &HostCtrl1);
  } else {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  return Status;
}

/**
  Supply SD card with lowest clock frequency at initialization.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The clock is supplied successfully.
  @retval Others            The clock isn't supplied successfully.

**/
EFI_STATUS
SdPeimHcInitClockFreq (
  IN UINTN                  Bar
  )
{
  EFI_STATUS                Status;
  SD_HC_SLOT_CAP            Capability;
  UINT32                    InitFreq;

  //
  // Calculate a divisor for SD clock frequency
  //
  Status = SdPeimHcGetCapability (Bar, &Capability);
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
  Status = SdPeimHcClockSupply (Bar, InitFreq);
  return Status;
}

/**
  Supply SD card with maximum voltage at initialization.

  Refer to SD Host Controller Simplified spec 3.0 Section 3.3 for details.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The voltage is supplied successfully.
  @retval Others            The voltage isn't supplied successfully.

**/
EFI_STATUS
SdPeimHcInitPowerVoltage (
  IN UINTN                  Bar
  )
{
  EFI_STATUS                Status;
  SD_HC_SLOT_CAP            Capability;
  UINT8                     MaxVoltage;
  UINT8                     HostCtrl2;

  //
  // Get the support voltage of the Host Controller
  //
  Status = SdPeimHcGetCapability (Bar, &Capability);
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
    Status = SdPeimHcOrMmio (Bar + SD_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
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
  Status = SdPeimHcPowerControl (Bar, MaxVoltage);

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
SdPeimHcInitTimeoutCtrl (
  IN UINTN                  Bar
  )
{
  EFI_STATUS                Status;
  UINT8                     Timeout;

  Timeout = 0x0E;
  Status  = SdPeimHcRwMmio (Bar + SD_HC_TIMEOUT_CTRL, FALSE, sizeof (Timeout), &Timeout);

  return Status;
}

/**
  Initial SD host controller with lowest clock frequency, max power and max timeout value
  at initialization.

  @param[in] Bar            The mmio base address of the slot to be accessed.

  @retval EFI_SUCCESS       The host controller is initialized successfully.
  @retval Others            The host controller isn't initialized successfully.

**/
EFI_STATUS
SdPeimHcInitHost (
  IN UINTN                  Bar
  )
{
  EFI_STATUS       Status;

  Status = SdPeimHcInitClockFreq (Bar);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SdPeimHcInitPowerVoltage (Bar);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SdPeimHcInitTimeoutCtrl (Bar);
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
SdPeimHcLedOnOff (
  IN UINTN                  Bar,
  IN BOOLEAN                On
  )
{
  EFI_STATUS                Status;
  UINT8                     HostCtrl1;

  if (On) {
    HostCtrl1 = BIT0;
    Status    = SdPeimHcOrMmio (Bar + SD_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
  } else {
    HostCtrl1 = (UINT8)~BIT0;
    Status    = SdPeimHcAndMmio (Bar + SD_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
  }

  return Status;
}

/**
  Build ADMA descriptor table for transfer.

  Refer to SD Host Controller Simplified spec 3.0 Section 1.13 for details.

  @param[in] Trb            The pointer to the SD_TRB instance.

  @retval EFI_SUCCESS       The ADMA descriptor table is created successfully.
  @retval Others            The ADMA descriptor table isn't created successfully.

**/
EFI_STATUS
BuildAdmaDescTable (
  IN SD_TRB                 *Trb
  )
{
  EFI_PHYSICAL_ADDRESS      Data;
  UINT64                    DataLen;
  UINT64                    Entries;
  UINT32                    Index;
  UINT64                    Remaining;
  UINT32                    Address;

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
    DEBUG ((EFI_D_INFO, "The buffer [0x%x] to construct ADMA desc is not aligned to 4 bytes boundary!\n", Data));
  }

  Entries = DivU64x32 ((DataLen + ADMA_MAX_DATA_PER_LINE - 1), ADMA_MAX_DATA_PER_LINE);

  Trb->AdmaDescSize = (UINTN)MultU64x32 (Entries, sizeof (SD_HC_ADMA_DESC_LINE));
  Trb->AdmaDesc     = SdPeimAllocateMem (Trb->Slot->Private->Pool, Trb->AdmaDescSize);
  if (Trb->AdmaDesc == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Remaining = DataLen;
  Address   = (UINT32)Data;
  for (Index = 0; Index < Entries; Index++) {
    if (Remaining <= ADMA_MAX_DATA_PER_LINE) {
      Trb->AdmaDesc[Index].Valid = 1;
      Trb->AdmaDesc[Index].Act   = 2;
      Trb->AdmaDesc[Index].Length  = (UINT16)Remaining;
      Trb->AdmaDesc[Index].Address = Address;
      break;
    } else {
      Trb->AdmaDesc[Index].Valid = 1;
      Trb->AdmaDesc[Index].Act   = 2;
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
  Create a new TRB for the SD cmd request.

  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Packet         A pointer to the SD command data structure.

  @return Created Trb or NULL.

**/
SD_TRB *
SdPeimCreateTrb (
  IN SD_PEIM_HC_SLOT          *Slot,
  IN SD_COMMAND_PACKET        *Packet
  )
{
  SD_TRB                      *Trb;
  EFI_STATUS                  Status;
  SD_HC_SLOT_CAP              Capability;
  EDKII_IOMMU_OPERATION       MapOp;
  UINTN                       MapLength;

  //
  // Calculate a divisor for SD clock frequency
  //
  Status = SdPeimHcGetCapability (Slot->SdHcBase, &Capability);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Trb = AllocateZeroPool (sizeof (SD_TRB));
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

  if (Packet->SdCmdBlk->CommandIndex == SD_SEND_TUNING_BLOCK) {
    Trb->Mode = SdPioMode;
  } else {
    if (Trb->Read) {
      MapOp = EdkiiIoMmuOperationBusMasterWrite;
    } else {
      MapOp = EdkiiIoMmuOperationBusMasterRead;
    }

    if (Trb->DataLen != 0) {
      MapLength = Trb->DataLen;
      Status = IoMmuMap (MapOp, Trb->Data, &MapLength, &Trb->DataPhy, &Trb->DataMap);

      if (EFI_ERROR (Status) || (MapLength != Trb->DataLen)) {
        DEBUG ((DEBUG_ERROR, "SdPeimCreateTrb: Fail to map data buffer.\n"));
        goto Error;
      }
    }

    if (Trb->DataLen == 0) {
      Trb->Mode = SdNoData;
    } else if (Capability.Adma2 != 0) {
      Trb->Mode = SdAdmaMode;
      Status = BuildAdmaDescTable (Trb);
      if (EFI_ERROR (Status)) {
        goto Error;
      }
    } else if (Capability.Sdma != 0) {
      Trb->Mode = SdSdmaMode;
    } else {
      Trb->Mode = SdPioMode;
    }
  }
  return Trb;

Error:
  SdPeimFreeTrb (Trb);
  return NULL;
}

/**
  Free the resource used by the TRB.

  @param[in] Trb        The pointer to the SD_TRB instance.

**/
VOID
SdPeimFreeTrb (
  IN SD_TRB           *Trb
  )
{
  if ((Trb != NULL) && (Trb->DataMap != NULL)) {
    IoMmuUnmap (Trb->DataMap);
  }

  if ((Trb != NULL) && (Trb->AdmaDesc != NULL)) {
    SdPeimFreeMem (Trb->Slot->Private->Pool, Trb->AdmaDesc, Trb->AdmaDescSize);
  }

  if (Trb != NULL) {
    FreePool (Trb);
  }
  return;
}

/**
  Check if the env is ready for execute specified TRB.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] Trb            The pointer to the SD_TRB instance.

  @retval EFI_SUCCESS       The env is ready for TRB execution.
  @retval EFI_NOT_READY     The env is not ready for TRB execution.
  @retval Others            Some erros happen.

**/
EFI_STATUS
SdPeimCheckTrbEnv (
  IN UINTN                  Bar,
  IN SD_TRB                 *Trb
  )
{
  EFI_STATUS                          Status;
  SD_COMMAND_PACKET                   *Packet;
  UINT32                              PresentState;

  Packet = Trb->Packet;

  if ((Packet->SdCmdBlk->CommandType == SdCommandTypeAdtc) ||
      (Packet->SdCmdBlk->ResponseType == SdResponseTypeR1b) ||
      (Packet->SdCmdBlk->ResponseType == SdResponseTypeR5b)) {
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

  Status = SdPeimHcCheckMmioSet (
             Bar + SD_HC_PRESENT_STATE,
             sizeof (PresentState),
             PresentState,
             0
             );

  return Status;
}

/**
  Wait for the env to be ready for execute specified TRB.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] Trb            The pointer to the SD_TRB instance.

  @retval EFI_SUCCESS       The env is ready for TRB execution.
  @retval EFI_TIMEOUT       The env is not ready for TRB execution in time.
  @retval Others            Some erros happen.

**/
EFI_STATUS
SdPeimWaitTrbEnv (
  IN UINTN                  Bar,
  IN SD_TRB                 *Trb
  )
{
  EFI_STATUS                          Status;
  SD_COMMAND_PACKET                   *Packet;
  UINT64                              Timeout;
  BOOLEAN                             InfiniteWait;

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
    Status = SdPeimCheckTrbEnv (Bar, Trb);
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
  @param[in] Trb            The pointer to the SD_TRB instance.

  @retval EFI_SUCCESS       The TRB is sent to host controller successfully.
  @retval Others            Some erros happen when sending this request to the host controller.

**/
EFI_STATUS
SdPeimExecTrb (
  IN UINTN                  Bar,
  IN SD_TRB                 *Trb
  )
{
  EFI_STATUS                          Status;
  SD_COMMAND_PACKET                   *Packet;
  UINT16                              Cmd;
  UINT16                              IntStatus;
  UINT32                              Argument;
  UINT16                              BlkCount;
  UINT16                              BlkSize;
  UINT16                              TransMode;
  UINT8                               HostCtrl1;
  UINT32                              SdmaAddr;
  UINT64                              AdmaAddr;

  Packet = Trb->Packet;
  //
  // Clear all bits in Error Interrupt Status Register
  //
  IntStatus = 0xFFFF;
  Status    = SdPeimHcRwMmio (Bar + SD_HC_ERR_INT_STS, FALSE, sizeof (IntStatus), &IntStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Clear all bits in Normal Interrupt Status Register
  //
  IntStatus = 0xFFFF;
  Status    = SdPeimHcRwMmio (Bar + SD_HC_NOR_INT_STS, FALSE, sizeof (IntStatus), &IntStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Set Host Control 1 register DMA Select field
  //
  if (Trb->Mode == SdAdmaMode) {
    HostCtrl1 = BIT4;
    Status = SdPeimHcOrMmio (Bar + SD_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  SdPeimHcLedOnOff (Bar, TRUE);

  if (Trb->Mode == SdSdmaMode) {
    if ((UINT64)(UINTN)Trb->DataPhy >= 0x100000000ul) {
      return EFI_INVALID_PARAMETER;
    }

    SdmaAddr = (UINT32)(UINTN)Trb->DataPhy;
    Status   = SdPeimHcRwMmio (Bar + SD_HC_SDMA_ADDR, FALSE, sizeof (SdmaAddr), &SdmaAddr);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else if (Trb->Mode == SdAdmaMode) {
    AdmaAddr = (UINT64)(UINTN)Trb->AdmaDesc;
    Status   = SdPeimHcRwMmio (Bar + SD_HC_ADMA_SYS_ADDR, FALSE, sizeof (AdmaAddr), &AdmaAddr);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  BlkSize = Trb->BlockSize;
  if (Trb->Mode == SdSdmaMode) {
    //
    // Set SDMA boundary to be 512K bytes.
    //
    BlkSize |= 0x7000;
  }

  Status = SdPeimHcRwMmio (Bar + SD_HC_BLK_SIZE, FALSE, sizeof (BlkSize), &BlkSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BlkCount = 0;
  if (Trb->Mode != SdNoData) {
    //
    // Calcuate Block Count.
    //
    BlkCount = (UINT16)(Trb->DataLen / Trb->BlockSize);
  }
  Status   = SdPeimHcRwMmio (Bar + SD_HC_BLK_COUNT, FALSE, sizeof (BlkCount), &BlkCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Argument = Packet->SdCmdBlk->CommandArgument;
  Status   = SdPeimHcRwMmio (Bar + SD_HC_ARG1, FALSE, sizeof (Argument), &Argument);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TransMode = 0;
  if (Trb->Mode != SdNoData) {
    if (Trb->Mode != SdPioMode) {
      TransMode |= BIT0;
    }
    if (Trb->Read) {
      TransMode |= BIT4;
    }
    if (BlkCount > 1) {
      TransMode |= BIT5 | BIT1;
    }
    //
    // SD memory card needs to use AUTO CMD12 feature.
    //
    if (BlkCount > 1) {
      TransMode |= BIT2;
    }
  }

  Status = SdPeimHcRwMmio (Bar + SD_HC_TRANS_MOD, FALSE, sizeof (TransMode), &TransMode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Cmd = (UINT16)LShiftU64(Packet->SdCmdBlk->CommandIndex, 8);
  if (Packet->SdCmdBlk->CommandType == SdCommandTypeAdtc) {
    Cmd |= BIT5;
  }
  //
  // Convert ResponseType to value
  //
  if (Packet->SdCmdBlk->CommandType != SdCommandTypeBc) {
    switch (Packet->SdCmdBlk->ResponseType) {
      case SdResponseTypeR1:
      case SdResponseTypeR5:
      case SdResponseTypeR6:
      case SdResponseTypeR7:
        Cmd |= (BIT1 | BIT3 | BIT4);
        break;
      case SdResponseTypeR2:
        Cmd |= (BIT0 | BIT3);
       break;
      case SdResponseTypeR3:
      case SdResponseTypeR4:
        Cmd |= BIT1;
        break;
      case SdResponseTypeR1b:
      case SdResponseTypeR5b:
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
  Status = SdPeimHcRwMmio (Bar + SD_HC_COMMAND, FALSE, sizeof (Cmd), &Cmd);
  return Status;
}

/**
  Check the TRB execution result.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] Trb            The pointer to the SD_TRB instance.

  @retval EFI_SUCCESS       The TRB is executed successfully.
  @retval EFI_NOT_READY     The TRB is not completed for execution.
  @retval Others            Some erros happen when executing this request.

**/
EFI_STATUS
SdPeimCheckTrbResult (
  IN UINTN                  Bar,
  IN SD_TRB                 *Trb
  )
{
  EFI_STATUS                          Status;
  SD_COMMAND_PACKET                   *Packet;
  UINT16                              IntStatus;
  UINT32                              Response[4];
  UINT32                              SdmaAddr;
  UINT8                               Index;
  UINT8                               SwReset;
  UINT32                              PioLength;

  SwReset = 0;
  Packet  = Trb->Packet;
  //
  // Check Trb execution result by reading Normal Interrupt Status register.
  //
  Status = SdPeimHcRwMmio (
             Bar + SD_HC_NOR_INT_STS,
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
      Status = SdPeimHcRwMmio (
                 Bar + SD_HC_ERR_INT_STS,
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
    Status = SdPeimHcRwMmio (
               Bar + SD_HC_ERR_INT_STS,
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

    Status = SdPeimHcRwMmio (
               Bar + SD_HC_SW_RST,
               FALSE,
               sizeof (SwReset),
               &SwReset
               );
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    Status = SdPeimHcWaitMmioSet (
               Bar + SD_HC_SW_RST,
               sizeof (SwReset),
               0xFF,
               0,
               SD_TIMEOUT
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
  if ((Trb->Mode == SdSdmaMode) && ((IntStatus & BIT3) == BIT3)) {
    //
    // Clear DMA interrupt bit.
    //
    IntStatus = BIT3;
    Status    = SdPeimHcRwMmio (
                  Bar + SD_HC_NOR_INT_STS,
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
    SdmaAddr = SD_SDMA_ROUND_UP ((UINT32)(UINTN)Trb->DataPhy, SD_SDMA_BOUNDARY);
    Status   = SdPeimHcRwMmio (
                 Bar + SD_HC_SDMA_ADDR,
                 FALSE,
                 sizeof (UINT32),
                 &SdmaAddr
                 );
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    Trb->DataPhy = (UINT32)(UINTN)SdmaAddr;
  }

  if ((Packet->SdCmdBlk->CommandType != SdCommandTypeAdtc) &&
      (Packet->SdCmdBlk->ResponseType != SdResponseTypeR1b) &&
      (Packet->SdCmdBlk->ResponseType != SdResponseTypeR5b)) {
    if ((IntStatus & BIT0) == BIT0) {
      Status = EFI_SUCCESS;
      goto Done;
    }
  }

  if (Packet->SdCmdBlk->CommandIndex == SD_SEND_TUNING_BLOCK) {
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
      SdPeimHcRwMmio (Bar + SD_HC_NOR_INT_STS, FALSE, sizeof (IntStatus), &IntStatus);
      //
      // Read data out from Buffer Port register
      //
      for (PioLength = 0; PioLength < Trb->DataLen; PioLength += 4) {
        SdPeimHcRwMmio (Bar + SD_HC_BUF_DAT_PORT, TRUE, 4, (UINT8*)Trb->Data + PioLength);
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
    if (Packet->SdCmdBlk->CommandType != SdCommandTypeBc) {
      for (Index = 0; Index < 4; Index++) {
        Status = SdPeimHcRwMmio (
                   Bar + SD_HC_RESPONSE + Index * 4,
                   TRUE,
                   sizeof (UINT32),
                   &Response[Index]
                   );
        if (EFI_ERROR (Status)) {
          SdPeimHcLedOnOff (Bar, FALSE);
          return Status;
        }
      }
      CopyMem (Packet->SdStatusBlk, Response, sizeof (Response));
    }
  }

  if (Status != EFI_NOT_READY) {
    SdPeimHcLedOnOff (Bar, FALSE);
  }

  return Status;
}

/**
  Wait for the TRB execution result.

  @param[in] Bar            The mmio base address of the slot to be accessed.
  @param[in] Trb            The pointer to the SD_TRB instance.

  @retval EFI_SUCCESS       The TRB is executed successfully.
  @retval Others            Some erros happen when executing this request.

**/
EFI_STATUS
SdPeimWaitTrbResult (
  IN UINTN                  Bar,
  IN SD_TRB                 *Trb
  )
{
  EFI_STATUS                        Status;
  SD_COMMAND_PACKET                 *Packet;
  UINT64                            Timeout;
  BOOLEAN                           InfiniteWait;

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
    Status = SdPeimCheckTrbResult (Bar, Trb);
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
  Sends SD command to an SD card that is attached to the SD controller.

  If Packet is successfully sent to the SD card, then EFI_SUCCESS is returned.

  If a device error occurs while sending the Packet, then EFI_DEVICE_ERROR is returned.

  If Slot is not in a valid range for the SD controller, then EFI_INVALID_PARAMETER
  is returned.

  If Packet defines a data command but both InDataBuffer and OutDataBuffer are NULL,
  EFI_INVALID_PARAMETER is returned.

  @param[in]     Slot           The slot number of the Sd card to send the command to.
  @param[in,out] Packet         A pointer to the SD command data structure.

  @retval EFI_SUCCESS           The SD Command Packet was sent by the host.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send the SD
                                command Packet.
  @retval EFI_INVALID_PARAMETER Packet, Slot, or the contents of the Packet is invalid.
  @retval EFI_INVALID_PARAMETER Packet defines a data command but both InDataBuffer and
                                OutDataBuffer are NULL.
  @retval EFI_NO_MEDIA          SD Device not present in the Slot.
  @retval EFI_UNSUPPORTED       The command described by the SD Command Packet is not
                                supported by the host controller.
  @retval EFI_BAD_BUFFER_SIZE   The InTransferLength or OutTransferLength exceeds the
                                limit supported by SD card ( i.e. if the number of bytes
                                exceed the Last LBA).

**/
EFI_STATUS
EFIAPI
SdPeimExecCmd (
  IN     SD_PEIM_HC_SLOT       *Slot,
  IN OUT SD_COMMAND_PACKET     *Packet
  )
{
  EFI_STATUS                   Status;
  SD_TRB                       *Trb;

  if (Packet == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->SdCmdBlk == NULL) || (Packet->SdStatusBlk == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->OutDataBuffer == NULL) && (Packet->OutTransferLength != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->InDataBuffer == NULL) && (Packet->InTransferLength != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Trb = SdPeimCreateTrb (Slot, Packet);
  if (Trb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = SdPeimWaitTrbEnv (Slot->SdHcBase, Trb);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = SdPeimExecTrb (Slot->SdHcBase, Trb);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = SdPeimWaitTrbResult (Slot->SdHcBase, Trb);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

Done:
  SdPeimFreeTrb (Trb);

  return Status;
}

/**
  Send command GO_IDLE_STATE to the device to make it go to Idle State.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The SD device is reset correctly.
  @retval Others            The device reset fails.

**/
EFI_STATUS
SdPeimReset (
  IN SD_PEIM_HC_SLOT        *Slot
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout     = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SD_GO_IDLE_STATE;
  SdCmdBlk.CommandType  = SdCommandTypeBc;
  SdCmdBlk.ResponseType = 0;
  SdCmdBlk.CommandArgument = 0;

  Status = SdPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SEND_IF_COND to the device to inquiry the SD Memory Card interface
  condition.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] SupplyVoltage  The supplied voltage by the host.
  @param[in] CheckPattern   The check pattern to be sent to the device.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimVoltageCheck (
  IN SD_PEIM_HC_SLOT        *Slot,
  IN UINT8                  SupplyVoltage,
  IN UINT8                  CheckPattern
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout     = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SD_SEND_IF_COND;
  SdCmdBlk.CommandType  = SdCommandTypeBcr;
  SdCmdBlk.ResponseType = SdResponseTypeR7;
  SdCmdBlk.CommandArgument = (SupplyVoltage << 8) | CheckPattern;

  Status = SdPeimExecCmd (Slot, &Packet);
  if (!EFI_ERROR (Status)) {
    if (SdStatusBlk.Resp0 != SdCmdBlk.CommandArgument) {
      return EFI_DEVICE_ERROR;
    }
  }

  return Status;
}

/**
  Send command SDIO_SEND_OP_COND to the device to see whether it is SDIO device.

  Refer to SDIO Simplified Spec 3 Section 3.2 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] VoltageWindow  The supply voltage window.
  @param[in] S18r           The boolean to show if it should switch to 1.8v.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdioSendOpCond (
  IN SD_PEIM_HC_SLOT        *Slot,
  IN UINT32                 VoltageWindow,
  IN BOOLEAN                S18r
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;
  UINT32                              Switch;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout     = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SDIO_SEND_OP_COND;
  SdCmdBlk.CommandType  = SdCommandTypeBcr;
  SdCmdBlk.ResponseType = SdResponseTypeR4;

  Switch = S18r ? BIT24 : 0;

  SdCmdBlk.CommandArgument = (VoltageWindow & 0xFFFFFF) | Switch;

  Status = SdPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SD_SEND_OP_COND to the device to see whether it is SDIO device.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  Slot           The slot number of the SD card to send the command to.
  @param[in]  Rca            The relative device address of addressed device.
  @param[in]  VoltageWindow  The supply voltage window.
  @param[in]  S18r           The boolean to show if it should switch to 1.8v.
  @param[in]  Xpc            The boolean to show if it should provide 0.36w power control.
  @param[in]  Hcs            The boolean to show if it support host capacity info.
  @param[out] Ocr            The buffer to store returned OCR register value.


  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimSendOpCond (
  IN     SD_PEIM_HC_SLOT              *Slot,
  IN     UINT16                       Rca,
  IN     UINT32                       VoltageWindow,
  IN     BOOLEAN                      S18r,
  IN     BOOLEAN                      Xpc,
  IN     BOOLEAN                      Hcs,
     OUT UINT32                       *Ocr
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;
  UINT32                              Switch;
  UINT32                              MaxPower;
  UINT32                              HostCapacity;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout     = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SD_APP_CMD;
  SdCmdBlk.CommandType  = SdCommandTypeAc;
  SdCmdBlk.ResponseType = SdResponseTypeR1;
  SdCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdPeimExecCmd (Slot, &Packet);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SdCmdBlk.CommandIndex = SD_SEND_OP_COND;
  SdCmdBlk.CommandType  = SdCommandTypeBcr;
  SdCmdBlk.ResponseType = SdResponseTypeR3;

  Switch       = S18r ? BIT24 : 0;
  MaxPower     = Xpc ? BIT28 : 0;
  HostCapacity = Hcs ? BIT30 : 0;
  SdCmdBlk.CommandArgument = (VoltageWindow & 0xFFFFFF) | Switch | MaxPower | HostCapacity;

  Status = SdPeimExecCmd (Slot, &Packet);
  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    *Ocr = SdStatusBlk.Resp0;
  }

  return Status;
}

/**
  Broadcast command ALL_SEND_CID to the bus to ask all the SD devices to send the
  data of their CID registers.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimAllSendCid (
  IN SD_PEIM_HC_SLOT        *Slot
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout        = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SD_ALL_SEND_CID;
  SdCmdBlk.CommandType  = SdCommandTypeBcr;
  SdCmdBlk.ResponseType = SdResponseTypeR2;
  SdCmdBlk.CommandArgument = 0;

  Status = SdPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SET_RELATIVE_ADDR to the SD device to assign a Relative device
  Address (RCA).

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[out] Rca           The relative device address to be assigned.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimSetRca (
  IN     SD_PEIM_HC_SLOT              *Slot,
     OUT UINT16                       *Rca
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout        = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SD_SET_RELATIVE_ADDR;
  SdCmdBlk.CommandType  = SdCommandTypeBcr;
  SdCmdBlk.ResponseType = SdResponseTypeR6;

  Status = SdPeimExecCmd (Slot, &Packet);
  if (!EFI_ERROR (Status)) {
    *Rca = (UINT16)(SdStatusBlk.Resp0 >> 16);
  }

  return Status;
}

/**
  Send command SEND_CSD to the SD device to get the data of the CSD register.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[in]  Rca           The relative device address of selected device.
  @param[out] Csd           The buffer to store the content of the CSD register.
                            Note the caller should ignore the lowest byte of this
                            buffer as the content of this byte is meaningless even
                            if the operation succeeds.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimGetCsd (
  IN     SD_PEIM_HC_SLOT              *Slot,
  IN     UINT16                       Rca,
     OUT SD_CSD                       *Csd
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout        = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SD_SEND_CSD;
  SdCmdBlk.CommandType  = SdCommandTypeAc;
  SdCmdBlk.ResponseType = SdResponseTypeR2;
  SdCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdPeimExecCmd (Slot, &Packet);
  if (!EFI_ERROR (Status)) {
    //
    // For details, refer to SD Host Controller Simplified Spec 3.0 Table 2-12.
    //
    CopyMem (((UINT8*)Csd) + 1, &SdStatusBlk.Resp0, sizeof (SD_CSD) - 1);
  }

  return Status;
}

/**
  Send command SELECT_DESELECT_CARD to the SD device to select/deselect it.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[in]  Rca           The relative device address of selected device.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimSelect (
  IN SD_PEIM_HC_SLOT        *Slot,
  IN UINT16                 Rca
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout        = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SD_SELECT_DESELECT_CARD;
  SdCmdBlk.CommandType  = SdCommandTypeAc;
  SdCmdBlk.ResponseType = SdResponseTypeR1b;
  SdCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command VOLTAGE_SWITCH to the SD device to switch the voltage of the device.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  Slot          The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimVoltageSwitch (
  IN SD_PEIM_HC_SLOT        *Slot
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout        = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SD_VOLTAGE_SWITCH;
  SdCmdBlk.CommandType  = SdCommandTypeAc;
  SdCmdBlk.ResponseType = SdResponseTypeR1;
  SdCmdBlk.CommandArgument = 0;

  Status = SdPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SET_BUS_WIDTH to the SD device to set the bus width.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address of addressed device.
  @param[in] BusWidth       The bus width to be set, it could be 1 or 4.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimSetBusWidth (
  IN SD_PEIM_HC_SLOT        *Slot,
  IN UINT16                 Rca,
  IN UINT8                  BusWidth
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;
  UINT8                               Value;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout     = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SD_APP_CMD;
  SdCmdBlk.CommandType  = SdCommandTypeAc;
  SdCmdBlk.ResponseType = SdResponseTypeR1;
  SdCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdPeimExecCmd (Slot, &Packet);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SdCmdBlk.CommandIndex = SD_SET_BUS_WIDTH;
  SdCmdBlk.CommandType  = SdCommandTypeAc;
  SdCmdBlk.ResponseType = SdResponseTypeR1;

  if (BusWidth == 1) {
    Value = 0;
  } else if (BusWidth == 4) {
    Value = 2;
  } else {
    return EFI_INVALID_PARAMETER;
  }
  SdCmdBlk.CommandArgument = Value & 0x3;

  Status = SdPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SWITCH_FUNC to the SD device to check switchable function or switch card function.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[in]  AccessMode    The value for access mode group.
  @param[in]  CommandSystem The value for command set group.
  @param[in]  DriveStrength The value for drive length group.
  @param[in]  PowerLimit    The value for power limit group.
  @param[in]  Mode          Switch or check function.
  @param[out] SwitchResp    The return switch function status.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimSwitch (
  IN     SD_PEIM_HC_SLOT              *Slot,
  IN     UINT8                        AccessMode,
  IN     UINT8                        CommandSystem,
  IN     UINT8                        DriveStrength,
  IN     UINT8                        PowerLimit,
  IN     BOOLEAN                      Mode,
     OUT UINT8                        *SwitchResp
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;
  UINT32                              ModeValue;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout        = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SD_SWITCH_FUNC;
  SdCmdBlk.CommandType  = SdCommandTypeAdtc;
  SdCmdBlk.ResponseType = SdResponseTypeR1;

  ModeValue = Mode ? BIT31 : 0;
  SdCmdBlk.CommandArgument = (AccessMode & 0xF) | ((PowerLimit & 0xF) << 4) | \
                             ((DriveStrength & 0xF) << 8) | ((DriveStrength & 0xF) << 12) | \
                             ModeValue;
  Packet.InDataBuffer     = SwitchResp;
  Packet.InTransferLength = 64;

  Status = SdPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SEND_STATUS to the addressed SD device to get its status register.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in]  Slot          The slot number of the SD card to send the command to.
  @param[in]  Rca           The relative device address of addressed device.
  @param[out] DevStatus     The returned device status.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimSendStatus (
  IN     SD_PEIM_HC_SLOT              *Slot,
  IN     UINT16                       Rca,
     OUT UINT32                       *DevStatus
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout     = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SD_SEND_STATUS;
  SdCmdBlk.CommandType  = SdCommandTypeAc;
  SdCmdBlk.ResponseType = SdResponseTypeR1;
  SdCmdBlk.CommandArgument = (UINT32)Rca << 16;

  Status = SdPeimExecCmd (Slot, &Packet);
  if (!EFI_ERROR (Status)) {
    *DevStatus = SdStatusBlk.Resp0;
  }

  return Status;
}

/**
  Send command READ_SINGLE_BLOCK/WRITE_SINGLE_BLOCK to the addressed SD device
  to read/write the specified number of blocks.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Lba            The logical block address of starting access.
  @param[in] BlockSize      The block size of specified SD device partition.
  @param[in] Buffer         The pointer to the transfer buffer.
  @param[in] BufferSize     The size of transfer buffer.
  @param[in] IsRead         Boolean to show the operation direction.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimRwSingleBlock (
  IN SD_PEIM_HC_SLOT                *Slot,
  IN EFI_LBA                        Lba,
  IN UINT32                         BlockSize,
  IN VOID                           *Buffer,
  IN UINTN                          BufferSize,
  IN BOOLEAN                        IsRead
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  //
  // Calculate timeout value through the below formula.
  // Timeout = (transfer size) / (2MB/s).
  // Taking 2MB/s as divisor is because it's the lowest
  // transfer speed of class 2.
  //
  Packet.Timeout       = (BufferSize / (2 * 1024 * 1024) + 1) * 1000 * 1000;;

  if (IsRead) {
    Packet.InDataBuffer     = Buffer;
    Packet.InTransferLength = (UINT32)BufferSize;

    SdCmdBlk.CommandIndex = SD_READ_SINGLE_BLOCK;
    SdCmdBlk.CommandType  = SdCommandTypeAdtc;
    SdCmdBlk.ResponseType = SdResponseTypeR1;
  } else {
    Packet.OutDataBuffer     = Buffer;
    Packet.OutTransferLength = (UINT32)BufferSize;

    SdCmdBlk.CommandIndex = SD_WRITE_SINGLE_BLOCK;
    SdCmdBlk.CommandType  = SdCommandTypeAdtc;
    SdCmdBlk.ResponseType = SdResponseTypeR1;
  }

  if (Slot->SectorAddressing) {
    SdCmdBlk.CommandArgument = (UINT32)Lba;
  } else {
    SdCmdBlk.CommandArgument = (UINT32)MultU64x32 (Lba, BlockSize);
  }

  Status = SdPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command READ_MULTIPLE_BLOCK/WRITE_MULTIPLE_BLOCK to the addressed SD device
  to read/write the specified number of blocks.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Lba            The logical block address of starting access.
  @param[in] BlockSize      The block size of specified SD device partition.
  @param[in] Buffer         The pointer to the transfer buffer.
  @param[in] BufferSize     The size of transfer buffer.
  @param[in] IsRead         Boolean to show the operation direction.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimRwMultiBlocks (
  IN SD_PEIM_HC_SLOT                *Slot,
  IN EFI_LBA                        Lba,
  IN UINT32                         BlockSize,
  IN VOID                           *Buffer,
  IN UINTN                          BufferSize,
  IN BOOLEAN                        IsRead
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  //
  // Calculate timeout value through the below formula.
  // Timeout = (transfer size) / (2MB/s).
  // Taking 2MB/s as divisor is because it's the lowest
  // transfer speed of class 2.
  //
  Packet.Timeout       = (BufferSize / (2 * 1024 * 1024) + 1) * 1000 * 1000;;

  if (IsRead) {
    Packet.InDataBuffer     = Buffer;
    Packet.InTransferLength = (UINT32)BufferSize;

    SdCmdBlk.CommandIndex = SD_READ_MULTIPLE_BLOCK;
    SdCmdBlk.CommandType  = SdCommandTypeAdtc;
    SdCmdBlk.ResponseType = SdResponseTypeR1;
  } else {
    Packet.OutDataBuffer     = Buffer;
    Packet.OutTransferLength = (UINT32)BufferSize;

    SdCmdBlk.CommandIndex = SD_WRITE_MULTIPLE_BLOCK;
    SdCmdBlk.CommandType  = SdCommandTypeAdtc;
    SdCmdBlk.ResponseType = SdResponseTypeR1;
  }

  if (Slot->SectorAddressing) {
    SdCmdBlk.CommandArgument = (UINT32)Lba;
  } else {
    SdCmdBlk.CommandArgument = (UINT32)MultU64x32 (Lba, BlockSize);
  }

  Status = SdPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Send command SEND_TUNING_BLOCK to the SD device for SDR104/SDR50 optimal sampling point
  detection.

  It may be sent up to 40 times until the host finishes the tuning procedure.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimSendTuningBlk (
  IN SD_PEIM_HC_SLOT        *Slot
  )
{
  SD_COMMAND_BLOCK                    SdCmdBlk;
  SD_STATUS_BLOCK                     SdStatusBlk;
  SD_COMMAND_PACKET                   Packet;
  EFI_STATUS                          Status;
  UINT8                               TuningBlock[64];

  ZeroMem (&SdCmdBlk, sizeof (SdCmdBlk));
  ZeroMem (&SdStatusBlk, sizeof (SdStatusBlk));
  ZeroMem (&Packet, sizeof (Packet));

  Packet.SdCmdBlk    = &SdCmdBlk;
  Packet.SdStatusBlk = &SdStatusBlk;
  Packet.Timeout     = SD_TIMEOUT;

  SdCmdBlk.CommandIndex = SD_SEND_TUNING_BLOCK;
  SdCmdBlk.CommandType  = SdCommandTypeAdtc;
  SdCmdBlk.ResponseType = SdResponseTypeR1;
  SdCmdBlk.CommandArgument = 0;

  Packet.InDataBuffer     = TuningBlock;
  Packet.InTransferLength = sizeof (TuningBlock);

  Status = SdPeimExecCmd (Slot, &Packet);

  return Status;
}

/**
  Tunning the sampling point of SDR104 or SDR50 bus speed mode.

  Command SD_SEND_TUNING_BLOCK may be sent up to 40 times until the host finishes the
  tuning procedure.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 and SD Host Controller
  Simplified Spec 3.0 Figure 2-29 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimTuningClock (
  IN SD_PEIM_HC_SLOT        *Slot
  )
{
  EFI_STATUS          Status;
  UINT8               HostCtrl2;
  UINT8               Retry;

  //
  // Notify the host that the sampling clock tuning procedure starts.
  //
  HostCtrl2 = BIT6;
  Status = SdPeimHcOrMmio (Slot->SdHcBase + SD_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Ask the device to send a sequence of tuning blocks till the tuning procedure is done.
  //
  Retry = 0;
  do {
    Status = SdPeimSendTuningBlk (Slot);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = SdPeimHcRwMmio (Slot->SdHcBase + SD_HC_HOST_CTRL2, TRUE, sizeof (HostCtrl2), &HostCtrl2);
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

  DEBUG ((EFI_D_ERROR, "SdPeimTuningClock: Send tuning block fails at %d times with HostCtrl2 %02x\n", Retry, HostCtrl2));
  //
  // Abort the tuning procedure and reset the tuning circuit.
  //
  HostCtrl2 = (UINT8)~(BIT6 | BIT7);
  Status = SdPeimHcAndMmio (Slot->SdHcBase + SD_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  return EFI_DEVICE_ERROR;
}

/**
  Switch the bus width to specified width.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 and
  SD Host Controller Simplified Spec 3.0 section Figure 3-7 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] BusWidth       The bus width to be set, it could be 4 or 8.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimSwitchBusWidth (
  IN SD_PEIM_HC_SLOT        *Slot,
  IN UINT16                 Rca,
  IN UINT8                  BusWidth
  )
{
  EFI_STATUS          Status;
  UINT32              DevStatus;

  Status = SdPeimSetBusWidth (Slot, Rca, BusWidth);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SdPeimSendStatus (Slot, Rca, &DevStatus);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Check the switch operation is really successful or not.
  //
  if ((DevStatus >> 16) != 0) {
    return EFI_DEVICE_ERROR;
  }

  Status = SdPeimHcSetBusWidth (Slot->SdHcBase, BusWidth);

  return Status;
}

/**
  Switch the high speed timing according to request.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 4.7 and
  SD Host Controller Simplified Spec 3.0 section Figure 2-29 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.
  @param[in] Rca            The relative device address to be assigned.
  @param[in] S18a           The boolean to show if it's a UHS-I SD card.

  @retval EFI_SUCCESS       The operation is done correctly.
  @retval Others            The operation fails.

**/
EFI_STATUS
SdPeimSetBusMode (
  IN SD_PEIM_HC_SLOT        *Slot,
  IN UINT16                 Rca,
  IN BOOLEAN                S18a
  )
{
  EFI_STATUS                   Status;
  SD_HC_SLOT_CAP               Capability;
  UINT32                       ClockFreq;
  UINT8                        BusWidth;
  UINT8                        AccessMode;
  UINT8                        HostCtrl1;
  UINT8                        HostCtrl2;
  UINT8                        SwitchResp[64];

  Status = SdPeimGetCsd (Slot, Rca, &Slot->Csd);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SdPeimSetBusMode: SdPeimGetCsd fails with %r\n", Status));
    return Status;
  }

  Status = SdPeimHcGetCapability (Slot->SdHcBase, &Capability);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SdPeimSelect (Slot, Rca);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SdPeimSetBusMode: SdPeimSelect fails with %r\n", Status));
    return Status;
  }

  BusWidth = 4;
  Status = SdPeimSwitchBusWidth (Slot, Rca, BusWidth);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SdPeimSetBusMode: SdPeimSwitchBusWidth fails with %r\n", Status));
    return Status;
  }

  //
  // Get the supported bus speed from SWITCH cmd return data group #1.
  //
  ZeroMem (SwitchResp, sizeof (SwitchResp));
  Status = SdPeimSwitch (Slot, 0xF, 0xF, 0xF, 0xF, FALSE, SwitchResp);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Calculate supported bus speed/bus width/clock frequency by host and device capability.
  //
  ClockFreq = 0;
  if (S18a && (Capability.Sdr104 != 0) && ((SwitchResp[13] & BIT3) != 0)) {
    ClockFreq = 208;
    AccessMode = 3;
  } else if (S18a && (Capability.Sdr50 != 0) && ((SwitchResp[13] & BIT2) != 0)) {
    ClockFreq = 100;
    AccessMode = 2;
  } else if (S18a && (Capability.Ddr50 != 0) && ((SwitchResp[13] & BIT4) != 0)) {
    ClockFreq = 50;
    AccessMode = 4;
  } else if ((SwitchResp[13] & BIT1) != 0) {
    ClockFreq = 50;
    AccessMode = 1;
  } else {
    ClockFreq = 25;
    AccessMode = 0;
  }

  DEBUG ((EFI_D_INFO, "SdPeimSetBusMode: AccessMode %d ClockFreq %d BusWidth %d\n", AccessMode, ClockFreq, BusWidth));

  Status = SdPeimSwitch (Slot, AccessMode, 0xF, 0xF, 0xF, TRUE, SwitchResp);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SdPeimSetBusMode: SdPeimSwitch fails with %r\n", Status));
    return Status;
  }

  if ((SwitchResp[16] & 0xF) != AccessMode) {
    DEBUG ((EFI_D_ERROR, "SdPeimSetBusMode: SdPeimSwitch to AccessMode %d ClockFreq %d BusWidth %d fails! The Switch response is 0x%1x\n", AccessMode, ClockFreq, BusWidth, SwitchResp[16] & 0xF));
    return EFI_DEVICE_ERROR;
  }
  //
  // Set to Hight Speed timing
  //
  if (AccessMode == 1) {
    HostCtrl1 = BIT2;
    Status = SdPeimHcOrMmio (Slot->SdHcBase + SD_HC_HOST_CTRL1, sizeof (HostCtrl1), &HostCtrl1);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  HostCtrl2 = (UINT8)~0x7;
  Status = SdPeimHcAndMmio (Slot->SdHcBase + SD_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  HostCtrl2 = AccessMode;
  Status = SdPeimHcOrMmio (Slot->SdHcBase + SD_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SdPeimHcClockSupply (Slot->SdHcBase, ClockFreq * 1000);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SdPeimSetBusMode: SdPeimHcClockSupply %r\n", Status));
    return Status;
  }

  if ((AccessMode == 3) || ((AccessMode == 2) && (Capability.TuningSDR50 != 0))) {
    Status = SdPeimTuningClock (Slot);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "SdPeimSetBusMode: SdPeimTuningClock fails with %r\n", Status));
      return Status;
    }
  }

  DEBUG ((EFI_D_INFO, "SdPeimSetBusMode: SdPeimSetBusMode %r\n", Status));

  return Status;
}

/**
  Execute SD device identification procedure.

  Refer to SD Physical Layer Simplified Spec 4.1 Section 3.6 for details.

  @param[in] Slot           The slot number of the SD card to send the command to.

  @retval EFI_SUCCESS       There is a SD card.
  @retval Others            There is not a SD card.

**/
EFI_STATUS
SdPeimIdentification (
  IN SD_PEIM_HC_SLOT        *Slot
  )
{
  EFI_STATUS                     Status;
  UINT32                         Ocr;
  UINT16                         Rca;
  BOOLEAN                        Xpc;
  BOOLEAN                        S18r;
  UINT64                         MaxCurrent;
  UINT64                         Current;
  UINT16                         ControllerVer;
  UINT8                          PowerCtrl;
  UINT32                         PresentState;
  UINT8                          HostCtrl2;
  SD_HC_SLOT_CAP                 Capability;
  UINTN                          Retry;
  //
  // 1. Send Cmd0 to the device
  //
  Status = SdPeimReset (Slot);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SdPeimIdentification: Executing Cmd0 fails with %r\n", Status));
    return Status;
  }
  //
  // 2. Send Cmd8 to the device
  //
  Status = SdPeimVoltageCheck (Slot, 0x1, 0xFF);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SdPeimIdentification: Executing Cmd8 fails with %r\n", Status));
    return Status;
  }
  //
  // 3. Send SDIO Cmd5 to the device to the SDIO device OCR register.
  //
  Status = SdioSendOpCond (Slot, 0, FALSE);
  if (!EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SdPeimIdentification: Found SDIO device, ignore it as we don't support\n"));
    return EFI_DEVICE_ERROR;
  }
  //
  // 4. Send Acmd41 with voltage window 0 to the device
  //
  Status = SdPeimSendOpCond (Slot, 0, 0, FALSE, FALSE, FALSE, &Ocr);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SdPeimIdentification: Executing SdPeimSendOpCond fails with %r\n", Status));
    return EFI_DEVICE_ERROR;
  }

  Status = SdPeimHcGetCapability (Slot->SdHcBase, &Capability);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SdPeimHcRwMmio (Slot->SdHcBase + SD_HC_MAX_CURRENT_CAP, TRUE, sizeof (Current), &Current);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Capability.Voltage33 != 0) {
    //
    // Support 3.3V
    //
    MaxCurrent = ((UINT32)Current & 0xFF) * 4;
  } else if (Capability.Voltage30 != 0) {
    //
    // Support 3.0V
    //
    MaxCurrent = (((UINT32)Current >> 8) & 0xFF) * 4;
  } else if (Capability.Voltage18 != 0) {
    //
    // Support 1.8V
    //
    MaxCurrent = (((UINT32)Current >> 16) & 0xFF) * 4;
  } else {
    ASSERT (FALSE);
    return EFI_DEVICE_ERROR;
  }

  if (MaxCurrent >= 150) {
    Xpc = TRUE;
  } else {
    Xpc = FALSE;
  }

  Status = SdPeimHcRwMmio (Slot->SdHcBase + SD_HC_CTRL_VER, TRUE, sizeof (ControllerVer), &ControllerVer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((ControllerVer & 0xFF) == 2) {
    S18r = TRUE;
  } else if (((ControllerVer & 0xFF) == 0) || ((ControllerVer & 0xFF) == 1)) {
    S18r = FALSE;
  } else {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }
  //
  // 5. Repeatly send Acmd41 with supply voltage window to the device.
  //    Note here we only support the cards complied with SD physical
  //    layer simplified spec version 2.0 and version 3.0 and above.
  //
  Ocr   = 0;
  Retry = 0;
  do {
    Status = SdPeimSendOpCond (Slot, 0, Ocr, S18r, Xpc, TRUE, &Ocr);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "SdPeimIdentification: SdPeimSendOpCond fails with %r Ocr %x, S18r %x, Xpc %x\n", Status, Ocr, S18r, Xpc));
      return EFI_DEVICE_ERROR;
    }

    if (Retry++ == 100) {
      DEBUG ((EFI_D_ERROR, "SdPeimIdentification: SdPeimSendOpCond fails too many times\n"));
      return EFI_DEVICE_ERROR;
    }
    MicroSecondDelay (10 * 1000);
  } while ((Ocr & BIT31) == 0);

  //
  // 6. If the S18a bit is set and the Host Controller supports 1.8V signaling
  //    (One of support bits is set to 1: SDR50, SDR104 or DDR50 in the
  //    Capabilities register), switch its voltage to 1.8V.
  //
  if ((Capability.Sdr50 != 0 ||
       Capability.Sdr104 != 0 ||
       Capability.Ddr50 != 0) &&
       ((Ocr & BIT24) != 0)) {
    Status = SdPeimVoltageSwitch (Slot);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "SdPeimIdentification: Executing SdPeimVoltageSwitch fails with %r\n", Status));
      Status = EFI_DEVICE_ERROR;
      goto Error;
    } else {
      Status = SdPeimHcStopClock (Slot->SdHcBase);
      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto Error;
      }

      SdPeimHcRwMmio (Slot->SdHcBase + SD_HC_PRESENT_STATE, TRUE, sizeof (PresentState), &PresentState);
      if (((PresentState >> 20) & 0xF) != 0) {
        DEBUG ((EFI_D_ERROR, "SdPeimIdentification: SwitchVoltage fails with PresentState = 0x%x\n", PresentState));
        Status = EFI_DEVICE_ERROR;
        goto Error;
      }
      HostCtrl2  = BIT3;
      SdPeimHcOrMmio (Slot->SdHcBase + SD_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);

      MicroSecondDelay (5000);

      SdPeimHcRwMmio (Slot->SdHcBase + SD_HC_HOST_CTRL2, TRUE, sizeof (HostCtrl2), &HostCtrl2);
      if ((HostCtrl2 & BIT3) == 0) {
        DEBUG ((EFI_D_ERROR, "SdPeimIdentification: SwitchVoltage fails with HostCtrl2 = 0x%x\n", HostCtrl2));
        Status = EFI_DEVICE_ERROR;
        goto Error;
      }

      SdPeimHcInitClockFreq (Slot->SdHcBase);

      MicroSecondDelay (1000);

      SdPeimHcRwMmio (Slot->SdHcBase + SD_HC_PRESENT_STATE, TRUE, sizeof (PresentState), &PresentState);
      if (((PresentState >> 20) & 0xF) != 0xF) {
        DEBUG ((EFI_D_ERROR, "SdPeimIdentification: SwitchVoltage fails with PresentState = 0x%x, It should be 0xF\n", PresentState));
        Status = EFI_DEVICE_ERROR;
        goto Error;
      }
    }
    DEBUG ((EFI_D_INFO, "SdPeimIdentification: Switch to 1.8v signal voltage success\n"));
  }

  Status = SdPeimAllSendCid (Slot);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SdPeimIdentification: Executing SdPeimAllSendCid fails with %r\n", Status));
    return Status;
  }

  Status = SdPeimSetRca (Slot, &Rca);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SdPeimIdentification: Executing SdPeimSetRca fails with %r\n", Status));
    return Status;
  }
  //
  // Enter Data Tranfer Mode.
  //
  DEBUG ((EFI_D_INFO, "Found a SD device at slot [%d]\n", Slot));

  Status = SdPeimSetBusMode (Slot, Rca, ((Ocr & BIT24) != 0));

  return Status;

Error:
  //
  // Set SD Bus Power = 0
  //
  PowerCtrl = (UINT8)~BIT0;
  Status = SdPeimHcAndMmio (Slot->SdHcBase + SD_HC_POWER_CTRL, sizeof (PowerCtrl), &PowerCtrl);
  return EFI_DEVICE_ERROR;
}
