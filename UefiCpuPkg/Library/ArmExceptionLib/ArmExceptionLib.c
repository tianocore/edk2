/* @file
*  Main file supporting the SEC Phase for Versatile Express
*
*  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
*  Copyright (c) 2011-2021, Arm Limited. All rights reserved.<BR>
*  Copyright (c) 2016 HP Development Company, L.P.
*  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <Library/CpuExceptionHandlerLib.h>

#include <Guid/DebugImageInfoTable.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/UefiLib.h>

VOID
ExceptionHandlersStart (
  VOID
  );

VOID
ExceptionHandlersEnd (
  VOID
  );

RETURN_STATUS
ArchVectorConfig (
  IN  UINTN  VectorBaseAddress
  );

// these globals are provided by the architecture specific source (Arm or AArch64)
extern UINTN                   gMaxExceptionNumber;
extern EFI_EXCEPTION_CALLBACK  gExceptionHandlers[];
extern PHYSICAL_ADDRESS        gExceptionVectorAlignmentMask;

/**
Initializes all CPU exceptions entries and provides the default exception handlers.

Caller should try to get an array of interrupt and/or exception vectors that are in use and need to
persist by EFI_VECTOR_HANDOFF_INFO defined in PI 1.3 specification.
If caller cannot get reserved vector list or it does not exists, set VectorInfo to NULL.
If VectorInfo is not NULL, the exception vectors will be initialized per vector attribute accordingly.

@param[in]  VectorInfo    Pointer to reserved vector list.

@retval EFI_SUCCESS           CPU Exception Entries have been successfully initialized
with default exception handlers.
@retval EFI_INVALID_PARAMETER VectorInfo includes the invalid content if VectorInfo is not NULL.
@retval EFI_UNSUPPORTED       This function is not supported.

**/
EFI_STATUS
EFIAPI
InitializeCpuExceptionHandlers (
  IN EFI_VECTOR_HANDOFF_INFO  *VectorInfo OPTIONAL
  )
{
  UINT64  VectorBase;

  // use VBAR to point to where our exception handlers are

  // The vector table must be aligned for the architecture.  If this
  // assertion fails ensure the appropriate FFS alignment is in effect,
  // which can be accomplished by ensuring the proper Align=X statement
  // in the platform packaging rules.  For ARM Align=32 is required and
  // for AArch64 Align=4K is required.  Align=Auto can be used but this
  // is known to cause an issue with populating the reset vector area
  // for encapsulated FVs.
  ASSERT (((UINTN)ExceptionHandlersStart & gExceptionVectorAlignmentMask) == 0);

  VectorBase = (UINT64)(UINTN)ExceptionHandlersStart;

  // call the architecture-specific routine to prepare for the new vector
  // configuration to take effect
  ArchVectorConfig ((UINTN)VectorBase);

  ArmWriteVBar ((UINTN)VectorBase);

  return RETURN_SUCCESS;
}

/**
Registers a function to be called from the processor exception handler. (On ARM/AArch64 this only
provides exception handlers, not interrupt handling which is provided through the Hardware Interrupt
Protocol.)

This function registers and enables the handler specified by ExceptionHandler for a processor
interrupt or exception type specified by ExceptionType. If ExceptionHandler is NULL, then the
handler for the processor interrupt or exception type specified by ExceptionType is uninstalled.
The installed handler is called once for each processor interrupt or exception.
NOTE: This function should be invoked after InitializeCpuExceptionHandlers() is invoked,
otherwise EFI_UNSUPPORTED returned.

@param[in]  ExceptionType     Defines which interrupt or exception to hook.
@param[in]  ExceptionHandler  A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
when a processor interrupt occurs. If this parameter is NULL, then the handler
will be uninstalled.

@retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
@retval EFI_ALREADY_STARTED   ExceptionHandler is not NULL, and a handler for ExceptionType was
previously installed.
@retval EFI_INVALID_PARAMETER ExceptionHandler is NULL, and a handler for ExceptionType was not
previously installed.
@retval EFI_UNSUPPORTED       The interrupt specified by ExceptionType is not supported,
or this function is not supported.
**/
RETURN_STATUS
RegisterCpuInterruptHandler (
  IN EFI_EXCEPTION_TYPE         ExceptionType,
  IN EFI_CPU_INTERRUPT_HANDLER  ExceptionHandler
  )
{
  if ((UINTN)ExceptionType > gMaxExceptionNumber) {
    return RETURN_UNSUPPORTED;
  }

  if ((ExceptionHandler != NULL) && (gExceptionHandlers[ExceptionType] != NULL)) {
    return RETURN_ALREADY_STARTED;
  }

  gExceptionHandlers[ExceptionType] = ExceptionHandler;

  return RETURN_SUCCESS;
}

VOID
EFIAPI
CommonCExceptionHandler (
  IN     EFI_EXCEPTION_TYPE  ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  if ((UINTN)ExceptionType <= gMaxExceptionNumber) {
    if (gExceptionHandlers[ExceptionType]) {
      gExceptionHandlers[ExceptionType](ExceptionType, SystemContext);
      return;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Unknown exception type %d\n", ExceptionType));
    ASSERT (FALSE);
  }

  DumpImageAndCpuContent (ExceptionType, SystemContext);
}

/**
  Setup separate stacks for certain exception handlers.
  If the input Buffer and BufferSize are both NULL, use global variable if possible.

  @param[in]       Buffer        Point to buffer used to separate exception stack.
  @param[in, out]  BufferSize    On input, it indicates the byte size of Buffer.
                                 If the size is not enough, the return status will
                                 be EFI_BUFFER_TOO_SMALL, and output BufferSize
                                 will be the size it needs.

  @retval EFI_SUCCESS             The stacks are assigned successfully.
  @retval EFI_UNSUPPORTED         This function is not supported.
  @retval EFI_BUFFER_TOO_SMALL    This BufferSize is too small.
**/
EFI_STATUS
EFIAPI
InitializeSeparateExceptionStacks (
  IN     VOID   *Buffer,
  IN OUT UINTN  *BufferSize
  )
{
  return EFI_SUCCESS;
}

/**
  Use the EFI Debug Image Table to lookup the FaultAddress and find which PE/COFF image
  it came from. As long as the PE/COFF image contains a debug directory entry a
  string can be returned. For ELF and Mach-O images the string points to the Mach-O or ELF
  image. Microsoft tools contain a pointer to the PDB file that contains the debug information.

  @param  FaultAddress         Address to find PE/COFF image for.
  @param  ImageBase            Return load address of found image
  @param  PeCoffSizeOfHeaders  Return the size of the PE/COFF header for the image that was found

  @retval NULL                 FaultAddress not in a loaded PE/COFF image.
  @retval                      Path and file name of PE/COFF image.

**/
CHAR8 *
GetImageName (
  IN  UINTN  FaultAddress,
  OUT UINTN  *ImageBase,
  OUT UINTN  *PeCoffSizeOfHeaders
  )
{
  EFI_STATUS                         Status;
  EFI_DEBUG_IMAGE_INFO_TABLE_HEADER  *DebugTableHeader;
  EFI_DEBUG_IMAGE_INFO               *DebugTable;
  UINTN                              Entry;
  CHAR8                              *Address;

  Status = EfiGetSystemConfigurationTable (&gEfiDebugImageInfoTableGuid, (VOID **)&DebugTableHeader);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  DebugTable = DebugTableHeader->EfiDebugImageInfoTable;
  if (DebugTable == NULL) {
    return NULL;
  }

  Address = (CHAR8 *)(UINTN)FaultAddress;
  for (Entry = 0; Entry < DebugTableHeader->TableSize; Entry++, DebugTable++) {
    if (DebugTable->NormalImage != NULL) {
      if ((DebugTable->NormalImage->ImageInfoType == EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL) &&
          (DebugTable->NormalImage->LoadedImageProtocolInstance != NULL))
      {
        if ((Address >= (CHAR8 *)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase) &&
            (Address <= ((CHAR8 *)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase + DebugTable->NormalImage->LoadedImageProtocolInstance->ImageSize)))
        {
          *ImageBase           = (UINTN)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase;
          *PeCoffSizeOfHeaders = PeCoffGetSizeOfHeaders ((VOID *)(UINTN)*ImageBase);
          return PeCoffLoaderGetPdbPointer (DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase);
        }
      }
    }
  }

  return NULL;
}
