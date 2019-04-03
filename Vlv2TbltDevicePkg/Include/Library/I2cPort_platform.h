/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

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
