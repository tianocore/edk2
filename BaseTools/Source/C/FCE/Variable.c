/** @file

 Read and edit the EFI variable.

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Fce.h"
#include "Variable.h"

extern LIST_ENTRY                  mAllVarListEntry;
extern MULTI_PLATFORM_PARAMETERS   mMultiPlatformParam;
extern G_EFI_FD_INFO               gEfiFdInfo;

EFI_GUID  gEfiVariableGuid     = EFI_VARIABLE_GUID;
/**

  Gets the pointer to the first variable header in given variable store area.

  @param VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the first variable header.

**/
static
VARIABLE_HEADER *
GetStartPointer (
  IN VARIABLE_STORE_HEADER       *VarStoreHeader
  )
{
  //
  // The end of variable store.
  //
  return (VARIABLE_HEADER *) HEADER_ALIGN (VarStoreHeader + 1);
}

/**

  Gets the pointer to the end of the variable storage area.

  This function gets pointer to the end of the variable storage
  area, according to the input variable store header.

  @param VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the end of the variable storage area.

**/
static
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER       *VarStoreHeader
  )
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *) HEADER_ALIGN ((UINTN) VarStoreHeader + VarStoreHeader->Size);
}


/**

  This code checks if variable header is valid or not.

  @param Variable        Pointer to the Variable Header.

  @retval TRUE           Variable header is valid.
  @retval FALSE          Variable header is not valid.

**/
static
BOOLEAN
IsValidVariableHeader (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if ((Variable == NULL) || (Variable->StartId != VARIABLE_DATA)) {
    return FALSE;
  }

  return TRUE;
}

/**

  This code gets the size of name of variable.

  @param Variable        Pointer to the Variable Header.

  @return UINTN          Size of variable in bytes.

**/
static
UINTN
NameSizeOfVariable (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if ((Variable->State    == (UINT8) (-1)) ||
      (Variable->DataSize == (UINT32) (-1)) ||
      (Variable->NameSize == (UINT32) (-1)) ||
      (Variable->Attributes == (UINT32) (-1))
      ) {
    return 0;
  }
  return (UINTN) Variable->NameSize;
}

/**

  This code gets the size of variable data.

  @param Variable        Pointer to the Variable Header.

  @return Size of variable in bytes.

**/
static
UINTN
DataSizeOfVariable (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if ((Variable->State    == (UINT8)  (-1)) ||
      (Variable->DataSize == (UINT32) (-1)) ||
      (Variable->NameSize == (UINT32) (-1)) ||
      (Variable->Attributes == (UINT32) (-1))
      ) {
    return 0;
  }
  return (UINTN) Variable->DataSize;
}

/**

  This code gets the pointer to the variable name.

  @param Variable        Pointer to the Variable Header.

  @return Pointer to Variable Name which is Unicode encoding.

**/
static
CHAR16 *
GetVariableNamePtr (
  IN  VARIABLE_HEADER   *Variable
  )
{

  return (CHAR16 *) (Variable + 1);
}

/**

  This code gets the pointer to the variable data.

  @param Variable        Pointer to the Variable Header.

  @return Pointer to Variable Data.

**/
static
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  UINTN Value;

  //
  // Be careful about pad size for alignment.
  //
  Value =  (UINTN) GetVariableNamePtr (Variable);
  Value += NameSizeOfVariable (Variable);
  Value += GET_PAD_SIZE (NameSizeOfVariable (Variable));

  return (UINT8 *) Value;
}

/**

  This code gets the pointer to the next variable header.

  @param Variable        Pointer to the Variable Header.

  @return Pointer to next variable header.

**/
static
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  UINTN Value;

  if (!IsValidVariableHeader (Variable)) {
    return NULL;
  }

  Value =  (UINTN) GetVariableDataPtr (Variable);
  Value += DataSizeOfVariable (Variable);
  Value += GET_PAD_SIZE (DataSizeOfVariable (Variable));

  //
  // Be careful about pad size for alignment.
  //
  return (VARIABLE_HEADER *) HEADER_ALIGN (Value);
}

/**
  Search and get a free space in the EFI variable zone

  @param VariableStoreHeader       The start of a EFI variable zone.
  @param VarListSize               The size of a variables needs to be allocated.
  @param FreeBeginVar              The dual pointer to the free NV space.

  @retval EFI_SUCCESS              Return the beginning of a free variable space.
  @retval RETURN_BUFFER_TOO_SMALL  Failed.
**/
static
EFI_STATUS
GetVariableVar (
  IN      VARIABLE_STORE_HEADER  *VariableStoreHeader,
  IN      UINT32                 VarListSize,
  IN OUT  CHAR8                  **FreeBeginVar
)
{
  BOOLEAN          Flag;
  VARIABLE_HEADER  *Variable;
  VARIABLE_HEADER  *EndOfVariable;
  CHAR8            *BeginVar;

  BeginVar      = NULL;
  Flag          = FALSE;
  Variable      = NULL;
  EndOfVariable = NULL;
  *FreeBeginVar = NULL;

  if (VariableStoreHeader == NULL) {
    *FreeBeginVar = NULL;
    return RETURN_INVALID_PARAMETER;
  }
  Variable      = GetStartPointer (VariableStoreHeader);
  EndOfVariable = GetEndPointer(VariableStoreHeader);
  //
  //Search the beginning of free NV
  //
  while (Variable != EndOfVariable) {
    BeginVar = (CHAR8 *)Variable;
    Variable = GetNextVariablePtr (Variable);
    if (Variable == NULL) {
      Flag = TRUE;
      break;
    }
  }
  //
  // Check whether the free space is more than what we want
  //
  if ((CHAR8 *)BeginVar + VarListSize > (CHAR8 *)EndOfVariable) {
    return RETURN_BUFFER_TOO_SMALL;
  }
  //
  // If not find the available space, return NULL
  //
  if (!Flag) {
    return RETURN_BUFFER_TOO_SMALL;
  }
  *FreeBeginVar = BeginVar;

  return EFI_SUCCESS;
}

/**
  Search whether the variable in VarList has existed in current NV.

  Parse the FFS or Fd image, and find the valid variable pointer.

  @param VariableStoreHeader    The start of a EFI variable zone.
  @param VarList                The pointer to the VarList

  @retval address               If the variable existed in current NV, return address
  @return NULL                  Otherwise, return NULL
**/
static
VARIABLE_HEADER  *
FindVariableInNv (
  IN     VARIABLE_STORE_HEADER  *VariableStoreHeader,
  IN     FORMSET_STORAGE        *Storage
  )
{
  BOOLEAN          Flag;
  VARIABLE_HEADER  *Variable;
  VARIABLE_HEADER  *EndOfVariable;
  CHAR16           *VariableName;

  Flag            = FALSE;
  Variable        = NULL;
  EndOfVariable   = NULL;
  VariableName    = NULL;

  if ((VariableStoreHeader == NULL) || (Storage == NULL) || (Storage->Name == NULL)) {
    return NULL;
  }
  Variable      = GetStartPointer (VariableStoreHeader);
  EndOfVariable = GetEndPointer(VariableStoreHeader);
  //
  // Parse and compare the variable in the NV space one by one
  //
  while ((Variable != EndOfVariable) && (Variable != NULL)) {
    VariableName = (CHAR16 *)((CHAR8 *)Variable + sizeof (VARIABLE_HEADER));
    if (!CompareGuid (&Variable->VendorGuid, &Storage->Guid) \
      && !FceStrCmp (Storage->Name, VariableName) \
      && (Variable->State == VAR_ADDED)) {
      Flag = TRUE;
      break;
    }
    Variable = GetNextVariablePtr (Variable);
  }
  if (!Flag) {
    return NULL;
  }
  return Variable;
}
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
  )
{
  EFI_FIRMWARE_VOLUME_HEADER    *VarAddr;
  LIST_ENTRY                    *StorageLink;
  FORMSET_STORAGE               *Storage;
  EFI_STATUS                    Status;
  CHAR8                         *NewAvailableAddr;
  CHAR8                         *DataBase;
  VARIABLE_HEADER               *VariableHeader;
  VARIABLE_STORE_HEADER         *VariableStoreHeader;
  UINTN                         VarNameSize;

  Status              = EFI_SUCCESS;
  DataBase            = NULL;
  NewAvailableAddr    = NULL;
  VarNameSize         = 0;
  VariableHeader      = NULL;
  VarAddr             = (EFI_FIRMWARE_VOLUME_HEADER   *) gEfiFdInfo.EfiVariableAddr;
  VariableStoreHeader = (VARIABLE_STORE_HEADER *)((CHAR8 *)VarAddr + VarAddr->HeaderLength);
  //
  //Parse the variable range, and check whether there is some existed ones.
  //
  StorageLink = GetFirstNode (StorageListHead);
  while (!IsNull (StorageListHead, StorageLink)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageLink);
     //
     // Ignore the invalid varlist node
     //
     if (Storage->Buffer == NULL) {
      StorageLink = GetNextNode (StorageListHead, StorageLink);
      continue;
     }
     //
     // Report error, if the variable name is invalid.
     //
     if ((Storage->Name == NULL) || (FceStrLen(Storage->Name) == 0)) {
       printf ("Error. One variable name is NULL. Its GUID is: ");
       PrintGuid(&(Storage->Guid));
       return EFI_INVALID_PARAMETER;
     }
     VariableHeader = FindVariableInNv (
                        VariableStoreHeader,
                        Storage
                        );

    if (VarToList) {
     //
     //Copy the data from NV to the VarList.
     //
     if (VariableHeader != NULL) {
       if (Storage->Buffer == NULL) {
         Storage->Buffer = calloc (Storage->Size, sizeof (CHAR8));
         ASSERT (Storage->Buffer != NULL);
       }
       DataBase    = (CHAR8*)GetVariableDataPtr (VariableHeader);
       memcpy (
         Storage->Buffer,
         (VOID *) DataBase,
         Storage->Size
        );
      }
    } else {
       //
       //If existed, copy the List data to the variable in NV directly. If not found, create a new one.
       //
       VarNameSize = 2 * (FceStrLen (Storage->Name) + 1);
       //
       //If this variable has existed in current FD, the data in VarList has
       // been updated, and this variable is not authenticated type, then
       // update it from VarList to the FD.
       //
       if ((VariableHeader != NULL)    \
         && (Storage->Buffer != NULL)
         ) {
           DataBase = (CHAR8*)GetVariableDataPtr (VariableHeader);
           memcpy (
             (VOID *) DataBase,
             Storage->Buffer,
             Storage->Size
           );
       } else if ((VariableHeader == NULL) && (Storage->Buffer != NULL)){
         //
         //If EfiVarstore is not EFI_VARIABLE_NON_VOLATILE, only skip it.
         //
         if (Storage->NewEfiVarstore
           && ((Storage->Attributes & EFI_VARIABLE_NON_VOLATILE) == 0)
         ) {
          StorageLink = GetNextNode (StorageListHead, StorageLink);
          continue;
         }
         //
         // Try to get the available zone from the efi variables
         //
         Status = GetVariableVar (
                    VariableStoreHeader,
                    Storage->Size + sizeof (VARIABLE_HEADER),
                    &NewAvailableAddr
                    );

         if (!EFI_ERROR (Status)) {
           //
           // Create the variable header
           //
           VariableHeader             = (VARIABLE_HEADER *) NewAvailableAddr;
           VariableHeader->StartId    = VARIABLE_DATA;
           VariableHeader->State      = VAR_ADDED;
           VariableHeader->Reserved   = 0x0;
           if (Storage->NewEfiVarstore) {
             VariableHeader->Attributes = Storage->Attributes;
           } else {
             VariableHeader->Attributes = EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS;
           }
           VariableHeader->NameSize   = VarNameSize;
           VariableHeader->DataSize   = Storage->Size;
           //
           //Copy the Guid, variable name, and data in sequence.
           //
           memcpy (
             (VOID *)&(VariableHeader->VendorGuid),
             &(Storage->Guid),
             sizeof (EFI_GUID)
             );
           NewAvailableAddr = NewAvailableAddr + sizeof (VARIABLE_HEADER);
           memcpy (
             (VOID *) NewAvailableAddr,
              Storage->Name,
              VarNameSize
              );

           NewAvailableAddr = NewAvailableAddr + VarNameSize + GET_PAD_SIZE (VarNameSize);
           memcpy (
             (VOID *) NewAvailableAddr,
             Storage->Buffer,
             Storage->Size * sizeof (CHAR8)
             );
         } else {
           printf ("Error. No available space in NV ram.\n");
           return EFI_OUT_OF_RESOURCES;
         }
       }
    }
    StorageLink = GetNextNode (StorageListHead, StorageLink);
  }
  return Status;
}

/**
  Remove the variable from Efi variable

  Found the variable with the same name in StorageListHead and remove it.

  @param StorageListHead   Decide which variale list be removed.

  @retval EFI_SUCCESS      Remove the variables successfully.
**/
EFI_STATUS
RemoveNormalEfiVariable (
  IN  LIST_ENTRY  *StorageListHead
  )
{
  EFI_FIRMWARE_VOLUME_HEADER    *VarAddr;
  LIST_ENTRY                    *StorageLink;
  FORMSET_STORAGE               *Storage;
  VARIABLE_HEADER               *VariableHeader;
  VARIABLE_STORE_HEADER         *VariableStoreHeader;

  VariableHeader      = NULL;
  VarAddr             = (EFI_FIRMWARE_VOLUME_HEADER   *) gEfiFdInfo.EfiVariableAddr;
  VariableStoreHeader = (VARIABLE_STORE_HEADER *)((CHAR8 *)VarAddr + VarAddr->HeaderLength);
  //
  //Parse the variable range, and check whether there is some existed ones.
  //
  StorageLink = GetFirstNode (StorageListHead);
  while (!IsNull (StorageListHead, StorageLink)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageLink);
    //
    // Ignore the invalid varlist node
    //
    if (Storage->Buffer == NULL) {
      StorageLink = GetNextNode (StorageListHead, StorageLink);
      continue;
    }
    //
    // Report error, if the variable name is invalid.
    //
    if ((Storage->Name == NULL) || (FceStrLen(Storage->Name) == 0)) {
      printf ("Error. One variable name is NULL. Its GUID is: ");
      PrintGuid(&(Storage->Guid));
      return EFI_INVALID_PARAMETER;
    }
    VariableHeader = FindVariableInNv (
                       VariableStoreHeader,
                       Storage
                       );
    if (VariableHeader != NULL) {
      VariableHeader->State = VAR_DELETED;
    }
    StorageLink = GetNextNode (StorageListHead, StorageLink);
  }
  return EFI_SUCCESS;
}

/**
  Check the store variable is no-authenticated or not

  @param VarToList     The pointer to the header of Variable Store.

  @retval TRUE         If no-authenticated, return TRUE.
  @retval FALSE        Otherwise, return FALSE.
**/

BOOLEAN
CheckNormalVarStoreOrNot (
  IN VOID  *VariableStoreHeader
  )
{
  if (!CompareGuid (
    &gEfiVariableGuid,
    &((VARIABLE_STORE_HEADER *)VariableStoreHeader)->Signature)
    ) {
    return TRUE;
  } else {
    return FALSE;
  }
}

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
  )
{
  EFI_STATUS                    Status;
  CHAR8                         *NewAvailableAddr;
  VARIABLE_HEADER               *VariableHeader;
  VARIABLE_STORE_HEADER         *VariableStoreHeader;
  UINTN                         VarNameSize;
  UINT32                        HeaderLength;

  Status              = EFI_SUCCESS;
  NewAvailableAddr    = NULL;
  VarNameSize         = 0;
  HeaderLength        = 0;
  VariableHeader      = NULL;
  VariableStoreHeader = NULL;

  if ((Storage->Name == NULL) || (FceStrLen(Storage->Name) == 0)) {
    printf ("Error. One variable name is NULL. Its GUID is: ");
    PrintGuid(&(Storage->Guid));
    return 0;
  }
  //
  // If the first storage under one specified platformId and defaultId, create the variable header
  //
  if (Index == 0) {
    HeaderLength = WriteDefaultAndPlatformId (StorageBeginning, Storage);
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) (StorageBeginning + HeaderLength);
    //
    //Create the Variable Storage header
    //
    memcpy (&(VariableStoreHeader->Signature), &gEfiVariableGuid, sizeof (EFI_GUID));
    VariableStoreHeader->Format = 0x5A;
    VariableStoreHeader->State  = 0xFE;
    //
    //Assign a big size here. It will be fixed after the storage under a specifed platformId and defaultId are all written.
    //
    VariableStoreHeader->Size   = gEfiFdInfo.FdSize;
  }

  VariableStoreHeader = (VARIABLE_STORE_HEADER *) (StorageBeginning + *(UINT16 *)StorageBeginning);

  Status = GetVariableVar (
             VariableStoreHeader,
             Storage->Size + sizeof (VARIABLE_HEADER),
             &NewAvailableAddr
           );
  if (EFI_ERROR (Status)) {
    return FAIL;
  }
  //
  // Create the variable header
  //
  VarNameSize                = 2 * (FceStrLen (Storage->Name) + 1);
  VariableHeader             = (VARIABLE_HEADER *) NewAvailableAddr;
  VariableHeader->StartId    = VARIABLE_DATA;
  VariableHeader->State      = VAR_ADDED;
  VariableHeader->Reserved   = 0x0;
  if (Storage->NewEfiVarstore) {
    VariableHeader->Attributes = Storage->Attributes;
  } else {
    VariableHeader->Attributes = EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS;
  }
  VariableHeader->NameSize   = VarNameSize;
  VariableHeader->DataSize   = Storage->Size;
  //
  //Copy the Guid, variable name, and data in sequence.
  //
  memcpy (
    (VOID *)&(VariableHeader->VendorGuid),
    &(Storage->Guid),
    sizeof (EFI_GUID)
  );
  NewAvailableAddr = NewAvailableAddr + sizeof (VARIABLE_HEADER);
  memcpy (
    (VOID *) NewAvailableAddr,
    Storage->Name,
    VarNameSize
  );

  NewAvailableAddr = NewAvailableAddr + VarNameSize + GET_PAD_SIZE (VarNameSize);
  memcpy (
    (VOID *) NewAvailableAddr,
    Storage->Buffer,
    Storage->Size * sizeof (CHAR8)
  );
  //
  // Return the length which is from the beginning of Binary
  //
  return ((UINT32) ((UINT8*)NewAvailableAddr - StorageBeginning) + Storage->Size);
}
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
  )
{
  EFI_STATUS                    Status;
  CHAR8                         *NewAvailableAddr;
  VARIABLE_HEADER               *VariableHeader;
  VARIABLE_STORE_HEADER         *VariableStoreHeader;
  UINTN                         VarNameSize;
  UINT32                        HeaderLength;
  PCD_DEFAULT_DATA              *PcdDefaultDataHeader;

  Status              = EFI_SUCCESS;
  NewAvailableAddr    = NULL;
  VarNameSize         = 0;
  HeaderLength        = 0;
  VariableHeader      = NULL;
  VariableStoreHeader = NULL;

  if ((Storage->Name == NULL) || (FceStrLen(Storage->Name) == 0)) {
    printf ("Error. One variable name is NULL. Its GUID is: ");
    PrintGuid(&(Storage->Guid));
    return 0;
  }
  //
  // If the first storage under one specified platformId and defaultId, create the variable header
  //
  if (Index == 0) {
    HeaderLength = WriteNvStoreDefaultAndPlatformId (StorageBeginning, Storage);
    PcdDefaultDataHeader = (PCD_DEFAULT_DATA *)StorageBeginning;
    PcdDefaultDataHeader->HeaderSize = HeaderLength;
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) (StorageBeginning + HeaderLength + 4);
    //
    //Create the Variable Storage header
    //
    memcpy (&(VariableStoreHeader->Signature), &gEfiVariableGuid, sizeof (EFI_GUID));
    VariableStoreHeader->Format = 0x5A;
    VariableStoreHeader->State  = 0xFE;
    //
    //Assign a big size here. It will be fixed after the storage under a specifed platformId and defaultId are all written.
    //
    VariableStoreHeader->Size   = gEfiFdInfo.FdSize;
  }
  PcdDefaultDataHeader = (PCD_DEFAULT_DATA *)StorageBeginning;
  VariableStoreHeader = (VARIABLE_STORE_HEADER *) (StorageBeginning + PcdDefaultDataHeader->HeaderSize + 4);
  Status = GetVariableVar (
             VariableStoreHeader,
             Storage->Size + sizeof (VARIABLE_HEADER),
             &NewAvailableAddr
           );
  if (EFI_ERROR (Status)) {
    return FAIL;
  }
  //
  // Create the variable header
  //
  VarNameSize                = 2 * (FceStrLen (Storage->Name) + 1);
  VariableHeader             = (VARIABLE_HEADER *) NewAvailableAddr;
  VariableHeader->StartId    = VARIABLE_DATA;
  VariableHeader->State      = VAR_ADDED;
  VariableHeader->Reserved   = 0x0;
  if (Storage->NewEfiVarstore) {
    VariableHeader->Attributes = Storage->Attributes;
  } else {
    VariableHeader->Attributes = EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS;
  }
  VariableHeader->NameSize   = VarNameSize;
  VariableHeader->DataSize   = Storage->Size;
  //
  //Copy the Guid, variable name, and data in sequence.
  //
  memcpy (
    (VOID *)&(VariableHeader->VendorGuid),
    &(Storage->Guid),
    sizeof (EFI_GUID)
  );

  NewAvailableAddr = NewAvailableAddr + sizeof (VARIABLE_HEADER);
  memcpy (
    (VOID *) NewAvailableAddr,
    Storage->Name,
    VarNameSize
  );

  NewAvailableAddr = NewAvailableAddr + VarNameSize + GET_PAD_SIZE (VarNameSize);
  memcpy (
    (VOID *) NewAvailableAddr,
    Storage->Buffer,
    Storage->Size * sizeof (CHAR8)
  );
  //
  // Return the length which is from the beginning of Binary
  //
  return ((UINT32) ((UINT8*)NewAvailableAddr - StorageBeginning - PcdDefaultDataHeader->HeaderSize - 4) + Storage->Size);
}
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
  )
{
  VARIABLE_HEADER               *EndOfVariable;
  VARIABLE_HEADER               *Variable;
  VARIABLE_STORE_HEADER         *VariableStoreHeader;
  FORMSET_STORAGE               *Storage;
  UINT32                        Length;
  PCD_DEFAULT_DATA             *PcdDefaultData;
  UINT8                         *DataBase;
  static UINT16                 PreDefaultId;
  static UINT64                 PrePlatformId;

  VariableStoreHeader = NULL;
  Variable            = NULL;
  Length              = 0;
  DataBase            = Binary;

  PcdDefaultData      = (PCD_DEFAULT_DATA *)DataBase;
  PrePlatformId       = PcdDefaultData->DefaultInfo[0].SkuId;
  PreDefaultId        = PcdDefaultData->DefaultInfo[0].DefaultId;
  VariableStoreHeader = (VARIABLE_STORE_HEADER *) (DataBase + PcdDefaultData->HeaderSize + 4);
  EndOfVariable       = GetEndPointer(VariableStoreHeader);

  for (Variable = GetStartPointer (VariableStoreHeader);
    Length < VariableStoreHeader->Size;
    Length += sizeof (VARIABLE_HEADER) + Variable->NameSize + Variable->DataSize
  ) {
    //
    // Create the storage
    //
    Storage = NULL;
    Storage = calloc (sizeof (FORMSET_STORAGE), sizeof (CHAR8));
    if (Storage == NULL) {
      printf ("Allocate memory failed.\n");
      return FAIL;
    }
    //
    // Store the DefaultId and PlatformId collected from the header to Storage.
    //
    Storage->DefaultId[0] = PreDefaultId;
    Storage->PlatformId[0] = PrePlatformId;
    Storage->DefaultPlatformIdNum = 0;

    Storage->Attributes     = Variable->Attributes;
    Storage->Size           = (UINT16)Variable->DataSize;
    Storage->Name           = calloc (Variable->NameSize, sizeof (UINT8));
    ASSERT (Storage->Name != NULL);
    Storage->Buffer         = calloc (Variable->DataSize, sizeof (UINT8));
    ASSERT (Storage->Buffer != NULL);
    memcpy (
      &(Storage->Guid),
      &(Variable->VendorGuid),
      sizeof (EFI_GUID)
    );
    memcpy (
      Storage->Name,
      (UINT8 *)Variable + sizeof (VARIABLE_HEADER),
      Variable->NameSize
    );
    memcpy (
      Storage->Buffer,
      (UINT8 *)Variable + sizeof (VARIABLE_HEADER) + Variable->NameSize + GET_PAD_SIZE (Variable->NameSize),
      Storage->Size * sizeof (CHAR8)
    );
    //
    // Assigned the value for comparison in verify mode
    //
    Storage->Type           = EFI_IFR_VARSTORE_EFI_OP;
    Storage->NewEfiVarstore = TRUE;
    InitializeListHead (&Storage->NameValueListHead);

    InsertTailList(StorageListEntry, &Storage->Link);
    //
    // If the last variable, exit.
    //
    if (Variable == EndOfVariable) {
      break;
    }

    Variable = GetNextVariablePtr (Variable);
    assert (Variable != NULL);
    if (!IsValidVariableHeader(Variable)) {
      break;
    }
  }

  return Length;
}
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
  )
{
  VARIABLE_HEADER               *EndOfVariable;
  VARIABLE_HEADER               *Variable;
  VARIABLE_STORE_HEADER         *VariableStoreHeader;
  FORMSET_STORAGE               *Storage;
  BOOLEAN                       ReadIdHeaderFlag;
  UINT32                        Length;
  EFI_COMMON_SECTION_HEADER     *SectionHeader;
  UINT8                         *DataBase;
  static UINT16                 PreDefaultId[MAX_PLATFORM_DEFAULT_ID_NUM];
  static UINT64                 PrePlatformId[MAX_PLATFORM_DEFAULT_ID_NUM];

  VariableStoreHeader = NULL;
  Variable            = NULL;
  ReadIdHeaderFlag    = TRUE;
  Length              = 0;
  SectionHeader       = (EFI_COMMON_SECTION_HEADER *)Binary;
  DataBase            = Binary + sizeof (EFI_COMMON_SECTION_HEADER);
  VariableStoreHeader = (VARIABLE_STORE_HEADER *) (DataBase + *(UINT16 *)DataBase);
  EndOfVariable       = GetEndPointer(VariableStoreHeader);

  for (Variable = GetStartPointer (VariableStoreHeader);
    Length < VariableStoreHeader->Size;
    Length += sizeof (VARIABLE_HEADER) + Variable->NameSize + Variable->DataSize
  ) {
    //
    // Create the storage
    //
    Storage = NULL;
    Storage = calloc (sizeof (FORMSET_STORAGE), sizeof (CHAR8));
    if (Storage == NULL) {
      printf ("Allocate memory failed.\n");
      return FAIL;
    }
    //
    // If access the first storage, read the platformId and defaultId
    //
    if (ReadIdHeaderFlag) {
      ReadDefaultAndPlatformIdFromBfv (DataBase, Storage);
      Length += sizeof (VARIABLE_HEADER) + Variable->NameSize + Variable->DataSize;
      ReadIdHeaderFlag = FALSE;
      memcpy (PreDefaultId, Storage->DefaultId, MAX_PLATFORM_DEFAULT_ID_NUM * sizeof (UINT16));
      memcpy (PrePlatformId, Storage->PlatformId, MAX_PLATFORM_DEFAULT_ID_NUM * sizeof (UINT64));
    } else {
      //
      // Store the DefaultId and PlatformId collected from the header to Storage.
      //
      memcpy (Storage->DefaultId, PreDefaultId, MAX_PLATFORM_DEFAULT_ID_NUM * sizeof (UINT16));
      memcpy (Storage->PlatformId, PrePlatformId, MAX_PLATFORM_DEFAULT_ID_NUM * sizeof (UINT64));
    }
    Storage->Attributes     = Variable->Attributes;
    Storage->Size           = (UINT16)Variable->DataSize;
    Storage->Name           = calloc (Variable->NameSize, sizeof (UINT8));
    ASSERT (Storage->Name != NULL);
    Storage->Buffer         = calloc (Variable->DataSize, sizeof (UINT8));
    ASSERT (Storage->Buffer != NULL);
    memcpy (
      &(Storage->Guid),
      &(Variable->VendorGuid),
      sizeof (EFI_GUID)
    );
    memcpy (
      Storage->Name,
      (UINT8 *)Variable + sizeof (VARIABLE_HEADER),
      Variable->NameSize
    );
    memcpy (
      Storage->Buffer,
      (UINT8 *)Variable + sizeof (VARIABLE_HEADER) + Variable->NameSize + GET_PAD_SIZE (Variable->NameSize),
      Storage->Size * sizeof (CHAR8)
    );
    //
    // Assigned the value for comparison in verify mode
    //
    Storage->Type           = EFI_IFR_VARSTORE_EFI_OP;
    Storage->NewEfiVarstore = TRUE;
    InitializeListHead (&Storage->NameValueListHead);

    InsertTailList(StorageListEntry, &Storage->Link);
    //
    // If the last variable, exit.
    //
    if (Variable == EndOfVariable) {
      break;
    }

    Variable = GetNextVariablePtr (Variable);
    assert (Variable != NULL);
  }
  //
  // Return the length which is from the beginning of Binary
  //
  Length = FvBufExpand3ByteSize (SectionHeader->Size);

  return Length;
}

/**
  Check whether exists the valid normal variables in NvStorage or not.

  @retval TRUE      If existed, return TRUE.
  @retval FALSE     Others
**/
BOOLEAN
ExistNormalEfiVarOrNot (
  IN  LIST_ENTRY  *StorageListHead
  )
{
  EFI_FIRMWARE_VOLUME_HEADER    *VarAddr;
  LIST_ENTRY                    *StorageLink;
  FORMSET_STORAGE               *Storage;
  VARIABLE_HEADER               *VariableHeader;
  VARIABLE_STORE_HEADER         *VariableStoreHeader;

  VariableHeader      = NULL;
  VarAddr             = (EFI_FIRMWARE_VOLUME_HEADER   *) gEfiFdInfo.EfiVariableAddr;
  VariableStoreHeader = (VARIABLE_STORE_HEADER *)((CHAR8 *)VarAddr + VarAddr->HeaderLength);
  //
  //Parse the variable range, and check whether there is some existed ones.
  //
  StorageLink = GetFirstNode (StorageListHead);
  while (!IsNull (StorageListHead, StorageLink)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageLink);
    //
    // Ignore the invalid varlist node
    //
    if ((Storage->Buffer == NULL)
      || (Storage->Name == NULL)
      || (FceStrLen(Storage->Name) == 0)
      ) {
     StorageLink = GetNextNode (StorageListHead, StorageLink);
     continue;
    }
    //
    // Report error, if the variable name is invalid.
    //
    if ((Storage->Name == NULL) || (FceStrLen(Storage->Name) == 0)) {
      StorageLink = GetNextNode (StorageListHead, StorageLink);
     continue;
    }
    VariableHeader = FindVariableInNv (
                       VariableStoreHeader,
                       Storage
                     );

    if ((VariableHeader != NULL)) {
       return TRUE;
    }
    StorageLink = GetNextNode (StorageListHead, StorageLink);
  }
  return FALSE;
}

/**
  Fix the size of variable header.

  @param  Binary            The pointer to the header of storage under specifed platformId and defaultId
  @param  Length            The length of binary.

**/
VOID
FixVariableHeaderSize (
  IN  UINT8   *BinaryBeginning,
  IN  UINT32  Length
  )
{
  VARIABLE_STORE_HEADER         *VariableStoreHeader;

  VariableStoreHeader       = (VARIABLE_STORE_HEADER *) (BinaryBeginning + *(UINT16 *)BinaryBeginning);
  VariableStoreHeader->Size =  Length -  *(UINT16 *)BinaryBeginning;
}

/**
  Fix the size of variable header.

  @param  Binary            The pointer to the header of storage under specifed platformId and defaultId
  @param  Length            The length of binary.

**/
VOID
FixNvStoreVariableHeaderSize (
  IN  UINT8   *BinaryBeginning,
  IN  UINT32  Length
  )
{
  VARIABLE_STORE_HEADER         *VariableStoreHeader;
  PCD_DEFAULT_DATA              *PcdDefaultDataHeader;

  PcdDefaultDataHeader      = (PCD_DEFAULT_DATA *)(BinaryBeginning);
  VariableStoreHeader       = (VARIABLE_STORE_HEADER *) (BinaryBeginning + PcdDefaultDataHeader->HeaderSize + 4);
  VariableStoreHeader->Size = Length;
  PcdDefaultDataHeader->DataSize = VariableStoreHeader->Size + PcdDefaultDataHeader->HeaderSize + 4;
}

