/**
**/
/**

Copyright (c) 2011  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



  @file
  PchReset.h

  @brief
  PCH Reset Protocol

**/
#ifndef _PCH_RESET_H_
#define _PCH_RESET_H_


//
#define PCH_RESET_PROTOCOL_GUID \
  { \
    0xdb63592c, 0xb8cc, 0x44c8, 0x91, 0x8c, 0x51, 0xf5, 0x34, 0x59, 0x8a, 0x5a \
  }
#define PCH_RESET_CALLBACK_PROTOCOL_GUID \
  { \
    0x3a3300ab, 0xc929, 0x487d, 0xab, 0x34, 0x15, 0x9b, 0xc1, 0x35, 0x62, 0xc0 \
  }
extern EFI_GUID                             gPchResetProtocolGuid;
extern EFI_GUID                             gPchResetCallbackProtocolGuid;

///
/// Forward reference for ANSI C compatibility
///
typedef struct _PCH_RESET_PROTOCOL          PCH_RESET_PROTOCOL;

typedef struct _PCH_RESET_CALLBACK_PROTOCOL PCH_RESET_CALLBACK_PROTOCOL;

///
/// Related Definitions
///
///
/// PCH Reset Types
///
typedef enum {
  ColdReset,
  WarmReset,
  ShutdownReset,
  PowerCycleReset,
  GlobalReset,
  GlobalResetWithEc
} PCH_RESET_TYPE;

///
/// Member functions
///
typedef
EFI_STATUS
(EFIAPI *PCH_RESET) (
  IN     PCH_RESET_PROTOCOL       * This,
  IN     PCH_RESET_TYPE           PchResetType
  )
/**

  @brief
  Execute Pch Reset from the host controller.

  @param[in] This                 Pointer to the PCH_RESET_PROTOCOL instance.
  @param[in] PchResetType         Pch Reset Types which includes ColdReset, WarmReset, ShutdownReset,
                                  PowerCycleReset, GlobalReset, GlobalResetWithEc

  @retval EFI_SUCCESS             Successfully completed.
  @retval EFI_INVALID_PARAMETER   If ResetType is invalid.

**/
;

typedef
EFI_STATUS
(EFIAPI *PCH_RESET_CALLBACK) (
  IN     PCH_RESET_TYPE           PchResetType
  )
/**

  @brief
  Execute call back function for Pch Reset.

  @param[in] PchResetType         Pch Reset Types which includes PowerCycle, Globalreset.

  @retval EFI_SUCCESS             The callback function has been done successfully
  @retval EFI_NOT_FOUND           Failed to find Pch Reset Callback protocol. Or, none of
                                  callback protocol is installed.
  @retval Others                  Do not do any reset from PCH

**/
;

///
/// Interface structure for the Pch Reset Protocol
///
struct _PCH_RESET_PROTOCOL {
  PCH_RESET Reset;
};

///
/// PCH_RESET_CALLBACK_PROTOCOL Structure Definition
///
struct _PCH_RESET_CALLBACK_PROTOCOL {
  PCH_RESET_CALLBACK  ResetCallback;
};

#endif
