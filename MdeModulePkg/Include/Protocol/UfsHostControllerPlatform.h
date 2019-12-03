/** @file
  EDKII_UFS_HC_PLATFORM_PROTOCOL definition.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EDKII_UFS_HC_PLATFORM_PROTOCOL_H__
#define __EDKII_UFS_HC_PLATFORM_PROTOCOL_H__

#include <Protocol/UfsHostController.h>

#define EDKII_UFS_HC_PLATFORM_PROTOCOL_VERSION 1

extern EFI_GUID  gEdkiiUfsHcPlatformProtocolGuid;

typedef struct _EDKII_UFS_HC_PLATFORM_PROTOCOL  EDKII_UFS_HC_PLATFORM_PROTOCOL;

typedef struct _EDKII_UFS_HC_DRIVER_INTERFACE  EDKII_UFS_HC_DRIVER_INTERFACE;

typedef struct {
  UINT32 Opcode;
  UINT32 Arg1;
  UINT32 Arg2;
  UINT32 Arg3;
} EDKII_UIC_COMMAND;

/**
  Execute UIC command

  @param[in]      This        Pointer to driver interface produced by the UFS controller.
  @param[in, out] UicCommand  Descriptor of the command that will be executed.

  @retval EFI_SUCCESS            Command executed successfully.
  @retval EFI_INVALID_PARAMETER  This or UicCommand is NULL.
  @retval Others                 Command failed to execute.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_UFS_EXEC_UIC_COMMAND) (
  IN      EDKII_UFS_HC_DRIVER_INTERFACE  *This,
  IN OUT  EDKII_UIC_COMMAND              *UicCommand
);

struct _EDKII_UFS_HC_DRIVER_INTERFACE {
  ///
  /// Protocol to accesss host controller MMIO and PCI registers.
  ///
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *UfsHcProtocol;
  ///
  /// Function implementing UIC command execution.
  ///
  EDKII_UFS_EXEC_UIC_COMMAND          UfsExecUicCommand;
};

typedef struct {
  UINT32 Capabilities;
  UINT32 Version;
} EDKII_UFS_HC_INFO;

/**
  Allows platform protocol to override host controller information

  @param[in]      ControllerHandle  Handle of the UFS controller.
  @param[in, out] HcInfo            Pointer EDKII_UFS_HC_INFO associated with host controller.

  @retval EFI_SUCCESS            Function completed successfully.
  @retval EFI_INVALID_PARAMETER  HcInfo is NULL.
  @retval Others                 Function failed to complete.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_UFS_HC_PLATFORM_OVERRIDE_HC_INFO) (
  IN     EFI_HANDLE         ControllerHandle,
  IN OUT EDKII_UFS_HC_INFO  *HcInfo
);

typedef enum {
  EdkiiUfsHcPreHce,
  EdkiiUfsHcPostHce,
  EdkiiUfsHcPreLinkStartup,
  EdkiiUfsHcPostLinkStartup
} EDKII_UFS_HC_PLATFORM_CALLBACK_PHASE;

/**
  Callback function for platform driver.

  @param[in]      ControllerHandle  Handle of the UFS controller.
  @param[in]      CallbackPhase     Specifies when the platform protocol is called
  @param[in, out] CallbackData      Data specific to the callback phase.
                                    For PreHce and PostHce - EDKII_UFS_HC_DRIVER_INTERFACE.
                                    For PreLinkStartup and PostLinkStartup - EDKII_UFS_HC_DRIVER_INTERFACE.

  @retval EFI_SUCCESS            Override function completed successfully.
  @retval EFI_INVALID_PARAMETER  CallbackPhase is invalid or CallbackData is NULL when phase expects valid data.
  @retval Others                 Function failed to complete.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_UFS_HC_PLATFORM_CALLBACK) (
  IN     EFI_HANDLE                            ControllerHandle,
  IN     EDKII_UFS_HC_PLATFORM_CALLBACK_PHASE  CallbackPhase,
  IN OUT VOID                                  *CallbackData
);

struct _EDKII_UFS_HC_PLATFORM_PROTOCOL {
  ///
  /// Version of the protocol.
  ///
  UINT32                                  Version;
  ///
  /// Allows platform driver to override host controller information.
  ///
  EDKII_UFS_HC_PLATFORM_OVERRIDE_HC_INFO  OverrideHcInfo;
  ///
  /// Allows platform driver to implement platform specific flows
  /// for host controller.
  ///
  EDKII_UFS_HC_PLATFORM_CALLBACK          Callback;
};

#endif

