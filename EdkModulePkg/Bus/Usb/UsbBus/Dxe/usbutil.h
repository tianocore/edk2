/*++
 
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:
    usbutil.h
  
  Abstract:
 
    Helper functions for USB
 
  Revision History
 
  
--*/

#ifndef _USB_UTIL_H
#define _USB_UTIL_H

//
// Following APIs are used to query Port Status
//
BOOLEAN
IsPortConnect (
  IN UINT16  PortStatus
  );

BOOLEAN
IsPortEnable (
  IN UINT16  PortStatus
  );

BOOLEAN
IsPortInReset (
  IN UINT16  PortStatus
  );

BOOLEAN
IsPortPowerApplied (
  IN UINT16  PortStatus
  );

BOOLEAN
IsPortLowSpeedDeviceAttached (
  IN UINT16  PortStatus
  );

BOOLEAN
IsPortSuspend (
  IN UINT16  PortStatus
  );

//
// Following APIs are used to query Port Change Status
//
BOOLEAN
IsPortConnectChange (
  IN UINT16  PortChangeStatus
  );

BOOLEAN
IsPortEnableDisableChange (
  IN UINT16  PortChangeStatus
  );

BOOLEAN
IsPortResetChange (
  IN UINT16  PortChangeStatus
  );

BOOLEAN
IsPortSuspendChange (
  IN UINT16  PortChangeStatus
  );

//
// Set device address;
//
EFI_STATUS
UsbSetDeviceAddress (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  AddressValue,
  OUT UINT32                  *Status
  );


#endif
