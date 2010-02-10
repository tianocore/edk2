/** @file
  Include file for SMM Base Protocol on SMM Base2 Protocol Thunk driver.
  
  Copyright (c) 2009 - 2010, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef  _SMM_BASE_ON_SMM_BASE2_THUNK_H_
#define  _SMM_BASE_ON_SMM_BASE2_THUNK_H_

#include <PiDxe.h>
#include <FrameworkSmm.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Guid/SmmBaseThunkCommunication.h>
#include <Guid/EventGroup.h>
#include <Protocol/SmmBase2.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/SmmBaseHelperReady.h>

/**
  Register a given driver into SMRAM. This is the equivalent of performing
  the LoadImage/StartImage into System Management Mode.

  @param[in]   This                  Protocol instance pointer.
  @param[in]   FilePath              Location of the image to be installed as the handler.
  @param[in]   SourceBuffer          Optional source buffer in case the image file
                                     is in memory.
  @param[in]   SourceSize            Size of the source image file, if in memory.
  @param[out]  ImageHandle           The handle that the base driver uses to decode 
                                     the handler. Unique among SMM handlers only, 
                                     not unique across DXE/EFI.
  @param[in]   LegacyIA32Binary      An optional parameter specifying that the associated 
                                     file is a real-mode IA-32 binary.

  @retval      EFI_SUCCESS           The operation was successful.
  @retval      EFI_OUT_OF_RESOURCES  There were no additional SMRAM resources to load the handler
  @retval      EFI_UNSUPPORTED       This platform does not support 16-bit handlers.
  @retval      EFI_UNSUPPORTED       Platform is in runtime.
  @retval      EFI_INVALID_PARAMETER The handlers was not the correct image type
**/
EFI_STATUS
EFIAPI
SmmBaseRegister (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  IN      EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN      VOID                      *SourceBuffer,
  IN      UINTN                     SourceSize,
  OUT     EFI_HANDLE                *ImageHandle,
  IN      BOOLEAN                   LegacyIA32Binary
  );

/**
  Removes a handler from execution within SMRAM.  This is the equivalent of performing
  the UnloadImage in System Management Mode.

  @param[in]  This                  Protocol instance pointer.
  @param[in]  ImageHandle           The handler to be removed.

  @retval     EFI_SUCCESS           The operation was successful
  @retval     EFI_INVALID_PARAMETER The handler did not exist
  @retval     EFI_UNSUPPORTED       Platform is in runtime.
**/
EFI_STATUS
EFIAPI
SmmBaseUnregister (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  IN      EFI_HANDLE                ImageHandle
  );

/**
  The SMM Inter-module Communicate Service Communicate() function
  provides a service to send/receive messages from a registered
  EFI service.  The BASE protocol driver is responsible for doing
  any of the copies such that the data lives in boot-service-accessible RAM.

  @param[in]      This                  Protocol instance pointer.
  @param[in]      ImageHandle           The handle of the registered driver.
  @param[in,out]  CommunicationBuffer   Pointer to the buffer to convey into SMRAM.
  @param[in,out]  BufferSize            The size of the data buffer being passed in.
                                        On exit, the size of data being returned.
                                        Zero if the handler does not wish to reply with any data.

  @retval         EFI_SUCCESS           The message was successfully posted
  @retval         EFI_INVALID_PARAMETER The buffer was NULL
**/
EFI_STATUS
EFIAPI
SmmBaseCommunicate (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  IN      EFI_HANDLE                ImageHandle,
  IN OUT  VOID                      *CommunicationBuffer,
  IN OUT  UINTN                     *BufferSize
  );

/**
  Register a callback to execute within SMM.
  This allows receipt of messages created with EFI_SMM_BASE_PROTOCOL.Communicate().

  @param[in]  This                  Protocol instance pointer.
  @param[in]  SmmImageHandle        Handle of the callback service.
  @param[in]  CallbackAddress       Address of the callback service.
  @param[in]  MakeLast              If present, will stipulate that the handler is posted to 
                                    be executed last in the dispatch table.
  @param[in]  FloatingPointSave     An optional parameter that informs the
                                    EFI_SMM_ACCESS_PROTOCOL Driver core if it needs to save
                                    the floating point register state. If any handler
                                    require this, the state will be saved for all handlers.

  @retval     EFI_SUCCESS           The operation was successful
  @retval     EFI_OUT_OF_RESOURCES  Not enough space in the dispatch queue
  @retval     EFI_UNSUPPORTED       Platform is in runtime.
  @retval     EFI_UNSUPPORTED       The caller is not in SMM.
**/
EFI_STATUS
EFIAPI
SmmBaseRegisterCallback (
  IN      EFI_SMM_BASE_PROTOCOL         *This,
  IN      EFI_HANDLE                    SmmImageHandle,
  IN      EFI_SMM_CALLBACK_ENTRY_POINT  CallbackAddress,
  IN      BOOLEAN                       MakeLast,
  IN      BOOLEAN                       FloatingPointSave
  );

/**
  This routine tells caller if execution context is SMM or not.

  @param[in]   This                   Protocol instance pointer.
  @param[out]  InSmm                  Whether the caller is inside SMM for IA-32
                                      or servicing a PMI for the Itanium processor
                                      family.

  @retval      EFI_SUCCESS            The operation was successful
  @retval      EFI_INVALID_PARAMETER  InSmm was NULL.
**/
EFI_STATUS
EFIAPI
SmmBaseInSmm (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  OUT     BOOLEAN                   *InSmm
  );

/**
  The SmmAllocatePool() function allocates a memory region of Size bytes from memory of
  type PoolType and returns the address of the allocated memory in the location referenced
  by Buffer.  This function allocates pages from EFI SMRAM Memory as needed to grow the
  requested pool type.  All allocations are eight-byte aligned.

  @param[in]   This                  Protocol instance pointer.
  @param[in]   PoolType              The type of pool to allocate.
                                     The only supported type is EfiRuntimeServicesData;
                                     the interface will internally map this runtime request to 
                                     SMRAM for IA-32 and leave as this type for the Itanium 
                                     processor family. Other types can be ignored.
  @param[in]   Size                  The number of bytes to allocate from the pool.
  @param[out]  Buffer                A pointer to a pointer to the allocated buffer if the call
                                     succeeds; undefined otherwise.

  @retval      EFI_SUCCESS           The requested number of bytes was allocated.
  @retval      EFI_OUT_OF_RESOURCES  The pool requested could not be allocated.
  @retval      EFI_UNSUPPORTED       Platform is in runtime.
**/
EFI_STATUS
EFIAPI
SmmBaseSmmAllocatePool (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  IN      EFI_MEMORY_TYPE           PoolType,
  IN      UINTN                     Size,
  OUT     VOID                      **Buffer
  );

/**
  The SmmFreePool() function returns the memory specified by Buffer to the system.
  On return, the memory's type is EFI SMRAM Memory.  The Buffer that is freed must
  have been allocated by SmmAllocatePool().

  @param[in]  This                  Protocol instance pointer.
  @param[in]  Buffer                Pointer to the buffer allocation.

  @retval     EFI_SUCCESS           The memory was returned to the system.
  @retval     EFI_INVALID_PARAMETER Buffer was invalid.
  @retval     EFI_UNSUPPORTED       Platform is in runtime.
**/
EFI_STATUS
EFIAPI
SmmBaseSmmFreePool (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  IN      VOID                      *Buffer
  );

/**
  The GetSmstLocation() function returns the location of the System Management
  Service Table.  The use of the API is such that a driver can discover the
  location of the SMST in its entry point and then cache it in some driver
  global variable so that the SMST can be invoked in subsequent callbacks.

  @param[in]  This                  Protocol instance pointer.
  @param[in]  Smst                  Pointer to the SMST.

  @retval     EFI_SUCCESS           The operation was successful
  @retval     EFI_INVALID_PARAMETER Smst was invalid.
  @retval     EFI_UNSUPPORTED       Not in SMM.
**/
EFI_STATUS
EFIAPI
SmmBaseGetSmstLocation (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  OUT     EFI_SMM_SYSTEM_TABLE      **Smst
  );

#endif  
