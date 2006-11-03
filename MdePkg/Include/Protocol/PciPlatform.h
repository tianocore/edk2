/** @file
  This file declares PlatfromOpRom protocols.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  PciPlatform.h

  @par Revision Reference:
  This protocol is defined in PCI Platform Support Specification
  Version 0.9

**/

#ifndef _PCI_PLATFORM_H_
#define _PCI_PLATFORM_H_

//
// Protocol for GUID.
//

#define EFI_PCI_PLATFORM_PROTOCOL_GUID \
{ 0x7d75280, 0x27d4, 0x4d69, {0x90, 0xd0, 0x56, 0x43, 0xe2, 0x38, 0xb3, 0x41} }

typedef struct _EFI_PCI_PLATFORM_PROTOCOL EFI_PCI_PLATFORM_PROTOCOL;

typedef    UINT32   EFI_PCI_PLATFORM_POLICY;


#define     EFI_RESERVE_NONE_IO_ALIAS        0x0000
#define     EFI_RESERVE_ISA_IO_ALIAS         0x0001
#define     EFI_RESERVE_ISA_IO_NO_ALIAS      0x0002
#define     EFI_RESERVE_VGA_IO_ALIAS         0x0004
#define     EFI_RESERVE_VGA_IO_NO_ALIAS      0x0008


typedef enum {
  ChipsetEntry,
  ChipsetExit,
  MaximumChipsetPhase
} EFI_PCI_CHIPSET_EXECUTION_PHASE;


/**
  The PlatformNotify() function can be used to notify the platform driver so that 
  it can perform platform-specific actions. No specific actions are required. 
  Eight notification points are defined at this time. More synchronization points 
  may be added as required in the future. The PCI bus driver calls the platform driver 
  twice for every Phase-once before the PCI Host Bridge Resource Allocation Protocol 
  driver is notified, and once after the PCI Host Bridge Resource Allocation Protocol 
  driver has been notified. 
  This member function may not perform any error checking on the input parameters. It 
  also does not return any error codes. If this member function detects any error condition, 
  it needs to handle those errors on its own because there is no way to surface any 
  errors to the caller.

  @param  This                  Pointer to the EFI_PCI_PLATFORM_PROTOCOL instance.
  @param  HostBridge            The handle of the host bridge controller.
  @param  Phase                 The phase of the PCI bus enumeration.
  @param  ChipsetPhase          Defines the execution phase of the PCI chipset driver.

  @retval EFI_SUCCESS           The function completed successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_PLATFORM_PHASE_NOTIFY) (
  IN EFI_PCI_PLATFORM_PROTOCOL                       *This,
  IN  EFI_HANDLE                                     HostBridge,
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE  Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase  
)
;


/**
  The PlatformPrepController() function can be used to notify the platform driver so that 
  it can perform platform-specific actions. No specific actions are required. 
  Several notification points are defined at this time. More synchronization points may be 
  added as required in the future. The PCI bus driver calls the platform driver twice for 
  every PCI controller-once before the PCI Host Bridge Resource Allocation Protocol driver 
  is notified, and once after the PCI Host Bridge Resource Allocation Protocol driver has 
  been notified. 
  This member function may not perform any error checking on the input parameters. It also 
  does not return any error codes. If this member function detects any error condition, it 
  needs to handle those errors on its own because there is no way to surface any errors to 
  the caller.  

  @param  This                  Pointer to the EFI_PCI_PLATFORM_PROTOCOL instance.
  @param  HostBridge            The associated PCI host bridge handle.
  @param  RootBridge            The associated PCI root bridge handle.
  @param  PciAddress            The address of the PCI device on the PCI bus.
  @param  Phase                 The phase of the PCI controller enumeration.
  @param  ChipsetPhase          Defines the execution phase of the PCI chipset driver.

  @retval EFI_SUCCESS           The function completed successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_PLATFORM_PREPROCESS_CONTROLLER) (
  IN  EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN  EFI_HANDLE                                     HostBridge,
  IN  EFI_HANDLE                                     RootBridge,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS    PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE   Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase
)
;


/**
  The GetPlatformPolicy() function retrieves the platform policy regarding PCI 
  enumeration. The PCI bus driver and the PCI Host Bridge Resource Allocation Protocol 
  driver can call this member function to retrieve the policy.

  @param  This                  Pointer to the EFI_PCI_PLATFORM_PROTOCOL instance.
  @param  PciPolicy             The platform policy with respect to VGA and ISA aliasing.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER PciPolicy is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_PLATFORM_GET_PLATFORM_POLICY) (
  IN  EFI_PCI_PLATFORM_PROTOCOL           *This,
  OUT EFI_PCI_PLATFORM_POLICY             *PciPolicy
)
;


/**
  The GetPciRom() function gets the PCI device's option ROM from a platform-specific location. 
  The option ROM will be loaded into memory. This member function is used to return an image 
  that is packaged as a PCI 2.2 option ROM. The image may contain both legacy and EFI option 
  ROMs. See the EFI 1.10 Specification for details. This member function can be used to return 
  option ROM images for embedded controllers. Option ROMs for embedded controllers are typically 
  stored in platform-specific storage, and this member function can retrieve it from that storage 
  and return it to the PCI bus driver. The PCI bus driver will call this member function before 
  scanning the ROM that is attached to any controller, which allows a platform to specify a ROM 
  image that is different from the ROM image on a PCI card.

  @param  This                  Pointer to the EFI_PCI_PLATFORM_PROTOCOL instance.
  @param  PciHandle             The handle of the PCI device.
  @param  RomImage              If the call succeeds, the pointer to the pointer to the option ROM image.
                                Otherwise, this field is undefined. The memory for RomImage is allocated
                                by EFI_PCI_PLATFORM_PROTOCOL.GetPciRom() using the EFI Boot Service AllocatePool().
                                It is the caller's responsibility to free the memory using the EFI Boot Service
                                FreePool(), when the caller is done with the option ROM.
  @param  RomSize               If the call succeeds, a pointer to the size of the option ROM size. Otherwise,
                                this field is undefined.

  @retval EFI_SUCCESS           The option ROM was available for this device and loaded into memory.
  @retval EFI_NOT_FOUND         No option ROM was available for this device.
  @retval EFI_OUT_OF_RESOURCES  No memory was available to load the option ROM.
  @retval EFI_DEVICE_ERROR      An error occurred in getting the option ROM.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_PLATFORM_GET_PCI_ROM) (        
  IN   EFI_PCI_PLATFORM_PROTOCOL    *This,
  IN   EFI_HANDLE                   PciHandle,
  OUT  VOID                         **RomImage,
  OUT  UINTN                        *RomSize              
)
;

/**
  @par Protocol Description:
  This protocol provides the interface between the PCI bus driver/PCI Host 
  Bridge Resource Allocation driver and a platform-specific driver to describe 
  the unique features of a platform.

  @param PlatformNotify
  The notification from the PCI bus enumerator to the platform that it is 
  about to enter a certain phase during the enumeration process.

  @param PlatformPrepController
  The notification from the PCI bus enumerator to the platform for each PCI 
  controller at several predefined points during PCI controller initialization.

  @param GetPlatformPolicy
  Retrieves the platform policy regarding enumeration.

  @param GetPciRom
  Gets the PCI device's option ROM from a platform-specific location.

**/
struct _EFI_PCI_PLATFORM_PROTOCOL {
  EFI_PCI_PLATFORM_PHASE_NOTIFY          PhaseNotify;
  EFI_PCI_PLATFORM_PREPROCESS_CONTROLLER PlatformPrepController;
  EFI_PCI_PLATFORM_GET_PLATFORM_POLICY   GetPlatformPolicy;
  EFI_PCI_PLATFORM_GET_PCI_ROM           GetPciRom;
};

extern EFI_GUID   gEfiPciPlatformProtocolGuid;


#endif
