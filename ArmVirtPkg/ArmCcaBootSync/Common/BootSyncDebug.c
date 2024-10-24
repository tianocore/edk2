/** @file
  Arm CCA Boot Sync Debug interfaces.

  Copyright (c) 2023, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "Include/BootSyncSecureChannel.h"

#if !defined (MDEPKG_NDEBUG)

/**
  A structure that maps the Boot Sync Message GUIDs to a text description.
**/
STATIC struct {
  EFI_GUID    *Guid;
  CHAR8       *Name;
} MessageMap[] = {
  { &gArmBootSyncFinGuid,         "ArmBootSyncFinGuid"         },
  { &gArmBootSyncKeyEncData,      "ArmBootSyncKeyEncData"      },
  { &gArmBootSyncKeyXchgReqGuid,  "ArmBootSyncKeyXchgReqGuid"  },
  { &gArmBootSyncKeyXchgRespGuid, "ArmBootSyncKeyXchgRespGuid" },
  { &gArmBootSyncNackGuid,        "ArmBootSyncNackGuid"        }
};

/**
  Performs a raw data dump of the buffer.

  @param[in] Desc    Description of the buffer.
  @param[in] Ptr     Pointer to the start of the buffer.
  @param[in] Length  The length of the buffer.
**/
VOID
EFIAPI
DumpRaw (
  IN CHAR8   *Desc,
  IN UINT8   *Ptr,
  IN UINT32  Length
  )
{
  UINTN  ByteCount;
  UINTN  PartLineChars;
  UINTN  AsciiBufferIndex;
  CHAR8  AsciiBuffer[17];

  ByteCount        = 0;
  AsciiBufferIndex = 0;

  DEBUG ((DEBUG_VERBOSE, "%a\n", Desc));
  DEBUG ((DEBUG_VERBOSE, "Address  : 0x%p\n", Ptr));
  DEBUG ((DEBUG_VERBOSE, "Length   : %d\n", Length));

  while (ByteCount < Length) {
    if ((ByteCount & 0x0F) == 0) {
      AsciiBuffer[AsciiBufferIndex] = '\0';
      DEBUG ((DEBUG_VERBOSE, "  %a\n%08X : ", AsciiBuffer, ByteCount));
      AsciiBufferIndex = 0;
    } else if ((ByteCount & 0x07) == 0) {
      DEBUG ((DEBUG_VERBOSE, "- "));
    }

    if ((*Ptr >= ' ') && (*Ptr < 0x7F)) {
      AsciiBuffer[AsciiBufferIndex++] = *Ptr;
    } else {
      AsciiBuffer[AsciiBufferIndex++] = '.';
    }

    DEBUG ((DEBUG_VERBOSE, "%02X ", *Ptr++));

    ByteCount++;
  }

  // Justify the final line using spaces before printing
  // the ASCII data.
  PartLineChars = (Length & 0x0F);
  if (PartLineChars != 0) {
    PartLineChars = 48 - (PartLineChars * 3);
    if ((Length & 0x0F) <= 8) {
      PartLineChars += 2;
    }

    while (PartLineChars > 0) {
      DEBUG ((DEBUG_VERBOSE, " "));
      PartLineChars--;
    }
  }

  // Print ASCII data for the final line.
  AsciiBuffer[AsciiBufferIndex] = '\0';
  DEBUG ((DEBUG_VERBOSE, "  %a\n\n", AsciiBuffer));
}

/**
  Prints the rolling hash of the messages exchanged so far.

  @param[in] SecChannel   Pointer to the secure channel.
**/
VOID
PrintHash (
  IN SECURE_CHANNEL  *SecChannel
  )
{
  UINT8  *Hash;

  ASSERT (SecChannel != NULL);

  Hash = SecChannel->RmHash;
  DEBUG ((
    DEBUG_VERBOSE,
    "RmHash[%p] " \
    "{%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x" \
    "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x}\n",
    SecChannel,
    Hash[0],
    Hash[1],
    Hash[2],
    Hash[3],
    Hash[4],
    Hash[5],
    Hash[6],
    Hash[7],
    Hash[8],
    Hash[9],
    Hash[10],
    Hash[11],
    Hash[12],
    Hash[13],
    Hash[14],
    Hash[15],
    Hash[16],
    Hash[17],
    Hash[18],
    Hash[19],
    Hash[20],
    Hash[21],
    Hash[22],
    Hash[23],
    Hash[24],
    Hash[25],
    Hash[26],
    Hash[27],
    Hash[28],
    Hash[29],
    Hash[30],
    Hash[31]
    ));
}

/**
  Prints the rolling hash of the messages exchanged so far.

  @param[in] Desc         A text describing the secure channel.
  @param[in] SecChannel   Pointer to the secure channel.
**/
VOID
DumpKeys (
  IN CHAR8           *Desc,
  IN SECURE_CHANNEL  *SecChannel
  )
{
  ASSERT (Desc != NULL);
  ASSERT (SecChannel != NULL);
  DEBUG ((DEBUG_VERBOSE, "DumpKeys: %a\n", Desc));
  DEBUG ((DEBUG_VERBOSE, "  SecChannel = %p\n", SecChannel));
  DEBUG ((DEBUG_VERBOSE, "  SessionCtx = %p\n", SecChannel->SessionCtx));
  DEBUG ((DEBUG_VERBOSE, "  PeerSessionCtx = %p\n", SecChannel->PeerSessionCtx));
  DEBUG ((DEBUG_VERBOSE, "  IV Prefix %x", SecChannel->IvPrefix));
  DEBUG ((DEBUG_VERBOSE, "  IV Sequence No %lx", SecChannel->IvSequenceNo));
  DumpRaw ("  SaltKeyBinding", SecChannel->SaltKeyBinding, SALT_SIZE);
  DumpRaw ("  SaltKeyEncryption", SecChannel->SaltKeyEncryption, SALT_SIZE);
  DumpRaw ("  Kb", SecChannel->Kb, BINDING_KEY_SIZE);
  DumpRaw ("  Ke", SecChannel->Ke, ENCRYPTION_KEY_SIZE);
  DumpRaw ("  RmHash", SecChannel->RmHash, ROLLING_MSG_HASH_SIZE);
  DEBUG ((DEBUG_VERBOSE, "---------------------\n"));
}

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
  )
{
  UINTN  Index;
  CHAR8  *MsgName;

  MsgName = "Unknown Message";
  for (Index = 0; Index < ARRAY_SIZE (MessageMap); Index++) {
    if (CompareGuid (MessageMap[Index].Guid, Guid)) {
      MsgName = MessageMap[Index].Name;
      break;
    }
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: %g - %a\n",
    IsSend ? "Send" : "Recv",
    Guid,
    MsgName
    ));
}

#endif // if !defined (MDEPKG_NDEBUG)
