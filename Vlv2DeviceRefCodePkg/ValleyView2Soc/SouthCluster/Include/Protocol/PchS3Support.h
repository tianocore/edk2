/**
**/
/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



  @file
  PchS3Support.h

  @brief
  This file defines the PCH S3 support Protocol.

**/
#ifndef _PCH_S3_SUPPORT_PROTOCOL_H_
#define _PCH_S3_SUPPORT_PROTOCOL_H_

#ifndef ECP_FLAG
#include <Pi/PiS3BootScript.h>
#endif

#define EFI_PCH_S3_SUPPORT_PROTOCOL_GUID \
  { \
    0xe287d20b, 0xd897, 0x4e1e, 0xa5, 0xd9, 0x97, 0x77, 0x63, 0x93, 0x6a, 0x4 \
  }

#include <Protocol/PchPlatformPolicy.h>

///
/// Extern the GUID for protocol users.
///
extern EFI_GUID                             gEfiPchS3SupportProtocolGuid;

///
/// Forward reference for ANSI C compatibility
///
typedef struct _EFI_PCH_S3_SUPPORT_PROTOCOL EFI_PCH_S3_SUPPORT_PROTOCOL;

typedef enum {
  PchS3ItemTypeSendCodecCommand,
  PchS3ItemTypePollStatus,
  PchS3ItemTypeInitPcieRootPortDownstream,
  PchS3ItemTypePcieSetPm,
  PchS3ItemTypePmTimerStall,
  PchS3ItemTypeMax
} EFI_PCH_S3_DISPATCH_ITEM_TYPE;

///
/// It's better not to use pointer here because the size of pointer in DXE is 8, but it's 4 in PEI
/// plug 4 to ParameterSize in PEIM if you really need it
///
typedef struct {
  UINT32                        HdaBar;
  UINT32                        CodecCmdData;
} EFI_PCH_S3_PARAMETER_SEND_CODEC_COMMAND;

typedef struct {
  UINT64                        MmioAddress;
  EFI_BOOT_SCRIPT_WIDTH         Width;
  UINT64                        Mask;
  UINT64                        Value;
  UINT32                        Timeout;  // us
} EFI_PCH_S3_PARAMETER_POLL_STATUS;

typedef struct {
  UINT8                         RootPortBus;
  UINT8                         RootPortDevice;
  UINT8                         RootPortFunc;
  UINT8                         TempBusNumberMin;
  UINT8                         TempBusNumberMax;
} EFI_PCH_S3_PARAMETER_INIT_PCIE_ROOT_PORT_DOWNSTREAM;

typedef struct {
  UINT8                         RootPortBus;
  UINT8                         RootPortDevice;
  UINT8                         RootPortFunc;
  PCH_PCI_EXPRESS_ASPM_CONTROL  RootPortAspm;
  UINT8                         NumOfDevAspmOverride;
  UINT32                        DevAspmOverrideAddr;
  UINT8                         TempBusNumberMin;
  UINT8                         TempBusNumberMax;
  UINT8                         NumOfDevLtrOverride;
  UINT32                        DevLtrOverrideAddr;
} EFI_PCH_S3_PARAMETER_PCIE_SET_PM;

typedef struct {
  UINT32                        DelayTime;  // us
} EFI_PCH_S3_PARAMETER_PM_TIMER_STALL;

typedef struct {
  EFI_PCH_S3_DISPATCH_ITEM_TYPE Type;
  VOID                          *Parameter;
} EFI_PCH_S3_DISPATCH_ITEM;

///
/// Member functions
///
typedef
EFI_STATUS
(EFIAPI *EFI_PCH_S3_SUPPORT_SET_S3_DISPATCH_ITEM) (
  IN     EFI_PCH_S3_SUPPORT_PROTOCOL   * This,
  IN     EFI_PCH_S3_DISPATCH_ITEM      * DispatchItem,
  OUT    EFI_PHYSICAL_ADDRESS          * S3DispatchEntryPoint
  );

/**

  @brief
  Set an item to be dispatched at S3 resume time. At the same time, the entry point
  of the PCH S3 support image is returned to be used in subsequent boot script save
  call

  @param[in] This                 Pointer to the protocol instance.
  @param[in] DispatchItem         The item to be dispatched.
  @param[in] S3DispatchEntryPoint The entry point of the PCH S3 support image.

  @retval EFI_STATUS              Successfully completed.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.

**/

///
/// Protocol definition
///
struct _EFI_PCH_S3_SUPPORT_PROTOCOL {
  EFI_PCH_S3_SUPPORT_SET_S3_DISPATCH_ITEM SetDispatchItem;
};

#endif
