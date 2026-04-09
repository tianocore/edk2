/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AndroidFastbootApp.h"

#include <Protocol/AndroidFastbootTransport.h>
#include <Protocol/AndroidFastbootPlatform.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/SimpleTextIn.h>

#include <Library/PcdLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/PrintLib.h>

/*
 * UEFI Application using the FASTBOOT_TRANSPORT_PROTOCOL and
 * FASTBOOT_PLATFORM_PROTOCOL to implement the Android Fastboot protocol.
 */

STATIC FASTBOOT_TRANSPORT_PROTOCOL  *mTransport;
STATIC FASTBOOT_PLATFORM_PROTOCOL   *mPlatform;

STATIC EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *mTextOut;

typedef enum {
  ExpectCmdState,
  ExpectDataState,
  FastbootStateMax
} ANDROID_FASTBOOT_STATE;

STATIC ANDROID_FASTBOOT_STATE  mState = ExpectCmdState;

// When in ExpectDataState, the number of bytes of data to expect:
STATIC UINT64  mNumDataBytes;
// .. and the number of bytes so far received this data phase
STATIC UINT64  mBytesReceivedSoFar;
// .. and the buffer to save data into
STATIC UINT8  *mDataBuffer = NULL;

// Event notify functions, from which gBS->Exit shouldn't be called, can signal
// this event when the application should exit
STATIC EFI_EVENT  mFinishedEvent;

STATIC EFI_EVENT  mFatalSendErrorEvent;

// This macro uses sizeof - only use it on arrays (i.e. string literals)
#define SEND_LITERAL(Str)            mTransport->Send (       \
                                        sizeof (Str) - 1,     \
                                        Str,                  \
                                        &mFatalSendErrorEvent \
                                        )
#define MATCH_CMD_LITERAL(Cmd, Buf)  !AsciiStrnCmp (Cmd, Buf, sizeof (Cmd) - 1)

#define IS_LOWERCASE_ASCII(Char)  (Char >= 'a' && Char <= 'z')

#define FASTBOOT_STRING_MAX_LENGTH   256
#define FASTBOOT_COMMAND_MAX_LENGTH  64

STATIC
VOID
HandleGetVar (
  IN CHAR8  *CmdArg
  )
{
  CHAR8       Response[FASTBOOT_COMMAND_MAX_LENGTH + 1] = "OKAY";
  EFI_STATUS  Status;

  // Respond to getvar:version with 0.4 (version of Fastboot protocol)
  if (!AsciiStrnCmp ("version", CmdArg, sizeof ("version") - 1)) {
    SEND_LITERAL ("OKAY" ANDROID_FASTBOOT_VERSION);
  } else {
    // All other variables are assumed to be platform specific
    Status = mPlatform->GetVar (CmdArg, Response + 4);
    if (EFI_ERROR (Status)) {
      SEND_LITERAL ("FAILSomething went wrong when looking up the variable");
    } else {
      mTransport->Send (AsciiStrLen (Response), Response, &mFatalSendErrorEvent);
    }
  }
}

STATIC
VOID
HandleDownload (
  IN CHAR8  *NumBytesString
  )
{
  CHAR8   Response[13];
  CHAR16  OutputString[FASTBOOT_STRING_MAX_LENGTH];

  // Argument is 8-character ASCII string hex representation of number of bytes
  // that will be sent in the data phase.
  // Response is "DATA" + that same 8-character string.

  // Replace any previously downloaded data
  if (mDataBuffer != NULL) {
    FreePool (mDataBuffer);
    mDataBuffer = NULL;
  }

  // Parse out number of data bytes to expect
  mNumDataBytes = AsciiStrHexToUint64 (NumBytesString);
  if (mNumDataBytes == 0) {
    mTextOut->OutputString (mTextOut, L"ERROR: Fail to get the number of bytes to download.\r\n");
    SEND_LITERAL ("FAILFailed to get the number of bytes to download");
    return;
  }

  UnicodeSPrint (OutputString, sizeof (OutputString), L"Downloading %d bytes\r\n", mNumDataBytes);
  mTextOut->OutputString (mTextOut, OutputString);

  mDataBuffer = AllocatePool (mNumDataBytes);
  if (mDataBuffer == NULL) {
    SEND_LITERAL ("FAILNot enough memory");
  } else {
    ZeroMem (Response, sizeof Response);
    AsciiSPrint (
      Response,
      sizeof Response,
      "DATA%x",
      (UINT32)mNumDataBytes
      );
    mTransport->Send (sizeof Response - 1, Response, &mFatalSendErrorEvent);

    mState              = ExpectDataState;
    mBytesReceivedSoFar = 0;
  }
}

STATIC
VOID
HandleFlash (
  IN CHAR8  *PartitionName
  )
{
  EFI_STATUS  Status;
  CHAR16      OutputString[FASTBOOT_STRING_MAX_LENGTH];

  // Build output string
  UnicodeSPrint (OutputString, sizeof (OutputString), L"Flashing partition %a\r\n", PartitionName);
  mTextOut->OutputString (mTextOut, OutputString);

  if (mDataBuffer == NULL) {
    // Doesn't look like we were sent any data
    SEND_LITERAL ("FAILNo data to flash");
    return;
  }

  Status = mPlatform->FlashPartition (
                        PartitionName,
                        mNumDataBytes,
                        mDataBuffer
                        );
  if (Status == EFI_NOT_FOUND) {
    SEND_LITERAL ("FAILNo such partition.");
    mTextOut->OutputString (mTextOut, L"No such partition.\r\n");
  } else if (EFI_ERROR (Status)) {
    SEND_LITERAL ("FAILError flashing partition.");
    mTextOut->OutputString (mTextOut, L"Error flashing partition.\r\n");
    DEBUG ((DEBUG_ERROR, "Couldn't flash image:  %r\n", Status));
  } else {
    mTextOut->OutputString (mTextOut, L"Done.\r\n");
    SEND_LITERAL ("OKAY");
  }
}

STATIC
VOID
HandleErase (
  IN CHAR8  *PartitionName
  )
{
  EFI_STATUS  Status;
  CHAR16      OutputString[FASTBOOT_STRING_MAX_LENGTH];

  // Build output string
  UnicodeSPrint (OutputString, sizeof (OutputString), L"Erasing partition %a\r\n", PartitionName);
  mTextOut->OutputString (mTextOut, OutputString);

  Status = mPlatform->ErasePartition (PartitionName);
  if (EFI_ERROR (Status)) {
    SEND_LITERAL ("FAILCheck device console.");
    DEBUG ((DEBUG_ERROR, "Couldn't erase image:  %r\n", Status));
  } else {
    SEND_LITERAL ("OKAY");
  }
}

STATIC
VOID
HandleBoot (
  VOID
  )
{
  EFI_STATUS  Status;

  mTextOut->OutputString (mTextOut, L"Booting downloaded image\r\n");

  if (mDataBuffer == NULL) {
    // Doesn't look like we were sent any data
    SEND_LITERAL ("FAILNo image in memory");
    return;
  }

  // We don't really have any choice but to report success, because once we
  // boot we lose control of the system.
  SEND_LITERAL ("OKAY");

  Status = BootAndroidBootImg (mNumDataBytes, mDataBuffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to boot downloaded image: %r\n", Status));
  }

  // We shouldn't get here
}

STATIC
VOID
HandleOemCommand (
  IN CHAR8  *Command
  )
{
  EFI_STATUS  Status;

  Status = mPlatform->DoOemCommand (Command);
  if (Status == EFI_NOT_FOUND) {
    SEND_LITERAL ("FAILOEM Command not recognised.");
  } else if (Status == EFI_DEVICE_ERROR) {
    SEND_LITERAL ("FAILError while executing command");
  } else if (EFI_ERROR (Status)) {
    SEND_LITERAL ("FAIL");
  } else {
    SEND_LITERAL ("OKAY");
  }
}

STATIC
VOID
AcceptCmd (
  IN        UINTN  Size,
  IN  CONST CHAR8  *Data
  )
{
  CHAR8  Command[FASTBOOT_COMMAND_MAX_LENGTH + 1];

  // Max command size is 64 bytes
  if (Size > FASTBOOT_COMMAND_MAX_LENGTH) {
    SEND_LITERAL ("FAILCommand too large");
    return;
  }

  // Commands aren't null-terminated. Let's get a null-terminated version.
  AsciiStrnCpyS (Command, sizeof Command, Data, Size);

  // Parse command
  if (MATCH_CMD_LITERAL ("getvar", Command)) {
    HandleGetVar (Command + sizeof ("getvar"));
  } else if (MATCH_CMD_LITERAL ("download", Command)) {
    HandleDownload (Command + sizeof ("download"));
  } else if (MATCH_CMD_LITERAL ("verify", Command)) {
    SEND_LITERAL ("FAILNot supported");
  } else if (MATCH_CMD_LITERAL ("flash", Command)) {
    HandleFlash (Command + sizeof ("flash"));
  } else if (MATCH_CMD_LITERAL ("erase", Command)) {
    HandleErase (Command + sizeof ("erase"));
  } else if (MATCH_CMD_LITERAL ("boot", Command)) {
    HandleBoot ();
  } else if (MATCH_CMD_LITERAL ("continue", Command)) {
    SEND_LITERAL ("OKAY");
    mTextOut->OutputString (mTextOut, L"Received 'continue' command. Exiting Fastboot mode\r\n");

    gBS->SignalEvent (mFinishedEvent);
  } else if (MATCH_CMD_LITERAL ("reboot", Command)) {
    if (MATCH_CMD_LITERAL ("reboot-booloader", Command)) {
      // fastboot_protocol.txt:
      //    "reboot-bootloader    Reboot back into the bootloader."
      // I guess this means reboot back into fastboot mode to save the user
      // having to do whatever they did to get here again.
      // Here we just reboot normally.
      SEND_LITERAL ("INFOreboot-bootloader not supported, rebooting normally.");
    }

    SEND_LITERAL ("OKAY");
    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

    // Shouldn't get here
    DEBUG ((DEBUG_ERROR, "Fastboot: gRT->ResetSystem didn't work\n"));
  } else if (MATCH_CMD_LITERAL ("powerdown", Command)) {
    SEND_LITERAL ("OKAY");
    gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);

    // Shouldn't get here
    DEBUG ((DEBUG_ERROR, "Fastboot: gRT->ResetSystem didn't work\n"));
  } else if (MATCH_CMD_LITERAL ("oem", Command)) {
    // The "oem" command isn't in the specification, but it was observed in the
    // wild, followed by a space, followed by the actual command.
    HandleOemCommand (Command + sizeof ("oem"));
  } else if (IS_LOWERCASE_ASCII (Command[0])) {
    // Commands starting with lowercase ASCII characters are reserved for the
    // Fastboot protocol. If we don't recognise it, it's probably the future
    // and there are new commands in the protocol.
    // (By the way, the "oem" command mentioned above makes this reservation
    //  redundant, but we handle it here to be spec-compliant)
    SEND_LITERAL ("FAILCommand not recognised. Check Fastboot version.");
  } else {
    HandleOemCommand (Command);
  }
}

STATIC
VOID
AcceptData (
  IN  UINTN  Size,
  IN  VOID   *Data
  )
{
  UINT32        RemainingBytes = mNumDataBytes - mBytesReceivedSoFar;
  CHAR16        OutputString[FASTBOOT_STRING_MAX_LENGTH];
  STATIC UINTN  Count = 0;

  // Protocol doesn't say anything about sending extra data so just ignore it.
  if (Size > RemainingBytes) {
    Size = RemainingBytes;
  }

  CopyMem (&mDataBuffer[mBytesReceivedSoFar], Data, Size);

  mBytesReceivedSoFar += Size;

  // Show download progress. Don't do it for every packet  as outputting text
  // might be time consuming - do it on the last packet and on every 32nd packet
  if (((Count++ % 32) == 0) || (Size == RemainingBytes)) {
    // (Note no newline in format string - it will overwrite the line each time)
    UnicodeSPrint (
      OutputString,
      sizeof (OutputString),
      L"\r%8d / %8d bytes downloaded (%d%%)",
      mBytesReceivedSoFar,
      mNumDataBytes,
      (mBytesReceivedSoFar * 100) / mNumDataBytes // percentage
      );
    mTextOut->OutputString (mTextOut, OutputString);
  }

  if (mBytesReceivedSoFar == mNumDataBytes) {
    // Download finished.

    mTextOut->OutputString (mTextOut, L"\r\n");
    SEND_LITERAL ("OKAY");
    mState = ExpectCmdState;
  }
}

/*
  This is the NotifyFunction passed to CreateEvent in the FastbootAppEntryPoint
  It will be called by the UEFI event framework when the transport protocol
  implementation signals that data has been received from the Fastboot host.
  The parameters are ignored.
*/
STATIC
VOID
DataReady (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN       Size;
  VOID        *Data;
  EFI_STATUS  Status;

  do {
    Status = mTransport->Receive (&Size, &Data);
    if (!EFI_ERROR (Status)) {
      if (mState == ExpectCmdState) {
        AcceptCmd (Size, (CHAR8 *)Data);
      } else if (mState == ExpectDataState) {
        AcceptData (Size, Data);
      } else {
        ASSERT (FALSE);
      }

      FreePool (Data);
    }
  } while (!EFI_ERROR (Status));

  // Quit if there was a fatal error
  if (Status != EFI_NOT_READY) {
    ASSERT (Status == EFI_DEVICE_ERROR);
    // (Put a newline at the beginning as we are probably in the data phase,
    //  so the download progress line, with no '\n' is probably on the console)
    mTextOut->OutputString (mTextOut, L"\r\nFatal error receiving data. Exiting.\r\n");
    gBS->SignalEvent (mFinishedEvent);
  }
}

/*
  Event notify for a fatal error in transmission.
*/
STATIC
VOID
FatalErrorNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mTextOut->OutputString (mTextOut, L"Fatal error sending command response. Exiting.\r\n");
  gBS->SignalEvent (mFinishedEvent);
}

EFI_STATUS
EFIAPI
FastbootAppEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_EVENT                       ReceiveEvent;
  EFI_EVENT                       WaitEventArray[2];
  UINTN                           EventIndex;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *TextIn;
  EFI_INPUT_KEY                   Key;

  mDataBuffer = NULL;

  Status = gBS->LocateProtocol (
                  &gAndroidFastbootTransportProtocolGuid,
                  NULL,
                  (VOID **)&mTransport
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fastboot: Couldn't open Fastboot Transport Protocol: %r\n", Status));
    return Status;
  }

  Status = gBS->LocateProtocol (&gAndroidFastbootPlatformProtocolGuid, NULL, (VOID **)&mPlatform);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fastboot: Couldn't open Fastboot Platform Protocol: %r\n", Status));
    return Status;
  }

  Status = mPlatform->Init ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fastboot: Couldn't initialise Fastboot Platform Protocol: %r\n", Status));
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiSimpleTextOutProtocolGuid, NULL, (VOID **)&mTextOut);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Fastboot: Couldn't open Text Output Protocol: %r\n",
      Status
      ));
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiSimpleTextInProtocolGuid, NULL, (VOID **)&TextIn);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fastboot: Couldn't open Text Input Protocol: %r\n", Status));
    return Status;
  }

  // Disable watchdog
  Status = gBS->SetWatchdogTimer (0, 0x10000, 0, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fastboot: Couldn't disable watchdog timer: %r\n", Status));
  }

  // Create event for receipt of data from the host
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  DataReady,
                  NULL,
                  &ReceiveEvent
                  );
  ASSERT_EFI_ERROR (Status);

  // Create event for exiting application when "continue" command is received
  Status = gBS->CreateEvent (0, TPL_CALLBACK, NULL, NULL, &mFinishedEvent);
  ASSERT_EFI_ERROR (Status);

  // Create event to pass to FASTBOOT_TRANSPORT_PROTOCOL.Send, signalling a
  // fatal error
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  FatalErrorNotify,
                  NULL,
                  &mFatalSendErrorEvent
                  );
  ASSERT_EFI_ERROR (Status);

  // Start listening for data
  Status = mTransport->Start (
                         ReceiveEvent
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Fastboot: Couldn't start transport: %r\n", Status));
    return Status;
  }

  // Talk to the user
  mTextOut->OutputString (
              mTextOut,
              L"Android Fastboot mode - version " ANDROID_FASTBOOT_VERSION ". Press RETURN or SPACE key to quit.\r\n"
              );

  // Quit when the user presses any key, or mFinishedEvent is signalled
  WaitEventArray[0] = mFinishedEvent;
  WaitEventArray[1] = TextIn->WaitForKey;
  while (1) {
    gBS->WaitForEvent (2, WaitEventArray, &EventIndex);
    Status = TextIn->ReadKeyStroke (gST->ConIn, &Key);
    if (Key.ScanCode == SCAN_NULL) {
      if ((Key.UnicodeChar == CHAR_CARRIAGE_RETURN) ||
          (Key.UnicodeChar == L' '))
      {
        break;
      }
    }
  }

  mTransport->Stop ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Warning: Fastboot Transport Stop: %r\n", Status));
  }

  mPlatform->UnInit ();

  return EFI_SUCCESS;
}
