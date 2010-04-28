/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  RtLedStatusCode.h
   
Abstract:

  Lib to provide status code reporting via LED.

--*/

#ifndef _EFI_LED_STATUS_CODE_H_
#define _EFI_LED_STATUS_CODE_H_

//
// Statements that include other files
//
#include "Tiano.h"
#include "EfiCommonLib.h"
#include "EfiRuntimeLib.h"
#include "EfiStatusCode.h"

//
// SIOINIT data
//
typedef struct {
  UINT8 RegAddress;
  UINT8 RegValue;
} SIO_INIT_DATA;

#define LED_DATA_OFFSET   0x0E
#define LED_STROBE_OFFSET 0x0A

#define LED_MASK_BIT      0x10
#define STROBE_MASK_BIT   0x08

#define GPIO_BASE(a, b)   (UINT16) ((a << 8) | (b))

#define SIO_GPIO_HIGH     0x08
#define SIO_GPIO_LOW      0x00

#define CONFIG_PORT0      0x2E
#define DATA_PORT0        0x2F

//
// logical device in  NSPC87417
//
#define SIO_GPIO  0x7

//
// Global register in NSPC87417
//
#define REG_LOGICAL_DEVICE  0x07

#define REG_SERVERIO_CNF1   0x21
#define REG_SERVERIO_CNF2   0x22
#define REG_SERVERIO_CNF3   0x23
#define REG_SERVERIO_CNF4   0x24
#define REG_SERVERIO_CNF6   0x26

#define ACTIVATE            0x30
#define LOGICAL_DEVICE_ON   0x01
#define LOGICAL_DEVICE_OFF  0x00
#define BASE_ADDRESS_HIGH   0x60
#define BASE_ADDRESS_LOW    0x61

//
// Register for GPIO
//
#define GPIO_GPSEL        0xF0

#define GPIO_GPCFG1       0xF1
#define PUSH_PULL         0x02
#define OUTPUT_BUFFER_EN  0x01

#define GPIO_GPEVR        0xF2
#define GPIO_EVENT_OFF    0x00

#endif
