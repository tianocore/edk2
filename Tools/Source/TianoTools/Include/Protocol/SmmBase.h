/** @file
  This file declares SMM Base abstraction protocol.
  This is the base level of compatiblity for SMM drivers.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SmmBase.h

  @par Revision Reference:
  This Protocol is defined in Framework of EFI SMM Core Interface Spec
  Version 0.9.

**/

#ifndef _SMM_BASE_H_
#define _SMM_BASE_H_

#define EFI_SMM_BASE_PROTOCOL_GUID \
  { \
    0x1390954D, 0xda95, 0x4227, {0x93, 0x28, 0x72, 0x82, 0xc2, 0x17, 0xda, 0xa8 } \
  }

typedef struct _EFI_SMM_BASE_PROTOCOL             EFI_SMM_BASE_PROTOCOL;

//
// SMM Handler Definition
//
#define EFI_HANDLER_SUCCESS         0x0000
#define EFI_HANDLER_CRITICAL_EXIT   0x0001
#define EFI_HANDLER_SOURCE_QUIESCED 0x0002
#define EFI_HANDLER_SOURCE_PENDING  0x0003

/**
  Entry Point to Callback service

  @param  SmmImageHandle A handle allocated by the SMM infrastructure code 
  to uniquely designate a specific DXE SMM driver. 
  
  @param  CommunicationBuffer A pointer to a collection of data in memory 
  that will be conveyed from a non-SMM environment into an SMM environment.
  The buffer must be contiguous, physically mapped, and be a physical address.
  
  @param  SourceSize The size of the CommunicationBuffer.

  @return Status code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CALLBACK_ENTRY_POINT) (
  IN EFI_HANDLE             SmmImageHandle,
  IN OUT VOID               *CommunicationBuffer OPTIONAL,
  IN OUT UINTN              *SourceSize OPTIONAL
  );

//
// SMM Base Protocol Definition
//
/**
  Register a given driver into SMRAM.This is the equivalent of performing
  the LoadImage/StartImage into System Management Mode.

  @param  This Protocol instance pointer.
  @param  FilePath Location of the image to be installed as the handler.
  @param  SourceBuffer Optional source buffer in case of the image file
  being in memory.
  @param  SourceSize Size of the source image file, if in memory.
  @param  ImageHandle Pointer to the handle that reflects the driver
  loaded into SMM.
  @param  LegacyIA32Binary The binary image to load is legacy 16 bit code.

  @retval  EFI_SUCCESS The operation was successful.
  @retval  EFI_OUT_OF_RESOURCES There were no additional SMRAM resources to load the handler
  @retval  EFI_UNSUPPORTED This platform does not support 16-bit handlers.
  @retval  EFI_UNSUPPORTED In runtime.
  @retval  EFI_INVALID_PARAMETER The handlers was not the correct image type

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_REGISTER_HANDLER) (
  IN EFI_SMM_BASE_PROTOCOL                           *This,
  IN  EFI_DEVICE_PATH_PROTOCOL                       *FilePath,
  IN  VOID                                           *SourceBuffer OPTIONAL,
  IN  UINTN                                          SourceSize,
  OUT EFI_HANDLE                                     *ImageHandle,
  IN  BOOLEAN                                        LegacyIA32Binary OPTIONAL
  )
;

/**
  Remove a given driver SMRAM.  This is the equivalent of performing
  the UnloadImage System Management Mode.

  @param  This Protocol instance pointer.
  @param  ImageHandle Pointer to the handle that reflects the driver
  loaded into SMM.

  @retval  EFI_SUCCESS The operation was successful
  @retval  EFI_INVALID_PARAMETER The handler did not exist
  @retval  EFI_UNSUPPORTED In runtime.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_UNREGISTER_HANDLER) (
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN EFI_HANDLE                     ImageHandle
  )
;

/**
  The SMM Inter-module Communicate Service Communicate() function 
  provides a services to send/received messages from a registered 
  EFI service.  The BASE protocol driver is responsible for doing 
  any of the copies such that the data lives in boot-service accessible RAM.

  @param  This Protocol instance pointer.
  @param  ImageHandle Pointer to the handle that reflects the driver
  loaded into SMM.
  @param  CommunicationBuffer Pointer to the buffer to convey into SMRAM.
  @param  SourceSize Size of the contents of buffer..

  @retval  EFI_SUCCESS The message was successfully posted
  @retval  EFI_INVALID_PARAMETER The buffer was NULL

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_COMMUNICATE) (
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN EFI_HANDLE                     ImageHandle,
  IN OUT VOID                       *CommunicationBuffer,
  IN OUT UINTN                      *SourceSize
  )
;

/**
  Register a callback to execute within SMM.   
  This allows receipt of messages created with the Boot Service COMMUNICATE.

  @param  This Protocol instance pointer.
  @param  CallbackAddress Address of the callback service
  @param  MakeFirst If present, will stipulate that the handler is posted
  to be the first module executed in the dispatch table.
  @param  MakeLast If present, will stipulate that the handler is posted
  to be last executed in the dispatch table.
  @param  FloatingPointSave This is an optional parameter which informs the
  EFI_SMM_ACCESS_PROTOCOL Driver core if it needs to save
  the floating point register state.  If any of the handlers
  require this, then the state will be saved for all of the handlers.

  @retval  EFI_SUCCESS The operation was successful
  @retval  EFI_OUT_OF_RESOURCES Not enough space in the dispatch queue
  @retval  EFI_UNSUPPORTED In runtime.
  @retval  EFI_UNSUPPORTED Not in SMM.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CALLBACK_SERVICE) (
  IN EFI_SMM_BASE_PROTOCOL                            *This,
  IN EFI_HANDLE                                       SmmImageHandle,
  IN EFI_SMM_CALLBACK_ENTRY_POINT                     CallbackAddress,
  IN BOOLEAN                                          MakeLast OPTIONAL,
  IN BOOLEAN                                          FloatingPointSave OPTIONAL
  )
;

/**
  The SmmAllocatePool() function allocates a memory region of Size bytes from memory of 
  type PoolType and returns the address of the allocated memory in the location referenced 
  by Buffer.  This function allocates pages from EFI SMRAM Memory as needed to grow the 
  requested pool type.  All allocations are eight-byte aligned.

  @param  This Protocol instance pointer.
  @param  PoolType The type of pool to allocate.
  The only supported type is EfiRuntimeServicesData;
  the interface will internally map this runtime request to SMRAM.
  @param  Size The number of bytes to allocate from the pool.
  @param  Buffer A pointer to a pointer to the allocated buffer if the call
  succeeds; undefined otherwise.

  @retval  EFI_SUCCESS The requested number of bytes was allocated.
  @retval  EFI_OUT_OF_RESOURCES The pool requested could not be allocated.
  @retval  EFI_INVALID_PARAMETER PoolType was invalid.
  @retval  EFI_UNSUPPORTED In runtime.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_ALLOCATE_POOL) (
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN EFI_MEMORY_TYPE                PoolType,
  IN UINTN                          Size,
  OUT VOID                          **Buffer
  )
;

/**
  The SmmFreePool() function returns the memory specified by Buffer to the system.  
  On return, the memory's type is EFI SMRAM Memory.  The Buffer that is freed must 
  have been allocated by SmmAllocatePool().

  @param  This Protocol instance pointer.
  @param  Buffer Pointer to the buffer allocation.

  @retval  EFI_SUCCESS The memory was returned to the system.
  @retval  EFI_INVALID_PARAMETER Buffer was invalid.
  @retval  EFI_UNSUPPORTED In runtime.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_FREE_POOL) (
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN VOID                           *Buffer
  )
;

/**
  This routine tells caller if execution context is SMM or not.

  @param  This Protocol instance pointer.
  @param  InSmm Whether the caller is inside SMM for IA-32 or servicing a PMI for the Itanium processor family.

  @retval  EFI_SUCCESS The operation was successful

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_INSIDE_OUT) (
  IN EFI_SMM_BASE_PROTOCOL          *This,
  OUT BOOLEAN                       *InSmm
  )
;

/**
  The GetSmstLocation() function returns the locatin of the System Management 
  Service Table.  The use of the API is such that a driver can discover the 
  location of the SMST in its entry point and then cache it in some driver 
  global variable so that the SMST can be invoked in subsequent callbacks.

  @param  This Protocol instance pointer.
  @param  Smst Pointer to the SMST.

  @retval  EFI_SUCCESS The operation was successful
  @retval  EFI_INVALID_PARAMETER Smst was invalid.
  @retval  EFI_UNSUPPORTED Not in SMM.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GET_SMST_LOCATION) (
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN OUT EFI_SMM_SYSTEM_TABLE       **Smst
  )
;

/**
  @par Protocol Description:
  This protocol is used to install SMM handlers for support of subsequent SMI/PMI 
  activations. This protocol is available on both IA-32 and Itanium-based systems.

  @param Register
  Registers a handler to run in System Management RAM (SMRAM).

  @param UnRegister
  Removes a handler from execution in SMRAM.

  @param Communicate
  Sends/receives a message for a registered handler.

  @param RegisterCallback
  Registers a callback from the constructor.

  @param InSmm
  Detects whether the caller is inside or outside of SMM. SName

  @param SmmAllocatePool
  Allocates SMRAM.

  @param SmmFreePool
  Deallocates SMRAM.

  @param GetSmstLocation
  Retrieves the location of the System Management System Table (SMST).

**/
struct _EFI_SMM_BASE_PROTOCOL {
  EFI_SMM_REGISTER_HANDLER    Register;
  EFI_SMM_UNREGISTER_HANDLER  UnRegister;
  EFI_SMM_COMMUNICATE         Communicate;
  EFI_SMM_CALLBACK_SERVICE    RegisterCallback;
  EFI_SMM_INSIDE_OUT          InSmm;
  EFI_SMM_ALLOCATE_POOL       SmmAllocatePool;
  EFI_SMM_FREE_POOL           SmmFreePool;
  EFI_SMM_GET_SMST_LOCATION   GetSmstLocation;
};

extern EFI_GUID gEfiSmmBaseProtocolGuid;

#endif
