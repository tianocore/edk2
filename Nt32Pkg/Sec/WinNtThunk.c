/**@file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
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

#include "SecMain.h"

//
// This pragma is needed for all the DLL entry points to be asigned to the array.
//  if warning 4232 is not dissabled a warning will be generated as a DLL entry
//  point could be modified dynamically. The SEC does not do that, so we must
//  disable the warning so we can compile the SEC. The previous method was to
//  asign each element in code. The disadvantage to that approach is it's harder
//  to tell if all the elements have been initailized properly.
//
#pragma warning(disable : 4232)
#pragma warning(disable : 4996)

#if __INTEL_COMPILER
#pragma warning ( disable : 144 )  
#endif

EFI_WIN_NT_THUNK_PROTOCOL mWinNtThunkTable = {
  EFI_WIN_NT_THUNK_PROTOCOL_SIGNATURE,
  GetProcAddress,
  GetTickCount,
  LoadLibraryEx,
  FreeLibrary,
  SetPriorityClass,
  SetThreadPriority,
  Sleep,
  SuspendThread,
  GetCurrentThread,
  GetCurrentThreadId,
  GetCurrentProcess,
  CreateThread,
  TerminateThread,
  SendMessage,
  ExitThread,
  ResumeThread,
  DuplicateHandle,
  InitializeCriticalSection,
  EnterCriticalSection,
  LeaveCriticalSection,
  DeleteCriticalSection,
  TlsAlloc,
  TlsFree,
  TlsSetValue,
  TlsGetValue,
  CreateSemaphore,
  WaitForSingleObject,
  ReleaseSemaphore,
  CreateConsoleScreenBuffer,
  FillConsoleOutputAttribute,
  FillConsoleOutputCharacter,
  GetConsoleCursorInfo,
  GetNumberOfConsoleInputEvents,
  PeekConsoleInput,
  ScrollConsoleScreenBuffer,
  ReadConsoleInput,
  SetConsoleActiveScreenBuffer,
  SetConsoleCursorInfo,
  SetConsoleCursorPosition,
  SetConsoleScreenBufferSize,
  SetConsoleTitleW,
  WriteConsoleInput,
  WriteConsoleOutput,
  CreateFile,
  DeviceIoControl,
  CreateDirectory,
  RemoveDirectory,
  GetFileAttributes,
  SetFileAttributes,
  CreateFileMapping,
  CloseHandle,
  DeleteFile,
  FindFirstFile,
  FindNextFile,
  FindClose,
  FlushFileBuffers,
  GetEnvironmentVariable,
  GetLastError,
  SetErrorMode,
  GetStdHandle,
  MapViewOfFileEx,
  ReadFile,
  SetEndOfFile,
  SetFilePointer,
  WriteFile,
  GetFileInformationByHandle,
  GetDiskFreeSpace,
  GetDiskFreeSpaceEx,
  MoveFile,
  SetFileTime,
  SystemTimeToFileTime,
  LocalFileTimeToFileTime,
  FileTimeToLocalFileTime,
  FileTimeToSystemTime,
  GetSystemTime,
  SetSystemTime,
  GetLocalTime,
  SetLocalTime,
  GetTimeZoneInformation,
  SetTimeZoneInformation,
  timeSetEvent,
  timeKillEvent,
  ClearCommError,
  EscapeCommFunction,
  GetCommModemStatus,
  GetCommState,
  SetCommState,
  PurgeComm,
  SetCommTimeouts,
  ExitProcess,
  _snwprintf,
  GetDesktopWindow,
  GetForegroundWindow,
  CreateWindowEx,
  ShowWindow,
  UpdateWindow,
  DestroyWindow,
  InvalidateRect,
  GetWindowDC,
  GetClientRect,
  AdjustWindowRect,
  SetDIBitsToDevice,
  BitBlt,
  GetDC,
  ReleaseDC,
  RegisterClassEx,
  UnregisterClass,
  BeginPaint,
  EndPaint,
  PostQuitMessage,
  DefWindowProc,
  LoadIcon,
  LoadCursor,
  GetStockObject,
  SetViewportOrgEx,
  SetWindowOrgEx,
  MoveWindow,
  GetWindowRect,
  GetMessage,
  TranslateMessage,
  DispatchMessage,
  GetProcessHeap,
  HeapAlloc,
  HeapFree,
  QueryPerformanceCounter,
  QueryPerformanceFrequency
};

#pragma warning(default : 4996)
#pragma warning(default : 4232)

EFI_WIN_NT_THUNK_PROTOCOL *gWinNt = &mWinNtThunkTable;
