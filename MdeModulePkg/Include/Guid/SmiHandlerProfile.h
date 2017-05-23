/** @file
  Header file for SMI handler profile definition.

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef  _SMI_HANDLER_PROFILE_H_
#define  _SMI_HANDLER_PROFILE_H_

#include <PiSmm.h>
#include <Protocol/SmmGpiDispatch2.h>
#include <Protocol/SmmIoTrapDispatch2.h>
#include <Protocol/SmmPeriodicTimerDispatch2.h>
#include <Protocol/SmmPowerButtonDispatch2.h>
#include <Protocol/SmmStandbyButtonDispatch2.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmSxDispatch2.h>
#include <Protocol/SmmUsbDispatch2.h>

typedef struct {
  UINT32                       Signature;
  UINT32                       Length;
  UINT32                       Revision;
  UINT8                        Reserved[4];
} SMM_CORE_DATABASE_COMMON_HEADER;

#define SMM_CORE_IMAGE_DATABASE_SIGNATURE SIGNATURE_32 ('S','C','I','D')
#define SMM_CORE_IMAGE_DATABASE_REVISION  0x0001

typedef struct {
  SMM_CORE_DATABASE_COMMON_HEADER     Header;
  EFI_GUID                            FileGuid;
  PHYSICAL_ADDRESS                    EntryPoint;
  PHYSICAL_ADDRESS                    ImageBase;
  UINT64                              ImageSize;
  UINT32                              ImageRef;
  UINT16                              PdbStringOffset;
  UINT8                               Reserved[2];
//CHAR8                               PdbString[];
} SMM_CORE_IMAGE_DATABASE_STRUCTURE;

#define SMM_CORE_SMI_DATABASE_SIGNATURE SIGNATURE_32 ('S','C','S','D')
#define SMM_CORE_SMI_DATABASE_REVISION  0x0001

typedef enum {
  SmmCoreSmiHandlerCategoryRootHandler,
  SmmCoreSmiHandlerCategoryGuidHandler,
  SmmCoreSmiHandlerCategoryHardwareHandler,
} SMM_CORE_SMI_HANDLER_CATEGORY;

//
// Context for SmmCoreSmiHandlerCategoryRootHandler:
//   NULL
// Context for SmmCoreSmiHandlerCategoryGuidHandler:
//   NULL
// Context for SmmCoreSmiHandlerCategoryHardwareHandler:
//   (NOTE: The context field should NOT include any data pointer.)
//   gEfiSmmSwDispatch2ProtocolGuid:            (EFI_SMM_SW_REGISTER_CONTEXT => SMI_HANDLER_PROFILE_SW_REGISTER_CONTEXT)
//   gEfiSmmSxDispatch2ProtocolGuid:            EFI_SMM_SX_REGISTER_CONTEXT
//   gEfiSmmPowerButtonDispatch2ProtocolGuid:   EFI_SMM_POWER_BUTTON_REGISTER_CONTEXT
//   gEfiSmmStandbyButtonDispatch2ProtocolGuid: EFI_SMM_STANDBY_BUTTON_REGISTER_CONTEXT
//   gEfiSmmPeriodicTimerDispatch2ProtocolGuid: EFI_SMM_PERIODIC_TIMER_CONTEXT
//   gEfiSmmGpiDispatch2ProtocolGuid:           EFI_SMM_GPI_REGISTER_CONTEXT
//   gEfiSmmIoTrapDispatch2ProtocolGuid:        EFI_SMM_IO_TRAP_REGISTER_CONTEXT
//   gEfiSmmUsbDispatch2ProtocolGuid:           (EFI_SMM_USB_REGISTER_CONTEXT => SMI_HANDLER_PROFILE_USB_REGISTER_CONTEXT)
//   Other:                                     GUID specific

typedef struct {
  EFI_USB_SMI_TYPE          Type;
  UINT32                    DevicePathSize;
//UINT8                     DevicePath[DevicePathSize];
} SMI_HANDLER_PROFILE_USB_REGISTER_CONTEXT;

typedef struct {
  UINT64                    SwSmiInputValue;
} SMI_HANDLER_PROFILE_SW_REGISTER_CONTEXT;

typedef struct {
  UINT32                Length;
  UINT32                ImageRef;
  PHYSICAL_ADDRESS      CallerAddr;
  PHYSICAL_ADDRESS      Handler;
  UINT16                ContextBufferOffset;
  UINT8                 Reserved[2];
  UINT32                ContextBufferSize;
//UINT8                 ContextBuffer[];
} SMM_CORE_SMI_HANDLER_STRUCTURE;

typedef struct {
  SMM_CORE_DATABASE_COMMON_HEADER     Header;
  EFI_GUID                            HandlerType;
  UINT32                              HandlerCategory;
  UINT32                              HandlerCount;
//SMM_CORE_SMI_HANDLER_STRUCTURE      Handler[HandlerCount];
} SMM_CORE_SMI_DATABASE_STRUCTURE;

//
// Layout:
// +-------------------------------------+
// | SMM_CORE_IMAGE_DATABASE_STRUCTURE   |
// +-------------------------------------+
// | SMM_CORE_SMI_DATABASE_STRUCTURE     |
// +-------------------------------------+
//



//
// SMM_CORE dump command
//
#define SMI_HANDLER_PROFILE_COMMAND_GET_INFO           0x1
#define SMI_HANDLER_PROFILE_COMMAND_GET_DATA_BY_OFFSET 0x2

typedef struct {
  UINT32                            Command;
  UINT32                            DataLength;
  UINT64                            ReturnStatus;
} SMI_HANDLER_PROFILE_PARAMETER_HEADER;

typedef struct {
  SMI_HANDLER_PROFILE_PARAMETER_HEADER    Header;
  UINT64                                  DataSize;
} SMI_HANDLER_PROFILE_PARAMETER_GET_INFO;

typedef struct {
  SMI_HANDLER_PROFILE_PARAMETER_HEADER    Header;
  //
  // On input, data buffer size.
  // On output, actual data buffer size copied.
  //
  UINT64                                  DataSize;
  PHYSICAL_ADDRESS                        DataBuffer;
  //
  // On input, data buffer offset to copy.
  // On output, next time data buffer offset to copy.
  //
  UINT64                                  DataOffset;
} SMI_HANDLER_PROFILE_PARAMETER_GET_DATA_BY_OFFSET;

#define SMI_HANDLER_PROFILE_GUID {0x49174342, 0x7108, 0x409b, {0x8b, 0xbe, 0x65, 0xfd, 0xa8, 0x53, 0x89, 0xf5}}

extern EFI_GUID gSmiHandlerProfileGuid;

typedef struct _SMI_HANDLER_PROFILE_PROTOCOL  SMI_HANDLER_PROFILE_PROTOCOL;

/**
  This function is called by SmmChildDispatcher module to report
  a new SMI handler is registered, to SmmCore.

  @param This            The protocol instance
  @param HandlerGuid     The GUID to identify the type of the handler.
                         For the SmmChildDispatch protocol, the HandlerGuid
                         must be the GUID of SmmChildDispatch protocol.
  @param Handler         The SMI handler.
  @param CallerAddress   The address of the module who registers the SMI handler.
  @param Context         The context of the SMI handler.
                         For the SmmChildDispatch protocol, the Context
                         must match the one defined for SmmChildDispatch protocol.
  @param ContextSize     The size of the context in bytes.
                         For the SmmChildDispatch protocol, the Context
                         must match the one defined for SmmChildDispatch protocol.

  @retval EFI_SUCCESS           The information is recorded.
  @retval EFI_OUT_OF_RESOURCES  There is no enough resource to record the information.
**/
typedef
EFI_STATUS
(EFIAPI  *SMI_HANDLER_PROFILE_REGISTER_HANDLER) (
  IN SMI_HANDLER_PROFILE_PROTOCOL   *This,
  IN EFI_GUID                       *HandlerGuid,
  IN EFI_SMM_HANDLER_ENTRY_POINT2   Handler,
  IN PHYSICAL_ADDRESS               CallerAddress,
  IN VOID                           *Context, OPTIONAL
  IN UINTN                          ContextSize OPTIONAL
  );

/**
  This function is called by SmmChildDispatcher module to report
  an existing SMI handler is unregistered, to SmmCore.

  @param This            The protocol instance
  @param HandlerGuid     The GUID to identify the type of the handler.
                         For the SmmChildDispatch protocol, the HandlerGuid
                         must be the GUID of SmmChildDispatch protocol.
  @param Handler         The SMI handler.
  @param Context         The context of the SMI handler.
                         If it is NOT NULL, it will be used to check what is registered.
  @param ContextSize     The size of the context in bytes.
                         If Context is NOT NULL, it will be used to check what is registered.

  @retval EFI_SUCCESS           The original record is removed.
  @retval EFI_NOT_FOUND         There is no record for the HandlerGuid and handler.
**/
typedef
EFI_STATUS
(EFIAPI  *SMI_HANDLER_PROFILE_UNREGISTER_HANDLER) (
  IN SMI_HANDLER_PROFILE_PROTOCOL   *This,
  IN EFI_GUID                       *HandlerGuid,
  IN EFI_SMM_HANDLER_ENTRY_POINT2   Handler,
  IN VOID                           *Context, OPTIONAL
  IN UINTN                          ContextSize OPTIONAL
  );

struct _SMI_HANDLER_PROFILE_PROTOCOL {
  SMI_HANDLER_PROFILE_REGISTER_HANDLER     RegisterHandler;
  SMI_HANDLER_PROFILE_UNREGISTER_HANDLER   UnregisterHandler;
};

#endif
