/**@file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#ifndef _EFI_PCI_DRIVER_OVERRRIDE_H
#define _EFI_PCI_DRIVER_OVERRRIDE_H

#define DRIVER_OVERRIDE_SIGNATURE EFI_SIGNATURE_32 ('d', 'r', 'o', 'v')

typedef struct {
  UINT32          Signature;
  LIST_ENTRY      Link;
  EFI_HANDLE      DriverImageHandle;
} PCI_DRIVER_OVERRIDE_LIST;


#define DRIVER_OVERRIDE_FROM_LINK(a) \
  CR (a, PCI_DRIVER_OVERRIDE_LIST, Link, DRIVER_OVERRIDE_SIGNATURE)

/**
  Initializes a PCI Driver Override Instance

  @param  PciIoDevice   Device instance

  @retval EFI_SUCCESS Operation success
**/
EFI_STATUS
InitializePciDriverOverrideInstance (
  PCI_IO_DEVICE  *PciIoDevice
  );

/**
  Add an overriding driver image
  
  @param PciIoDevice        Instance of PciIo device
  @param DriverImageHandle  new added driver image
  
  @retval EFI_OUT_OF_RESOURCES no memory resource for new driver instance
  @retval EFI_SUCCESS       Success add driver
**/
EFI_STATUS
AddDriver (
  IN PCI_IO_DEVICE     *PciIoDevice,
  IN EFI_HANDLE        DriverImageHandle
  );


/**
  Get a overriding driver image
  @param  This                Pointer to instance of EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL
  @param  DriverImageHandle   Override driver image,
  
  @retval EFI_SUCCESS                 Success to get driver image handle
  @retval EFI_NOT_FOUND               can not find override driver image
  @retval EFI_INVALID_PARAMETER       Invalid parameter
**/
EFI_STATUS
EFIAPI
GetDriver (
  IN EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL              *This,
  IN OUT EFI_HANDLE                                         *DriverImageHandle
  );

#endif
