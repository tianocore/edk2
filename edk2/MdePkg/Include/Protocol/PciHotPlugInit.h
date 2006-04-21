/** @file
  This file declares EFI PCI Hot Plug Init Protocol

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  PciHotPlugInit.h

  @par Revision Reference:
  This protocol is defined in Framework of EFI Hot Plug Pci Initialization Protocol Spec
  Version 0.9

**/

#ifndef _EFI_PCI_HOT_PLUG_INIT_H
#define _EFI_PCI_HOT_PLUG_INIT_H

//
// Global ID for the PCI Hot Plug Protocol
//
#define EFI_PCI_HOT_PLUG_INIT_PROTOCOL_GUID \
  { 0xaa0e8bc1, 0xdabc, 0x46b0, {0xa8, 0x44, 0x37, 0xb8, 0x16, 0x9b, 0x2b, 0xea } }

  
typedef struct _EFI_PCI_HOT_PLUG_INIT_PROTOCOL EFI_PCI_HOT_PLUG_INIT_PROTOCOL;

#define  EFI_HPC_STATE_INITIALIZED    0x01
#define  EFI_HPC_STATE_ENABLED        0x02

typedef UINT16 EFI_HPC_STATE;


typedef struct{
  EFI_DEVICE_PATH_PROTOCOL  *HpcDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *HpbDevicePath;
} EFI_HPC_LOCATION;


typedef enum{
  EfiPaddingPciBus,
  EfiPaddingPciRootBridge
} EFI_HPC_PADDING_ATTRIBUTES;

/**
  Returns a list of root Hot Plug Controllers (HPCs) that require initialization 
  during the boot process.

  @param  This Pointer to the EFI_PCI_HOT_PLUG_INIT_PROTOCOL instance.
  
  @param  HpcCount The number of root HPCs that were returned.
  
  @param  HpcList The list of root HPCs. HpcCount defines the number of 
  elements in this list.

  @retval EFI_SUCCESS HpcList was returned.
  
  @retval EFI_OUT_OF_RESOURCES HpcList was not returned due to insufficient resources.
  
  @retval EFI_INVALID_PARAMETER HpcCount is NULL or HpcList is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GET_ROOT_HPC_LIST) (
  IN EFI_PCI_HOT_PLUG_INIT_PROTOCOL   *This,
  OUT UINTN                           *HpcCount,
  OUT EFI_HPC_LOCATION                **HpcList
);

/**
  Initializes one root Hot Plug Controller (HPC). This process may causes 
  initialization of its subordinate buses. 

  @param  This Pointer to the EFI_PCI_HOT_PLUG_INIT_PROTOCOL instance.
  
  @param  HpcDevicePath The device path to the HPC that is being initialized.
  
  @param  HpcPciAddress The address of the HPC function on the PCI bus.
  
  @param  Event The event that should be signaled when the HPC initialization 
  is complete.
  
  @param  HpcState The state of the HPC hardware. 

  @retval EFI_SUCCESS If Event is NULL, the specific HPC was successfully 
  initialized. If Event is not NULL,  Event will be signaled at a later time 
  when initialization is complete.
  
  @retval EFI_UNSUPPORTED This instance of EFI_PCI_HOT_PLUG_INIT_PROTOCOL 
  does not support the specified HPC.
  
  @retval EFI_OUT_OF_RESOURCES Initialization failed due to insufficient 
  resources.
  
  @retval EFI_INVALID_PARAMETER HpcState is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_INITIALIZE_ROOT_HPC) (
  IN EFI_PCI_HOT_PLUG_INIT_PROTOCOL     *This,
  IN  EFI_DEVICE_PATH_PROTOCOL          *HpcDevicePath,
  IN  UINT64                            HpcPciAddress,
  IN  EFI_EVENT                         Event, OPTIONAL
  OUT EFI_HPC_STATE                     *HpcState
);

/**
  Returns the resource padding that is required by the PCI bus that is controlled 
  by the specified Hot Plug Controller (HPC).

  @param  This Pointer to the EFI_PCI_HOT_PLUG_INIT_PROTOCOL instance.
  
  @param  HpcDevicePath The device path to the HPC.
  
  @param  HpcPciAddress The address of the HPC function on the PCI bus. 
  
  @param  HpcState The state of the HPC hardware. 
  
  @param  Padding The amount of resource padding that is required by the 
  PCI bus under the control of the specified HPC. 
  
  @param  Attributes Describes how padding is accounted for. The padding 
  is returned in the form of ACPI 2.0 resource descriptors. 

  @retval EFI_SUCCESS The resource padding was successfully returned.
  
  @retval EFI_UNSUPPORTED This instance of the EFI_PCI_HOT_PLUG_INIT_PROTOCOL 
  does not support the specified HPC.
  
  @retval EFI_NOT_READY This function was called before HPC initialization is complete.
  
  @retval EFI_INVALID_PARAMETER  HpcState or Padding or Attributes is NULL.
  
  @retval EFI_OUT_OF_RESOURCES ACPI 2.0 resource descriptors for Padding 
  cannot be allocated due to insufficient resources.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GET_PCI_HOT_PLUG_PADDING) (
  IN EFI_PCI_HOT_PLUG_INIT_PROTOCOL     *This,
  IN  EFI_DEVICE_PATH_PROTOCOL          *HpcDevicePath,
  IN  UINT64                            HpcPciAddress,
  OUT EFI_HPC_STATE                     *HpcState,
  OUT VOID                              **Padding,
  OUT EFI_HPC_PADDING_ATTRIBUTES        *Attributes
); 


//
// Prototypes for the PCI Hot Plug Init Protocol
//

/**
  @par Protocol Description:
  This protocol provides the necessary functionality to initialize the 
  Hot Plug Controllers (HPCs) and the buses that they control. This protocol 
  also provides information regarding resource padding. 

  @param GetRootHpcList
  Returns a list of root HPCs and the buses that they control.

  @param InitializeRootHpc
  Initializes the specified root HPC.

  @param GetResourcePadding
  Returns the resource padding that is required by the HPC.

**/
struct _EFI_PCI_HOT_PLUG_INIT_PROTOCOL {
  EFI_GET_ROOT_HPC_LIST                                  GetRootHpcList;
  EFI_INITIALIZE_ROOT_HPC                                InitializeRootHpc;
  EFI_GET_PCI_HOT_PLUG_PADDING                           GetResourcePadding;
};

extern EFI_GUID gEfiPciHotPlugInitProtocolGuid;

#endif
