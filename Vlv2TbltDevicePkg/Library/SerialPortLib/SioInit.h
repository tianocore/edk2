/** @file
  Header file of Serial port hardware definition.

  Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   
  This software and associated documentation
  (if any) is furnished under a license and may only be used or
  copied in accordance with the terms of the license.  Except as
  permitted by such license, no part of this software or
  documentation may be reproduced, stored in a retrieval system, or
  transmitted in any form or by any means without the express written
  consent of Intel Corporation.

  Module Name:  PlatformSerialPortLib.h

**/

#ifndef _SIO_INIT_H_
#define _SIO_INIT_H_

#define WPCN381U_CONFIG_INDEX               0x2E
#define WPCN381U_CONFIG_DATA                0x2F
#define WPCN381U_CONFIG_INDEX1              0x164E
#define WPCN381U_CONFIG_DATA1               0x164F
#define WPCN381U_CHIP_ID                    0xF4
#define WDCP376_CHIP_ID                     0xF1

//
// SIO Logical Devices Numbers
//
#define WPCN381U_LDN_UART0                  0x03   // LDN for Serial Port Controller
#define WPCN381U_LDN_UART1                  0x02   // LDN for Parallel Port Controller
#define WPCN381U_LDN_PS2K                   0x06   // LDN for PS2 Keyboard Controller
#define WPCN381U_LDN_PS2M                   0x05   // LDN for PS2 Mouse Controller
#define WPCN381U_KB_BASE1_ADDRESS           0x60   // Base Address of KB controller
#define WPCN381U_KB_BASE2_ADDRESS           0x64   // Base Address of KB controller
#define SIO_KBC_CLOCK                       0x01   // 0/1/2 - 8/12/16 MHz KBC Clock Source
#define WPCN381U_LDN_GPIO                   0x07   // LDN for GPIO

//
// SIO Registers Layout
//
#define WPCN381U_LD_SEL_REGISTER            0x07   // Logical Device Select Register Address
#define WPCN381U_DEV_ID_REGISTER            0x20   // Device Identification Register Address
#define WPCN381U_ACTIVATE_REGISTER          0x30   // Device Identification Register Address
#define WPCN381U_BASE1_HI_REGISTER          0x60   // Device BaseAddres Register #1 MSB Address
#define WPCN381U_BASE1_LO_REGISTER          0x61   // Device BaseAddres Register #1 LSB Address
#define WPCN381U_BASE2_HI_REGISTER          0x62   // Device BaseAddres Register #1 MSB Address
#define WPCN381U_BASE2_LO_REGISTER          0x63   // Device Ba1eAddres Register #1 LSB Address
#define WPCN381U_IRQ1_REGISTER              0x70   // Device IRQ Register #1 Address
#define WPCN381U_IRQ2_REGISTER              0x71   // Device IRQ Register #2 Address

//
// SIO Activation Values
//
#define WPCN381U_ACTIVATE_VALUE             0x01   // Value to activate Device
#define WPCN381U_DEACTIVATE_VALUE           0x00   // Value to deactivate Device

//
// SIO GPIO
//
#define WPCN381U_GPIO_BASE_ADDRESS          0x0A20 // SIO GPIO Base Address

//
// SIO Serial Port Settings
//
#define WPCN381U_SERIAL_PORT0_BASE_ADDRESS  0x03F8 // Base Address of Serial Port 0 (COMA / UART0)
#define WPCN381U_SERIAL_PORT1_BASE_ADDRESS  0x02F8 // Base Address of Serial Port 1 (COMB / UART1)

#endif
