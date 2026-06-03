/** @file
  Arm CCA Boot Sync Debug interfaces.

  Copyright (c) 2023, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/DebugLib.h>
#include "Include/BootSyncSecureChannel.h"

#if !defined (MDEPKG_NDEBUG)

/**
  Performs a raw data dump of the buffer.

  @param [in] Desc    Description of the buffer.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  The length of the buffer.
**/
VOID
EFIAPI
DumpRaw (
  IN CHAR8   *Desc,
  IN UINT8   *Ptr,
  IN UINT32  Length
  );

/**
  Prints the rolling hash of the messages exchanged so far.

  @param[in] SecChannel   Pointer to the secure channel.
**/
VOID
PrintHash (
  IN SECURE_CHANNEL  *SecChannel
  );

/**
  Prints the rolling hash of the messages exchanged so far.

  @param[in] Desc         A text describing the secure channel.
  @param[in] SecChannel   Pointer to the secure channel.
**/
VOID
DumpKeys (
  IN CHAR8           *Desc,
  IN SECURE_CHANNEL  *SecChannel
  );

/**
  Prints the message info.

  @param[in] Guid         Message GUID.
  @param[in] IsSend       Message is sent or received.
                          TRUE  - Send
                          FALSE - Receive
**/
VOID
EFIAPI
PrintMessageInfo (
  EFI_GUID  *Guid,
  BOOLEAN   IsSend
  );

/**
  Performs a raw data dump of the buffer.

  @param [in] Desc    Description of the buffer.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  The length of the buffer.
**/
#define DBG_DUMP_RAW(Desc, Ptr, Length)  {      \
    if (DebugPrintLevelEnabled (DEBUG_VERBOSE)) {  \
      DumpRaw (Desc, Ptr, Length);              \
    }                                           \
  }

/**
  Prints the rolling hash of the messages exchanged so far.

  @param[in] SecChannel   Pointer to the secure channel.
**/
#define DBG_PRINT_HASH(SecChannel)  {           \
    if (DebugPrintLevelEnabled (DEBUG_VERBOSE)) {  \
      PrintHash (SecChannel);                   \
    }                                           \
  }

/**
  Prints the rolling hash of the messages exchanged so far.

  @param[in] Desc         A text describing the secure channel.
  @param[in] SecChannel   Pointer to the secure channel.
**/
#define DBG_DUMP_KEYS(Desc, SecChannel)  {      \
    if (DebugPrintLevelEnabled (DEBUG_VERBOSE)) {  \
      DumpKeys (Desc, SecChannel);              \
    }                                           \
  }

/**
  Prints the message info.

  @param[in] Guid         Message GUID.
  @param[in] IsSend       Message is sent or received.
                          TRUE  - Send
                          FALSE - Receive
**/
#define DBG_PRINT_MSGINFO(Guid, IsSend)  {      \
    if (DebugPrintLevelEnabled (DEBUG_VERBOSE)) {  \
      PrintMessageInfo (Guid, IsSend);          \
    }                                           \
  }
#else

/**
  Performs a raw data dump of the buffer.

  @param [in] Desc    Description of the buffer.
  @param [in] Ptr     Pointer to the start of the buffer.
  @param [in] Length  The length of the buffer.
**/
#define DBG_DUMP_RAW(Desc, Ptr, Length)

/**
  Prints the rolling hash of the messages exchanged so far.

  @param[in] SecChannel   Pointer to the secure channel.
**/
#define DBG_PRINT_HASH(SecChannel)

/**
  Prints the rolling hash of the messages exchanged so far.

  @param[in] Desc         A text describing the secure channel.
  @param[in] SecChannel   Pointer to the secure channel.
**/
#define DBG_DUMP_KEYS(Desc, SecChannel)

/**
  Prints the message info.

  @param[in] Guid         Message GUID.
  @param[in] IsSend       Message is sent or received.
                          TRUE  - Send
                          FALSE - Receive
**/
#define DBG_PRINT_MSGINFO(Guid, IsSend)

#endif // if defined (MDEPKG_NDEBUG)
