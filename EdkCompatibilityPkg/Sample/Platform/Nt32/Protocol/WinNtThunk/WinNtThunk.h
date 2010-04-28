/*++

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  WinNtThunk.h

Abstract:

  This protocol allows an EFI driver (DLL) in the NT emulation envirnment
  to make Win32 API calls.

  NEVER make a Win32 call directly, always make the call via this protocol.

  There are no This pointers on the protocol member functions as they map
  exactly into Win32 system calls.

  YOU MUST include EfiWinNT.h in place of Efi.h to make this file compile.

--*/

#ifndef _WIN_NT_THUNK_H_
#define _WIN_NT_THUNK_H_

#define EFI_WIN_NT_THUNK_PROTOCOL_GUID \
  { \
    0x58c518b1, 0x76f3, 0x11d4, {0xbc, 0xea, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81} \
  }

//
// The following APIs require EfiWinNT.h. In some environmnets the GUID
// definitions are needed but the EfiWinNT.h is not included.
// EfiWinNT.h is needed to support WINDOWS API requirements.
//
#ifdef _EFI_WIN_NT_H_

typedef
WINBASEAPI
VOID
(WINAPI *WinNtSleep) (
  DWORD Milliseconds
  );

typedef
WINBASEAPI
DWORD
(WINAPI *WinNtSuspendThread) (
  HANDLE hThread
  );

typedef
WINBASEAPI
HANDLE
(WINAPI *WinNtGetCurrentThread) (
  VOID
  );

typedef
WINBASEAPI
DWORD
(WINAPI *WinNtGetCurrentThreadId) (
  VOID
  );

typedef
WINBASEAPI
HANDLE
(WINAPI *WinNtGetCurrentProcess) (
  VOID
  );

typedef
WINBASEAPI
HANDLE
(WINAPI *WinNtCreateThread) (
  LPSECURITY_ATTRIBUTES   lpThreadAttributes,
  DWORD                   dwStackSize,
  LPTHREAD_START_ROUTINE  lpStartAddress,
  LPVOID                  lpParameter,
  DWORD                   dwCreationFlags,
  LPDWORD                 lpThreadId
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtTerminateThread) (
  HANDLE hThread,
  DWORD  dwExitCode
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSendMessage) (
  HWND    hWnd,
  UINT    Msg,
  WPARAM  wParam,
  LPARAM  lParam
  );

typedef
WINBASEAPI
VOID
(WINAPI *WinNtExitThread) (
  DWORD   dwExitCode
  );

typedef
WINBASEAPI
DWORD
(WINAPI *WinNtResumeThread) (
  HANDLE hThread
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetThreadPriority) (
  HANDLE    hThread,
  INTN      nPriority
  );

typedef
WINBASEAPI
VOID
(WINAPI *WinNtInitializeCriticalSection) (
  LPCRITICAL_SECTION lpCriticalSection
  );

typedef
WINBASEAPI
VOID
(WINAPI *WinNtDeleteCriticalSection) (
  LPCRITICAL_SECTION lpCriticalSection
  );

typedef
WINBASEAPI
VOID
(WINAPI *WinNtEnterCriticalSection) (
  LPCRITICAL_SECTION lpCriticalSection
  );

typedef
WINBASEAPI
VOID
(WINAPI *WinNtLeaveCriticalSection) (
  LPCRITICAL_SECTION lpCriticalSection
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtTlsAlloc) (
  VOID
  );

typedef
WINBASEAPI
LPVOID
(WINAPI *WinNtTlsGetValue) (
  DWORD dwTlsIndex
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtTlsSetValue) (
  DWORD  dwTlsIndex,
  LPVOID lpTlsValue
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtTlsFree) (
  DWORD dwTlsIndex
  );

typedef
WINBASEAPI
HANDLE
(WINAPI *WinNtCreateSemaphore) (
  LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
  LONG                  lInitialCount,
  LONG                  lMaximumCount,
  LPCWSTR               lpName
  );

typedef
WINBASEAPI
DWORD
(WINAPI *WinNtWaitForSingleObject) (
  HANDLE  hHandle,
  DWORD   dwMilliseconds
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtReleaseSemaphore) (
  HANDLE  hSemaphore,
  LONG    lReleaseCount,
  LPLONG  lpPreviousCount
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtDuplicateHandle) (
  HANDLE   hSourceProcessHandle,
  HANDLE   hSourceHandle,
  HANDLE   hTargetProcessHandle,
  LPHANDLE lpTargetHandle,
  DWORD    dwDesiredAccess,
  BOOL     bInheritHandle,
  DWORD    dwOptions
  );

typedef
WINBASEAPI
HANDLE
(WINAPI *WinNtCreateConsoleScreenBuffer) (
  DWORD                       DesiredAccess,
  DWORD                       ShareMode,
  CONST SECURITY_ATTRIBUTES   *SecurityAttributes,
  DWORD                       Flags,
  LPVOID                      ScreenBufferData
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetConsoleScreenBufferSize) (
  HANDLE  ConsoleOutput,
  COORD   Size
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetConsoleActiveScreenBuffer) (
  HANDLE  ConsoleOutput
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtFillConsoleOutputAttribute) (
  HANDLE  ConsoleOutput,
  WORD    Attribute,
  DWORD   Length,
  COORD   WriteCoord,
  LPDWORD NumberOfAttrsWritten
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtFillConsoleOutputCharacter) (
  HANDLE  ConsoleOutput,
  TCHAR   Character,
  DWORD   Length,
  COORD   WriteCoord,
  LPDWORD NumberOfCharsWritten
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtWriteConsoleOutput) (
  HANDLE          ConsoleOutput,
  CONST CHAR_INFO *Buffer,
  COORD           BufferSize,
  COORD           BufferCoord,
  PSMALL_RECT     WriteRegion
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtScrollConsoleScreenBuffer) (
  HANDLE            ConsoleOutput,
  CONST SMALL_RECT  *ScrollRectangle,
  CONST SMALL_RECT  *ClipRectangle,
  COORD             DestinationOrigin,
  CONST CHAR_INFO   *Fill
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetConsoleTitleW) (
  LPCTSTR   ConsoleTitle
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtGetConsoleCursorInfo) (
  HANDLE                ConsoleOutput,
  PCONSOLE_CURSOR_INFO  ConsoleCursorInfo
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetConsoleCursorInfo) (
  HANDLE                      ConsoleOutput,
  CONST CONSOLE_CURSOR_INFO   *ConsoleCursorInfo
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetPriorityClass) (
  HANDLE  Process,
  DWORD   PriorityClass
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtWriteConsoleInput) (
  HANDLE              ConsoleInput,
  CONST INPUT_RECORD  *Buffer,
  DWORD               Legnth,
  LPDWORD             NumberOfEventsWritten
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtGetNumberOfConsoleInputEvents) (
  HANDLE              ConsoleInput,
  LPDWORD             NumberOfEvents
  );

typedef
WINBASEAPI
HANDLE
(WINAPI *WinNtGetStdHandle) (
  DWORD   StdHandle
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtReadConsoleInput) (
  HANDLE              ConsoleInput,
  PINPUT_RECORD       Buffer,
  DWORD               Length,
  LPDWORD             NumberOfEventsRead
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtPeekConsoleInput) (
  HANDLE              ConsoleInput,
  PINPUT_RECORD       Buffer,
  DWORD               Length,
  LPDWORD             NumberOfEventsRead
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetConsoleCursorPosition) (
  HANDLE              ConsoleInput,
  COORD               CursorPosition
  );

typedef
WINBASEAPI
HANDLE
(WINAPI *WinNtCreateFile) (
  LPCWSTR               FileName,
  DWORD                 DesiredAccess,
  DWORD                 SharedMode,
  LPSECURITY_ATTRIBUTES SecurityAttributes,
  DWORD                 CreationDisposition,
  DWORD                 FlagsAndAttributes,
  HANDLE                TemplateFile
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtDeviceIoControl) (
  HANDLE                DeviceHandle,
  DWORD                 IoControlCode,
  LPVOID                InBuffer,
  DWORD                 InBufferSize,
  LPVOID                OutBuffer,
  DWORD                 OutBufferSize,
  LPDWORD               BytesReturned,
  LPOVERLAPPED          Overlapped
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtCreateDirectory) (
  LPCWSTR               PathName,
  LPSECURITY_ATTRIBUTES SecurityAttributes
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtRemoveDirectory) (
  LPCWSTR               PathName
  );

typedef
WINBASEAPI
DWORD
(WINAPI *WinNtGetFileAttributes) (
  LPCWSTR               FileName
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetFileAttributes) (
  LPCWSTR               FileName,
  DWORD                 FileAttributes
  );

typedef
WINBASEAPI
HANDLE
(WINAPI *WinNtCreateFileMapping) (
  HANDLE                  FileHandle,
  LPSECURITY_ATTRIBUTES   Attributes,
  DWORD                   Protect,
  DWORD                   MaximumSizeHigh,
  DWORD                   MaximumSizeLow,
  LPCTSTR                 Name
  );

typedef
WINBASEAPI
LPVOID
(WINAPI *WinNtMapViewOfFileEx) (
  HANDLE                  FileHandle,
  DWORD                   DesiredAccess,
  DWORD                   FileOffsetHigh,
  DWORD                   FileOffsetLow,
  DWORD                   NumberOfBytesToMap,
  LPVOID                  BaseAddress
  );

typedef
WINBASEAPI
DWORD
(WINAPI *WinNtGetEnvironmentVariable) (
  LPCTSTR Name,
  LPTSTR  Buffer,
  DWORD   Size
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtCloseHandle) (
  HANDLE    Object
  );

typedef
WINBASEAPI
DWORD
(WINAPI *WinNtSetFilePointer) (
  HANDLE    FileHandle,
  LONG      DistanceToMove,
  PLONG     DistanceToHoveHigh,
  DWORD     MoveMethod
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetEndOfFile) (
  HANDLE    FileHandle
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtReadFile) (
  HANDLE        FileHandle,
  LPVOID        Buffer,
  DWORD         NumberOfBytesToRead,
  LPDWORD       NumberOfBytesRead,
  LPOVERLAPPED  Overlapped
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtWriteFile) (
  HANDLE        FileHandle,
  LPCVOID       Buffer,
  DWORD         NumberOfBytesToWrite,
  LPDWORD       NumberOfBytesWritten,
  LPOVERLAPPED  Overlapped
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtGetFileInformationByHandle) (
  HANDLE                      FileHandle,
  BY_HANDLE_FILE_INFORMATION  *FileInfo
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtGetDiskFreeSpace) (
  LPCTSTR     RootPathName,
  LPDWORD     SectorsPerCluster,
  LPDWORD     BytesPerSector,
  LPDWORD     NumberOfFreeClusters,
  LPDWORD     TotalNumberOfClusters
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtGetDiskFreeSpaceEx) (
  LPCTSTR             DirectoryName,
  PULARGE_INTEGER     FreeBytesAvailable,
  PULARGE_INTEGER     TotalNumberOfBytes,
  PULARGE_INTEGER     TotoalNumberOfFreeBytes
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtMoveFile) (
  LPCTSTR     ExistingFileName,
  LPCTSTR     NewFileName
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetFileTime) (
  HANDLE      FileHandle,
  FILETIME    *CreationTime,
  FILETIME    *LastAccessTime,
  FILETIME    *LastWriteTime
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSystemTimeToFileTime) (
  SYSTEMTIME  * SystemTime,
  FILETIME    * FileTime
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtDeleteFile) (
  LPCTSTR   FileName
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtFlushFileBuffers) (
  HANDLE
  );

typedef
WINBASEAPI
DWORD
(WINAPI *WinNtGetLastError) (
  VOID
  );

typedef
WINBASEAPI
UINT
(WINAPI *WinNtSetErrorMode) (
  UINT  Mode
  );

typedef
WINBASEAPI
DWORD
(WINAPI *WinNtGetTickCount) (
  VOID
  );

typedef
WINBASEAPI
HMODULE
(WINAPI *WinNtLoadLibraryEx) (
  LPCTSTR LibFileName,
  HANDLE  FileHandle,
  DWORD   Flags
  );

typedef
WINBASEAPI
FARPROC
(WINAPI *WinNtGetProcAddress) (
  HMODULE Module,
  LPCSTR  ProcName
  );

typedef
WINBASEAPI
DWORD
(WINAPI *WinNtGetTimeZoneInformation) (
  LPTIME_ZONE_INFORMATION timeZoneInformation
  );

typedef
WINBASEAPI
MMRESULT
(WINAPI *WinNttimeSetEvent) (
  UINT           uDelay,
  UINT           uResolution,
  LPTIMECALLBACK lpTimeProc,
  DWORD_PTR      dwUser,
  UINT           fuEvent
  );

typedef
WINBASEAPI
MMRESULT
(WINAPI *WinNttimeKillEvent) (
  UINT           uTimerID
  );

typedef
WINBASEAPI
DWORD
(WINAPI *WinNtSetTimeZoneInformation) (
  LPTIME_ZONE_INFORMATION timeZoneInformation
  );

typedef
WINBASEAPI
VOID
(WINAPI *WinNtGetSystemTime) (
  LPSYSTEMTIME        SystemTime
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetSystemTime) (
  CONST SYSTEMTIME    *SystemTime
  );

typedef
WINBASEAPI
VOID
(WINAPI *WinNtGetLocalTime) (
  LPSYSTEMTIME        SystemTime
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetLocalTime) (
  CONST SYSTEMTIME    *SystemTime
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtFileTimeToLocalFileTime) (
  CONST FILETIME  *FileTime,
  LPFILETIME      LocalFileTime
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtFileTimeToSystemTime) (
  CONST FILETIME  *FileTime,
  LPSYSTEMTIME    SystemTime
  );

typedef
WINBASEAPI
HANDLE
(WINAPI *WinNtFindFirstFile) (
  LPCTSTR           FileName,
  LPWIN32_FIND_DATA FindFileData
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtFindNextFile) (
  HANDLE            FindFile,
  LPWIN32_FIND_DATA FindFileData
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtFindClose) (
  HANDLE            FindFile
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtGetCommState) (
  HANDLE  FileHandle,
  LPDCB   DCB
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetCommState) (
  HANDLE  FileHandle,
  LPDCB   DCB
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetCommState) (
  HANDLE  FileHandle,
  LPDCB   DCB
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtSetCommTimeouts) (
  HANDLE          FileHandle,
  LPCOMMTIMEOUTS  CommTimeouts
  );

typedef
WINBASEAPI
VOID
(WINAPI *WinNtExitProcess) (
  UINT uExitCode  // exit code for all threads
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtPurgeComm) (
  HANDLE  FileHandle,
  DWORD   Flags
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtEscapeCommFunction) (
  HANDLE  FileHandle,
  DWORD   Func
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtGetCommModemStatus) (
  HANDLE  FileHandle,
  LPDWORD ModemStat
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtClearCommError) (
  HANDLE    FileHandle,
  LPDWORD   Errors,
  LPCOMSTAT Stat
  );

typedef
WINUSERAPI
INT32
(WINAPIV *WinNtSprintf) (
  LPWSTR    Buffer,
  LPCWSTR   String,
  ...
  );

typedef
WINUSERAPI
HWND
(WINAPI *WinNtGetDesktopWindow) (
  VOID
  );

typedef
WINUSERAPI
HWND
(WINAPI *WinNtGetForegroundWindow) (
  VOID
  );

typedef
WINUSERAPI
HWND
(WINAPI *WinNtCreateWindowEx) (
  DWORD     dwExStyle,
  LPCTSTR   lpClassName,
  LPCTSTR   lpWindowName,
  DWORD     dwStyle,
  INT32     x,
  INT32     y,
  INT32     nWidth,
  INT32     nHeight,
  HWND      hWndParent,
  HMENU     hMenu,
  HINSTANCE hInstance,
  LPVOID    *lpParam
  );

typedef
WINUSERAPI
BOOL
(WINAPI *WinNtUpdateWindow) (
  HWND      hWnd
  );

typedef
WINUSERAPI
BOOL
(WINAPI *WinNtShowWindow) (
  HWND        hWnd,
  INT32       nCmdShow
  );

typedef
WINGDIAPI
BOOL
(WINAPI *WinNtDestroyWindow) (
  HWND    hWnd
  );

typedef
WINUSERAPI
HDC
(WINAPI *WinNtGetWindowDC) (
  HWND    hWnd
  );

typedef
WINUSERAPI
BOOL
(WINAPI *WinNtGetClientRect) (
  HWND    hWnd,
  LPRECT  lpRect
  );

typedef
WINUSERAPI
BOOL
(WINAPI *WinNtAdjustWindowRect) (
  LPRECT  lpRect,
  DWORD   dwStyle,
  BOOL    bMenu
  );

typedef
WINGDIAPI
INT32
(WINAPI *WinNtSetDIBitsToDevice) (
  HDC,
  INT32,
  INT32,
  DWORD,
  DWORD,
  INT32,
  INT32,
  UINT,
  UINT,
  CONST VOID       *,
  CONST BITMAPINFO *,
  UINT
  );

typedef
WINGDIAPI
BOOL
(WINAPI *WinNtBitBlt) (
  HDC,
  INT32,
  INT32,
  INT32,
  INT32,
  HDC,
  INT32,
  INT32,
  DWORD
  );

typedef
WINUSERAPI
BOOL
(WINAPI *WinNtInvalidateRect) (
  HWND        hWnd,
  CONST RECT  *lpRect,
  BOOL        bErase
  );

typedef
WINUSERAPI
HDC
(WINAPI *WinNtGetDC) (
  HWND    hWnd
  );

typedef
WINUSERAPI
INT32
(WINAPI *WinNtReleaseDC) (
  HWND    hWnd,
  HDC     hDC
  );

typedef
WINUSERAPI
ATOM
(WINAPI *WinNtRegisterClassEx) (
  CONST   WNDCLASSEX *
  );

typedef
WINUSERAPI
BOOL
(WINAPI *WinNtUnregisterClass) (
  LPCTSTR   lpClassName,
  HINSTANCE hInstance
  );

typedef
WINUSERAPI
HDC
(WINAPI *WinNtBeginPaint) (
  HWND          hWnd,
  LPPAINTSTRUCT lpPaint
  );

typedef
WINUSERAPI
BOOL
(WINAPI *WinNtEndPaint) (
  HWND                hWnd,
  CONST PAINTSTRUCT   *lpPaint
  );

typedef
WINUSERAPI
VOID
(WINAPI *WinNtPostQuitMessage) (
  INT32   nExitCode
  );

typedef
WINUSERAPI
LRESULT
(WINAPI *WinNtDefWindowProc) (
  HWND    hWnd,
  UINT    Msg,
  WPARAM  wParam,
  LPARAM  lParam
  );

typedef
WINUSERAPI
HICON
(WINAPI *WinNtLoadIcon) (
  HINSTANCE hInstance,
  LPCTSTR   lpIconName
  );

typedef
WINUSERAPI
HCURSOR
(WINAPI *WinNtLoadCursor) (
  HINSTANCE   hInstance,
  LPCTSTR     lpCursorName
  );

typedef
WINGDIAPI
HGDIOBJ
(WINAPI *WinNtGetStockObject) (
  INT32
  );

typedef
WINGDIAPI
BOOL
(WINAPI *WinNtSetViewportOrgEx) (
  HDC,
  INT32,
  INT32,
  LPPOINT
  );

typedef
WINGDIAPI
BOOL
(WINAPI *WinNtSetWindowOrgEx) (
  HDC,
  INT32,
  INT32,
  LPPOINT
  );
typedef
WINGDIAPI
BOOL
(WINAPI *WinNtMoveWindow) (
  HWND,
  INT32,
  INT32,
  INT32,
  INT32,
  BOOL
  );

typedef
WINGDIAPI
BOOL
(WINAPI *WinNtGetWindowRect) (
  HWND,
  LPRECT
  );

typedef
WINUSERAPI
BOOL
(WINAPI *WinNtGetMessage) (
  LPMSG     lpMsg,
  HWND      hWnd,
  UINT      wMsgFilterMin,
  UINT      wMsgFilterMax
  );

typedef
WINUSERAPI
BOOL
(WINAPI *WinNtTranslateMessage) (
  CONST MSG *lpMsg
  );

typedef
WINUSERAPI
BOOL
(WINAPI *WinNtDispatchMessage) (
  CONST MSG *lpMsg
  );

typedef
WINUSERAPI
HANDLE
(WINAPI *WinNtGetProcessHeap) ();

typedef
WINUSERAPI
LPVOID
(WINAPI *WinNtHeapAlloc) (
  HANDLE  hHeap,
  DWORD   dwFlags,
  SIZE_T  dwBytes
  );

typedef
WINUSERAPI
BOOL
(WINAPI *WinNtHeapFree) (
  HANDLE  hHeap,
  DWORD   dwFlags,
  LPVOID  lpMem
  );

typedef
WINBASEAPI
BOOL
(WINAPI *WinNtFreeLibrary) (
  HANDLE  ModHandle
  );
//
//
//
EFI_FORWARD_DECLARATION (EFI_WIN_NT_THUNK_PROTOCOL);

#define EFI_WIN_NT_THUNK_PROTOCOL_SIGNATURE EFI_SIGNATURE_32 ('N', 'T', 'T', 'T')

typedef struct _EFI_WIN_NT_THUNK_PROTOCOL {
  UINT64                              Signature;

  //
  // Win32 Process APIs
  //
  WinNtGetProcAddress                 GetProcAddress;
  WinNtGetTickCount                   GetTickCount;
  WinNtLoadLibraryEx                  LoadLibraryEx;
  WinNtFreeLibrary                    FreeLibrary;

  WinNtSetPriorityClass               SetPriorityClass;
  WinNtSetThreadPriority              SetThreadPriority;
  WinNtSleep                          Sleep;

  WinNtSuspendThread                  SuspendThread;
  WinNtGetCurrentThread               GetCurrentThread;
  WinNtGetCurrentThreadId             GetCurrentThreadId;
  WinNtGetCurrentProcess              GetCurrentProcess;
  WinNtCreateThread                   CreateThread;
  WinNtTerminateThread                TerminateThread;
  WinNtSendMessage                    SendMessage;
  WinNtExitThread                     ExitThread;
  WinNtResumeThread                   ResumeThread;
  WinNtDuplicateHandle                DuplicateHandle;

  //
  // Wint32 Mutex primitive
  //
  WinNtInitializeCriticalSection      InitializeCriticalSection;
  WinNtEnterCriticalSection           EnterCriticalSection;
  WinNtLeaveCriticalSection           LeaveCriticalSection;
  WinNtDeleteCriticalSection          DeleteCriticalSection;
  WinNtTlsAlloc                       TlsAlloc;
  WinNtTlsFree                        TlsFree;
  WinNtTlsSetValue                    TlsSetValue;
  WinNtTlsGetValue                    TlsGetValue;
  WinNtCreateSemaphore                CreateSemaphore;
  WinNtWaitForSingleObject            WaitForSingleObject;
  WinNtReleaseSemaphore               ReleaseSemaphore;

  //
  // Win32 Console APIs
  //
  WinNtCreateConsoleScreenBuffer      CreateConsoleScreenBuffer;
  WinNtFillConsoleOutputAttribute     FillConsoleOutputAttribute;
  WinNtFillConsoleOutputCharacter     FillConsoleOutputCharacter;
  WinNtGetConsoleCursorInfo           GetConsoleCursorInfo;
  WinNtGetNumberOfConsoleInputEvents  GetNumberOfConsoleInputEvents;
  WinNtPeekConsoleInput               PeekConsoleInput;
  WinNtScrollConsoleScreenBuffer      ScrollConsoleScreenBuffer;
  WinNtReadConsoleInput               ReadConsoleInput;

  WinNtSetConsoleActiveScreenBuffer   SetConsoleActiveScreenBuffer;
  WinNtSetConsoleCursorInfo           SetConsoleCursorInfo;
  WinNtSetConsoleCursorPosition       SetConsoleCursorPosition;
  WinNtSetConsoleScreenBufferSize     SetConsoleScreenBufferSize;
  WinNtSetConsoleTitleW               SetConsoleTitleW;
  WinNtWriteConsoleInput              WriteConsoleInput;
  WinNtWriteConsoleOutput             WriteConsoleOutput;

  //
  // Win32 File APIs
  //
  WinNtCreateFile                     CreateFile;
  WinNtDeviceIoControl                DeviceIoControl;
  WinNtCreateDirectory                CreateDirectory;
  WinNtRemoveDirectory                RemoveDirectory;
  WinNtGetFileAttributes              GetFileAttributes;
  WinNtSetFileAttributes              SetFileAttributes;
  WinNtCreateFileMapping              CreateFileMapping;
  WinNtCloseHandle                    CloseHandle;
  WinNtDeleteFile                     DeleteFile;
  WinNtFindFirstFile                  FindFirstFile;
  WinNtFindNextFile                   FindNextFile;
  WinNtFindClose                      FindClose;
  WinNtFlushFileBuffers               FlushFileBuffers;
  WinNtGetEnvironmentVariable         GetEnvironmentVariable;
  WinNtGetLastError                   GetLastError;
  WinNtSetErrorMode                   SetErrorMode;
  WinNtGetStdHandle                   GetStdHandle;
  WinNtMapViewOfFileEx                MapViewOfFileEx;
  WinNtReadFile                       ReadFile;
  WinNtSetEndOfFile                   SetEndOfFile;
  WinNtSetFilePointer                 SetFilePointer;
  WinNtWriteFile                      WriteFile;
  WinNtGetFileInformationByHandle     GetFileInformationByHandle;
  WinNtGetDiskFreeSpace               GetDiskFreeSpace;
  WinNtGetDiskFreeSpaceEx             GetDiskFreeSpaceEx;
  WinNtMoveFile                       MoveFile;
  WinNtSetFileTime                    SetFileTime;
  WinNtSystemTimeToFileTime           SystemTimeToFileTime;

  //
  // Win32 Time APIs
  //
  WinNtFileTimeToLocalFileTime        FileTimeToLocalFileTime;
  WinNtFileTimeToSystemTime           FileTimeToSystemTime;
  WinNtGetSystemTime                  GetSystemTime;
  WinNtSetSystemTime                  SetSystemTime;
  WinNtGetLocalTime                   GetLocalTime;
  WinNtSetLocalTime                   SetLocalTime;
  WinNtGetTimeZoneInformation         GetTimeZoneInformation;
  WinNtSetTimeZoneInformation         SetTimeZoneInformation;
  WinNttimeSetEvent                   timeSetEvent;
  WinNttimeKillEvent                  timeKillEvent;

  //
  // Win32 Serial APIs
  //
  WinNtClearCommError                 ClearCommError;
  WinNtEscapeCommFunction             EscapeCommFunction;
  WinNtGetCommModemStatus             GetCommModemStatus;
  WinNtGetCommState                   GetCommState;
  WinNtSetCommState                   SetCommState;
  WinNtPurgeComm                      PurgeComm;
  WinNtSetCommTimeouts                SetCommTimeouts;

  WinNtExitProcess                    ExitProcess;

  WinNtSprintf                        SPrintf;

  WinNtGetDesktopWindow               GetDesktopWindow;
  WinNtGetForegroundWindow            GetForegroundWindow;
  WinNtCreateWindowEx                 CreateWindowEx;
  WinNtShowWindow                     ShowWindow;
  WinNtUpdateWindow                   UpdateWindow;
  WinNtDestroyWindow                  DestroyWindow;
  WinNtInvalidateRect                 InvalidateRect;
  WinNtGetWindowDC                    GetWindowDC;
  WinNtGetClientRect                  GetClientRect;
  WinNtAdjustWindowRect               AdjustWindowRect;
  WinNtSetDIBitsToDevice              SetDIBitsToDevice;
  WinNtBitBlt                         BitBlt;
  WinNtGetDC                          GetDC;
  WinNtReleaseDC                      ReleaseDC;
  WinNtRegisterClassEx                RegisterClassEx;
  WinNtUnregisterClass                UnregisterClass;

  WinNtBeginPaint                     BeginPaint;
  WinNtEndPaint                       EndPaint;
  WinNtPostQuitMessage                PostQuitMessage;
  WinNtDefWindowProc                  DefWindowProc;
  WinNtLoadIcon                       LoadIcon;
  WinNtLoadCursor                     LoadCursor;
  WinNtGetStockObject                 GetStockObject;
  WinNtSetViewportOrgEx               SetViewportOrgEx;
  WinNtSetWindowOrgEx                 SetWindowOrgEx;
  WinNtMoveWindow                     MoveWindow;
  WinNtGetWindowRect                  GetWindowRect;

  WinNtGetMessage                     GetMessage;
  WinNtTranslateMessage               TranslateMessage;
  WinNtDispatchMessage                DispatchMessage;

  WinNtGetProcessHeap                 GetProcessHeap;
  WinNtHeapAlloc                      HeapAlloc;
  WinNtHeapFree                       HeapFree;

} EFI_WIN_NT_THUNK_PROTOCOL;

#endif

extern EFI_GUID gEfiWinNtThunkProtocolGuid;

#endif
