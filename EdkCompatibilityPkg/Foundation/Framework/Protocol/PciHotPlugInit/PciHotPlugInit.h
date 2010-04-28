/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    PciHotPlugInit.h
    
Abstract:

    EFI PCI Hot Plug Init Protocol

Revision History

--*/

#ifndef _EFI_PCI_HOT_PLUG_INIT_H
#define _EFI_PCI_HOT_PLUG_INIT_H

//
// Global ID for the PCI Hot Plug Protocol
//
#define EFI_PCI_HOT_PLUG_INIT_PROTOCOL_GUID \
  { 0xaa0e8bc1, 0xdabc, 0x46b0, {0xa8, 0x44, 0x37, 0xb8, 0x16, 0x9b, 0x2b, 0xea} }

  
EFI_FORWARD_DECLARATION (EFI_PCI_HOT_PLUG_INIT_PROTOCOL);

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

typedef
EFI_STATUS
(EFIAPI *EFI_GET_ROOT_HPC_LIST) (
  IN EFI_PCI_HOT_PLUG_INIT_PROTOCOL    *This,
  OUT UINTN                            *HpcCount,
  OUT EFI_HPC_LOCATION                **HpcList
); 

typedef
EFI_STATUS
(EFIAPI *EFI_INITIALIZE_ROOT_HPC) (
  IN EFI_PCI_HOT_PLUG_INIT_PROTOCOL     *This,
  IN  EFI_DEVICE_PATH_PROTOCOL          *HpcDevicePath,
  IN  UINT64                            HpcPciAddress,
  IN  EFI_EVENT                         Event, OPTIONAL
  OUT EFI_HPC_STATE                    *HpcState
); 

typedef
EFI_STATUS
(EFIAPI *EFI_GET_PCI_HOT_PLUG_PADDING) (
  IN EFI_PCI_HOT_PLUG_INIT_PROTOCOL       *This,
  IN  EFI_DEVICE_PATH_PROTOCOL          *HpcDevicePath,
  IN  UINT64                            HpcPciAddress,
  OUT EFI_HPC_STATE                    *HpcState,
  OUT VOID                              **Padding,
  OUT EFI_HPC_PADDING_ATTRIBUTES       *Attributes
); 


//
// Prototypes for the PCI Hot Plug Init Protocol
//


struct _EFI_PCI_HOT_PLUG_INIT_PROTOCOL {
  EFI_GET_ROOT_HPC_LIST                                  GetRootHpcList;
  EFI_INITIALIZE_ROOT_HPC                                InitializeRootHpc;
  EFI_GET_PCI_HOT_PLUG_PADDING                           GetResourcePadding;
};


extern EFI_GUID gEfiPciHotPlugInitProtocolGuid;

#endif
