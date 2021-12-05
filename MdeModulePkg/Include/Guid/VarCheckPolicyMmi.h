/** @file -- VarCheckPolicyMmiCommon.h
This header contains communication definitions that are shared between DXE
and the MM component of VarCheckPolicy.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _VAR_CHECK_POLICY_MMI_COMMON_H_
#define _VAR_CHECK_POLICY_MMI_COMMON_H_

#define   VAR_CHECK_POLICY_COMM_SIG       SIGNATURE_32('V', 'C', 'P', 'C')
#define   VAR_CHECK_POLICY_COMM_REVISION  1

#pragma pack(push, 1)

typedef struct _VAR_CHECK_POLICY_COMM_HEADER {
  UINT32        Signature;
  UINT32        Revision;
  UINT32        Command;
  EFI_STATUS    Result;
} VAR_CHECK_POLICY_COMM_HEADER;

typedef struct _VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS {
  BOOLEAN    State;
} VAR_CHECK_POLICY_COMM_IS_ENABLED_PARAMS;

typedef struct _VAR_CHECK_POLICY_COMM_DUMP_PARAMS {
  UINT32     PageRequested;
  UINT32     TotalSize;
  UINT32     PageSize;
  BOOLEAN    HasMore;
} VAR_CHECK_POLICY_COMM_DUMP_PARAMS;

#pragma pack(pop)

// Make sure that we will hold at least the headers.
#define   VAR_CHECK_POLICY_MM_COMM_BUFFER_SIZE  MAX((OFFSET_OF(EFI_MM_COMMUNICATE_HEADER, Data) + sizeof (VAR_CHECK_POLICY_COMM_HEADER) + EFI_PAGES_TO_SIZE(1)), EFI_PAGES_TO_SIZE(4))
#define   VAR_CHECK_POLICY_MM_DUMP_BUFFER_SIZE  (VAR_CHECK_POLICY_MM_COMM_BUFFER_SIZE - \
                                                    (OFFSET_OF(EFI_MM_COMMUNICATE_HEADER, Data) + \
                                                      sizeof(VAR_CHECK_POLICY_COMM_HEADER) + \
                                                      sizeof(VAR_CHECK_POLICY_COMM_DUMP_PARAMS)))
STATIC_ASSERT (
  VAR_CHECK_POLICY_MM_DUMP_BUFFER_SIZE < VAR_CHECK_POLICY_MM_COMM_BUFFER_SIZE,
  "an integer underflow may have occurred calculating VAR_CHECK_POLICY_MM_DUMP_BUFFER_SIZE"
  );

#define   VAR_CHECK_POLICY_COMMAND_DISABLE     0x0001
#define   VAR_CHECK_POLICY_COMMAND_IS_ENABLED  0x0002
#define   VAR_CHECK_POLICY_COMMAND_REGISTER    0x0003
#define   VAR_CHECK_POLICY_COMMAND_DUMP        0x0004
#define   VAR_CHECK_POLICY_COMMAND_LOCK        0x0005

#endif // _VAR_CHECK_POLICY_MMI_COMMON_H_
