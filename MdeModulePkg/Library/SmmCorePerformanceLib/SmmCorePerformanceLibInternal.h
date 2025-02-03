/** @file
  Master header files for SmmCorePerformanceLib instance.

  This header file holds the prototypes of the SMM Performance and PerformanceEx Protocol published by this
  library instance at its constructor.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_CORE_PERFORMANCE_LIB_INTERNAL_H_
#define _SMM_CORE_PERFORMANCE_LIB_INTERNAL_H_

#include <Guid/EventGroup.h>
#include <Guid/ExtendedFirmwarePerformance.h>
#include <Guid/FirmwarePerformance.h>
#include <Guid/Performance.h>
#include <Guid/PerformanceMeasurement.h>
#include <Guid/ZeroGuid.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PerformanceLib.h>
#include <Library/SmmMemLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/TimerLib.h>

#include <Protocol/DevicePathToText.h>
#include <Protocol/LoadedImage.h>

#define STRING_SIZE              (FPDT_STRING_EVENT_RECORD_NAME_LENGTH * sizeof (CHAR8))
#define FIRMWARE_RECORD_BUFFER   0x1000
#define CACHE_HANDLE_GUID_COUNT  0x100

extern BOOLEAN  mPerformanceMeasurementEnabled;

//
// Library internal function declarations
//

/**
  Return a pointer to the loaded image protocol for the given handle.

  @param[in]  Handle      A handle to query for the loaded image protocol.

  @return A pointer to a loaded image protocol instance or null if the handle does not support load image protocol.
**/
EFI_LOADED_IMAGE_PROTOCOL *
GetLoadedImageProtocol (
  IN EFI_HANDLE  Handle
  );

/**
  Get the module name from the PDB file name in the image header.

  @param[in]  ImageBase     The base address of the image.
  @param[out] NameString    The buffer to store the name string.
  @param[in]  BufferSize    The size of the buffer in bytes.

  @retval EFI_SUCCESS           The name string is successfully retrieved.
  @retval EFI_INVALID_PARAMETER A pointer argument provided is null.
  @retval EFI_NOT_FOUND         The module name was not found.

**/
EFI_STATUS
GetModuleNameFromPdbString (
  IN VOID    *ImageBase,
  OUT CHAR8  *NameString,
  IN UINTN   BufferSize
  );

/**
  Get the module name from the user interface section.

  @param[in]  ModuleGuid    The GUID of the module.
  @param[out] NameString    The buffer to store the name string.
  @param[in]  BufferSize    The size of the buffer in bytes.

  @retval EFI_SUCCESS      The name string is successfully retrieved.
  @retval EFI_NOT_FOUND    The module name was not found.

**/
EFI_STATUS
GetNameFromUiSection (
  IN EFI_GUID  *ModuleGuid,
  OUT CHAR8    *NameString,
  IN UINTN     BufferSize
  );

/**
  Common initialization code for the MM Core Performance Library.

  @param[in] ExitBootServicesProtocolGuid  The GUID of the ExitBootServices protocol.

  @retval     EFI_SUCCESS           The MM Core Performance Library was initialized successfully.
  @retval     Others                The MM Core Performance Library was not initialized successfully.
 **/
EFI_STATUS
InitializeMmCorePerformanceLibCommon (
  IN CONST EFI_GUID  *ExitBootServicesProtocolGuid
  );

/**
  A library internal MM-instance specific implementation to check if a buffer outside MM is valid.

  This function is provided so Standalone MM and Traditional MM may use a different implementation
  of data buffer check logic.

  @param[in] Buffer  The buffer start address to be checked.
  @param[in] Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture.
  @retval FALSE This buffer is not valid per processor architecture.
**/
BOOLEAN
MmCorePerformanceIsNonPrimaryBufferValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  );

/**
  A library internal MM-instance specific implementation to check if a comm buffer is valid.

  This function is provided so Standalone MM and Traditional MM may use a different implementation
  of comm buffer check logic.

  @param[in] Buffer  The buffer start address to be checked.
  @param[in] Length  The buffer length to be checked.

  @retval TRUE  This communicate buffer is valid per processor architecture.
  @retval FALSE This communicate buffer is not valid per processor architecture.
**/
BOOLEAN
MmCorePerformanceIsPrimaryBufferValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  );

//
// Interface declarations for SMM PerformanceMeasurement Protocol.
//

/**
  Create performance record with event description and a timestamp.

  @param CallerIdentifier  - Image handle or pointer to caller ID GUID.
  @param Guid              - Pointer to a GUID.
  @param String            - Pointer to a string describing the measurement.
  @param TimeStamp         - 64-bit time stamp.
  @param Address           - Pointer to a location in memory relevant to the measurement.
  @param Identifier        - Performance identifier describing the type of measurement.
  @param Attribute         - The attribute of the measurement. According to attribute can create a start
                             record for PERF_START/PERF_START_EX, or a end record for PERF_END/PERF_END_EX,
                             or a general record for other Perf macros.

  @retval EFI_SUCCESS           - Successfully created performance record.
  @retval EFI_OUT_OF_RESOURCES  - Ran out of space to store the records.
  @retval EFI_INVALID_PARAMETER - Invalid parameter passed to function - NULL
                                  pointer or invalid PerfId.
**/
EFI_STATUS
EFIAPI
CreatePerformanceMeasurement (
  IN CONST VOID                        *CallerIdentifier  OPTIONAL,
  IN CONST VOID                        *Guid      OPTIONAL,
  IN CONST CHAR8                       *String    OPTIONAL,
  IN       UINT64                      TimeStamp  OPTIONAL,
  IN       UINT64                      Address    OPTIONAL,
  IN       UINT32                      Identifier,
  IN       PERF_MEASUREMENT_ATTRIBUTE  Attribute
  );

#endif
