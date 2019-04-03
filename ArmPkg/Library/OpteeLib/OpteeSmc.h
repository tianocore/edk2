/** @file
  OP-TEE SMC header file.

  Copyright (c) 2018, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _OPTEE_SMC_H_
#define _OPTEE_SMC_H_

/* Returned in Arg0 only from Trusted OS functions */
#define OPTEE_SMC_RETURN_OK                     0x0

#define OPTEE_SMC_RETURN_FROM_RPC               0x32000003
#define OPTEE_SMC_CALL_WITH_ARG                 0x32000004
#define OPTEE_SMC_GET_SHARED_MEMORY_CONFIG      0xb2000007

#define OPTEE_SMC_SHARED_MEMORY_CACHED          1

#define OPTEE_SMC_RETURN_UNKNOWN_FUNCTION       0xffffffff
#define OPTEE_SMC_RETURN_RPC_PREFIX_MASK        0xffff0000
#define OPTEE_SMC_RETURN_RPC_PREFIX             0xffff0000
#define OPTEE_SMC_RETURN_RPC_FOREIGN_INTERRUPT  0xffff0004

#define OPTEE_MESSAGE_COMMAND_OPEN_SESSION      0
#define OPTEE_MESSAGE_COMMAND_INVOKE_FUNCTION   1
#define OPTEE_MESSAGE_COMMAND_CLOSE_SESSION     2

#define OPTEE_MESSAGE_ATTRIBUTE_META            0x100

#define OPTEE_LOGIN_PUBLIC                      0x0

typedef struct {
  UINTN    Base;
  UINTN    Size;
} OPTEE_SHARED_MEMORY_INFORMATION;

//
// UUID struct compliant with RFC4122 (network byte order).
//
typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} RFC4122_UUID;

#endif
