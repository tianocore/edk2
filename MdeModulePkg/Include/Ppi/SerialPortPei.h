/** @file
  PPI that is installed after the initialization of a serial stream device 
  is complete.  

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PEI_SERIAL_PORT_PPI_H__
#define __PEI_SERIAL_PORT_PPI_H__

#define PEI_SERIAL_PORT_PPI \
  { \
    0x490e9d85, 0x8aef, 0x4193, { 0x8e, 0x56, 0xf7, 0x34, 0xa9, 0xff, 0xac, 0x8b } \
  }

extern EFI_GUID gPeiSerialPortPpiGuid;

#endif
