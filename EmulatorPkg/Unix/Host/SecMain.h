/*++ @file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SEC_MAIN_H__
#define _SEC_MAIN_H__

//
// Name mangle to prevent build errors. I.e conflicts between EFI and OS
//
#define NTOHL   _UNIX_EFI_NAME_MANGLE_NTOHL_
#define HTONL   _UNIX_EFI_NAME_MANGLE_HTONL_
#define NTOHS   _UNIX_EFI_NAME_MANGLE_NTOHS_
#define HTONS   _UNIX_EFI_NAME_MANGLE_HTOHS_
#define B0      _UNIX_EFI_NAME_MANGLE_B0_

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termios.h>
#include <sys/time.h>

#if __CYGWIN__
#include <sys/dirent.h>
#else
#include <sys/dir.h>
#endif

#include <sys/mman.h>
#include <dlfcn.h>

#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/statvfs.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>

#ifdef __APPLE__
#include <net/if_dl.h>
#include <net/bpf.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/disk.h>
#define _XOPEN_SOURCE
#ifndef _Bool
  #define _Bool char // for clang debug
#endif
#else
#include <termio.h>
#include <sys/vfs.h>
#include <linux/fs.h>
#endif

#include <utime.h>

#undef NTOHL
#undef HTONL
#undef NTOHS
#undef HTONS
#undef B0


#include <PiPei.h>
#include <Uefi.h>

#include <Library/PeCoffLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>

#include <Library/ThunkPpiList.h>
#include <Library/ThunkProtocolList.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/EmuMagicPageLib.h>

#include <Ppi/EmuThunk.h>
#include <Ppi/StatusCode.h>

#include <Protocol/SimplePointer.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/SimpleFileSystem.h>

#include <Protocol/EmuThunk.h>
#include <Protocol/EmuIoThunk.h>
#include <Protocol/EmuGraphicsWindow.h>
#include <Protocol/EmuThread.h>
#include <Protocol/EmuBlockIo.h>
#include <Protocol/EmuSnp.h>

#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>


#include "Gasket.h"


#define STACK_SIZE                0x20000

typedef struct {
  EFI_PHYSICAL_ADDRESS  Address;
  UINT64                Size;
} EMU_FD_INFO;

typedef struct {
  EFI_PHYSICAL_ADDRESS  Memory;
  UINT64                Size;
} EMU_SYSTEM_MEMORY;


#define MAX_IMAGE_CONTEXT_TO_MOD_HANDLE_ARRAY_SIZE 0x100

typedef struct {
  PE_COFF_LOADER_IMAGE_CONTEXT   *ImageContext;
  VOID                           *ModHandle;
} IMAGE_CONTEXT_TO_MOD_HANDLE;


EFI_STATUS
EFIAPI
SecUnixPeiLoadFile (
  VOID                  *Pe32Data,
  EFI_PHYSICAL_ADDRESS  *ImageAddress,
  UINT64                *ImageSize,
  EFI_PHYSICAL_ADDRESS  *EntryPoint
  );

int
main (
  IN  int   Argc,
  IN  char  **Argv,
  IN  char  **Envp
  );

VOID
SecLoadFromCore (
  IN  UINTN   LargestRegion,
  IN  UINTN   LargestRegionSize,
  IN  UINTN   BootFirmwareVolumeBase,
  IN  VOID    *PeiCoreFile
  );

EFI_STATUS
SecLoadFile (
  IN  VOID                    *Pe32Data,
  IN  EFI_PHYSICAL_ADDRESS    *ImageAddress,
  IN  UINT64                  *ImageSize,
  IN  EFI_PHYSICAL_ADDRESS    *EntryPoint
  );

EFI_STATUS
SecFfsFindPeiCore (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  OUT VOID                        **Pe32Data
  );

EFI_STATUS
SecFfsFindNextFile (
  IN EFI_FV_FILETYPE             SearchType,
  IN EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  IN OUT EFI_FFS_FILE_HEADER     **FileHeader
  );

EFI_STATUS
SecFfsFindSectionData (
  IN EFI_SECTION_TYPE      SectionType,
  IN EFI_FFS_FILE_HEADER   *FfsFileHeader,
  IN OUT VOID              **SectionData
  );

EFI_STATUS
EFIAPI
SecUnixPeCoffLoaderLoadAsDll (
  IN CHAR8    *PdbFileName,
  IN VOID     **ImageEntryPoint,
  OUT VOID    **ModHandle
  );

EFI_STATUS
EFIAPI
SecUnixPeCoffLoaderFreeLibrary (
  OUT VOID    *ModHandle
  );

EFI_STATUS
SecUnixFdAddress (
  IN     UINTN                 Index,
  IN OUT EFI_PHYSICAL_ADDRESS  *FdBase,
  IN OUT UINT64                *FdSize,
  IN OUT EFI_PHYSICAL_ADDRESS  *FixUp
  )
;

EFI_STATUS
EFIAPI
GasketSecUnixFdAddress (
  IN     UINTN                 Index,
  IN OUT EFI_PHYSICAL_ADDRESS  *FdBase,
  IN OUT UINT64                *FdSize,
  IN OUT EFI_PHYSICAL_ADDRESS  *FixUp
  )
;


EFI_STATUS
GetImageReadFunction (
  IN PE_COFF_LOADER_IMAGE_CONTEXT          *ImageContext,
  IN EFI_PHYSICAL_ADDRESS                  *TopOfMemory
  );

EFI_STATUS
EFIAPI
SecImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  );

CHAR16                            *
AsciiToUnicode (
  IN  CHAR8   *Ascii,
  IN  UINTN   *StrLen OPTIONAL
  );

UINTN
CountSeperatorsInString (
  IN  const CHAR16   *String,
  IN  CHAR16   Seperator
  );

EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  );

EFI_STATUS
EFIAPI
GasketSecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  );


RETURN_STATUS
EFIAPI
SecPeCoffGetEntryPoint (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  );

VOID
EFIAPI
SecPeCoffRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

VOID
EFIAPI
SecPeCoffLoaderUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );


VOID
PeiSwitchStacks (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack
  );

VOID
SecInitThunkProtocol (
  VOID
  );


EFI_PHYSICAL_ADDRESS *
MapMemory (
  INTN fd,
  UINT64 length,
  INTN   prot,
  INTN   flags);

EFI_STATUS
MapFile (
  IN  CHAR8                     *FileName,
  IN OUT  EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                    *Length
  );

EFI_STATUS
MapFd0 (
  IN  CHAR8                     *FileName,
  IN OUT  EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                    *Length
  );

BOOLEAN
EfiSystemMemoryRange (
  IN  VOID *MemoryAddress
  );


VOID SecSleep (UINT64 Nanoseconds);
VOID SecEnableInterrupt (VOID);
VOID SecDisableInterrupt (VOID);
BOOLEAN SecInterruptEanbled (VOID);


extern EMU_THUNK_PROTOCOL    gEmuThunkProtocol;
extern EMU_IO_THUNK_PROTOCOL gX11ThunkIo;
extern EMU_IO_THUNK_PROTOCOL gPosixFileSystemThunkIo;
extern EMU_IO_THUNK_PROTOCOL gPthreadThunkIo;
extern EMU_IO_THUNK_PROTOCOL gBlockIoThunkIo;
extern EMU_IO_THUNK_PROTOCOL gSnpThunkIo;

#endif
