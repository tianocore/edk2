/** @file

  The internal header file includes the common header files, defines
  internal structure and functions used by EmuVariable module.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VARIABLE_H_
#define _VARIABLE_H_

#include <Uefi.h>

#include <Protocol/VariableWrite.h>
#include <Protocol/Variable.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Guid/VariableFormat.h>
#include <Guid/GlobalVariable.h>

#include <Guid/EventGroup.h>

#define GET_VARIABLE_NAME_PTR(a)  (CHAR16 *) ((UINTN) (a) + sizeof (VARIABLE_HEADER))

///
/// The size of a 3 character ISO639 language code.
///
#define ISO_639_2_ENTRY_SIZE    3

typedef enum {
  Physical,
  Virtual
} VARIABLE_POINTER_TYPE;

typedef struct {
  VARIABLE_HEADER *CurrPtr;
  VARIABLE_HEADER *EndPtr;
  VARIABLE_HEADER *StartPtr;
  BOOLEAN         Volatile;
} VARIABLE_POINTER_TRACK;

typedef struct {
  EFI_PHYSICAL_ADDRESS  VolatileVariableBase;
  EFI_PHYSICAL_ADDRESS  NonVolatileVariableBase;
  EFI_LOCK              VariableServicesLock;
} VARIABLE_GLOBAL;

typedef struct {
  VARIABLE_GLOBAL VariableGlobal[2];
  UINTN           VolatileLastVariableOffset;
  UINTN           NonVolatileLastVariableOffset;
  UINTN           CommonVariableTotalSize;
  UINTN           HwErrVariableTotalSize;
  CHAR8           *PlatformLangCodes;
  CHAR8           *LangCodes;
  CHAR8           *PlatformLang;
  CHAR8           Lang[ISO_639_2_ENTRY_SIZE + 1];
} ESAL_VARIABLE_GLOBAL;

///
/// Don't use module globals after the SetVirtualAddress map is signaled
///
extern ESAL_VARIABLE_GLOBAL *mVariableModuleGlobal;

/**
  Initializes variable store area for non-volatile and volatile variable.

  This function allocates and initializes memory space for global context of ESAL
  variable service and variable store area for non-volatile and volatile variable.

  @param  ImageHandle           The Image handle of this driver.
  @param  SystemTable           The pointer of EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
EFIAPI
VariableCommonInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

/**
  Entry point of EmuVariable service module.

  This function is the entry point of EmuVariable service module.
  It registers all interfaces of Variable Services, initializes
  variable store for non-volatile and volatile variables, and registers
  notification function for EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

  @param  ImageHandle   The Image handle of this driver.
  @param  SystemTable   The pointer of EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS   Variable service successfully initialized.

**/
EFI_STATUS
EFIAPI
VariableServiceInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
VariableClassAddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );

/**
  This code finds variable in storage blocks (Volatile or Non-Volatile).
  
  @param  VariableName           A Null-terminated Unicode string that is the name of
                                 the vendor's variable.
  @param  VendorGuid             A unique identifier for the vendor.
  @param  Attributes             If not NULL, a pointer to the memory location to return the 
                                 attributes bitmask for the variable.
  @param  DataSize               Size of Data found. If size is less than the
                                 data, this value contains the required size.
  @param  Data                   On input, the size in bytes of the return Data buffer.  
                                 On output, the size of data returned in Data.
  @param  Global                 Pointer to VARIABLE_GLOBAL structure

  @retval EFI_SUCCESS            The function completed successfully. 
  @retval EFI_NOT_FOUND          The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   DataSize is too small for the result.  DataSize has 
                                 been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER  VariableName or VendorGuid or DataSize is NULL.

**/
EFI_STATUS
EFIAPI
EmuGetVariable (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          *VendorGuid,
  OUT     UINT32            *Attributes OPTIONAL,
  IN OUT  UINTN             *DataSize,
  OUT     VOID              *Data,
  IN      VARIABLE_GLOBAL   *Global
  );

/**

  This code finds the next available variable.

  @param  VariableNameSize       Size of the variable.
  @param  VariableName           On input, supplies the last VariableName that was returned by GetNextVariableName().
                                 On output, returns the Null-terminated Unicode string of the current variable.
  @param  VendorGuid             On input, supplies the last VendorGuid that was returned by GetNextVariableName().
                                 On output, returns the VendorGuid of the current variable.  
  @param  Global                 Pointer to VARIABLE_GLOBAL structure.

  @retval EFI_SUCCESS            The function completed successfully. 
  @retval EFI_NOT_FOUND          The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   VariableNameSize is too small for the result. 
                                 VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER  VariableNameSize or VariableName or VendorGuid is NULL.

**/
EFI_STATUS
EFIAPI
EmuGetNextVariableName (
  IN OUT  UINTN             *VariableNameSize,
  IN OUT  CHAR16            *VariableName,
  IN OUT  EFI_GUID          *VendorGuid,
  IN      VARIABLE_GLOBAL   *Global
  );

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  @param  VariableName           A Null-terminated Unicode string that is the name of the vendor's
                                 variable.  Each VariableName is unique for each 
                                 VendorGuid.  VariableName must contain 1 or more 
                                 Unicode characters.  If VariableName is an empty Unicode 
                                 string, then EFI_INVALID_PARAMETER is returned.
  @param  VendorGuid             A unique identifier for the vendor
  @param  Attributes             Attributes bitmask to set for the variable
  @param  DataSize               The size in bytes of the Data buffer.  A size of zero causes the
                                 variable to be deleted.
  @param  Data                   The contents for the variable
  @param  Global                 Pointer to VARIABLE_GLOBAL structure
  @param  VolatileOffset         The offset of last volatile variable
  @param  NonVolatileOffset      The offset of last non-volatile variable

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as 
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits was supplied, or the 
                                 DataSize exceeds the maximum allowed, or VariableName is an empty 
                                 Unicode string, or VendorGuid is NULL.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved due to a hardware failure.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only or cannot be deleted.
  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.

**/
EFI_STATUS
EFIAPI
EmuSetVariable (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINT32                  Attributes,
  IN UINTN                   DataSize,
  IN VOID                    *Data,
  IN VARIABLE_GLOBAL         *Global,
  IN UINTN                   *VolatileOffset,
  IN UINTN                   *NonVolatileOffset
  );

/**

  This code returns information about the EFI variables.

  @param  Attributes                   Attributes bitmask to specify the type of variables
                                       on which to return information.
  @param  MaximumVariableStorageSize   On output the maximum size of the storage space available for 
                                       the EFI variables associated with the attributes specified.  
  @param  RemainingVariableStorageSize Returns the remaining size of the storage space available for EFI 
                                       variables associated with the attributes specified.
  @param  MaximumVariableSize          Returns the maximum size of an individual EFI variable 
                                       associated with the attributes specified.
  @param  Global                       Pointer to VARIABLE_GLOBAL structure.

  @retval EFI_SUCCESS                  Valid answer returned.
  @retval EFI_INVALID_PARAMETER        An invalid combination of attribute bits was supplied
  @retval EFI_UNSUPPORTED              The attribute is not supported on this platform, and the 
                                       MaximumVariableStorageSize, RemainingVariableStorageSize, 
                                       MaximumVariableSize are undefined.

**/
EFI_STATUS
EFIAPI
EmuQueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize,
  IN  VARIABLE_GLOBAL        *Global
  );

#endif
