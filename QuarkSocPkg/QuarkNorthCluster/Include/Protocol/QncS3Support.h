/** @file
This file defines the QNC S3 support Protocol.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/
#ifndef _QNC_S3_SUPPORT_PROTOCOL_H_
#define _QNC_S3_SUPPORT_PROTOCOL_H_

//
// Extern the GUID for protocol users.
//
extern EFI_GUID                             gEfiQncS3SupportProtocolGuid;

//
// Forward reference for ANSI C compatibility
//
typedef struct _EFI_QNC_S3_SUPPORT_PROTOCOL EFI_QNC_S3_SUPPORT_PROTOCOL;

typedef enum {
  QncS3ItemTypeInitPcieRootPortDownstream,
  QncS3ItemTypeMax
} EFI_QNC_S3_DISPATCH_ITEM_TYPE;

//
// It's better not to use pointer here because the size of pointer in DXE is 8, but it's 4 in PEI
// plug 4 to ParameterSize in PEIM if you really need it
//
typedef struct {
  UINT32                        Reserved;
} EFI_QNC_S3_PARAMETER_INIT_PCIE_ROOT_PORT_DOWNSTREAM;

typedef union {
  EFI_QNC_S3_PARAMETER_INIT_PCIE_ROOT_PORT_DOWNSTREAM   PcieRootPortData;
} EFI_DISPATCH_CONTEXT_UNION;

typedef struct {
  EFI_QNC_S3_DISPATCH_ITEM_TYPE Type;
  VOID                          *Parameter;
} EFI_QNC_S3_DISPATCH_ITEM;

//
// Member functions
//
typedef
EFI_STATUS
(EFIAPI *EFI_QNC_S3_SUPPORT_SET_S3_DISPATCH_ITEM) (
  IN     EFI_QNC_S3_SUPPORT_PROTOCOL   * This,
  IN     EFI_QNC_S3_DISPATCH_ITEM      * DispatchItem,
  OUT    VOID                         **S3DispatchEntryPoint,
  OUT    VOID                         **Context
  );

/*++

Routine Description:

  Set an item to be dispatched at S3 resume time. At the same time, the entry point
  of the QNC S3 support image is returned to be used in subsequent boot script save
  call

Arguments:

  This                    - Pointer to the protocol instance.
  DispatchItem            - The item to be dispatched.
  S3DispatchEntryPoint    - The entry point of the QNC S3 support image.

Returns:

  EFI_STATUS

--*/

//
// Protocol definition
//
struct _EFI_QNC_S3_SUPPORT_PROTOCOL {
  EFI_QNC_S3_SUPPORT_SET_S3_DISPATCH_ITEM SetDispatchItem;
};

#endif
