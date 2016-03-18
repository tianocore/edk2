/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __LINUX_LOADER_H__
#define __LINUX_LOADER_H__

#include <Library/BdsLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/EfiShellParameters.h>
#include <Protocol/EfiShell.h>

#include <libfdt.h>

//
// Definitions
//

#define MAX_MSG_LEN 80

#define LINUX_UIMAGE_SIGNATURE    0x56190527
#define LINUX_KERNEL_MAX_OFFSET   (SystemMemoryBase + PcdGet32(PcdArmLinuxKernelMaxOffset))
#define LINUX_ATAG_MAX_OFFSET     (SystemMemoryBase + PcdGet32(PcdArmLinuxAtagMaxOffset))
#define LINUX_FDT_MAX_OFFSET      (SystemMemoryBase + PcdGet32(PcdArmLinuxFdtMaxOffset))

#define ARM_FDT_MACHINE_TYPE      0xFFFFFFFF

// Additional size that could be used for FDT entries added by the UEFI OS Loader
// Estimation based on: EDID (300bytes) + bootargs (200bytes) + initrd region (20bytes)
//                      + system memory region (20bytes) + mp_core entries (200 bytes)
#define FDT_ADDITIONAL_ENTRIES_SIZE     0x300

//
// Global variables
//
extern CONST EFI_GUID mLinuxLoaderHiiGuid;
extern EFI_HANDLE mLinuxLoaderHiiHandle;

//
// Local Types
//
typedef struct _SYSTEM_MEMORY_RESOURCE {
  LIST_ENTRY                  Link; // This attribute must be the first entry of this structure (to avoid pointer computation)
  EFI_PHYSICAL_ADDRESS        PhysicalStart;
  UINT64                      ResourceLength;
} SYSTEM_MEMORY_RESOURCE;

typedef VOID (*LINUX_KERNEL)(UINT32 Zero, UINT32 Arch, UINTN ParametersBase);

//
// Functions
//
EFI_STATUS
PrintHii (
  IN CONST CHAR8          *Language OPTIONAL,
  IN CONST EFI_STRING_ID  HiiFormatStringId,
  ...
  );

VOID
PrintHelp (
  IN CONST CHAR8  *Language OPTIONAL
  );

EFI_STATUS
ProcessShellParameters (
  OUT  CHAR16   **KernelPath,
  OUT  CHAR16   **FdtPath,
  OUT  CHAR16   **InitrdPath,
  OUT  CHAR16   **LinuxCommandLine,
  OUT  UINTN    *AtagMachineType
  );

EFI_STATUS
ProcessAppCommandLine (
  OUT  CHAR16   **KernelTextDevicePath,
  OUT  CHAR16   **FdtTextDevicePath,
  OUT  CHAR16   **InitrdTextDevicePath,
  OUT  CHAR16   **LinuxCommandLine,
  OUT  UINTN    *AtagMachineType
  );

VOID
PrintPerformance (
  VOID
  );

EFI_STATUS
GetSystemMemoryResources (
  IN  LIST_ENTRY *ResourceList
  );

EFI_STATUS
PrepareFdt (
  IN     EFI_PHYSICAL_ADDRESS SystemMemoryBase,
  IN     CONST CHAR8*         CommandLineArguments,
  IN     EFI_PHYSICAL_ADDRESS InitrdImage,
  IN     UINTN                InitrdImageSize,
  IN OUT EFI_PHYSICAL_ADDRESS *FdtBlobBase,
  IN OUT UINTN                *FdtBlobSize
  );

/**
  Start a Linux kernel from a Device Path

  @param  SystemMemoryBase      Base of the system memory
  @param  LinuxKernel           Device Path to the Linux Kernel
  @param  Parameters            Linux kernel arguments
  @param  Fdt                   Device Path to the Flat Device Tree
  @param  MachineType           ARM machine type value

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.
  @retval RETURN_UNSUPPORTED    ATAG is not support by this architecture

**/
EFI_STATUS
BootLinuxAtag (
  IN  EFI_PHYSICAL_ADDRESS      SystemMemoryBase,
  IN  EFI_DEVICE_PATH_PROTOCOL* LinuxKernelDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL* InitrdDevicePath,
  IN  CONST CHAR8*              CommandLineArguments,
  IN  UINTN                     MachineType
  );

/**
  Start a Linux kernel from a Device Path

  @param[in]  LinuxKernelDevicePath  Device Path to the Linux Kernel
  @param[in]  InitrdDevicePath       Device Path to the Initrd
  @param[in]  Arguments              Linux kernel arguments

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BootLinuxFdt (
  IN  EFI_PHYSICAL_ADDRESS      SystemMemoryBase,
  IN  EFI_DEVICE_PATH_PROTOCOL* LinuxKernelDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL* InitrdDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL* FdtDevicePath,
  IN  CONST CHAR8*              Arguments
  );

#endif /* __LINUX_LOADER_H__ */
