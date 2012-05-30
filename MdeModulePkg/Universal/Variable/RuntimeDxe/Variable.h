/** @file

  The internal header file includes the common header files, defines
  internal structure and functions used by Variable modules.

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
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
#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>
#include <Guid/VariableFormat.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/HardwareErrorVariable.h>

#define VARIABLE_RECLAIM_THRESHOLD (1024)

///
/// The size of a 3 character ISO639 language code.
///
#define ISO_639_2_ENTRY_SIZE    3

typedef enum {
  VariableStoreTypeVolatile,
  VariableStoreTypeHob,
  VariableStoreTypeNv,
  VariableStoreTypeMax
} VARIABLE_STORE_TYPE;

typedef struct {
  VARIABLE_HEADER *CurrPtr;
  VARIABLE_HEADER *EndPtr;
  VARIABLE_HEADER *StartPtr;
  BOOLEAN         Volatile;
} VARIABLE_POINTER_TRACK;

typedef struct {
  EFI_PHYSICAL_ADDRESS  HobVariableBase;
  EFI_PHYSICAL_ADDRESS  VolatileVariableBase;
  EFI_PHYSICAL_ADDRESS  NonVolatileVariableBase;
  EFI_LOCK              VariableServicesLock;
  UINT32                ReentrantState;
} VARIABLE_GLOBAL;

typedef struct {
  VARIABLE_GLOBAL VariableGlobal;
  UINTN           VolatileLastVariableOffset;
  UINTN           NonVolatileLastVariableOffset;
  UINTN           CommonVariableTotalSize;
  UINTN           HwErrVariableTotalSize;
  CHAR8           *PlatformLangCodes;
  CHAR8           *LangCodes;
  CHAR8           *PlatformLang;
  CHAR8           Lang[ISO_639_2_ENTRY_SIZE + 1];
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbInstance;
} VARIABLE_MODULE_GLOBAL;

typedef struct {
  EFI_GUID    *Guid;
  CHAR16      *Name;
  UINT32      Attributes;
  UINTN       DataSize;
  VOID        *Data;
} VARIABLE_CACHE_ENTRY;

/**
  Writes a buffer to variable storage space, in the working block.

  This function writes a buffer to variable storage space into a firmware
  volume block device. The destination is specified by the parameter
  VariableBase. Fault Tolerant Write protocol is used for writing.

  @param  VariableBase   Base address of the variable to write.
  @param  Buffer         Point to the data buffer.
  @param  BufferSize     The number of bytes of the data Buffer.

  @retval EFI_SUCCESS    The function completed successfully.
  @retval EFI_NOT_FOUND  Fail to locate Fault Tolerant Write protocol.
  @retval EFI_ABORTED    The function could not complete successfully.

**/
EFI_STATUS
FtwVariableSpace (
  IN EFI_PHYSICAL_ADDRESS   VariableBase,
  IN UINT8                  *Buffer,
  IN UINTN                  BufferSize
  );


/**
  Update the variable region with Variable information. These are the same 
  arguments as the EFI Variable services.

  @param[in] VariableName       Name of variable.

  @param[in] VendorGuid         Guid of variable.

  @param[in] Data               Variable data.

  @param[in] DataSize           Size of data. 0 means delete.

  @param[in] Attributes         Attribues of the variable.

  @param[in] Variable           The variable information that is used to keep track of variable usage.

  @retval EFI_SUCCESS           The update operation is success.

  @retval EFI_OUT_OF_RESOURCES  Variable region is full, cannot write other data into this region.

**/
EFI_STATUS
UpdateVariable (
  IN      CHAR16          *VariableName,
  IN      EFI_GUID        *VendorGuid,
  IN      VOID            *Data,
  IN      UINTN           DataSize,
  IN      UINT32          Attributes OPTIONAL,
  IN      VARIABLE_POINTER_TRACK *Variable
  );


/**
  Return TRUE if ExitBootServices () has been called.
  
  @retval TRUE If ExitBootServices () has been called.
**/
BOOLEAN
AtRuntime (
  VOID
  );

/**
  Initializes a basic mutual exclusion lock.

  This function initializes a basic mutual exclusion lock to the released state 
  and returns the lock.  Each lock provides mutual exclusion access at its task 
  priority level.  Since there is no preemption or multiprocessor support in EFI,
  acquiring the lock only consists of raising to the locks TPL.
  If Lock is NULL, then ASSERT().
  If Priority is not a valid TPL value, then ASSERT().

  @param  Lock       A pointer to the lock data structure to initialize.
  @param  Priority   EFI TPL is associated with the lock.

  @return The lock.

**/
EFI_LOCK *
InitializeLock (
  IN OUT EFI_LOCK  *Lock,
  IN EFI_TPL        Priority
  );

  
/**
  Acquires lock only at boot time. Simply returns at runtime.

  This is a temperary function that will be removed when
  EfiAcquireLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiAcquireLock() at boot time, and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to acquire.

**/
VOID
AcquireLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  );


/**
  Releases lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiReleaseLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiReleaseLock() at boot time and simply returns
  at runtime.

  @param  Lock         A pointer to the lock to release.

**/
VOID
ReleaseLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  );  

/**
  Retrive the FVB protocol interface by HANDLE.

  @param[in]  FvBlockHandle     The handle of FVB protocol that provides services for
                                reading, writing, and erasing the target block.
  @param[out] FvBlock           The interface of FVB protocol

  @retval EFI_SUCCESS           The interface information for the specified protocol was returned.
  @retval EFI_UNSUPPORTED       The device does not support the FVB protocol.
  @retval EFI_INVALID_PARAMETER FvBlockHandle is not a valid EFI_HANDLE or FvBlock is NULL.
  
**/
EFI_STATUS
GetFvbByHandle (
  IN  EFI_HANDLE                          FvBlockHandle,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvBlock
  );


/**
  Retrive the Swap Address Range protocol interface.

  @param[out] SarProtocol       The interface of SAR protocol

  @retval EFI_SUCCESS           The SAR protocol instance was found and returned in SarProtocol.
  @retval EFI_NOT_FOUND         The SAR protocol instance was not found.
  @retval EFI_INVALID_PARAMETER SarProtocol is NULL.

**/
EFI_STATUS
GetSarProtocol (
  OUT VOID                                **SarProtocol
  );

/**
  Function returns an array of handles that support the FVB protocol
  in a buffer allocated from pool. 

  @param[out]  NumberHandles    The number of handles returned in Buffer.
  @param[out]  Buffer           A pointer to the buffer to return the requested
                                array of  handles that support FVB protocol.

  @retval EFI_SUCCESS           The array of handles was returned in Buffer, and the number of
                                handles in Buffer was returned in NumberHandles.
  @retval EFI_NOT_FOUND         No FVB handle was found.
  @retval EFI_OUT_OF_RESOURCES  There is not enough pool memory to store the matching results.
  @retval EFI_INVALID_PARAMETER NumberHandles is NULL or Buffer is NULL.
  
**/
EFI_STATUS
GetFvbCountAndBuffer (
  OUT UINTN                               *NumberHandles,
  OUT EFI_HANDLE                          **Buffer
  );

/**
  Initializes variable store area for non-volatile and volatile variable.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
VariableCommonInitialize (
  VOID
  );

/**
  This function reclaims variable storage if free size is below the threshold.
  
**/
VOID
ReclaimForOS(
  VOID
  );  


/**
  Initializes variable write service after FVB was ready.

  @retval EFI_SUCCESS          Function successfully executed.
  @retval Others               Fail to initialize the variable service.

**/
EFI_STATUS
VariableWriteServiceInitialize (
  VOID
  );
  
/**
  Retrive the SMM Fault Tolerent Write protocol interface.

  @param[out] FtwProtocol       The interface of SMM Ftw protocol

  @retval EFI_SUCCESS           The SMM SAR protocol instance was found and returned in SarProtocol.
  @retval EFI_NOT_FOUND         The SMM SAR protocol instance was not found.
  @retval EFI_INVALID_PARAMETER SarProtocol is NULL.

**/
EFI_STATUS
GetFtwProtocol (
  OUT VOID                                **FtwProtocol
  );

/**
  Get the proper fvb handle and/or fvb protocol by the given Flash address.

  @param[in] Address        The Flash address.
  @param[out] FvbHandle     In output, if it is not NULL, it points to the proper FVB handle.
  @param[out] FvbProtocol   In output, if it is not NULL, it points to the proper FVB protocol.

**/
EFI_STATUS
GetFvbInfoByAddress (
  IN  EFI_PHYSICAL_ADDRESS                Address,
  OUT EFI_HANDLE                          *FvbHandle OPTIONAL,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvbProtocol OPTIONAL
  );

/**

  This code finds variable in storage blocks (Volatile or Non-Volatile).

  @param VariableName               Name of Variable to be found.
  @param VendorGuid                 Variable vendor GUID.
  @param Attributes                 Attribute value of the variable found.
  @param DataSize                   Size of Data found. If size is less than the
                                    data, this value contains the required size.
  @param Data                       Data pointer.
                      
  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
VariableServiceGetVariable (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          *VendorGuid,
  OUT     UINT32            *Attributes OPTIONAL,
  IN OUT  UINTN             *DataSize,
  OUT     VOID              *Data
  );

/**

  This code Finds the Next available variable.

  @param VariableNameSize           Size of the variable name.
  @param VariableName               Pointer to variable name.
  @param VendorGuid                 Variable Vendor Guid.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
VariableServiceGetNextVariableName (
  IN OUT  UINTN             *VariableNameSize,
  IN OUT  CHAR16            *VariableName,
  IN OUT  EFI_GUID          *VendorGuid
  );

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  @param VariableName                     Name of Variable to be found.
  @param VendorGuid                       Variable vendor GUID.
  @param Attributes                       Attribute value of the variable found
  @param DataSize                         Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param Data                             Data pointer.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Set successfully.
  @return EFI_OUT_OF_RESOURCES            Resource not enough to set variable.
  @return EFI_NOT_FOUND                   Not found.
  @return EFI_WRITE_PROTECTED             Variable is read-only.

**/
EFI_STATUS
EFIAPI
VariableServiceSetVariable (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINT32                  Attributes,
  IN UINTN                   DataSize,
  IN VOID                    *Data
  );

/**

  This code returns information about the EFI variables.

  @param Attributes                     Attributes bitmask to specify the type of variables
                                        on which to return information.
  @param MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                        for the EFI variables associated with the attributes specified.
  @param RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                        for EFI variables associated with the attributes specified.
  @param MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                        associated with the attributes specified.

  @return EFI_INVALID_PARAMETER         An invalid combination of attribute bits was supplied.
  @return EFI_SUCCESS                   Query successfully.
  @return EFI_UNSUPPORTED               The attribute is not supported on this platform.

**/
EFI_STATUS
EFIAPI
VariableServiceQueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize
  );  
  
extern VARIABLE_MODULE_GLOBAL  *mVariableModuleGlobal;

#endif
