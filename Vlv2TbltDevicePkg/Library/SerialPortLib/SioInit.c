/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

    SioInit.c

Abstract:

    Functions for LpcSio initialization

--*/

#include "PlatformSerialPortLib.h"
#include "SioInit.h"

typedef struct {
  UINT8 Register;
  UINT8 Value;
} EFI_SIO_TABLE;

EFI_SIO_TABLE mSioTableWpcn381u[] = {
    {0x29, 0x0A0},
    {WPCN381U_LD_SEL_REGISTER, WPCN381U_LDN_UART0},                                       // Select UART0 device
    {WPCN381U_BASE1_HI_REGISTER, (UINT8)(WPCN381U_SERIAL_PORT0_BASE_ADDRESS >> 8)},       // Set Base Address MSB
    {WPCN381U_BASE1_LO_REGISTER, (UINT8)(WPCN381U_SERIAL_PORT0_BASE_ADDRESS & 0x00FF)},   // Set Base Address LSB
    {WPCN381U_IRQ1_REGISTER, 0x014},                                                      // Set to IRQ4
    {WPCN381U_ACTIVATE_REGISTER, WPCN381U_ACTIVATE_VALUE},                                // Enable it with Activation bit
    {WPCN381U_LD_SEL_REGISTER, WPCN381U_LDN_UART1},                                       // Select UART1 device
    {WPCN381U_BASE1_HI_REGISTER, (UINT8)(WPCN381U_SERIAL_PORT1_BASE_ADDRESS >> 8)},       // Set Base Address MSB
    {WPCN381U_BASE1_LO_REGISTER, (UINT8)(WPCN381U_SERIAL_PORT1_BASE_ADDRESS & 0x00FF)},   // Set Base Address LSB
    {WPCN381U_IRQ1_REGISTER, 0x013},                                                      // Set to IRQ3
    {WPCN381U_ACTIVATE_REGISTER, WPCN381U_ACTIVATE_VALUE},                                // Enable it with Activation bit
    {WPCN381U_LD_SEL_REGISTER, WPCN381U_LDN_GPIO},                                        // Select GPIO device
    {WPCN381U_BASE1_HI_REGISTER, (UINT8)(WPCN381U_GPIO_BASE_ADDRESS >> 8)},               // Set Base Address MSB
    {WPCN381U_BASE1_LO_REGISTER, (UINT8)(WPCN381U_GPIO_BASE_ADDRESS & 0x00FF)},           // Set Base Address LSB
    {WPCN381U_ACTIVATE_REGISTER, WPCN381U_ACTIVATE_VALUE},                                // Enable it with Activation bit
    {0x21, 0x001},                                                                        // Global Device Enable
    {0x26, 0x000}
};

EFI_SIO_TABLE mSioTableWdcp376[] = {
    {0x29, 0x0A0},
    {WPCN381U_LD_SEL_REGISTER, WPCN381U_LDN_UART0},                                       // Select UART0 device
    {WPCN381U_BASE1_HI_REGISTER, (UINT8)(WPCN381U_SERIAL_PORT0_BASE_ADDRESS >> 8)},       // Set Base Address MSB
    {WPCN381U_BASE1_LO_REGISTER, (UINT8)(WPCN381U_SERIAL_PORT0_BASE_ADDRESS & 0x00FF)},   // Set Base Address LSB
    {WPCN381U_IRQ1_REGISTER, 0x014},                                                      // Set to IRQ4
    {WPCN381U_ACTIVATE_REGISTER, WPCN381U_ACTIVATE_VALUE},                                // Enable it with Activation bit
    {WPCN381U_LD_SEL_REGISTER, WPCN381U_LDN_UART1},                                       // Select UART1 device
    {WPCN381U_BASE1_HI_REGISTER, (UINT8)(WPCN381U_SERIAL_PORT1_BASE_ADDRESS >> 8)},       // Set Base Address MSB
    {WPCN381U_BASE1_LO_REGISTER, (UINT8)(WPCN381U_SERIAL_PORT1_BASE_ADDRESS & 0x00FF)},   // Set Base Address LSB
    {WPCN381U_IRQ1_REGISTER, 0x013},                                                      // Set to IRQ3
    {WPCN381U_ACTIVATE_REGISTER, WPCN381U_ACTIVATE_VALUE},                                // Enable it with Activation bit
    {WPCN381U_LD_SEL_REGISTER, WPCN381U_LDN_GPIO},                                        // Select GPIO device
    {WPCN381U_BASE1_HI_REGISTER, (UINT8)(WPCN381U_GPIO_BASE_ADDRESS >> 8)},               // Set Base Address MSB
    {WPCN381U_BASE1_LO_REGISTER, (UINT8)(WPCN381U_GPIO_BASE_ADDRESS & 0x00FF)},           // Set Base Address LSB
    {WPCN381U_ACTIVATE_REGISTER, WPCN381U_ACTIVATE_VALUE},                                // Enable it with Activation bit
    {0x21, 0x001},                                                                        // Global Device Enable
    {0x26, 0x000},
    {WPCN381U_LD_SEL_REGISTER, WPCN381U_LDN_PS2K},                                        // Select PS2 Keyboard
    {WPCN381U_BASE1_HI_REGISTER, (UINT8)(WPCN381U_KB_BASE1_ADDRESS >> 8)},                // Set Base Address MSB
    {WPCN381U_BASE1_LO_REGISTER, (UINT8)(WPCN381U_KB_BASE1_ADDRESS & 0x00FF)},            // Set Base Address LSB
    {WPCN381U_BASE2_HI_REGISTER, (UINT8)(WPCN381U_KB_BASE2_ADDRESS >> 8)},                // Set Base Address MSB
    {WPCN381U_BASE2_LO_REGISTER, (UINT8)(WPCN381U_KB_BASE2_ADDRESS & 0x00FF)},            // Set Base Address LSB
    {WPCN381U_IRQ1_REGISTER, 0x011},                                                      // Set to IRQ1
    {0xF0, (SIO_KBC_CLOCK << 6)},                                                         // Select KBC Clock Source
    {WPCN381U_ACTIVATE_REGISTER, WPCN381U_ACTIVATE_VALUE},                                // Enable it with Activation bit
    {WPCN381U_LD_SEL_REGISTER, WPCN381U_LDN_PS2M},                                        // Select PS2 Mouse
    {WPCN381U_IRQ1_REGISTER, 0x01c},                                                      // Set to IRQ12
    {WPCN381U_ACTIVATE_REGISTER, WPCN381U_ACTIVATE_VALUE}                                 // Enable it with Activation bit
};

/**
  Initialization for SIO.

  @param FfsHeader     FV this PEIM was loaded from.
  @param PeiServices   General purpose services available to every PEIM.

  None

**/
VOID
InitializeSio (
  VOID
  )
{
  UINT16          Index;
  UINT16          IndexPort;
  UINT16          DataPort;

  //
  // Super I/O initialization for Winbond WPCN381U
  //
  IndexPort  = WPCN381U_CONFIG_INDEX;
  DataPort   = WPCN381U_CONFIG_DATA;

  //
  // Check for Winbond WPCN381U
  //
  IoWrite8 (IndexPort, WPCN381U_DEV_ID_REGISTER);   // Winbond WPCN381U Device ID register is 0x20

  if (IoRead8 (DataPort) == WPCN381U_CHIP_ID) {   // Winbond WPCN381U Device ID is 0xF4
    //
    // Configure WPCN381U SIO
    //
    for (Index = 0; Index < sizeof (mSioTableWpcn381u) / sizeof (EFI_SIO_TABLE); Index++) {
      IoWrite8 (IndexPort, mSioTableWpcn381u[Index].Register);
      IoWrite8 (DataPort, mSioTableWpcn381u[Index].Value);
    }
  }

  if (IoRead8 (DataPort) == WDCP376_CHIP_ID) {   // Winbond WDCP376 Device ID is 0xF1
    //
    // Configure WDCP376 SIO
    //
    for (Index = 0; Index < sizeof (mSioTableWdcp376) / sizeof (EFI_SIO_TABLE); Index++) {
      IoWrite8 (IndexPort, mSioTableWdcp376[Index].Register);
      IoWrite8 (DataPort, mSioTableWdcp376[Index].Value);
    }
  }
  return;
}
