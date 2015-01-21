/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#ifndef _I2C_PORT_H
#define _I2C_PORT_H

//
//  Types
//

//
// Context passed from platform (board) layer to the I2C port driver.
//
typedef struct {
  EFI_PHYSICAL_ADDRESS BaseAddress;
  UINT32 InputFrequencyHertz;
} I2C_PIO_PLATFORM_CONTEXT;

#endif  //  _I2C_PORT_A0_H
