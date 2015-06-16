/** @file
  Internal header file for Extended SAL variable service module.

Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VARIABLE_H_
#define _VARIABLE_H_

#include <PiDxe.h>

#include <Protocol/VariableWrite.h>
#include <Protocol/FaultTolerantWrite.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/Variable.h>
#include <Protocol/ExtendedSalBootService.h>
#include <Protocol/ExtendedSalServiceClasses.h>

#include <Guid/GlobalVariable.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/EventGroup.h>

#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ExtendedSalLib.h>
#include <Library/BaseCryptLib.h>

#define MAX_NAME_SIZE            0x100
#define NUM_VAR_NAME             9  // Number of pre-defined variable name to be referenced
#define VAR_PLATFORM_LANG_CODES  0  // Index of "PlatformLangCodes" variable
#define VAR_LANG_CODES           1  // Index of "LangCodes" variable
#define VAR_PLATFORM_LANG        2  // Index of "PlatformLang" variable
#define VAR_LANG                 3  // Index of "Lang" variable
#define VAR_HW_ERR_REC           4  // Index of "HwErrRecXXXX" variable
#define VAR_AUTH_KEY_DB          5  // Index of "AuthVarKeyDatabase" variable
#define VAR_SETUP_MODE           6  // Index of "SetupMode" variable
#define VAR_PLATFORM_KEY         7  // Index of "PK" variable
#define VAR_KEY_EXCHANGE_KEY     8  // Index of "KEK" variable

///
/// "AuthVarKeyDatabase" variable for the Public Key store.
///
#define AUTHVAR_KEYDB_NAME      L"AuthVarKeyDatabase"
#define AUTHVAR_KEYDB_NAME_SIZE 38

///
/// The maximum size of the public key database, restricted by maximum individal EFI 
/// varible size, and excluding the variable header and name size.
///
#define MAX_KEYDB_SIZE  (FixedPcdGet32 (PcdMaxVariableSize) - sizeof (AUTHENTICATED_VARIABLE_HEADER) - AUTHVAR_KEYDB_NAME_SIZE)
#define MAX_KEY_NUM     (MAX_KEYDB_SIZE / EFI_CERT_TYPE_RSA2048_SIZE)

///
/// The size of a 3 character ISO639 language code.
///
#define ISO_639_2_ENTRY_SIZE    3

typedef enum {
  Physical,
  Virtual
} VARIABLE_POINTER_TYPE;

typedef struct {
  EFI_PHYSICAL_ADDRESS CurrPtr;
  EFI_PHYSICAL_ADDRESS EndPtr;
  EFI_PHYSICAL_ADDRESS StartPtr;
  BOOLEAN              Volatile;
} VARIABLE_POINTER_TRACK;

typedef struct {
  EFI_PHYSICAL_ADDRESS  VolatileVariableBase;
  EFI_PHYSICAL_ADDRESS  NonVolatileVariableBase;
  EFI_LOCK              VariableServicesLock;
} VARIABLE_GLOBAL;

typedef struct {
  VARIABLE_GLOBAL VariableGlobal[2];
  CHAR16          *VariableName[2][NUM_VAR_NAME];
  EFI_GUID        *GlobalVariableGuid[2];
  UINTN           VolatileLastVariableOffset;
  UINTN           NonVolatileLastVariableOffset;
  UINTN           CommonVariableTotalSize;
  UINTN           HwErrVariableTotalSize;
  CHAR8           *PlatformLangCodes[2];
  CHAR8           *LangCodes[2];
  CHAR8           *PlatformLang[2];
  CHAR8           Lang[ISO_639_2_ENTRY_SIZE + 1];
  UINT32          FvbInstance;
  UINT32          ReentrantState;
  EFI_GUID        *AuthenticatedVariableGuid[2];
  EFI_GUID        *CertRsa2048Sha256Guid[2];
  EFI_GUID        *ImageSecurityDatabaseGuid[2];
  VOID            *HashContext[2];             // Hash context pointer
  UINT8           KeyList[MAX_KEYDB_SIZE];     // Cached Platform Key list
  UINT8           PubKeyStore[MAX_KEYDB_SIZE]; // Cached Public Key list
} ESAL_VARIABLE_GLOBAL;

typedef struct {
  EFI_GUID    *Guid;
  CHAR16      *Name;
  UINT32      Attributes;
  UINTN       DataSize;
  VOID        *Data;
} VARIABLE_CACHE_ENTRY;


extern ESAL_VARIABLE_GLOBAL  *mVariableModuleGlobal;

//
// Functions
//

/**
  Initializes variable store area for non-volatile and volatile variable.

  This function allocates and initializes memory space for global context of ESAL
  variable service and variable store area for non-volatile and volatile variable.

  @param[in]  ImageHandle       The Image handle of this driver.
  @param[in]  SystemTable       The pointer of EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate enough memory resource.

**/
EFI_STATUS
VariableCommonInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

/**
  Entry point of Extended SAL Variable service module.

  This function is the entry point of Extended SAL Variable service module.
  It registers all functions of Extended SAL Variable class, initializes
  variable store for non-volatile and volatile variables, and registers
  notification function for EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

  @param[in]  ImageHandle   The Image handle of this driver.
  @param[in]  SystemTable   The pointer of EFI_SYSTEM_TABLE.

  @retval     EFI_SUCCESS   Extended SAL Variable Services Class successfully registered.

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

  @param[in]  Event        The event whose notification function is being invoked.
  @param[in]  Context      The pointer to the notification function's context.

**/
VOID
EFIAPI
VariableClassAddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );

/**
  Implements EsalGetVariable function of Extended SAL Variable Services Class.

  This function implements EsalGetVariable function of Extended SAL Variable Services Class.
  It is equivalent in functionality to the EFI Runtime Service GetVariable().
  
  @param[in]      VariableName    A Null-terminated Unicode string that is the name of
                                  the vendor's variable.
  @param[in]      VendorGuid      A unique identifier for the vendor.
  @param[out]     Attributes      If not NULL, a pointer to the memory location to return the 
                                  attributes bitmask for the variable.
  @param[in, out] DataSize        Size of Data found. If size is less than the
                                  data, this value contains the required size.
  @param[out]     Data            On input, the size in bytes of the return Data buffer.  
                                  On output, the size of data returned in Data.
  @param[in]      VirtualMode     Current calling mode for this function.
  @param[in]      Global          Context of this Extended SAL Variable Services Class call.

  @retval EFI_SUCCESS            The function completed successfully. 
  @retval EFI_NOT_FOUND          The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   DataSize is too small for the result.  DataSize has 
                                 been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER  VariableName is NULL.
  @retval EFI_INVALID_PARAMETER  VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER  DataSize is NULL.
  @retval EFI_INVALID_PARAMETER  DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION The variable could not be retrieved due to an authentication failure.

**/
EFI_STATUS
EFIAPI
EsalGetVariable (
  IN      CHAR16                *VariableName,
  IN      EFI_GUID              *VendorGuid,
  OUT     UINT32                *Attributes OPTIONAL,
  IN OUT  UINTN                 *DataSize,
  OUT     VOID                  *Data,
  IN      BOOLEAN               VirtualMode,
  IN      ESAL_VARIABLE_GLOBAL  *Global
  );

/**
  Implements EsalGetNextVariableName function of Extended SAL Variable Services Class.

  This function implements EsalGetNextVariableName function of Extended SAL Variable Services Class.
  It is equivalent in functionality to the EFI Runtime Service GetNextVariableName().
  
  @param[in, out] VariableNameSize Size of the variable
  @param[in, out] VariableName     On input, supplies the last VariableName that was returned by GetNextVariableName().
                                   On output, returns the Null-terminated Unicode string of the current variable.
  @param[in, out] VendorGuid       On input, supplies the last VendorGuid that was returned by GetNextVariableName().
                                   On output, returns the VendorGuid of the current variable.  
  @param[in]      VirtualMode      Current calling mode for this function.
  @param[in]      Global           Context of this Extended SAL Variable Services Class call.

  @retval EFI_SUCCESS             The function completed successfully. 
  @retval EFI_NOT_FOUND           The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL    VariableNameSize is too small for the result. 
                                  VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER   VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER   VariableName is NULL.
  @retval EFI_INVALID_PARAMETER   VendorGuid is NULL.
  @retval EFI_DEVICE_ERROR        The variable name could not be retrieved due to a hardware error.

**/
EFI_STATUS
EFIAPI
EsalGetNextVariableName (
  IN OUT  UINTN                 *VariableNameSize,
  IN OUT  CHAR16                *VariableName,
  IN OUT  EFI_GUID              *VendorGuid,
  IN      BOOLEAN               VirtualMode,
  IN      ESAL_VARIABLE_GLOBAL  *Global
  );

/**
  Implements EsalSetVariable function of Extended SAL Variable Services Class.

  This function implements EsalSetVariable function of Extended SAL Variable Services Class.
  It is equivalent in functionality to the EFI Runtime Service SetVariable().
  
  @param[in]  VariableName       A Null-terminated Unicode string that is the name of the vendor's
                                 variable.  Each VariableName is unique for each 
                                 VendorGuid.  VariableName must contain 1 or more 
                                 Unicode characters.  If VariableName is an empty Unicode 
                                 string, then EFI_INVALID_PARAMETER is returned.
  @param[in]  VendorGuid         A unique identifier for the vendor.
  @param[in]  Attributes         Attributes bitmask to set for the variable.
  @param[in]  DataSize           The size in bytes of the Data buffer.  A size of zero causes the
                                 variable to be deleted.
  @param[in]  Data               The contents for the variable.
  @param[in]  VirtualMode        Current calling mode for this function.
  @param[in]  Global             Context of this Extended SAL Variable Services Class call.

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as 
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits was supplied, or the 
                                 DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER  VariableName is an empty Unicode string.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved due to a hardware failure.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION The variable could not be retrieved due to an authentication failure.
  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.

**/
EFI_STATUS
EFIAPI
EsalSetVariable (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINT32                  Attributes,
  IN UINTN                   DataSize,
  IN VOID                    *Data,
  IN BOOLEAN                 VirtualMode,
  IN ESAL_VARIABLE_GLOBAL    *Global
  );

/**
  Implements EsalQueryVariableInfo function of Extended SAL Variable Services Class.

  This function implements EsalQueryVariableInfo function of Extended SAL Variable Services Class.
  It is equivalent in functionality to the EFI Runtime Service QueryVariableInfo().

  @param[in]  Attributes                   Attributes bitmask to specify the type of variables
                                           on which to return information.
  @param[out] MaximumVariableStorageSize   On output the maximum size of the storage space available for 
                                           the EFI variables associated with the attributes specified.  
  @param[out] RemainingVariableStorageSize Returns the remaining size of the storage space available for EFI 
                                           variables associated with the attributes specified.
  @param[out] MaximumVariableSize          Returns the maximum size of an individual EFI variable 
                                           associated with the attributes specified.
  @param[in]  VirtualMode                  Current calling mode for this function
  @param[in]  Global                       Context of this Extended SAL Variable Services Class call

  @retval EFI_SUCCESS                      Valid answer returned.
  @retval EFI_INVALID_PARAMETER            An invalid combination of attribute bits was supplied.
  @retval EFI_UNSUPPORTED                  The attribute is not supported on this platform, and the 
                                           MaximumVariableStorageSize, RemainingVariableStorageSize, 
                                           MaximumVariableSize are undefined.
**/
EFI_STATUS
EFIAPI
EsalQueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize,
  IN  BOOLEAN                VirtualMode,
  IN  ESAL_VARIABLE_GLOBAL   *Global
  );

/**
  Writes a buffer to variable storage space.

  This function writes a buffer to variable storage space into firmware
  volume block device. The destination is specified by parameter
  VariableBase. Fault Tolerant Write protocol is used for writing.

  @param[in] VariableBase The base address of the variable to write.
  @param[in] Buffer       Points to the data buffer.
  @param[in] BufferSize   The number of bytes of the data Buffer.

  @retval EFI_SUCCESS     The function completed successfully.
  @retval EFI_NOT_FOUND   Fail to locate Fault Tolerant Write protocol.
  @retval Other           The function could not complete successfully.

**/
EFI_STATUS
FtwVariableSpace (
  IN EFI_PHYSICAL_ADDRESS   VariableBase,
  IN UINT8                  *Buffer,
  IN UINTN                  BufferSize
  );

/**
  Finds variable in volatile and non-volatile storage areas.

  This code finds variable in volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  Otherwise, VariableName and VendorGuid are compared.

  @param[in]  VariableName            Name of the variable to be found.
  @param[in]  VendorGuid              Vendor GUID to be found.
  @param[out] PtrTrack                VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.
  @param[in]  Global                  Pointer to VARIABLE_GLOBAL structure, including
                                      base of volatile variable storage area, base of
                                      NV variable storage area, and a lock.
  @param[in]  Instance                Instance of FV Block services.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL.
  @retval EFI_SUCCESS                 Variable successfully found.
  @retval EFI_INVALID_PARAMETER       Variable not found.

**/
EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global,
  IN  UINTN                   Instance
  );

/**
  Gets the pointer to variable data area.

  This function gets the pointer to variable data area.
  The variable is specified by its variable header.

  @param[in]  VariableAddress    Start address of variable header.
  @param[in]  Volatile           TRUE  - Variable is volatile.
                                 FALSE - Variable is non-volatile.
  @param[in]  Global             Pointer to VARAIBLE_GLOBAL structure.
  @param[in]  Instance           Instance of FV Block services.
  @param[out] VariableData       Buffer to hold variable data for output.

**/
VOID
GetVariableDataPtr (
  IN  EFI_PHYSICAL_ADDRESS   VariableAddress,
  IN  BOOLEAN                Volatile,
  IN  VARIABLE_GLOBAL        *Global,
  IN  UINTN                  Instance,
  OUT CHAR16                 *VariableData
  );

/**
  Gets the size of variable data area.

  This function gets the size of variable data area.
  The variable is specified by its variable header.
  If variable header contains raw data, just return 0.

  @param[in]  Variable  Pointer to the variable header.

  @return               Size of variable data area in bytes.

**/
UINTN
DataSizeOfVariable (
  IN  AUTHENTICATED_VARIABLE_HEADER     *Variable
  );

/**
  Update the variable region with Variable information. These are the same 
  arguments as the EFI Variable services.

  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Data               Variable data.
  @param[in] DataSize           Size of data. 0 means delete.
  @param[in] Attributes         Attributes of the variable.
  @param[in] KeyIndex           Index of associated public key.
  @param[in] MonotonicCount     Value of associated monotonic count. 
  @param[in] VirtualMode        Current calling mode for this function.
  @param[in] Global             Context of this Extended SAL Variable Services Class call.
  @param[in] Variable           The variable information which is used to keep track of variable usage.

  @retval EFI_SUCCESS           The update operation is success.
  @retval EFI_OUT_OF_RESOURCES  Variable region is full, can not write other data into this region.

**/
EFI_STATUS
EFIAPI
UpdateVariable (
  IN      CHAR16                  *VariableName,
  IN      EFI_GUID                *VendorGuid,
  IN      VOID                    *Data,
  IN      UINTN                   DataSize,
  IN      UINT32                  Attributes OPTIONAL,  
  IN      UINT32                  KeyIndex  OPTIONAL,
  IN      UINT64                  MonotonicCount  OPTIONAL,
  IN      BOOLEAN                 VirtualMode,
  IN      ESAL_VARIABLE_GLOBAL    *Global,
  IN      VARIABLE_POINTER_TRACK  *Variable
  );

/**
  Checks variable header.

  This function checks if variable header is valid or not.

  @param[in]  VariableAddress    Start address of variable header.
  @param[in]  Volatile           TRUE  - Variable is volatile.
                                 FALSE - Variable is non-volatile.
  @param[in]  Global             Pointer to VARAIBLE_GLOBAL structure.
  @param[in]  Instance           Instance of FV Block services.
  @param[out] VariableHeader     Pointer to AUTHENTICATED_VARIABLE_HEADER for output.

  @retval TRUE                   Variable header is valid.
  @retval FALSE                  Variable header is not valid.

**/
BOOLEAN
IsValidVariableHeader (
  IN  EFI_PHYSICAL_ADDRESS              VariableAddress,
  IN  BOOLEAN                           Volatile,
  IN  VARIABLE_GLOBAL                   *Global,
  IN  UINTN                             Instance,
  OUT AUTHENTICATED_VARIABLE_HEADER     *VariableHeader  OPTIONAL
  );

/**
  Flush the HOB variable to NV variable storage.
**/
VOID
FlushHob2Nv (
  VOID
  );

#endif
