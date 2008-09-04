/**@file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _EFI_PCI_POWER_MANAGEMENT_H
#define _EFI_PCI_POWER_MANAGEMENT_H

/**
  This function is intended to turn off PWE assertion and
  put the device to D0 state if the device supports
  PCI Power Management.

  @param PciIoDevice  Pci device instance

  @retval EFI_UNSUPPORTED Device do not support power management
  @retval EFI_SUCCESS     Success
**/
EFI_STATUS
ResetPowerManagementFeature (
  IN PCI_IO_DEVICE *PciIoDevice
  );

#endif
