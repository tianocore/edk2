/** @file
  This file declares SMM Base abstraction protocol.
  This protocol is used to install SMM handlers for support of subsequent SMI/PMI activations. This
  protocol is available on both IA-32 and Itanium-based systems.
 
  The EFI_SMM_BASE_PROTOCOL is a set of services that is exported by a processor device. It is
  a required protocol for the platform processor. This protocol can be used in both boot services and
  runtime mode. However, only the following member functions need to exist during runtime:
  - InSmm()
  - Communicate()
  This protocol is responsible for registering the handler services. The order in which the handlers are
  executed is prescribed only with respect to the MakeLast flag in the RegisterCallback()
  service. The driver exports these registration and unregistration services in boot services mode, but
  the registered handlers will execute through the preboot and runtime. The only way to change the
  behavior of a registered driver after ExitBootServices() has been invoked is to use some
  private communication mechanism with the driver to order it to quiesce. This model permits typical
  use cases, such as invoking the handler to enter ACPI mode, where the OS loader would make this
  call before boot services are terminated. On the other hand, handlers for services such as chipset
  workarounds for the century rollover in CMOS should provide commensurate services throughout
  preboot and OS runtime.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This Protocol is defined in Framework of EFI SMM Core Interface Spec
  Version 0.9.

**/

#ifndef _SMM_BASE_H_
#define _SMM_BASE_H_

//
// Share some common definitions with PI SMM
//
#include <Framework/SmmCis.h>
#include <Protocol/SmmCommunication.h>

///
/// Global ID for the EFI_SMM_BASE_PROTOCOL.
///
#define EFI_SMM_BASE_PROTOCOL_GUID \
  { \
    0x1390954D, 0xda95, 0x4227, {0x93, 0x28, 0x72, 0x82, 0xc2, 0x17, 0xda, 0xa8 } \
  }

///
/// Forward declaration for EFI_SMM_BASE_PROTOCOL.
///
typedef struct _EFI_SMM_BASE_PROTOCOL  EFI_SMM_BASE_PROTOCOL;

///
/// EFI SMM Handler return codes
///
///@{
#define EFI_HANDLER_SUCCESS         0x0000
#define EFI_HANDLER_CRITICAL_EXIT   0x0001
#define EFI_HANDLER_SOURCE_QUIESCED 0x0002
#define EFI_HANDLER_SOURCE_PENDING  0x0003
///@}

/**
  Entry Point to Callback service

  @param[in]  SmmImageHandle        A handle allocated by the SMM infrastructure code
                                    to uniquely designate a specific DXE SMM driver.
  @param[in]  CommunicationBuffer   A pointer to a collection of data in memory
                                    that will be conveyed from a non-SMM environment 
                                    into an SMM environment. The buffer must be 
                                    contiguous and physically mapped, and must be 
                                    a physical address.
  @param[in]  SourceSize            The size of the CommunicationBuffer.

  @return     Status code

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CALLBACK_ENTRY_POINT)(
  IN EFI_HANDLE             SmmImageHandle,
  IN OUT VOID               *CommunicationBuffer OPTIONAL,
  IN OUT UINTN              *SourceSize OPTIONAL
  );

//
// SMM Base Protocol Definition
//
/**
  Register a given driver into SMRAM. This is the equivalent of performing
  the LoadImage/StartImage into System Management Mode.

  @param[in]   This                  The protocol instance pointer.
  @param[in]   FilePath              The location of the image to be installed as the handler.
  @param[in]   SourceBuffer          An optional source buffer in case the image file
                                     is in memory.
  @param[in]   SourceSize            The size of the source image file, if in memory.
  @param[out]  ImageHandle           The handle that the base driver uses to decode 
                                     the handler. Unique among SMM handlers only; 
                                     not unique across DXE/EFI.
  @param[in]   LegacyIA32Binary      An optional parameter specifying that the associated 
                                     file is a real-mode IA-32 binary.

  @retval      EFI_SUCCESS           The operation was successful.
  @retval      EFI_OUT_OF_RESOURCES  There were no additional SMRAM resources to load the handler
  @retval      EFI_UNSUPPORTED       This platform does not support 16-bit handlers.
  @retval      EFI_UNSUPPORTED       The platform is in runtime.
  @retval      EFI_INVALID_PARAMETER The handlers were not the correct image type.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_REGISTER_HANDLER)(
  IN  EFI_SMM_BASE_PROTOCOL                          *This,
  IN  EFI_DEVICE_PATH_PROTOCOL                       *FilePath,
  IN  VOID                                           *SourceBuffer OPTIONAL,
  IN  UINTN                                          SourceSize,
  OUT EFI_HANDLE                                     *ImageHandle,
  IN  BOOLEAN                                        LegacyIA32Binary OPTIONAL
  );

/**
  Removes a handler from execution within SMRAM.  This is the equivalent of performing
  the UnloadImage in System Management Mode.

  @param[in]  This                  The protocol instance pointer.
  @param[in]  ImageHandle           The handler to be removed.

  @retval     EFI_SUCCESS           The operation was successful.
  @retval     EFI_INVALID_PARAMETER The handler did not exist.
  @retval     EFI_UNSUPPORTED       The platform is in runtime.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_UNREGISTER_HANDLER)(
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN EFI_HANDLE                     ImageHandle
  );

/**
  The SMM Inter-module Communicate Service Communicate() function
  provides a service to send/receive messages from a registered
  EFI service.  The BASE protocol driver is responsible for doing
  any of the copies such that the data lives in boot-service-accessible RAM.

  @param[in]      This                  The protocol instance pointer.
  @param[in]      ImageHandle           The handle of the registered driver.
  @param[in,out]  CommunicationBuffer   The pointer to the buffer to convey into SMRAM.
  @param[in,out]  SourceSize            The size of the data buffer being passed in.
                                        On exit, the size of data being returned.
                                        Zero if the handler does not wish to reply with any data.

  @retval         EFI_SUCCESS           The message was successfully posted.
  @retval         EFI_INVALID_PARAMETER The buffer was NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_COMMUNICATE)(
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN EFI_HANDLE                     ImageHandle,
  IN OUT VOID                       *CommunicationBuffer,
  IN OUT UINTN                      *SourceSize
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

  @retval     EFI_SUCCESS           The operation was successful.
  @retval     EFI_OUT_OF_RESOURCES  Not enough space in the dispatch queue.
  @retval     EFI_UNSUPPORTED       The platform is in runtime.
  @retval     EFI_UNSUPPORTED       The caller is not in SMM.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CALLBACK_SERVICE)(
  IN EFI_SMM_BASE_PROTOCOL                            *This,
  IN EFI_HANDLE                                       SmmImageHandle,
  IN EFI_SMM_CALLBACK_ENTRY_POINT                     CallbackAddress,
  IN BOOLEAN                                          MakeLast OPTIONAL,
  IN BOOLEAN                                          FloatingPointSave OPTIONAL
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
  @retval      EFI_UNSUPPORTED       The platform is in runtime.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_ALLOCATE_POOL)(
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN EFI_MEMORY_TYPE                PoolType,
  IN UINTN                          Size,
  OUT VOID                          **Buffer
  );

/**
  The SmmFreePool() function returns the memory specified by Buffer to the system.
  On return, the memory's type is EFI SMRAM Memory.  The Buffer that is freed must
  have been allocated by SmmAllocatePool().

  @param[in]  This                  The protocol instance pointer.
  @param[in]  Buffer                The pointer to the buffer allocation.

  @retval     EFI_SUCCESS           The memory was returned to the system.
  @retval     EFI_INVALID_PARAMETER The buffer was invalid.
  @retval     EFI_UNSUPPORTED       The platform is in runtime.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_FREE_POOL)(
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN VOID                           *Buffer
  );

/**
  This routine tells caller if execution context is SMM or not.

  @param[in]   This                   The protocol instance pointer.
  @param[out]  InSmm                  Whether the caller is inside SMM for IA-32
                                      or servicing a PMI for the Itanium processor
                                      family.

  @retval      EFI_SUCCESS            The operation was successful.
  @retval      EFI_INVALID_PARAMETER  InSmm was NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_INSIDE_OUT)(
  IN EFI_SMM_BASE_PROTOCOL          *This,
  OUT BOOLEAN                       *InSmm
  );

/**
  The GetSmstLocation() function returns the location of the System Management
  Service Table.  The use of the API is such that a driver can discover the
  location of the SMST in its entry point and then cache it in some driver
  global variable so that the SMST can be invoked in subsequent callbacks.

  @param[in]  This                  The protocol instance pointer.
  @param[in]  Smst                  The pointer to the SMST.

  @retval     EFI_SUCCESS           The operation was successful
  @retval     EFI_INVALID_PARAMETER Smst was invalid.
  @retval     EFI_UNSUPPORTED       Not in SMM.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_GET_SMST_LOCATION)(
  IN EFI_SMM_BASE_PROTOCOL          *This,
  IN OUT EFI_SMM_SYSTEM_TABLE       **Smst
  );

///
/// This protocol is used to install SMM handlers for support of subsequent SMI/PMI
/// activations. This protocol is available on both IA-32 and Itanium-based systems.
///
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
