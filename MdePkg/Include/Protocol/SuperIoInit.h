/** @file
  The EFI_SIO_INIT_PROTOCOL provides the I/O resource information to the
  Super I/O bus driver. This protocol is mandatory for Super I/O controllers
  if the Super I/O controller is to be managed by the Super I/O bus driver.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __EFI_SUPER_IO_INIT_PROTOCOL_H__
#define __EFI_SUPER_IO_INIT_PROTOCOL_H__

#define EFI_SIO_INIT_PROTOCOL_GUID \
  { 0x9fe35634, 0x87ca, 0x4569, { 0xbf, 0x55, 0xda, 0x24, 0xef, 0x41, 0x64, 0xd2 } }

typedef struct {
  ///
  /// The Config Port of the Super I/O controller.
  ///
  UINT16   ConfigPort;

  ///
  /// The Index Port of the Super I/O controller.
  ///
  UINT16   IndexPort;

  ///
  /// The Data Port of the Super I/O controller.
  ///
  UINT16   DataPort;
} EFI_SIO_INIT_PROTOCOL;

extern EFI_GUID gEfiSioInitProtocolGuid;

#endif // __EFI_SUPER_IO_INIT_PROTOCOL_H__
