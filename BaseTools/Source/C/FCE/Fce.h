/** @file

 FCE is a tool which enables developers to retrieve and change HII configuration ("Setup")
 data in Firmware Device files (".fd" files).

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FCE_H_
#define _FCE_H_ 1

//#define NDEBUG

#include "Common.h"
#include "IfrParse.h"
#include "VariableCommon.h"
#include "BinaryParse.h"
#include "BinaryCreate.h"
///
/// Utility global variables
///
#define UTILITY_MAJOR_VERSION      0
#define UTILITY_MINOR_VERSION      34

#define UTILITY_NAME               "FCE"

#define SUCCESS                    0
#define FAIL                       1
#define VR_FAIL                    2
#define MAX_INPUT_ALLOCATE_SIZE    256

///
/// The type of file input and operations
///
typedef enum {
  INFD,
  OUTFD,
  OUTTXT,
  SETUPTXT
} FILETYPE;

typedef enum {
  NONE,
  READ,
  UPDATE,
  UPDATE_REMOVE,
  UPDATE_IGNORE,
  VERIFY,
  UPDATEQ
} OPERATION_TYPE;

typedef struct _GUID_SEC_TOOL_ENTRY {
  EFI_GUID                     Guid;
  CHAR8*                       Name;
  CHAR8*                       Path;
  struct _GUID_SEC_TOOL_ENTRY  *Next;
} GUID_SEC_TOOL_ENTRY;

///
/// The tag for use in identifying UNICODE files.
/// If the file is UNICODE, the first 16 bits of the file will equal this value.
///
enum {
  BigUnicodeFileTag    = 0xFEFF,
  LittleUnicodeFileTag = 0xFFFE
};

typedef enum {
  ASCII,
  BIG_UCS2,
  LITTLE_UCS2
} FILE_TYPE;

/**
  Exchange the data between Efi variable and the data of VarList when the
  variable use the authenticated variable header

  If VarToList is TRUE, copy the efi variable data to the VarList; Otherwise,
  update the data from varlist to efi variable.

  @param VarToList          The flag to control the direction of exchange.
   @param StorageListHead   Decide which variale list be updated

  @retval EFI_SUCCESS           Get the address successfully.
  @retval EFI_OUT_OF_RESOURCES  No available in the EFI variable zone.
  @retval EFI_INVALID_PARAMETER Invalid variable name.
**/
EFI_STATUS
SynAuthEfiVariable (
  IN  BOOLEAN     VarToList,
  IN  LIST_ENTRY  *StorageListHead
  );

/**
  Remove the variable from Efi variable

  Found the variable with the same name in StorageListHead and remove it.

  @param StorageListHead   Decide which variale list be removed.

  @retval EFI_SUCCESS      Remove the variables successfully.
**/
EFI_STATUS
RemoveAuthEfiVariable (
  IN  LIST_ENTRY  *StorageListHead
  );

/**
  Exchange the data between Efi variable and the data of VarList when the
  variable use the time stamp authenticated variable header

  If VarToList is TRUE, copy the efi variable data to the VarList; Otherwise,
  update the data from varlist to efi variable.

  @param VarToList              The flag to control the direction of exchange.
  @param StorageListHead        Decide which variale list be updated

  @retval EFI_SUCCESS           Get the address successfully.
  @retval EFI_OUT_OF_RESOURCES  No available in the EFI variable zone.
  @retval EFI_INVALID_PARAMETER Invalid variable name.
**/

EFI_STATUS
SynAuthEfiVariableBasedTime (
  IN  BOOLEAN     VarToList,
  IN  LIST_ENTRY  *StorageListHead
  );

/**
  Remove the variable from Efi variable

  Found the variable with the same name in StorageListHead and remove it.

  @param StorageListHead   Decide which variale list be removed.

  @retval EFI_SUCCESS      Remove the variables successfully.
**/
EFI_STATUS
RemoveAuthEfiVariableBasedTime (
  IN  LIST_ENTRY  *StorageListHead
  );

/**
  Exchange the data between Efi variable and the data of VarList when the
  variable use the authenticated variable header

  If VarToList is TRUE, copy the efi variable data to the VarList; Otherwise,
  update the data from varlist to efi variable.

  @param VarToList         The flag to control the direction of exchange.
  @param StorageListHead   Decide which variale list be updated

  @retval EFI_SUCCESS           Get the address successfully.
  @retval EFI_OUT_OF_RESOURCES  No available in the EFI variable zone.
**/

EFI_STATUS
SynEfiVariable (
  IN  BOOLEAN     VarToList,
  IN  LIST_ENTRY  *StorageListHead
  );

/**
  Remove the variable from Efi variable

  Found the variable with the same name in StorageListHead and remove it.

  @param StorageListHead   Decide which variale list be removed.

  @retval EFI_SUCCESS      Remove the variables successfully.
**/
EFI_STATUS
RemoveNormalEfiVariable (
  IN  LIST_ENTRY  *StorageListHead
  );

/**
  Read all defaultId and platformId from binary.

  @param  Binary        The pointer to the bianry
  @param  Storage       The pointer to the Storage
**/
VOID
ReadDefaultAndPlatformIdFromBfv (
  IN  UINT8             *Binary,
  IN  FORMSET_STORAGE   *Storage
);

/**
  Store all defaultId and platformId to binary.

  @param  Binary        The pointer to the bianry
  @param  Storage       The pointer to the Storage

  @retval Length        Return the length of the header
**/

UINT32
WriteDefaultAndPlatformId (
  IN  UINT8             *Binary,
  IN  FORMSET_STORAGE   *Storage
);

/**
  Store all defaultId and platformId to binary.

  @param  Binary        The pointer to the bianry
  @param  Storage       The pointer to the Storage

  @retval Length        Return the length of the header
**/
UINT32
WriteNvStoreDefaultAndPlatformId (
  IN  UINT8             *Binary,
  IN  FORMSET_STORAGE   *Storage
);

/**
  Copy variable to binary in multi-platform mode

  @param  Storage           The pointer to a storage in storage list.
  @param  StorageBeginning  The pointer to the beginning of storage under specifed platformId and defaultId
  @param  Index             The number of the storage. If the Index is 0, record the variable header to
                            the binary. Or else, only record the storage.

  @return length          The length of storage
**/
UINT32
CopyVariableToBinary (
  IN      FORMSET_STORAGE   *Storage,
  IN OUT  UINT8             *StorageBeginning,
  IN      UINT32            Index
  );

/**
  Copy variable to binary in multi-platform mode

  @param  Storage           The pointer to a storage in storage list.
  @param  StorageBeginning  The pointer to the beginning of storage under specifed platformId and defaultId
  @param  Index             The number of the storage. If the Index is 0, record the variable header to
                            the binary. Or else, only record the storage.

  @return length          The length of storage
**/
UINT32
CopyVariableToNvStoreBinary (
  IN      FORMSET_STORAGE   *Storage,
  IN OUT  UINT8             *StorageBeginning,
  IN      UINT32            Index
  );


/**
  Read variable to storage list in multi-platform mode

  @param  Binary            The pointer to the header of storage under specifed platformId and defaultId
  @param  StorageListEntry  The pointer to the storage list.

  @return length          The length of storage
**/

UINT32
ReadNvStoreVariableToList (
  IN      UINT8             *Binary,
  IN      LIST_ENTRY        *StorageListEntry
  );

/**
  Read variable to storage list in multi-platform mode

  @param  Binary            The pointer to the header of storage under specifed platformId and defaultId
  @param  StorageListEntry  The pointer to the storage list.

  @return length          The length of storage
**/
UINT32
ReadVariableToList (
  IN      UINT8             *Binary,
  IN      LIST_ENTRY        *StorageListEntry
  );

/**
  Check whether exists the valid normal variables in NvStorage or not.

  @retval TRUE      If existed, return TRUE.
  @retval FALSE     Others
**/
BOOLEAN
ExistNormalEfiVarOrNot (
  IN  LIST_ENTRY  *StorageListHead
  );

/**
  Fix the size of variable header.

  @param  Binary            The pointer to the header of storage under specifed platformId and defaultId
  @param  Length            The length of binary.

**/
VOID
FixVariableHeaderSize (
  IN  UINT8   *BinaryBeginning,
  IN  UINT32  Length
  );

/**
  Fix the size of variable header.

  @param  Binary            The pointer to the header of storage under specifed platformId and defaultId
  @param  Length            The length of binary.

**/

VOID
FixNvStoreVariableHeaderSize (
  IN  UINT8   *BinaryBeginning,
  IN  UINT32  Length
  );
/**
  Copy time-based authenticated variable to binary in multi-platform mode

  @param  Storage           The pointer to a storage in storage list.
  @param  StorageBeginning  The pointer to the beginning of storage under specifed platformId and defaultId
  @param  Index             The number of the storage. If the Index is 0, record the variable header to
                            the binary. Or else, only record the storage.
  @return length            The length of storage
**/
UINT32
CopyTimeBasedVariableToBinary (
  IN      FORMSET_STORAGE   *Storage,
  IN OUT  UINT8             *StorageBeginning,
  IN      UINT32            Index
  );

/**
  Read time-based authenticated variable to storage list in multi-platform mode

  @param  Binary            The pointer to the header of storage under specifed platformId and defaultId
  @param  StorageListEntry  The pointer to the storage list.

  @return length          The length of storage
**/
UINT32
ReadTimeBasedVariableToList (
  IN      UINT8             *Binary,
  IN      LIST_ENTRY        *StorageListEntry
  );

/**
  Check whether exists the valid time-based variables in NvStorage or not.

  @retval TRUE      If existed, return TRUE.
  @retval FALSE     Others
**/
BOOLEAN
ExistTimeBasedEfiVarOrNot (
  IN  LIST_ENTRY  *StorageListHead
  );

/**
  Fix the size of time-based variable header.

  @param  Binary            The pointer to the header of storage under specifed platformId and defaultId
  @param  Length            The length of binary.

**/
VOID
FixBasedTimeVariableHeaderSize (
  IN  UINT8   *BinaryBeginning,
  IN  UINT32  Length
  );

/**
  Copy Monotonic-Based authenticated variable to binary in multi-platform mode

  @param  Storage           The pointer to a storage in storage list.
  @param  StorageBeginning  The pointer to the beginning of storage under specifed platformId and defaultId
  @param  Index             The number of the storage. If the Index is 0, record the variable header to
                            the binary. Or else, only record the storage.

  @return length            The length of storage
**/
UINT32
CopyMonotonicBasedVariableToBinary (
  IN      FORMSET_STORAGE   *Storage,
  IN OUT  UINT8             *StorageBeginning,
  IN      UINT32            Index
  );

/**
  Read Monotonic-based authenticated variable to storage list in multi-platform mode

  @param  Binary            The pointer to the header of storage under specifed platformId and defaultId
  @param  StorageListEntry  The pointer to the storage list.

  @return length          The length of storage
**/
UINT32
ReadMonotonicBasedVariableToList (
  IN      UINT8             *Binary,
  IN      LIST_ENTRY        *StorageListEntry
  );

/**
  Check whether exists the valid MonotonicBased variables in NvStorage or not.

  @retval TRUE      If existed, return TRUE.
  @retval FALSE     Others
**/
BOOLEAN
ExistMonotonicBasedEfiVarOrNot (
  IN  LIST_ENTRY  *StorageListHead
  );

/**
  Fix the size of montonic variable header.

  @param  Binary            The pointer to the header of storage under specifed platformId and defaultId
  @param  Length            The length of binary.

**/
VOID
FixMontonicVariableHeaderSize (
  IN  UINT8   *BinaryBeginning,
  IN  UINT32  Length
  );

/**
  FCE application entry point

  @param  argc     The number of input parameters.
  @param  *argv[]  The array pointer to the parameters.

  @retval  0       The application exited normally.
  @retval  1       An error occurred.
  @retval  2       An error about check occurred.

**/
int
main (
  int       argc,
  char      *argv[]
  );

#endif
