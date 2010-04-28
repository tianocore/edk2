/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  VariableStore.h

Abstract:

Revision History

--*/

#ifndef _VARIABLE_STORE_H
#define _VARIABLE_STORE_H

//
// The variable store protocol interface is specific to the reference
// implementation. The initialization code adds variable store devices
// to the system, and the FW connects to the devices to provide the
// variable store interfaces through these devices.
//

//
// Variable Store Device protocol
//
#define EFI_VARIABLE_STORE_PROTOCOL_GUID    \
  { 0xf088cd91, 0xa046, 0x11d2, {0x8e, 0x42, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

EFI_FORWARD_DECLARATION (EFI_VARIABLE_STORE_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *EFI_CLEAR_STORE) (
  IN EFI_VARIABLE_STORE_PROTOCOL   *This,
  IN OUT VOID                              *Scratch
  );

typedef
EFI_STATUS
(EFIAPI *EFI_READ_STORE) (
  IN EFI_VARIABLE_STORE_PROTOCOL   *This,
  IN UINTN                                 Offset,
  IN UINTN                                 BufferSize,
  OUT VOID                                 *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_UPDATE_STORE) (
  IN EFI_VARIABLE_STORE_PROTOCOL   *This,
  IN UINTN                                 Offset,
  IN UINTN                                 BufferSize,
  IN VOID                                  *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_CLEANUP_STORE) (
  IN EFI_VARIABLE_STORE_PROTOCOL   *This
  );

struct _EFI_VARIABLE_STORE_PROTOCOL {
  
  //
  // Number of banks and bank size
  //
  UINT32                      Attributes;
  UINT32                      BankSize;

  //
  // Functions to access the storage banks
  //
  EFI_CLEAR_STORE             ClearStore;
  EFI_READ_STORE              ReadStore;
  EFI_UPDATE_STORE            UpdateStore;
  EFI_CLEANUP_STORE           CleanupStore;

};

//
//
//  ClearStore()        - A function to clear the requested storage bank.  A cleared
//                        bank contains all "on" bits.
//
//  ReadStore()         - Read data from the requested store.
//
//  UpdateStore()       - Updates data on the requested store. The FW will only
//                        ever issue updates to clear bits in the store. Updates must 
//                        be performed in LSb to MSb order of the update buffer.
//
//  CleanupStore()      - Do garbage collection and reclaim operation.
//

extern EFI_GUID gEfiVariableStoreProtocolGuid;

#endif // _VARIABLE_STORE_H
