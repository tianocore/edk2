/** @file

  Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _GASKET_H_
#define _GASKET_H_

//
// EMU_THUNK_PROTOCOL gaskets (EFIAPI to UNIX ABI)
//

UINTN
EFIAPI
GasketSecWriteStdErr (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  );

EFI_STATUS
EFIAPI
GasketSecConfigStdIn (
  VOID
  );

UINTN
EFIAPI
GasketSecWriteStdOut (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  );

UINTN
EFIAPI
GasketSecReadStdIn (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  );

BOOLEAN
EFIAPI
GasketSecPollStdIn (
  VOID
  );

VOID *
EFIAPI
GasketSecMalloc (
  IN  UINTN  Size
  );

VOID *
EFIAPI
GasketSecValloc (
  IN  UINTN  Size
  );

BOOLEAN
EFIAPI
GasketSecFree (
  IN  VOID  *Ptr
  );

RETURN_STATUS
EFIAPI
GasketSecPeCoffGetEntryPoint (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  );

VOID
EFIAPI
GasketSecPeCoffRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

VOID
EFIAPI
GasketSecPeCoffUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

VOID
EFIAPI
GasketSecSetTimer (
  IN  UINT64                  PeriodMs,
  IN  EMU_SET_TIMER_CALLBACK  CallBack
  );

VOID
EFIAPI
GasketSecEnableInterrupt (
  VOID
  );

VOID
EFIAPI
GasketSecDisableInterrupt (
  VOID
  );

UINT64
EFIAPI
GasketQueryPerformanceFrequency (
  VOID
  );

UINT64
EFIAPI
GasketQueryPerformanceCounter (
  VOID
  );

VOID
EFIAPI
GasketSecSleep (
  IN  UINT64  Nanoseconds
  );

VOID
EFIAPI
GasketSecCpuSleep (
  VOID
  );

VOID
EFIAPI
GasketSecExit (
  UINTN  Status
  );

VOID
EFIAPI
GasketSecGetTime (
  OUT  EFI_TIME              *Time,
  OUT EFI_TIME_CAPABILITIES  *Capabilities OPTIONAL
  );

VOID
EFIAPI
GasketSecSetTime (
  IN  EFI_TIME  *Time
  );

EFI_STATUS
EFIAPI
GasketSecGetNextProtocol (
  IN  BOOLEAN                EmuBusDriver,
  OUT EMU_IO_THUNK_PROTOCOL  **Instance  OPTIONAL
  );

// PPIs produced by SEC

EFI_STATUS
EFIAPI
GasketSecUnixPeiAutoScan (
  IN  UINTN                 Index,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryBase,
  OUT UINT64                *MemorySize
  );

VOID *
EFIAPI
GasketSecEmuThunkAddress (
  VOID
  );

EFI_STATUS
EFIAPI
GasketSecUnixUnixFwhAddress (
  IN OUT UINT64                *FwhSize,
  IN OUT EFI_PHYSICAL_ADDRESS  *FwhBase
  );

//
// Reverse (UNIX to EFIAPI) gaskets
//

typedef
void
(EFIAPI *CALL_BACK)(
  UINT64 Delta
  );

UINTN
ReverseGasketUint64 (
  UINTN   CallBack,
  UINT64  a
  );

UINTN
ReverseGasketUint64Uint64 (
  VOID  *CallBack,
  VOID  *Context,
  VOID  *Key
  );

//
// Gasket functions for EFI_EMU_UGA_IO_PROTOCOL
//

EFI_STATUS
EFIAPI
GasketX11Size (
  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsWindowsIo,
  UINT32                        Width,
  UINT32                        Height
  );

EFI_STATUS
EFIAPI
GasketX11CheckKey (
  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsWindowsIo
  );

EFI_STATUS
EFIAPI
GasketX11GetKey (
  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsWindowsIo,
  EFI_KEY_DATA                  *key
  );

EFI_STATUS
EFIAPI
GasketX11KeySetState (
  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsWindowsIo,
  EFI_KEY_TOGGLE_STATE          *KeyToggleState
  );

EFI_STATUS
EFIAPI
GasketX11RegisterKeyNotify (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL                      *GraphicsWindowsIo,
  IN EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK  MakeCallBack,
  IN EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK  BreakCallBack,
  IN VOID                                              *Context
  );

EFI_STATUS
EFIAPI
GasketX11Blt (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL    *GraphicsWindows,
  IN  EFI_UGA_PIXEL                   *BltBuffer OPTIONAL,
  IN  EFI_UGA_BLT_OPERATION           BltOperation,
  IN  EMU_GRAPHICS_WINDOWS__BLT_ARGS  *Args
  );

EFI_STATUS
EFIAPI
GasketX11CheckPointer (
  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsWindowsIo
  );

EFI_STATUS
EFIAPI
GasketX11GetPointerState (
  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsWindowsIo,
  EFI_SIMPLE_POINTER_STATE      *state
  );

EFI_STATUS
EFIAPI
GasketX11GraphicsWindowOpen (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketX11GraphicsWindowClose (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  );

// Pthreads

UINTN
EFIAPI
GasketPthreadMutexLock (
  IN VOID  *Mutex
  );

UINTN
EFIAPI
GasketPthreadMutexUnLock (
  IN VOID  *Mutex
  );

UINTN
EFIAPI
GasketPthreadMutexTryLock (
  IN VOID  *Mutex
  );

VOID *
EFIAPI
GasketPthreadMutexInit (
  IN VOID
  );

UINTN
EFIAPI
GasketPthreadMutexDestroy (
  IN VOID  *Mutex
  );

UINTN
EFIAPI
GasketPthreadCreate (
  IN  VOID                       *Thread,
  IN  VOID                       *Attribute,
  IN  THREAD_THUNK_THREAD_ENTRY  Start,
  IN  VOID                       *Context
  );

VOID
EFIAPI
GasketPthreadExit (
  IN VOID  *ValuePtr
  );

UINTN
EFIAPI
GasketPthreadSelf (
  VOID
  );

EFI_STATUS
EFIAPI
GasketPthreadOpen (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketPthreadClose (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  );

// PosixFileSystem

EFI_STATUS
EFIAPI
GasketPosixOpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL               **Root
  );

EFI_STATUS
EFIAPI
GasketPosixFileOpen (
  IN EFI_FILE_PROTOCOL   *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN CHAR16              *FileName,
  IN UINT64              OpenMode,
  IN UINT64              Attributes
  );

EFI_STATUS
EFIAPI
GasketPosixFileCLose (
  IN EFI_FILE_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketPosixFileDelete (
  IN EFI_FILE_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketPosixFileRead (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  );

EFI_STATUS
EFIAPI
GasketPosixFileWrite (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  );

EFI_STATUS
EFIAPI
GasketPosixFileSetPossition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  );

EFI_STATUS
EFIAPI
GasketPosixFileGetPossition (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            *Position
  );

EFI_STATUS
EFIAPI
GasketPosixFileGetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  );

EFI_STATUS
EFIAPI
GasketPosixFileSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  );

EFI_STATUS
EFIAPI
GasketPosixFileFlush (
  IN EFI_FILE_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketPosixFileSystmeThunkOpen (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketPosixFileSystmeThunkClose (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketEmuBlockIoReset (
  IN EMU_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  );

EFI_STATUS
EFIAPI
GasketEmuBlockIoReadBlocks (
  IN     EMU_BLOCK_IO_PROTOCOL  *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                LBA,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
  OUT VOID                      *Buffer
  );

EFI_STATUS
EFIAPI
GasketEmuBlockIoWriteBlocks (
  IN     EMU_BLOCK_IO_PROTOCOL  *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                LBA,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
  IN     VOID                   *Buffer
  );

EFI_STATUS
EFIAPI
GasketEmuBlockIoFlushBlocks (
  IN     EMU_BLOCK_IO_PROTOCOL  *This,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token
  );

EFI_STATUS
EFIAPI
GasketEmuBlockIoCreateMapping (
  IN     EMU_BLOCK_IO_PROTOCOL  *This,
  IN     EFI_BLOCK_IO_MEDIA     *Media
  );

EFI_STATUS
EFIAPI
GasketBlockIoThunkOpen (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketBlockIoThunkClose (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketSnpThunkOpen (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketSnpThunkClose (
  IN  EMU_IO_THUNK_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketSnpCreateMapping (
  IN     EMU_SNP_PROTOCOL         *This,
  IN     EFI_SIMPLE_NETWORK_MODE  *Media
  );

EFI_STATUS
EFIAPI
GasketSnpStart (
  IN EMU_SNP_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketSnpStop (
  IN EMU_SNP_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketSnpInitialize (
  IN EMU_SNP_PROTOCOL  *This,
  IN UINTN             ExtraRxBufferSize  OPTIONAL,
  IN UINTN             ExtraTxBufferSize  OPTIONAL
  );

EFI_STATUS
EFIAPI
GasketSnpReset (
  IN EMU_SNP_PROTOCOL  *This,
  IN BOOLEAN           ExtendedVerification
  );

EFI_STATUS
EFIAPI
GasketSnpShutdown (
  IN EMU_SNP_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
GasketSnpReceiveFilters (
  IN EMU_SNP_PROTOCOL  *This,
  IN UINT32            Enable,
  IN UINT32            Disable,
  IN BOOLEAN           ResetMCastFilter,
  IN UINTN             MCastFilterCnt     OPTIONAL,
  IN EFI_MAC_ADDRESS   *MCastFilter OPTIONAL
  );

EFI_STATUS
EFIAPI
GasketSnpStationAddress (
  IN EMU_SNP_PROTOCOL  *This,
  IN BOOLEAN           Reset,
  IN EFI_MAC_ADDRESS   *New OPTIONAL
  );

EFI_STATUS
EFIAPI
GasketSnpStatistics (
  IN EMU_SNP_PROTOCOL         *This,
  IN BOOLEAN                  Reset,
  IN OUT UINTN                *StatisticsSize   OPTIONAL,
  OUT EFI_NETWORK_STATISTICS  *StatisticsTable  OPTIONAL
  );

EFI_STATUS
EFIAPI
GasketSnpMCastIpToMac (
  IN EMU_SNP_PROTOCOL  *This,
  IN BOOLEAN           IPv6,
  IN EFI_IP_ADDRESS    *IP,
  OUT EFI_MAC_ADDRESS  *MAC
  );

EFI_STATUS
EFIAPI
GasketSnpNvData (
  IN EMU_SNP_PROTOCOL  *This,
  IN BOOLEAN           ReadWrite,
  IN UINTN             Offset,
  IN UINTN             BufferSize,
  IN OUT VOID          *Buffer
  );

EFI_STATUS
EFIAPI
GasketSnpGetStatus (
  IN EMU_SNP_PROTOCOL  *This,
  OUT UINT32           *InterruptStatus OPTIONAL,
  OUT VOID             **TxBuf OPTIONAL
  );

EFI_STATUS
EFIAPI
GasketSnpTransmit (
  IN EMU_SNP_PROTOCOL  *This,
  IN UINTN             HeaderSize,
  IN UINTN             BufferSize,
  IN VOID              *Buffer,
  IN EFI_MAC_ADDRESS   *SrcAddr  OPTIONAL,
  IN EFI_MAC_ADDRESS   *DestAddr OPTIONAL,
  IN UINT16            *Protocol OPTIONAL
  );

EFI_STATUS
EFIAPI
GasketSnpReceive (
  IN EMU_SNP_PROTOCOL  *This,
  OUT UINTN            *HeaderSize OPTIONAL,
  IN OUT UINTN         *BufferSize,
  OUT VOID             *Buffer,
  OUT EFI_MAC_ADDRESS  *SrcAddr    OPTIONAL,
  OUT EFI_MAC_ADDRESS  *DestAddr   OPTIONAL,
  OUT UINT16           *Protocol   OPTIONAL
  );

#endif
