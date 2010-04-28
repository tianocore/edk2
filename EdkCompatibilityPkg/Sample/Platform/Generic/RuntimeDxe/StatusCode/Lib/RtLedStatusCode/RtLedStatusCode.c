/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  RtLedStatusCode.c
   
Abstract:

  Lib to provide LED status code reporting Routines.

  In general you should use PPI's, but some times a monolithic driver
  is better. The best justification for monolithic code is debug.

--*/

#include "RtLedStatusCode.h"

//
// Prepare the data to initialize LPC chipset for Server Io Configuration
// This is hardcoded init value and would vary from platform to platform.
//
static SIO_INIT_DATA  mSioInitData[] = {
  //
  // Program magic values in ServerI/O configuration registers
  //
  {
    REG_SERVERIO_CNF1,
    0x19
  },
  {
    REG_SERVERIO_CNF2,
    0x22
  },
  {
    REG_SERVERIO_CNF3,
    0x76
  },
  {
    REG_SERVERIO_CNF4,
    0x26
  },
  //
  // Force the parallel port to be disabled, override reg 30 setting
  //
  {
    REG_SERVERIO_CNF6,
    0x02
  },
  //
  // Select GPIO device and setup GPIO base address
  //
  {
    REG_LOGICAL_DEVICE,
    SIO_GPIO
  },
  {
    ACTIVATE,
    LOGICAL_DEVICE_OFF
  },
  {
    BASE_ADDRESS_HIGH,
    SIO_GPIO_HIGH
  },
  {
    BASE_ADDRESS_LOW,
    SIO_GPIO_LOW
  },
  {
    ACTIVATE,
    LOGICAL_DEVICE_ON
  },
  //
  // Select DLED STB, post code LED, ZZ_POST_CLK_LED_L
  //
  {
    GPIO_GPSEL,
    0x43
  },
  //
  // Push pull output enable
  //
  {
    GPIO_GPCFG1,
    PUSH_PULL | OUTPUT_BUFFER_EN
  },
  //
  // Disable Event IRQ routing
  //
  {
    GPIO_GPEVR,
    GPIO_EVENT_OFF
  },
  //
  // Select DLED STB, ZZ_POST_DATA_LED_L
  //
  {
    GPIO_GPSEL,
    0x54
  },
  //
  // Push pull output enable
  //
  {
    GPIO_GPCFG1,
    PUSH_PULL | OUTPUT_BUFFER_EN
  },
  //
  // Disable Event IRQ routing
  //
  {
    GPIO_GPEVR,
    GPIO_EVENT_OFF
  },
  //
  // Select Select ACPI_MODE_IND_L
  //
  {
    GPIO_GPSEL,
    0x63
  },
  //
  // Push pull output enable
  //
  {
    GPIO_GPCFG1,
    PUSH_PULL | OUTPUT_BUFFER_EN
  },

  {
    0xff,
    0xff
  }
};

VOID
RtLedInitializeStatusCode (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

  Initialization routine. Initializes LPC47817 to configure GPIO for LED

Arguments: 

  None

Returns: 

  None

--*/
{
  UINT8   OutputData;
  UINT16  ConfigPort;
  UINT16  DataPort;
  UINT32  Index;

  //
  // hard code for sio init
  //
  ConfigPort  = CONFIG_PORT0;
  DataPort    = DATA_PORT0;

  //
  // Initialize Sio from table to enable SererIoCfg and GPIO
  //
  Index = 0;
  while ((mSioInitData[Index]).RegAddress != 0xff) {
    OutputData = (UINT8) mSioInitData[Index].RegAddress;

    IoWrite8 (ConfigPort, OutputData);

    OutputData = (UINT8) mSioInitData[Index].RegValue;
    IoWrite8 (DataPort, OutputData);

    Index++;
  }

  return ;
}

BOOLEAN
CodeTypeToProgressCode (
  IN  EFI_STATUS_CODE_TYPE    CodeType,
  IN  EFI_STATUS_CODE_VALUE   Value,
  OUT UINT8                   *PostCode
  )
{
  //
  // Convert Value to an 8 bit post code
  //
  if (((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE)) {
    *PostCode = (UINT8) (((Value & EFI_STATUS_CODE_CLASS_MASK) >> 24) << 5);
    *PostCode = (UINT8) (*PostCode | (UINT8) (((Value & EFI_STATUS_CODE_SUBCLASS_MASK) >> 16) & 0x1f));
    return TRUE;
  }

  return FALSE;
}

VOID
SendDataToPort (
  IN  UINT8   Data,
  IN  UINT16  DataOffset
  )
/*++

Routine Description:

  Writes the data to control LED output at desired port

Arguments: 

  Data        -   Data in bit0 is the relevant data
  DataOffset  -   Port address to access GPIO54

Returns: 

  None

--*/
{
  UINT8 PinData;

  //
  // Read current Pin State of GPIO54
  //
  PinData = IoRead8 (DataOffset);

  //
  // Set GPIO54 pin to zero
  //
  PinData &= 0xEF;

  if (Data & 0x01) {
    //
    // Set GPIO54 pin to 1 if data is 1
    // otherwise it will be set to 0
    //
    PinData |= LED_MASK_BIT;
  }

  IoWrite8 (DataOffset, PinData);
}

VOID
StrobeData (
  IN  UINT16  StrobeOffset
  )
/*++

Routine Description:

  Controls the strobe to move the value from LSB to MSB in steps.

Arguments: 

  StrobeOffset  -   Port address to access GPIO43. This pin controls the shifting
                    of bit value from LSB to MSB

Returns: 

  None

--*/
{
  UINT8 StrobeData;

  StrobeData = IoRead8 (StrobeOffset);

  //
  // Make bit 3 of data to be zero
  //
  StrobeData &= 0xF7;

  IoWrite8 (StrobeOffset, StrobeData);

  //
  // Make bit 3 as 1 to perform the strobe to shift the data in 74HCT164
  //
  StrobeData |= STROBE_MASK_BIT;

  IoWrite8 (StrobeOffset, StrobeData);
}

VOID
SendDataToLed (
  UINT8                   Data
  )
{
  UINT16  GpioBase;
  UINT16  DataOffset;
  UINT16  StrobeOffset;
  UINTN   Index;
  UINTN   DataBitPosition;
  UINT8   TempData;

  GpioBase        = GPIO_BASE (SIO_GPIO_HIGH, SIO_GPIO_LOW);

  DataOffset      = (UINT16) (GpioBase + LED_DATA_OFFSET);

  StrobeOffset    = (UINT16) (GpioBase + LED_STROBE_OFFSET);

  DataBitPosition = 7;

  Data            = (UINT8) (~Data);

  TempData        = Data;

  for (Index = 0; Index < 8; Index++) {
    SendDataToPort ((UINT8) (TempData >> DataBitPosition), DataOffset);
    StrobeData (StrobeOffset);
    DataBitPosition--;
  }
  //
  // To fix 5 Volt leakage problem
  //
  SendDataToPort (0, DataOffset);

}

EFI_STATUS
RtLedReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
/*++

Routine Description:

  Provide a LED status code

Arguments:

  Same as ReportStatusCode PPI
    
Returns:

  Status -  EFI_SUCCESS if the interface could be successfully
            installed

--*/
{
  UINT8 ProgressCode;

  if (CodeTypeToProgressCode (CodeType, Value, &ProgressCode)) {
    SendDataToLed (ProgressCode);
  }

  return EFI_SUCCESS;
}
