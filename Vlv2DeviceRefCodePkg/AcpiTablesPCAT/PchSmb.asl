/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved   *;
;
; This program and the accompanying materials are licensed and made available under
; the terms and conditions of the BSD License that accompanies this distribution.
; The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;*                                                                        *;
;*                                                                        *;
;**************************************************************************/


// Define various SMBus PCI Configuration Space Registers.

OperationRegion(SMBP,PCI_Config,0x40,0xC0)
Field(SMBP,DWordAcc,NoLock,Preserve)
{
  ,     2,
  I2CE, 1
}

// SMBus Send Byte - This function will write a single byte of
// data to a specific Slave Device per SMBus Send Byte Protocol.
//      Arg0 = Address
//      Arg1 = Data
//      Return: Success = 1
//              Failure = 0

      Method(SSXB,2,Serialized)
{
  OperationRegion(SMPB,PCI_Config,0x20,4)
  Field(SMPB,DWordAcc,NoLock,Preserve)
  {
    ,     5,
    SBAR, 11
  }

  // Define various SMBus IO Mapped Registers.

  OperationRegion(SMBI,SystemIO,ShiftLeft(SBAR,5),0x10)
  Field(SMBI,ByteAcc,NoLock,Preserve)
  {
    HSTS, 8,        // 0 - Host Status Register
    Offset(0x02),
    HCON, 8,        // 2 - Host Control
    HCOM, 8,        // 3 - Host Command
    TXSA, 8,        // 4 - Transmit Slave Address
    DAT0, 8,        // 5 - Host Data 0
    DAT1, 8,        // 6 - Host Data 1
    HBDR, 8,        // 7 - Host Block Data
    PECR, 8,        // 8 - Packer Error Check
    RXSA, 8,        // 9 - Receive Slave Address
    SDAT, 16,       // A - Slave Data
  }

  // Step 1:  Confirm the ICHx SMBus is ready to perform
  // communication.

  If(STRT())
  {
    Return(0)
  }

  // Step 2:  Initiate a Send Byte.

  Store(0,I2CE)                           // Ensure SMbus Mode.
  Store(0xBF,HSTS)                        // Clear all but INUSE_STS.
  Store(Arg0,TXSA)                        // Write Address in TXSA.
  Store(Arg1,HCOM)                        // Data in HCOM.

  // Set the SMBus Host control register to 0x48.
  //   Bit 7:    =  0  = reserved
  //   Bit 6:    =  1  = start
  //   Bit 5:    =  0  = disregard, I2C related bit
  //   Bits 4:2: = 001 = Byte Protocol
  //   Bit 1:    =  0  = Normal Function
  //   Bit 0:    =  0  = Disable interrupt generation

  Store(0x48,HCON)

  // Step 3:  Exit the Method correctly.

  If(COMP)
  {
    Or(HSTS,0xFF,HSTS)              // Clear INUSE_STS and others..
    Return(1)                       // Return Success.
  }

  Return(0)
}

// SMBus Receive Byte - This function will write a single byte
// of data to a specific Slave Device per SMBus Receive Byte
// Protocol.
//      Arg0 = Address
//      Return: Success = Byte-Size Value
//              Failure = Word-Size Value = FFFFh.

Method(SRXB,1,Serialized)
{
  OperationRegion(SMPB,PCI_Config,0x20,4)
  Field(SMPB,DWordAcc,NoLock,Preserve)
  {
    ,     5,
    SBAR, 11
  }

  // Define various SMBus IO Mapped Registers.

  OperationRegion(SMBI,SystemIO,ShiftLeft(SBAR,5),0x10)
  Field(SMBI,ByteAcc,NoLock,Preserve)
  {
    HSTS, 8,        // 0 - Host Status Register
    Offset(0x02),
    HCON, 8,        // 2 - Host Control
    HCOM, 8,        // 3 - Host Command
    TXSA, 8,        // 4 - Transmit Slave Address
    DAT0, 8,        // 5 - Host Data 0
    DAT1, 8,        // 6 - Host Data 1
    HBDR, 8,        // 7 - Host Block Data
    PECR, 8,        // 8 - Packer Error Check
    RXSA, 8,        // 9 - Receive Slave Address
    SDAT, 16,       // A - Slave Data
  }
  // Step 1:  Confirm the ICHx SMBus is ready to perform
  // communication.

  If(STRT())
  {
    Return(0xFFFF)
  }

  // Step 2:  Initiate a Receive Byte.

  Store(0,I2CE)                           // Ensure SMbus Mode.
  Store(0xBF,HSTS)                        // Clear all but INUSE_STS.
  Store(Or(Arg0,1),TXSA)                  // Read Address in TXSA.

  // Set the SMBus Host control register to 0x48.
  //   Bit 7:    =  0  = reserved
  //   Bit 6:    =  1  = start
  //   Bit 5:    =  0  = disregard, I2C related bit
  //   Bits 4:2: = 001 = Byte Protocol
  //   Bit 1:    =  0  = Normal Function
  //   Bit 0:    =  0  = Disable interrupt generation

  Store(0x44,HCON)

  // Step 3:  Exit the Method correctly.

  If(COMP)
  {
    Or(HSTS,0xFF,HSTS)              // Clear INUSE_STS and others..
    Return(DAT0)                    // Return Success.
  }

  Return(0xFFFF)                          // Return Failure.
}

// SMBus Write Byte - This function will write a single byte
// of data to a specific Slave Device per SMBus Write Byte
// Protocol.
//      Arg0 = Address
//      Arg1 = Command
//      Arg2 = Data
//      Return: Success = 1
//              Failure = 0

Method(SWRB,3,Serialized)
{
  OperationRegion(SMPB,PCI_Config,0x20,4)
  Field(SMPB,DWordAcc,NoLock,Preserve)
  {
    ,     5,
    SBAR, 11
  }

  // Define various SMBus IO Mapped Registers.

  OperationRegion(SMBI,SystemIO,ShiftLeft(SBAR,5),0x10)
  Field(SMBI,ByteAcc,NoLock,Preserve)
  {
    HSTS, 8,        // 0 - Host Status Register
    Offset(0x02),
    HCON, 8,        // 2 - Host Control
    HCOM, 8,        // 3 - Host Command
    TXSA, 8,        // 4 - Transmit Slave Address
    DAT0, 8,        // 5 - Host Data 0
    DAT1, 8,        // 6 - Host Data 1
    HBDR, 8,        // 7 - Host Block Data
    PECR, 8,        // 8 - Packer Error Check
    RXSA, 8,        // 9 - Receive Slave Address
    SDAT, 16,       // A - Slave Data
  }
  // Step 1:  Confirm the ICHx SMBus is ready to perform communication.

  If(STRT())
  {
    Return(0)
  }

  // Step 2:  Initiate a Write Byte.

  Store(0,I2CE)                           // Ensure SMbus Mode.
  Store(0xBF,HSTS)                        // Clear all but INUSE_STS.
  Store(Arg0,TXSA)                        // Write Address in TXSA.
  Store(Arg1,HCOM)                        // Command in HCOM.
  Store(Arg2,DAT0)                        // Data in DAT0.

  // Set the SMBus Host control register to 0x48.
  //   Bit 7:    =  0  = reserved
  //   Bit 6:    =  1  = start
  //   Bit 5:    =  0  = disregard, I2C related bit
  //   Bits 4:2: = 010 = Byte Data Protocol
  //   Bit 1:    =  0  = Normal Function
  //   Bit 0:    =  0  = Disable interrupt generation

  Store(0x48,HCON)

  // Step 3:  Exit the Method correctly.

  If(COMP)
  {
    Or(HSTS,0xFF,HSTS)              // Clear INUSE_STS and others..
    Return(1)                       // Return Success.
  }

  Return(0)                               // Return Failure.
}

// SMBus Read Byte - This function will read a single byte of data
// from a specific slave device per SMBus Read Byte Protocol.
//      Arg0 = Address
//      Arg1 = Command
//      Return: Success = Byte-Size Value
//              Failure = Word-Size Value

Method(SRDB,2,Serialized)
{
  OperationRegion(SMPB,PCI_Config,0x20,4)
  Field(SMPB,DWordAcc,NoLock,Preserve)
  {
    ,     5,
    SBAR, 11
  }

  // Define various SMBus IO Mapped Registers.

  OperationRegion(SMBI,SystemIO,ShiftLeft(SBAR,5),0x10)
  Field(SMBI,ByteAcc,NoLock,Preserve)
  {
    HSTS, 8,        // 0 - Host Status Register
    Offset(0x02),
    HCON, 8,        // 2 - Host Control
    HCOM, 8,        // 3 - Host Command
    TXSA, 8,        // 4 - Transmit Slave Address
    DAT0, 8,        // 5 - Host Data 0
    DAT1, 8,        // 6 - Host Data 1
    HBDR, 8,        // 7 - Host Block Data
    PECR, 8,        // 8 - Packer Error Check
    RXSA, 8,        // 9 - Receive Slave Address
    SDAT, 16,       // A - Slave Data
  }
  // Step 1:  Confirm the ICHx SMBus is ready to perform communication.

  If(STRT())
  {
    Return(0xFFFF)
  }

  // Step 2:  Initiate a Read Byte.

  Store(0,I2CE)                           // Ensure SMbus Mode.
  Store(0xBF,HSTS)                        // Clear all but INUSE_STS.
  Store(Or(Arg0,1),TXSA)                  // Read Address in TXSA.
  Store(Arg1,HCOM)                        // Command in HCOM.

  // Set the SMBus Host control register to 0x48.
  //   Bit 7:    =  0  = reserved
  //   Bit 6:    =  1  = start
  //   Bit 5:    =  0  = disregard, I2C related bit
  //   Bits 4:2: = 010 = Byte Data Protocol
  //   Bit 1:    =  0  = Normal Function
  //   Bit 0:    =  0  = Disable interrupt generation

  Store(0x48,HCON)

  // Step 3:  Exit the Method correctly.

  If(COMP)
  {
    Or(HSTS,0xFF,HSTS)              // Clear INUSE_STS and others..
    Return(DAT0)                    // Return Success.
  }

  Return(0xFFFF)                          // Return Failure.
}

// SMBus Write Word - This function will write a single word
// of data to a specific Slave Device per SMBus Write Word
// Protocol.
//      Arg0 = Address
//      Arg1 = Command
//      Arg2 = Data (16 bits in size)
//      Return: Success = 1
//              Failure = 0

Method(SWRW,3,Serialized)
{
  OperationRegion(SMPB,PCI_Config,0x20,4)
  Field(SMPB,DWordAcc,NoLock,Preserve)
  {
    ,     5,
    SBAR, 11
  }

  // Define various SMBus IO Mapped Registers.

  OperationRegion(SMBI,SystemIO,ShiftLeft(SBAR,5),0x10)
  Field(SMBI,ByteAcc,NoLock,Preserve)
  {
    HSTS, 8,        // 0 - Host Status Register
    Offset(0x02),
	HCON, 8,        // 2 - Host Control
	HCOM, 8,        // 3 - Host Command
	TXSA, 8,        // 4 - Transmit Slave Address
	DAT0, 8,        // 5 - Host Data 0
	DAT1, 8,        // 6 - Host Data 1
	HBDR, 8,        // 7 - Host Block Data
	PECR, 8,        // 8 - Packer Error Check
	RXSA, 8,        // 9 - Receive Slave Address
	SDAT, 16,       // A - Slave Data
  }
  // Step 1:  Confirm the ICHx SMBus is ready to perform communication.

  If(STRT())
  {
    Return(0)
  }

  // Step 2:  Initiate a Write Word.

  Store(0,I2CE)                           // Ensure SMbus Mode.
  Store(0xBF,HSTS)                        // Clear all but INUSE_STS.
  Store(Arg0,TXSA)                        // Write Address in TXSA.
  Store(Arg1,HCOM)                        // Command in HCOM.
  And(Arg2,0xFF,DAT1)                     // Low byte Data in DAT1.
  And(ShiftRight(Arg2,8),0xFF,DAT0)       // High byte Data in DAT0.

  // Set the SMBus Host control register to 0x4C.
  //   Bit 7:    =  0  = reserved
  //   Bit 6:    =  1  = start
  //   Bit 5:    =  0  = disregard, I2C related bit
  //   Bits 4:2: = 011 = Word Data Protocol
  //   Bit 1:    =  0  = Normal Function
  //   Bit 0:    =  0  = Disable interrupt generation

  Store(0x4C,HCON)

  // Step 3:  Exit the Method correctly.

  If(COMP())
  {
    Or(HSTS,0xFF,HSTS)              // Clear INUSE_STS and others.
    Return(1)                       // Return Success.
  }

  Return(0)                               // Return Failure.
}

// SMBus Read Word - This function will read a single byte of data
// from a specific slave device per SMBus Read Word Protocol.
//      Arg0 = Address
//      Arg1 = Command
//      Return: Success = Word-Size Value
//              Failure = Dword-Size Value

Method(SRDW,2,Serialized)
{
  OperationRegion(SMPB,PCI_Config,0x20,4)
  Field(SMPB,DWordAcc,NoLock,Preserve)
  {
    ,     5,
    SBAR, 11
  }

  // Define various SMBus IO Mapped Registers.

  OperationRegion(SMBI,SystemIO,ShiftLeft(SBAR,5),0x10)
  Field(SMBI,ByteAcc,NoLock,Preserve)
  {
    HSTS, 8,        // 0 - Host Status Register
	Offset(0x02),
	HCON, 8,        // 2 - Host Control
	HCOM, 8,        // 3 - Host Command
	TXSA, 8,        // 4 - Transmit Slave Address
	DAT0, 8,        // 5 - Host Data 0
	DAT1, 8,        // 6 - Host Data 1
	HBDR, 8,        // 7 - Host Block Data
	PECR, 8,        // 8 - Packer Error Check
	RXSA, 8,        // 9 - Receive Slave Address
	SDAT, 16,       // A - Slave Data
  }
  // Step 1:  Confirm the ICHx SMBus is ready to perform communication.

  If(STRT())
  {
    Return(0xFFFF)
  }

  // Step 2:  Initiate a Read Word.

  Store(0,I2CE)                           // Ensure SMbus Mode.
  Store(0xBF,HSTS)                        // Clear all but INUSE_STS.
  Store(Or(Arg0,1),TXSA)                  // Read Address in TXSA.
  Store(Arg1,HCOM)                        // Command in HCOM.

  // Set the SMBus Host control register to 0x4C.
  //   Bit 7:    =  0  = reserved
  //   Bit 6:    =  1  = start
  //   Bit 5:    =  0  = disregard, I2C related bit
  //   Bits 4:2: = 011 = Word Data Protocol
  //   Bit 1:    =  0  = Normal Function
  //   Bit 0:    =  0  = Disable interrupt generation

  Store(0x4C,HCON)

  // Step 3:  Exit the Method correctly.

  If(COMP())
  {
    Or(HSTS,0xFF,HSTS)                      // Clear INUSE_STS and others.
    Return(Or(ShiftLeft(DAT0,8),DAT1))      // Return Success.
  }

  Return(0xFFFFFFFF)                      // Return Failure.
}

// SMBus Block Write - This function will write an entire block of data
// to a specific slave device per SMBus Block Write Protocol.
//      Arg0 = Address
//      Arg1 = Command
//      Arg2 = Buffer of Data to Write
//      Arg3 = 1 = I2C Block Write, 0 = SMBus Block Write
//      Return: Success = 1
//              Failure = 0

Method(SBLW,4,Serialized)
{
  OperationRegion(SMPB,PCI_Config,0x20,4)
  Field(SMPB,DWordAcc,NoLock,Preserve)
  {
    ,     5,
    SBAR, 11
  }

  // Define various SMBus IO Mapped Registers.

  OperationRegion(SMBI,SystemIO,ShiftLeft(SBAR,5),0x10)
  Field(SMBI,ByteAcc,NoLock,Preserve)
  {
    HSTS, 8,        // 0 - Host Status Register
	Offset(0x02),
	HCON, 8,        // 2 - Host Control
	HCOM, 8,        // 3 - Host Command
	TXSA, 8,        // 4 - Transmit Slave Address
	DAT0, 8,        // 5 - Host Data 0
	DAT1, 8,        // 6 - Host Data 1
	HBDR, 8,        // 7 - Host Block Data
	PECR, 8,        // 8 - Packer Error Check
	RXSA, 8,        // 9 - Receive Slave Address
	SDAT, 16,       // A - Slave Data
  }
  // Step 1:  Confirm the ICHx SMBus is ready to perform communication.

  If(STRT())
  {
    Return(0)
  }

  // Step 2:  Initiate a Block Write.

  Store(Arg3,I2CE)                        // Select the proper protocol.
  Store(0xBF,HSTS)                        // Clear all but INUSE_STS.
  Store(Arg0,TXSA)                        // Write Address in TXSA.
  Store(Arg1,HCOM)                        // Command in HCOM.
  Store(Sizeof(Arg2),DAT0)                // Count in DAT0.
  Store(0,Local1)                         // Init Pointer to Buffer.
  Store(DerefOf(Index(Arg2,0)),HBDR)      // First Byte in HBD Register.

  // Set the SMBus Host control register to 0x48.
  //   Bit 7:    =  0  = reserved
  //   Bit 6:    =  1  = start
  //   Bit 5:    =  0  = disregard, I2C related bit
  //   Bits 4:2: = 101 = Block Protocol
  //   Bit 1:    =  0  = Normal Function
  //   Bit 0:    =  0  = Disable interrupt generation

  Store(0x54,HCON)

  // Step 3:  Send the entire Block of Data.

  While(LGreater(Sizeof(Arg2),Local1))
  {
    // Wait up to 200ms for Host Status to get set.

    Store(4000,Local0)              // 4000 * 50us = 200ms.

    While(LAnd(LNot(And(HSTS,0x80)),Local0))
    {
      Decrement(Local0)       // Decrement Count.
      Stall(50)               // Delay = 50us.
    }

    If(LNot(Local0))                // Timeout?
    {
      KILL()                  // Yes.  Kill Communication.
      Return(0)               // Return failure.
    }

    Store(0x80,HSTS)                // Clear Host Status.
    Increment(Local1)               // Point to Next Byte.

    // Place next byte in HBDR if last byte has not been sent.

    If(LGreater(Sizeof(Arg2),Local1))
    {
      Store(DerefOf(Index(Arg2,Local1)),HBDR)
    }
  }

  // Step 4:  Exit the Method correctly.

  If(COMP())
  {
    Or(HSTS,0xFF,HSTS)              // Clear all status bits.
    Return(1)                       // Return Success.
  }

  Return(0)                               // Return Failure.
}

// SMBus Block Read - This function will read a block of data from
// a specific slave device per SMBus Block Read Protocol.
//      Arg0 = Address
//      Arg1 = Command
//      Arg2 = 1 = I2C Block Write, 0 = SMBus Block Write
//      Return: Success = Data Buffer (First Byte = length)
//              Failure = 0

Method(SBLR,3,Serialized)
{
  OperationRegion(SMPB,PCI_Config,0x20,4)
  Field(SMPB,DWordAcc,NoLock,Preserve)
  {
    ,     5,
    SBAR, 11
  }

  // Define various SMBus IO Mapped Registers.

  OperationRegion(SMBI,SystemIO,ShiftLeft(SBAR,5),0x10)
  Field(SMBI,ByteAcc,NoLock,Preserve)
  {
    HSTS, 8,        // 0 - Host Status Register
	Offset(0x02),
	HCON, 8,        // 2 - Host Control
	HCOM, 8,        // 3 - Host Command
	TXSA, 8,        // 4 - Transmit Slave Address
	DAT0, 8,        // 5 - Host Data 0
	DAT1, 8,        // 6 - Host Data 1
	HBDR, 8,        // 7 - Host Block Data
	PECR, 8,        // 8 - Packer Error Check
	RXSA, 8,        // 9 - Receive Slave Address
	SDAT, 16,       // A - Slave Data
  }
  Name(TBUF, Buffer(256) {})

  // Step 1:  Confirm the ICHx SMBus is ready to perform communication.

  If(STRT())
  {
    Return(0)
  }

  // Step 2:  Initiate a Block Read.

  Store(Arg2,I2CE)                        // Select the proper protocol.
  Store(0xBF,HSTS)                        // Clear all but INUSE_STS.
  Store(Or(Arg0,1),TXSA)                  // Read Address in TXSA.
  Store(Arg1,HCOM)                        // Command in HCOM.

  // Set the SMBus Host control register to 0x48.
  //   Bit 7:    =  0  = reserved
  //   Bit 6:    =  1  = start
  //   Bit 5:    =  0  = disregard, I2C related bit
  //   Bits 4:2: = 101 = Block Protocol
  //   Bit 1:    =  0  = Normal Function
  //   Bit 0:    =  0  = Disable interrupt generation

  Store(0x54,HCON)

  // Step 3:  Wait up to 200ms to get the Data Count.

  Store(4000,Local0)                      // 4000 * 50us = 200ms.

  While(LAnd(LNot(And(HSTS,0x80)),Local0))
  {
    Decrement(Local0)               // Decrement Count.
    Stall(50)                       // Delay = 50us.
  }

  If(LNot(Local0))                        // Timeout?
  {
    KILL()                          // Yes.  Kill Communication.
    Return(0)                       // Return failure.
  }

  Store(DAT0,Index(TBUF,0))               // Get the Data Count.
  Store(0x80,HSTS)                        // Clear Host Status.
  Store(1,Local1)                         // Local1 = Buffer Pointer.

  // Step 4:  Get the Block Data and store it.

  While(LLess(Local1,DerefOf(Index(TBUF,0))))
  {
    // Wait up to 200ms for Host Status to get set.

    Store(4000,Local0)              // 4000 * 50us = 200ms.

    While(LAnd(LNot(And(HSTS,0x80)),Local0))
    {
      Decrement(Local0)       // Decrement Count.
      Stall(50)               // Delay = 50us.
    }

    If(LNot(Local0))                // Timeout?
    {
      KILL()                  // Yes.  Kill Communication.
      Return(0)               // Return failure.
    }

    Store(HBDR,Index(TBUF,Local1))  // Place into Buffer.
    Store(0x80,HSTS)                // Clear Host Status.
    Increment(Local1)
  }

  // Step 5:  Exit the Method correctly.

  If(COMP())
  {
    Or(HSTS,0xFF,HSTS)              // Clear INUSE_STS and others.
    Return(TBUF)                    // Return Success.
  }

  Return(0)                               // Return Failure.
}


// SMBus Start Check
//      Return: Success = 0
//              Failure = 1

Method(STRT,0,Serialized)
{
  OperationRegion(SMPB,PCI_Config,0x20,4)
  Field(SMPB,DWordAcc,NoLock,Preserve)
  {
    ,     5,
    SBAR, 11
  }

  // Define various SMBus IO Mapped Registers.

  OperationRegion(SMBI,SystemIO,ShiftLeft(SBAR,5),0x10)
  Field(SMBI,ByteAcc,NoLock,Preserve)
  {
    HSTS, 8,        // 0 - Host Status Register
	Offset(0x02),
	HCON, 8,        // 2 - Host Control
	HCOM, 8,        // 3 - Host Command
	TXSA, 8,        // 4 - Transmit Slave Address
	DAT0, 8,        // 5 - Host Data 0
	DAT1, 8,        // 6 - Host Data 1
	HBDR, 8,        // 7 - Host Block Data
	PECR, 8,        // 8 - Packer Error Check
	RXSA, 8,        // 9 - Receive Slave Address
	SDAT, 16,       // A - Slave Data
  }
  // Wait up to 200ms to confirm the SMBus Semaphore has been
  // released (In Use Status = 0).  Note that the Sleep time may take
  // longer as the This function will yield the Processor such that it
  // may perform different tasks during the delay.

  Store(200,Local0)                       // 200 * 1ms = 200ms.

  While(Local0)
  {
    If(And(HSTS,0x40))              // In Use Set?
    {
      Decrement(Local0)       // Yes.  Decrement Count.
      Sleep(1)                // Delay = 1ms.
      If(LEqual(Local0,0))    // Count = 0?
      {
        Return(1)       // Return failure.
      }
    }
    Else
    {
      Store(0,Local0)         // In Use Clear.  Continue.
    }
  }

  // In Use Status = 0 during last read, which will make subsequent
  // reads return In Use Status = 1 until software clears it.  All
  // software using ICHx SMBus should check this bit before initiating
  // any SMBus communication.

  // Wait up to 200ms to confirm the Host Interface is
  // not processing a command.

  Store(4000,Local0)                      // 4000 * 50us = 200ms.

  While(Local0)
  {
    If(And(HSTS,0x01))              // Host Busy Set?
    {
      Decrement(Local0)       // Decrement Count.
      Stall(50)               // Delay = 50us.
      If(LEqual(Local0,0))    // Count = 0?
      {
        KILL()          // Yes.  Kill Communication.
      }
    }
    Else
    {
      Return(0)
    }
  }

  Return(1)                               // Timeout.  Return failure.
}

// SMBus Completion Check
//      Return: Success = 1
//              Failure = 0

Method(COMP,0,Serialized)
{
  OperationRegion(SMPB,PCI_Config,0x20,4)
  Field(SMPB,DWordAcc,NoLock,Preserve)
  {
    ,     5,
    SBAR, 11
  }

  // Define various SMBus IO Mapped Registers.

  OperationRegion(SMBI,SystemIO,ShiftLeft(SBAR,5),0x10)
  Field(SMBI,ByteAcc,NoLock,Preserve)
  {
    HSTS, 8,        // 0 - Host Status Register
	Offset(0x02),
	HCON, 8,        // 2 - Host Control
	HCOM, 8,        // 3 - Host Command
	TXSA, 8,        // 4 - Transmit Slave Address
	DAT0, 8,        // 5 - Host Data 0
	DAT1, 8,        // 6 - Host Data 1
	HBDR, 8,        // 7 - Host Block Data
	PECR, 8,        // 8 - Packer Error Check
	RXSA, 8,        // 9 - Receive Slave Address
	SDAT, 16,       // A - Slave Data
  }
  // Wait for up to 200ms for the Completion Command
  // Status to get set.

  Store(4000,Local0)                      // 4000 * 50us = 200ms.

  While(Local0)
  {
    If(And(HSTS,0x02))              // Completion Status Set?
    {
      Return(1)               // Yes.  We are done.
    }
    Else
    {
      Decrement(Local0)       // Decrement Count.
      Stall(50)               // Delay 50us.
      If(LEqual(Local0,0))    // Count = 0?
      {
        KILL()          // Yes.  Kill Communication.
      }
    }
  }

  Return(0)                               // Timeout.  Return Failure.
}

// SMBus Kill Command

Method(KILL,0,Serialized)
{
  OperationRegion(SMPB,PCI_Config,0x20,4)
  Field(SMPB,DWordAcc,NoLock,Preserve)
  {
    ,     5,
    SBAR, 11
  }

  // Define various SMBus IO Mapped Registers.

  OperationRegion(SMBI,SystemIO,ShiftLeft(SBAR,5),0x10)
  Field(SMBI,ByteAcc,NoLock,Preserve)
  {
    HSTS, 8,        // 0 - Host Status Register
	Offset(0x02),
	HCON, 8,        // 2 - Host Control
	HCOM, 8,        // 3 - Host Command
	TXSA, 8,        // 4 - Transmit Slave Address
	DAT0, 8,        // 5 - Host Data 0
	DAT1, 8,        // 6 - Host Data 1
	HBDR, 8,        // 7 - Host Block Data
	PECR, 8,        // 8 - Packer Error Check
	RXSA, 8,        // 9 - Receive Slave Address
	SDAT, 16,       // A - Slave Data
  }
  Or(HCON,0x02,HCON)                      // Yes.  Send Kill command.
  Or(HSTS,0xFF,HSTS)                      // Clear all status.
}
