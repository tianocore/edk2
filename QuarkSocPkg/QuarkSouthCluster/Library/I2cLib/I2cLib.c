/** @file
I2C Library for Quark I2C Controller.
Follows I2C Controller setup instructions as detailed in
Quark DataSheet (doc id: 329676) Section 19.1/19.1.3.


Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"

/**
  The Called to Common Service Entry.

  @return None.

**/

VOID
I2cCommonServiceEntry  (
  OUT UINT16 *SaveCmdPtr,
  OUT UINT32 *SaveBar0Ptr
  )
{
  *SaveBar0Ptr = IohMmPci32 (0, I2C_Bus, I2C_Device, I2C_Func, PCI_BAR0);
  if (((*SaveBar0Ptr) & B_IOH_I2C_GPIO_MEMBAR_ADDR_MASK) == 0) {

    IohMmPci32(0, I2C_Bus, I2C_Device, I2C_Func, PCI_BAR0) =
      FixedPcdGet32 (PcdIohI2cMmioBase) & B_IOH_I2C_GPIO_MEMBAR_ADDR_MASK;

    //
    // also Save Cmd Register, Setup by InitializeInternal later during xfers.
    //
    *SaveCmdPtr = IohMmPci16 (0, I2C_Bus, I2C_Device, I2C_Func, PCI_CMD);
  }
}

/**
  The Called on Common Service Exit.

  @return None.

**/
VOID
I2cCommonServiceExit  (
  IN CONST UINT16 SaveCmd,
  IN CONST UINT32 SaveBar0

  )
{
  if ((SaveBar0 & B_IOH_I2C_GPIO_MEMBAR_ADDR_MASK) == 0) {
    IohMmPci16 (0, I2C_Bus, I2C_Device, I2C_Func, PCI_CMD) = SaveCmd;
    IohMmPci32 (0, I2C_Bus, I2C_Device, I2C_Func, PCI_BAR0) = SaveBar0;
  }
}


/**
  The GetI2CIoPortBaseAddress() function gets IO port base address of I2C Controller.

  Always reads PCI configuration space to get MMIO base address of I2C Controller.

  @return The IO port base address of I2C controller.

**/
UINTN
GetI2CIoPortBaseAddress (
  VOID
  )
{
  UINTN     I2CIoPortBaseAddress;

  //
  // Get I2C Memory Mapped registers base address.
  //
  I2CIoPortBaseAddress = IohMmPci32(0, I2C_Bus, I2C_Device, I2C_Func, PCI_BAR0);

  //
  // Make sure that the IO port base address has been properly set.
  //
  ASSERT (I2CIoPortBaseAddress != 0);
  ASSERT (I2CIoPortBaseAddress != 0xFF);

  return I2CIoPortBaseAddress;
}


/**
  The EnableI2CMmioSpace() function enables access to I2C MMIO space.

**/
VOID
EnableI2CMmioSpace (
  VOID
  )
{
  UINT8 PciCmd;

  //
  // Read PCICMD.  Bus=0, Dev=0, Func=0, Reg=0x4
  //
  PciCmd = IohMmPci8(0, I2C_Bus, I2C_Device, I2C_Func, PCI_REG_PCICMD);

  //
  // Enable Bus Master(Bit2), MMIO Space(Bit1) & I/O Space(Bit0)
  //
  PciCmd |= 0x7;
  IohMmPci8(0, I2C_Bus, I2C_Device, I2C_Func, PCI_REG_PCICMD) = PciCmd;

}

/**
  The DisableI2CController() functions disables I2C Controller.

**/
VOID
DisableI2CController (
  VOID
  )
{
  UINTN       I2CIoPortBaseAddress;
  UINT32      Addr;
  UINT32      Data;
  UINT8       PollCount;

  PollCount = 0;

  //
  // Get I2C Memory Mapped registers base address.
  //
  I2CIoPortBaseAddress = GetI2CIoPortBaseAddress ();

  //
  // Disable the I2C Controller by setting IC_ENABLE.ENABLE to zero
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_ENABLE;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  Data &= ~B_I2C_REG_ENABLE;
  *((volatile UINT32 *) (UINTN)(Addr)) = Data;

  //
  // Read the IC_ENABLE_STATUS.IC_EN Bit to check if Controller is disabled
  //
  Data = 0xFF;
  Addr = I2CIoPortBaseAddress + I2C_REG_ENABLE_STATUS;
  Data = *((volatile UINT32 *) (UINTN)(Addr)) & I2C_REG_ENABLE_STATUS;
  while (Data != 0) {
    //
    // Poll the IC_ENABLE_STATUS.IC_EN Bit to check if Controller is disabled, until timeout (TI2C_POLL*MAX_T_POLL_COUNT).
    //
    PollCount++;
    if (PollCount >= MAX_T_POLL_COUNT) {
      break;
    }
    MicroSecondDelay(TI2C_POLL);
    Data = *((volatile UINT32 *) (UINTN)(Addr));
    Data &= I2C_REG_ENABLE_STATUS;
  }

  //
  // Asset if controller does not enter Disabled state.
  //
  ASSERT (PollCount < MAX_T_POLL_COUNT);

  //
  // Read IC_CLR_INTR register to automatically clear the combined interrupt,
  // all individual interrupts and the IC_TX_ABRT_SOURCE register.
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_CLR_INT;
  Data = *((volatile UINT32 *) (UINTN)(Addr));

}

/**
  The EnableI2CController() function enables the I2C Controller.

**/
VOID
EnableI2CController (
  VOID
  )
{
  UINTN   I2CIoPortBaseAddress;
  UINT32  Addr;
  UINT32  Data;

  //
  // Get I2C Memory Mapped registers base address.
  //
  I2CIoPortBaseAddress = GetI2CIoPortBaseAddress ();

  //
  // Enable the I2C Controller by setting IC_ENABLE.ENABLE to 1
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_ENABLE;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  Data |= B_I2C_REG_ENABLE;
  *((volatile UINT32 *) (UINTN)(Addr)) = Data;

  //
  // Clear overflow and abort error status bits before transactions.
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_CLR_RX_OVER;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  Addr = I2CIoPortBaseAddress + I2C_REG_CLR_TX_OVER;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  Addr = I2CIoPortBaseAddress + I2C_REG_CLR_TX_ABRT;
  Data = *((volatile UINT32 *) (UINTN)(Addr));

}

/**
  The WaitForStopDet() function waits until I2C STOP Condition occurs,
  indicating transfer completion.

  @retval EFI_SUCCESS           Stop detected.
  @retval EFI_TIMEOUT           Timeout while waiting for stop condition.
  @retval EFI_ABORTED           Tx abort signaled in HW status register.
  @retval EFI_DEVICE_ERROR      Tx or Rx overflow detected.

**/
EFI_STATUS
WaitForStopDet (
  VOID
  )
{
  UINTN       I2CIoPortBaseAddress;
  UINT32      Addr;
  UINT32      Data;
  UINT32      PollCount;
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  PollCount = 0;

  //
  // Get I2C Memory Mapped registers base address.
  //
  I2CIoPortBaseAddress = GetI2CIoPortBaseAddress ();

  //
  // Wait for STOP Detect.
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_RAW_INTR_STAT;

  do {
    Data = *((volatile UINT32 *) (UINTN)(Addr));
    if ((Data & I2C_REG_RAW_INTR_STAT_TX_ABRT) != 0) {
      Status = EFI_ABORTED;
      break;
    }
    if ((Data & I2C_REG_RAW_INTR_STAT_TX_OVER) != 0) {
      Status = EFI_DEVICE_ERROR;
      break;
    }
    if ((Data & I2C_REG_RAW_INTR_STAT_RX_OVER) != 0) {
      Status = EFI_DEVICE_ERROR;
      break;
    }
    if ((Data & I2C_REG_RAW_INTR_STAT_STOP_DET) != 0) {
      Status = EFI_SUCCESS;
      break;
    }
    MicroSecondDelay(TI2C_POLL);
    PollCount++;
    if (PollCount >= MAX_STOP_DET_POLL_COUNT) {
      Status = EFI_TIMEOUT;
      break;
    }

  } while (TRUE);

  return Status;
}

/**

  The InitializeInternal() function initialises internal I2C Controller
  register values that are commonly required for I2C Write and Read transfers.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @retval EFI_SUCCESS           I2C Operation completed successfully.

**/
EFI_STATUS
InitializeInternal (
  IN  EFI_I2C_ADDR_MODE  AddrMode
  )
{
  UINTN       I2CIoPortBaseAddress;
  UINTN       Addr;
  UINT32      Data;
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  //
  // Enable access to I2C Controller MMIO space.
  //
  EnableI2CMmioSpace ();

  //
  // Disable I2C Controller initially
  //
  DisableI2CController ();

  //
  // Get I2C Memory Mapped registers base address.
  //
  I2CIoPortBaseAddress = GetI2CIoPortBaseAddress ();

  //
  // Clear START_DET
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_CLR_START_DET;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  Data &= ~B_I2C_REG_CLR_START_DET;
  *((volatile UINT32 *) (UINTN)(Addr)) = Data;

  //
  // Clear STOP_DET
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_CLR_STOP_DET;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  Data &= ~B_I2C_REG_CLR_STOP_DET;
  *((volatile UINT32 *) (UINTN)(Addr)) = Data;

  //
  // Set addressing mode to user defined (7 or 10 bit) and
  // speed mode to that defined by PCD (standard mode default).
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_CON;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  // Set Addressing Mode
  if (AddrMode == EfiI2CSevenBitAddrMode) {
    Data &= ~B_I2C_REG_CON_10BITADD_MASTER;
  } else {
    Data |= B_I2C_REG_CON_10BITADD_MASTER;
  }
  // Set Speed Mode
  Data &= ~B_I2C_REG_CON_SPEED;
  if (FeaturePcdGet (PcdI2CFastModeEnabled)) {
    Data |= BIT2;
  } else {
    Data |= BIT1;
  }
  *((volatile UINT32 *) (UINTN)(Addr)) = Data;

  Data = *((volatile UINT32 *) (UINTN)(Addr));

  return Status;

}

/**

  The WriteByte() function provides a standard way to execute a
  standard single byte write to an IC2 device (without accessing
  sub-addresses), as defined in the I2C Specification.

  @param  I2CAddress      I2C Slave device address
  @param  Value           The 8-bit value to write.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Controller aborted xfer.
  @retval EFI_DEVICE_ERROR      Device error detected by controller.

**/
EFI_STATUS
EFIAPI
WriteByte (
  IN  UINTN          I2CAddress,
  IN  UINT8          Value
  )
{
  UINTN       I2CIoPortBaseAddress;
  UINTN       Addr;
  UINT32      Data;
  EFI_STATUS  Status;

  //
  // Get I2C Memory Mapped registers base address
  //
  I2CIoPortBaseAddress = GetI2CIoPortBaseAddress ();

  //
  // Write to the IC_TAR register the address of the slave device to be addressed
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_TAR;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  Data &= ~B_I2C_REG_TAR;
  Data |= I2CAddress;
  *((volatile UINT32 *) (UINTN)(Addr)) = Data;

  //
  // Enable the I2C Controller
  //
  EnableI2CController ();

  //
  // Write the data and transfer direction to the IC_DATA_CMD register.
  // Also specify that transfer should be terminated by STOP condition.
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_DATA_CMD;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  Data &= 0xFFFFFF00;
  Data |= (UINT8)Value;
  Data &= ~B_I2C_REG_DATA_CMD_RW;
  Data |= B_I2C_REG_DATA_CMD_STOP;
  *((volatile UINT32 *) (UINTN)(Addr)) = Data;

  //
  // Wait for transfer completion.
  //
  Status = WaitForStopDet ();

  //
  // Ensure I2C Controller disabled.
  //
  DisableI2CController();

  return Status;
}

/**

  The ReadByte() function provides a standard way to execute a
  standard single byte read to an IC2 device (without accessing
  sub-addresses), as defined in the I2C Specification.

  @param  I2CAddress      I2C Slave device address
  @param  ReturnDataPtr   Pointer to location to receive read byte.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Controller aborted xfer.
  @retval EFI_DEVICE_ERROR      Device error detected by controller.

**/
EFI_STATUS
EFIAPI
ReadByte (
  IN  UINTN          I2CAddress,
  OUT UINT8          *ReturnDataPtr
  )
{
  UINTN       I2CIoPortBaseAddress;
  UINTN       Addr;
  UINT32      Data;
  EFI_STATUS  Status;

  //
  // Get I2C Memory Mapped registers base address.
  //
  I2CIoPortBaseAddress = GetI2CIoPortBaseAddress ();

  //
  // Write to the IC_TAR register the address of the slave device to be addressed
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_TAR;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  Data &= ~B_I2C_REG_TAR;
  Data |= I2CAddress;
  *((volatile UINT32 *) (UINTN)(Addr)) = Data;

  //
  // Enable the I2C Controller
  //
  EnableI2CController ();

  //
  // Write transfer direction to the IC_DATA_CMD register and
  // specify that transfer should be terminated by STOP condition.
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_DATA_CMD;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  Data &= 0xFFFFFF00;
  Data |= B_I2C_REG_DATA_CMD_RW;
  Data |= B_I2C_REG_DATA_CMD_STOP;
  *((volatile UINT32 *) (UINTN)(Addr)) = Data;

  //
  // Wait for transfer completion
  //
  Status = WaitForStopDet ();
  if (!EFI_ERROR(Status)) {

    //
    // Clear RX underflow before reading IC_DATA_CMD.
    //
    Addr = I2CIoPortBaseAddress + I2C_REG_CLR_RX_UNDER;
    Data = *((volatile UINT32 *) (UINTN)(Addr));

    //
    // Obtain and return read data byte from RX buffer (IC_DATA_CMD[7:0]).
    //
    Addr = I2CIoPortBaseAddress + I2C_REG_DATA_CMD;
    Data = *((volatile UINT32 *) (UINTN)(Addr));
    Data &= 0x000000FF;
    *ReturnDataPtr = (UINT8) Data;

    Addr = I2CIoPortBaseAddress + I2C_REG_RAW_INTR_STAT;
    Data = *((volatile UINT32 *) (UINTN)(Addr));
    Data &= I2C_REG_RAW_INTR_STAT_RX_UNDER;
    if (Data != 0) {
      Status = EFI_DEVICE_ERROR;
    }
  }

  //
  // Ensure I2C Controller disabled.
  //
  DisableI2CController ();

  return Status;
}

/**

  The WriteMultipleByte() function provides a standard way to execute
  multiple byte writes to an IC2 device (e.g. when accessing sub-addresses or
  when writing block of data), as defined in the I2C Specification.

  @param I2CAddress   The I2C slave address of the device
                      with which to communicate.

  @param WriteBuffer  Contains the value of byte to be written to the
                      I2C slave device.

  @param Length       No. of bytes to be written.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Tx abort signaled in HW status register.
  @retval EFI_DEVICE_ERROR      Tx overflow detected.

**/
EFI_STATUS
EFIAPI
WriteMultipleByte (
  IN  UINTN          I2CAddress,
  IN  UINT8          *WriteBuffer,
  IN  UINTN          Length
  )
{
  UINTN       I2CIoPortBaseAddress;
  UINTN       Index;
  UINTN       Addr;
  UINT32      Data;
  EFI_STATUS  Status;

  if (Length > I2C_FIFO_SIZE) {
    return EFI_UNSUPPORTED;  // Routine does not handle xfers > fifo size.
  }

  I2CIoPortBaseAddress = GetI2CIoPortBaseAddress ();

  //
  // Write to the IC_TAR register the address of the slave device to be addressed
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_TAR;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  Data &= ~B_I2C_REG_TAR;
  Data |= I2CAddress;
  *((volatile UINT32 *) (UINTN)(Addr)) = Data;

  //
  // Enable the I2C Controller
  //
  EnableI2CController ();

  //
  // Write the data and transfer direction to the IC_DATA_CMD register.
  // Also specify that transfer should be terminated by STOP condition.
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_DATA_CMD;
  for (Index = 0; Index < Length; Index++) {
    Data = *((volatile UINT32 *) (UINTN)(Addr));
    Data &= 0xFFFFFF00;
    Data |= (UINT8)WriteBuffer[Index];
    Data &= ~B_I2C_REG_DATA_CMD_RW;
    if (Index == (Length-1)) {
      Data |= B_I2C_REG_DATA_CMD_STOP;
    }
    *((volatile UINT32 *) (UINTN)(Addr)) = Data;
  }

  //
  // Wait for transfer completion
  //
  Status = WaitForStopDet ();

  //
  // Ensure I2C Controller disabled.
  //
  DisableI2CController ();
  return Status;
}

/**

  The ReadMultipleByte() function provides a standard way to execute
  multiple byte writes to an IC2 device (e.g. when accessing sub-addresses or
  when reading block of data), as defined in the I2C Specification (I2C combined
  write/read protocol).

  @param I2CAddress   The I2C slave address of the device
                      with which to communicate.

  @param Buffer       Contains the value of byte data written or read from the
                      I2C slave device.

  @param WriteLength  No. of bytes to be written. In this case data
                      written typically contains sub-address or sub-addresses
                      in Hi-Lo format, that need to be read (I2C combined
                      write/read protocol).

  @param ReadLength   No. of bytes to be read from I2C slave device.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Tx abort signaled in HW status register.
  @retval EFI_DEVICE_ERROR      Rx underflow or Rx/Tx overflow detected.

**/
EFI_STATUS
EFIAPI
ReadMultipleByte (
  IN  UINTN          I2CAddress,
  IN  OUT UINT8      *Buffer,
  IN  UINTN          WriteLength,
  IN  UINTN          ReadLength
  )
{
  UINTN       I2CIoPortBaseAddress;
  UINTN       Index;
  UINTN       Addr;
  UINT32      Data;
  UINT8       PollCount;
  EFI_STATUS  Status;

  if (WriteLength > I2C_FIFO_SIZE || ReadLength > I2C_FIFO_SIZE) {
    return EFI_UNSUPPORTED;  // Routine does not handle xfers > fifo size.
  }

  I2CIoPortBaseAddress = GetI2CIoPortBaseAddress ();

  //
  // Write to the IC_TAR register the address of the slave device to be addressed
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_TAR;
  Data = *((volatile UINT32 *) (UINTN)(Addr));
  Data &= ~B_I2C_REG_TAR;
  Data |= I2CAddress;
  *((volatile UINT32 *) (UINTN)(Addr)) = Data;

  //
  // Enable the I2C Controller
  //
  EnableI2CController ();

  //
  // Write the data (sub-addresses) to the IC_DATA_CMD register.
  //
  Addr = I2CIoPortBaseAddress + I2C_REG_DATA_CMD;
  for (Index = 0; Index < WriteLength; Index++) {
    Data = *((volatile UINT32 *) (UINTN)(Addr));
    Data &= 0xFFFFFF00;
    Data |= (UINT8)Buffer[Index];
    Data &= ~B_I2C_REG_DATA_CMD_RW;
    *((volatile UINT32 *) (UINTN)(Addr)) = Data;
  }

  //
  // Issue Read Transfers for each byte (Restart issued when write/read bit changed).
  //
  for (Index = 0; Index < ReadLength; Index++) {
    Data = *((volatile UINT32 *) (UINTN)(Addr));
    Data |= B_I2C_REG_DATA_CMD_RW;
    // Issue a STOP for last read transfer.
    if (Index == (ReadLength-1)) {
      Data |= B_I2C_REG_DATA_CMD_STOP;
    }
    *((volatile UINT32 *) (UINTN)(Addr)) = Data;
  }

  //
  // Wait for STOP condition.
  //
  Status = WaitForStopDet ();
  if (!EFI_ERROR(Status)) {

    //
    // Poll Receive FIFO Buffer Level register until valid (upto MAX_T_POLL_COUNT times).
    //
    Data = 0;
    PollCount = 0;
    Addr = I2CIoPortBaseAddress + I2C_REG_RXFLR;
    Data = *((volatile UINT32 *) (UINTN)(Addr));
    while ((Data != ReadLength) && (PollCount < MAX_T_POLL_COUNT)) {
      MicroSecondDelay(TI2C_POLL);
      PollCount++;
      Data = *((volatile UINT32 *) (UINTN)(Addr));
    }

    Addr = I2CIoPortBaseAddress + I2C_REG_RAW_INTR_STAT;
    Data = *((volatile UINT32 *) (UINTN)(Addr));

    //
    // If no timeout or device error then read rx data.
    //
    if (PollCount == MAX_T_POLL_COUNT) {
      Status = EFI_TIMEOUT;
    } else if ((Data & I2C_REG_RAW_INTR_STAT_RX_OVER) != 0) {
      Status = EFI_DEVICE_ERROR;
    } else {

      //
      // Clear RX underflow before reading IC_DATA_CMD.
      //
      Addr = I2CIoPortBaseAddress + I2C_REG_CLR_RX_UNDER;
      Data = *((volatile UINT32 *) (UINTN)(Addr));

      //
      // Read data.
      //
      Addr = I2CIoPortBaseAddress + I2C_REG_DATA_CMD;
      for (Index = 0; Index < ReadLength; Index++) {
        Data = *((volatile UINT32 *) (UINTN)(Addr));
        Data &= 0x000000FF;
        *(Buffer+Index) = (UINT8)Data;
      }
      Addr = I2CIoPortBaseAddress + I2C_REG_RAW_INTR_STAT;
      Data = *((volatile UINT32 *) (UINTN)(Addr));
      Data &= I2C_REG_RAW_INTR_STAT_RX_UNDER;
      if (Data != 0) {
        Status = EFI_DEVICE_ERROR;
      } else {
        Status = EFI_SUCCESS;
      }
    }
  }

  //
  // Ensure I2C Controller disabled.
  //
  DisableI2CController ();

  return Status;
}

/**

  The I2cWriteByte() function is a wrapper function for the WriteByte function.
  Provides a standard way to execute a standard single byte write to an IC2 device
  (without accessing sub-addresses), as defined in the I2C Specification.

  @param SlaveAddress The I2C slave address of the device
                      with which to communicate.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @param Buffer       Contains the value of byte data to execute to the
                      I2C slave device.


  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_INVALID_PARAMETER  This or Buffer pointers are invalid.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Controller aborted xfer.
  @retval EFI_DEVICE_ERROR      Device error detected by controller.

**/
EFI_STATUS
EFIAPI
I2cWriteByte (
  IN        EFI_I2C_DEVICE_ADDRESS  SlaveAddress,
  IN        EFI_I2C_ADDR_MODE       AddrMode,
  IN OUT VOID                       *Buffer
  )
{
  EFI_STATUS Status;
  UINTN      I2CAddress;
  UINT16            SaveCmd;
  UINT32            SaveBar0;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  SaveCmd = 0;
  SaveBar0 = 0;

  I2cCommonServiceEntry (&SaveCmd, &SaveBar0);

  Status = EFI_SUCCESS;

  I2CAddress = SlaveAddress.I2CDeviceAddress;
  Status = InitializeInternal (AddrMode);
  if (!EFI_ERROR(Status)) {
    Status = WriteByte (I2CAddress, *(UINT8 *) Buffer);
  }

  I2cCommonServiceExit (SaveCmd, SaveBar0);
  return Status;
}

/**

  The I2cReadByte() function is a wrapper function for the ReadByte function.
  Provides a standard way to execute a standard single byte read to an I2C device
  (without accessing sub-addresses), as defined in the I2C Specification.

  @param SlaveAddress The I2C slave address of the device
                      with which to communicate.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @param Buffer       Contains the value of byte data read from the
                      I2C slave device.


  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_INVALID_PARAMETER This or Buffer pointers are invalid.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Controller aborted xfer.
  @retval EFI_DEVICE_ERROR      Device error detected by controller.


**/
EFI_STATUS
EFIAPI
I2cReadByte (
  IN        EFI_I2C_DEVICE_ADDRESS  SlaveAddress,
  IN        EFI_I2C_ADDR_MODE       AddrMode,
  IN OUT    VOID                    *Buffer
  )
{
  EFI_STATUS Status;
  UINTN      I2CAddress;
  UINT16     SaveCmd;
  UINT32     SaveBar0;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  SaveCmd = 0;
  SaveBar0 =0;

  I2cCommonServiceEntry (&SaveCmd, &SaveBar0);

  Status = EFI_SUCCESS;

  I2CAddress = SlaveAddress.I2CDeviceAddress;

  Status = InitializeInternal (AddrMode);
  if (!EFI_ERROR(Status)) {
    Status = ReadByte (I2CAddress, (UINT8 *) Buffer);
  }
  I2cCommonServiceExit (SaveCmd, SaveBar0);
  return Status;
}

/**

  The I2cWriteMultipleByte() function is a wrapper function for the
  WriteMultipleByte() function. Provides a standard way to execute multiple
  byte writes to an I2C device (e.g. when accessing sub-addresses or writing
  block of data), as defined in the I2C Specification.

  @param SlaveAddress The I2C slave address of the device
                      with which to communicate.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @param Length       No. of bytes to be written.

  @param Buffer       Contains the value of byte to be written to the
                      I2C slave device.

  @retval EFI_SUCCESS            Transfer success.
  @retval EFI_INVALID_PARAMETER  This, Length or Buffer pointers are invalid.
  @retval EFI_UNSUPPORTED        Unsupported input param.
  @retval EFI_TIMEOUT            Timeout while waiting xfer.
  @retval EFI_ABORTED            Controller aborted xfer.
  @retval EFI_DEVICE_ERROR       Device error detected by controller.

**/
EFI_STATUS
EFIAPI
I2cWriteMultipleByte (
  IN        EFI_I2C_DEVICE_ADDRESS  SlaveAddress,
  IN        EFI_I2C_ADDR_MODE       AddrMode,
  IN UINTN                          *Length,
  IN OUT    VOID                    *Buffer
  )
{
  EFI_STATUS Status;
  UINTN      I2CAddress;
  UINT16     SaveCmd;
  UINT32     SaveBar0;

    if (Buffer == NULL || Length == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  SaveCmd = 0;
  SaveBar0 =0;

  I2cCommonServiceEntry (&SaveCmd, &SaveBar0);
  Status = EFI_SUCCESS;

  I2CAddress = SlaveAddress.I2CDeviceAddress;

  Status = InitializeInternal (AddrMode);
  if (!EFI_ERROR(Status)) {
    Status = WriteMultipleByte (I2CAddress, Buffer, (*Length));
  }

  I2cCommonServiceExit (SaveCmd, SaveBar0);
  return Status;
}

/**

  The I2cReadMultipleByte() function is a wrapper function for the ReadMultipleByte() function.
  Provides a standard way to execute multiple byte writes to an I2C device
  (e.g. when accessing sub-addresses or when reading block of data), as defined
  in the I2C Specification (I2C combined write/read protocol).

  @param SlaveAddress The I2C slave address of the device
                      with which to communicate.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @param WriteLength  No. of bytes to be written. In this case data
                      written typically contains sub-address or sub-addresses
                      in Hi-Lo format, that need to be read (I2C combined
                      write/read protocol).

  @param ReadLength   No. of bytes to be read from I2C slave device.

  @param Buffer       Contains the value of byte data read from the
                      I2C slave device.

  @retval EFI_SUCCESS            Transfer success.
  @retval EFI_INVALID_PARAMETER  This, WriteLength, ReadLength or Buffer
                                 pointers are invalid.
  @retval EFI_UNSUPPORTED        Unsupported input param.
  @retval EFI_TIMEOUT            Timeout while waiting xfer.
  @retval EFI_ABORTED            Controller aborted xfer.
  @retval EFI_DEVICE_ERROR       Device error detected by controller.

**/
EFI_STATUS
EFIAPI
I2cReadMultipleByte (
  IN        EFI_I2C_DEVICE_ADDRESS  SlaveAddress,
  IN        EFI_I2C_ADDR_MODE       AddrMode,
  IN UINTN                          *WriteLength,
  IN UINTN                          *ReadLength,
  IN OUT    VOID                    *Buffer
  )
{
  EFI_STATUS        Status;
  UINTN             I2CAddress;
  UINT16            SaveCmd;
  UINT32            SaveBar0;

  if (Buffer == NULL || WriteLength == NULL || ReadLength == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  SaveCmd = 0;
  SaveBar0 =0;

  I2cCommonServiceEntry (&SaveCmd, &SaveBar0);

  Status = EFI_SUCCESS;

  I2CAddress = SlaveAddress.I2CDeviceAddress;
  Status = InitializeInternal (AddrMode);
  if (!EFI_ERROR(Status)) {
    Status = ReadMultipleByte (I2CAddress, Buffer, (*WriteLength), (*ReadLength));
  }
  I2cCommonServiceExit (SaveCmd, SaveBar0);
  return Status;
}


