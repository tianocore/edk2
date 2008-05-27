/** @file
  PCI Incompatible device support Libary.

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

**/

#ifndef __PCI_INCOMPATIBLE_DEVICE_SUPPORT_LIB_H__
#define __PCI_INCOMPATIBLE_DEVICE_SUPPORT_LIB_H__

#define PCI_REGISTER_READ    0xfffffffffffffff1ULL
#define PCI_REGISTER_WRITE   0xfffffffffffffff2ULL
#define VALUE_NOCARE         0xffffffffffffffffULL

//
// PCI device device information
//
typedef struct {
  UINT64              VendorID;
  UINT64              DeviceID;
  UINT64              RevisionID;
  UINT64              SubsystemVendorID;
  UINT64              SubsystemID;
} EFI_PCI_DEVICE_INFO;


//
// store hardcode value of resgister
//
typedef struct {
  UINT64              AndValue;
  UINT64              OrValue;
} EFI_PCI_REGISTER_VALUE_DATA;

//
// store access width information
//
typedef struct {
  UINT64              StartOffset;
  UINT64              EndOffset;
  UINT64              Width;
} EFI_PCI_REGISTER_ACCESS_DATA;


//
// ACPI resource descriptor
//
typedef struct {
  UINT64  ResType;
  UINT64  GenFlag;
  UINT64  SpecificFlag;
  UINT64  AddrSpaceGranularity;
  UINT64  AddrRangeMin;
  UINT64  AddrRangeMax;
  UINT64  AddrTranslationOffset;
  UINT64  AddrLen;
} EFI_PCI_RESOUCE_DESCRIPTOR;

/**
  Checks the incompatible device list for ACPI resource update and return
  the configuration.

  This function searches the incompatible device list according to request
  information. If the PCI device belongs to the devices list, corresponding
  configuration informtion will be returned, in the meantime return EFI_SUCCESS.

  @param  PciDeviceInfo       A pointer to PCI device information.
  @param  Configuration       Returned information.

  @retval returns EFI_SUCCESS if check incompatible device ok.
          Otherwise return EFI_UNSUPPORTED.
**/
RETURN_STATUS
EFIAPI
PciResourceUpdateCheck (
  IN  EFI_PCI_DEVICE_INFO           *PciDeviceInfo,
  OUT VOID                          *Configuration
  );

/**
  Checks the incompatible device list and return configuration register mask values.

  This function searches the incompatible device list according to request
  information. If the PCI device belongs to the devices list, corresponding
  configuration informtion will be returned, in the meantime return EFI_SUCCESS.

  @param  PciDeviceInfo       A pointer to EFI_PCI_DEVICE_INFO.
  @param  AccessType          Access Type, READ or WRITE.
  @param  Offset              The address within the PCI configuration space.
  @param  Configuration       Returned information.

  @retval returns EFI_SUCCESS if check incompatible device ok.
          Otherwise return EFI_UNSUPPORTED.
**/
RETURN_STATUS
EFIAPI
PciRegisterUpdateCheck (
  IN  EFI_PCI_DEVICE_INFO           *PciDeviceInfo,
  IN  UINT64                        AccessType,
  IN  UINT64                        Offset,
  OUT VOID                          *Configuration
  );

/**
  Checks the incompatible device list for access width incompatibility and
  return the configuration

  This function searches the incompatible device list for access width
  incompatibility according to request information. If the PCI device
  belongs to the devices list, corresponding configuration informtion
  will be returned, in the meantime return EFI_SUCCESS.

  @param  PciDeviceInfo       A pointer to PCI device information.
  @param  AccessType          Access type, READ or WRITE.
  @param  Offset              The address within the PCI configuration space.
  @param  AccessWidth         Access width needs to check incompatibility.
  @param  Configuration       Returned information.

  @retval returns EFI_SUCCESS if check incompatible device ok.
          Otherwise return EFI_UNSUPPORTED.
**/
RETURN_STATUS
EFIAPI
PciRegisterAccessCheck (
  IN  EFI_PCI_DEVICE_INFO           *PciDeviceInfo,
  IN  UINT64                        AccessType,
  IN  UINT64                        Offset,
  IN  UINT64                        AccessWidth,
  OUT VOID                          *Configuration
  );

#endif

