/**@file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  WinNtThunk.c

Abstract:

  Since the SEC is the only windows program in our emulation we
  must use a Tiano mechanism to export Win32 APIs to other modules.
  This is the role of the EFI_WIN_NT_THUNK_PROTOCOL.

  The mWinNtThunkTable exists so that a change to EFI_WIN_NT_THUNK_PROTOCOL
  will cause an error in initializing the array if all the member functions
  are not added. It looks like adding a element to end and not initializing
  it may cause the table to be initaliized with the members at the end being
  set to zero. This is bad as jumping to zero will case the NT32 to crash.

  All the member functions in mWinNtThunkTable are Win32
  API calls, so please reference Microsoft documentation.


  gWinNt is a a public exported global that contains the initialized
  data.

**/

#include "WinHost.h"

UINTN
SecWriteStdErr (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  )
{
  BOOL  Success;
  DWORD CharCount;

  CharCount = (DWORD)NumberOfBytes;
  Success = WriteFile (
    GetStdHandle (STD_ERROR_HANDLE),
    Buffer,
    CharCount,
    &CharCount,
    NULL
    );

  return Success ? CharCount : 0;
}


EFI_STATUS
SecConfigStdIn (
  VOID
  )
{
  BOOL     Success;
  DWORD    Mode;

  Success = GetConsoleMode (GetStdHandle (STD_INPUT_HANDLE), &Mode);
  if (Success) {
    //
    // Disable buffer (line input), echo, mouse, window
    //
    Success = SetConsoleMode (
                GetStdHandle (STD_INPUT_HANDLE),
                Mode | ENABLE_VIRTUAL_TERMINAL_INPUT & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT)
                );
  }
  if (Success) {
    //
    // Enable terminal mode
    //
    Success = GetConsoleMode (GetStdHandle (STD_OUTPUT_HANDLE), &Mode);
    if (Success) {
      Success = SetConsoleMode (
        GetStdHandle (STD_OUTPUT_HANDLE),
        Mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN
      );
    }
  }
  return Success ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}

UINTN
SecWriteStdOut (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  )
{
  BOOL  Success;
  DWORD CharCount;

  CharCount = (DWORD)NumberOfBytes;
  Success = WriteFile (
    GetStdHandle (STD_OUTPUT_HANDLE),
    Buffer,
    CharCount,
    &CharCount,
    NULL
    );

  return Success ? CharCount : 0;
}

BOOLEAN
SecPollStdIn (
  VOID
  )
{
  BOOL           Success;
  INPUT_RECORD   Record;
  DWORD          RecordNum;

  do {
    Success = GetNumberOfConsoleInputEvents (GetStdHandle (STD_INPUT_HANDLE), &RecordNum);
    if (!Success || (RecordNum == 0)) {
      break;
    }
    Success = PeekConsoleInput (
      GetStdHandle (STD_INPUT_HANDLE),
      &Record,
      1,
      &RecordNum
    );
    if (Success && (RecordNum == 1)) {
      if (Record.EventType == KEY_EVENT && Record.Event.KeyEvent.bKeyDown) {
        return TRUE;
      } else {
        //
        // Consume the non-key event.
        //
        Success = ReadConsoleInput (
          GetStdHandle (STD_INPUT_HANDLE),
          &Record,
          1,
          &RecordNum
        );
      }
    }
  } while (Success);

  return FALSE;
}

UINTN
SecReadStdIn (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  )
{
  BOOL           Success;
  INPUT_RECORD   Record;
  DWORD          RecordNum;
  UINTN          BytesReturn;

  if (!SecPollStdIn ()) {
    return 0;
  }
  Success = ReadConsoleInput (
    GetStdHandle (STD_INPUT_HANDLE),
    &Record,
    1,
    &RecordNum
  );
  ASSERT (Success && (RecordNum == 1) && (Record.EventType == KEY_EVENT) && (Record.Event.KeyEvent.bKeyDown));
  NumberOfBytes = MIN (Record.Event.KeyEvent.wRepeatCount, NumberOfBytes);
  BytesReturn   = NumberOfBytes;
  while (NumberOfBytes-- != 0) {
    Buffer[NumberOfBytes] = Record.Event.KeyEvent.uChar.AsciiChar;
  }
  return BytesReturn;
}


VOID *
SecAlloc (
  IN  UINTN Size
  )
{
  return malloc ((size_t)Size);
}

BOOLEAN
SecFree (
  IN  VOID *Ptr
  )
{
  if (EfiSystemMemoryRange (Ptr)) {
    // If an address range is in the EFI memory map it was alloced via EFI.
    // So don't free those ranges and let the caller know.
    return FALSE;
  }

  free (Ptr);
  return TRUE;
}

VOID
SecSetTimer (
  IN  UINT64                  TimerPeriod,
  IN  EMU_SET_TIMER_CALLBACK  Callback
)
{
}

VOID
SecInitializeThunk (
  VOID
)
{
}

VOID
SecEnableInterrupt (
  VOID
  )
{
}


VOID
SecDisableInterrupt (
  VOID
  )
{
}


UINT64
SecQueryPerformanceFrequency (
  VOID
  )
{
  // Hard code to nanoseconds
  return 1000000000ULL;
}

UINT64
SecQueryPerformanceCounter (
  VOID
  )
{
  return 0;
}



VOID
SecSleep (
  IN  UINT64 Nanoseconds
  )
{
  Sleep ((DWORD)DivU64x32 (Nanoseconds, 1000000));
}


VOID
SecCpuSleep (
  VOID
  )
{
  Sleep (1);
}


VOID
SecExit (
  UINTN   Status
  )
{
  exit ((int)Status);
}


VOID
SecGetTime (
  OUT  EFI_TIME               *Time,
  OUT EFI_TIME_CAPABILITIES   *Capabilities OPTIONAL
  )
{
}

EFI_STATUS
SecSetTime (
  IN  EFI_TIME               *Time
)
{
  return EFI_SUCCESS;
}

EMU_THUNK_PROTOCOL gEmuThunkProtocol = {
  SecWriteStdErr,
  SecConfigStdIn,
  SecWriteStdOut,
  SecReadStdIn,
  SecPollStdIn,
  SecAlloc,
  NULL,
  SecFree,
  SecPeCoffGetEntryPoint,
  PeCoffLoaderRelocateImageExtraAction,
  PeCoffLoaderUnloadImageExtraAction,
  SecEnableInterrupt,
  SecDisableInterrupt,
  SecQueryPerformanceFrequency,
  SecQueryPerformanceCounter,
  SecSleep,
  SecCpuSleep,
  SecExit,
  SecGetTime,
  SecSetTime,
  SecSetTimer,
  GetNextThunkProtocol
};


#pragma warning(default : 4996)
#pragma warning(default : 4232)

