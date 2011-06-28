/** @file
  Emulator Thunk to abstract OS services from pure EFI code

  Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EMU_THUNK_PROTOCOL_H__
#define __EMU_THUNK_PROTOCOL_H__

#define EMU_THUNK_PROTOCOL_GUID  \
 { 0x5CF32E0B, 0x8EDF, 0x2E44, { 0x9C, 0xDA, 0x93, 0x20, 0x5E, 0x99, 0xEC, 0x1C } }

// neded for things like EFI_TIME_CAPABILITIES
#include <Uefi.h>

#include <Library/PeCoffExtraActionLib.h>

#include <Protocol/EmuIoThunk.h>
#include <Protocol/DevicePath.h>


typedef struct {
  VENDOR_DEVICE_PATH  VendorDevicePath;
  UINT32              Instance;
} EMU_VENDOR_DEVICE_PATH_NODE;

typedef struct {
  EMU_VENDOR_DEVICE_PATH_NODE Vendor;
  EFI_DEVICE_PATH_PROTOCOL    EndDevicePath;
} EMU_THUNK_DEVICE_PATH;



typedef struct _EMU_THUNK_PROTOCOL  EMU_THUNK_PROTOCOL;



typedef
UINTN
(EFIAPI *EMU_WRITE_STD_ERROR) (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  );

typedef
EFI_STATUS
(EFIAPI *EMU_CONFIG_STD_IN) (
  VOID
  );

typedef
UINTN
(EFIAPI *EMU_WRITE_STD_OUT) (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  );

typedef
UINTN
(EFIAPI *EMU_READ_STD_IN) (
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
  );

typedef
BOOLEAN
(EFIAPI *EMU_POLL_STD_IN) (
  VOID
  );


typedef
VOID *
(EFIAPI *EMU_OS_MALLOC) (
  IN  UINTN Size
  );

typedef
VOID *
(EFIAPI *EMU_OS_VMALLOC) (
  IN  UINTN Size
  );

typedef
BOOLEAN
(EFIAPI *EMU_OS_FREE) (
  IN  VOID *Ptr
  );


typedef
EFI_STATUS
(EFIAPI *EMU_PE_COFF_GET_ENTRY_POINT) (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  );

typedef
VOID
(EFIAPI *EMU_PE_COFF_RELOCATE_EXTRA_ACTION) (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

typedef
VOID
(EFIAPI *EMU_PE_COFF_UNLOAD_EXTRA_ACTION) (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

typedef
VOID
(EFIAPI *EMU_ENABLE_INERRUPTS) (
  VOID
  );

typedef
VOID
(EFIAPI *EMU_DISABLE_INERRUPTS) (
  VOID
  );

typedef
UINT64
(EFIAPI *EMU_QUERY_PERFORMANCE_FREQENCY) (
  VOID
  );

typedef
UINT64
(EFIAPI *EMU_QUERY_PERFORMANCE_COUNTER) (
  VOID
  );

typedef
VOID
(EFIAPI *EMU_SLEEP) (
  IN  UINT64    Milliseconds
  );

typedef
VOID
(EFIAPI *EMU_CPU_SLEEP) (
  VOID
  );

typedef
VOID
(EFIAPI *EMU_EXIT) (
  IN  UINTN    Status
  );

typedef
VOID
(EFIAPI *EMU_GET_TIME) (
  OUT  EFI_TIME               *Time,
  OUT EFI_TIME_CAPABILITIES   *Capabilities OPTIONAL
  );

typedef
VOID
(EFIAPI *EMU_SET_TIME) (
  IN   EFI_TIME               *Time
  );


typedef
VOID
(EFIAPI EMU_SET_TIMER_CALLBACK) (
  IN  UINT64  DeltaMs
  );

typedef
VOID
(EFIAPI *EMU_SET_TIMER) (
  IN  UINT64                  PeriodMs,
  IN  EMU_SET_TIMER_CALLBACK  CallBack
  );



/**
  Enumerates the current set of protocol instances that abstract OS services from EFI.

  A given protocol can have multiple instances. Usually a protocol is configured via a
  single PCD string. The data associated for each instance is seperated via a ! in the string.
  EMU_IO_THUNK_PROTOCOL_CLOSE.ConfigString will contain the information in the PCD string up to the next !.
  Thus each instance has a unique ConfigString.

  @param  EmuBusDriver          TRUE means only return protocol instances that need to be produced
                                by the EmuBusDriver. FALSE means return all possible protocols
  @param  Instance              On input the protocol to search for, or NULL to start a search
                                of all the supported protocol instances.
  @param  NextProtocol          On output it represents the next value to be passed into Protocol.
  @param  Interface             A pointer to the EMU_IO_THUNK_PROTOCOL_CLOSE interface.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The next protocol instance was not found.
  @retval EFI_INVALID_PARAMETER Instance is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EMU_GET_NEXT_PROTOCOL) (
  IN  BOOLEAN                 EmuBusDriver,
  OUT EMU_IO_THUNK_PROTOCOL   **Instance  OPTIONAL
  );


struct _EMU_THUNK_PROTOCOL {
  // Used for early debug printing
  EMU_WRITE_STD_ERROR               WriteStdErr;
  EMU_CONFIG_STD_IN                 ConfigStdIn;
  EMU_WRITE_STD_OUT                 WriteStdOut;
  EMU_READ_STD_IN                   ReadStdIn;
  EMU_POLL_STD_IN                   PollStdIn;

  //
  // Map OS malloc/free so we can use OS based guard malloc
  //
  EMU_OS_MALLOC                     Malloc;
  EMU_OS_VMALLOC                    Valloc;
  EMU_OS_FREE                       Free;


  ///
  /// PE/COFF loader hooks to get symbols loaded
  ///
  EMU_PE_COFF_GET_ENTRY_POINT       PeCoffGetEntryPoint;
  EMU_PE_COFF_RELOCATE_EXTRA_ACTION PeCoffRelocateImageExtraAction;
  EMU_PE_COFF_UNLOAD_EXTRA_ACTION   PeCoffUnloadImageExtraAction;

  ///
  /// DXE Architecture Protocol Services
  ///
  EMU_ENABLE_INERRUPTS              EnableInterrupt;
  EMU_DISABLE_INERRUPTS             DisableInterrupt;
  EMU_QUERY_PERFORMANCE_FREQENCY    QueryPerformanceFrequency;
  EMU_QUERY_PERFORMANCE_COUNTER     QueryPerformanceCounter;

  EMU_SLEEP                         Sleep;
  EMU_CPU_SLEEP                     CpuSleep;
  EMU_EXIT                          Exit;
  EMU_GET_TIME                      GetTime;
  EMU_SET_TIME                      SetTime;
  EMU_SET_TIMER                     SetTimer;

  ///
  /// Generic System Services
  ///
  EMU_GET_NEXT_PROTOCOL             GetNextProtocol;
};

extern EFI_GUID gEmuThunkProtocolGuid;

#endif
