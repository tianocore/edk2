/** @file
  Load option library functions which relate with creating and processing load options.

Copyright (c) 2011 - 2017, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalBm.h"

GLOBAL_REMOVE_IF_UNREFERENCED
  CHAR16 *mBmLoadOptionName[] = {
    L"Driver",
    L"SysPrep",
    L"Boot",
    L"PlatformRecovery"
  };

GLOBAL_REMOVE_IF_UNREFERENCED
  CHAR16 *mBmLoadOptionOrderName[] = {
    EFI_DRIVER_ORDER_VARIABLE_NAME,
    EFI_SYS_PREP_ORDER_VARIABLE_NAME,
    EFI_BOOT_ORDER_VARIABLE_NAME,
    NULL  // PlatformRecovery#### doesn't have associated *Order variable
  };

/**
  Call Visitor function for each variable in variable storage.

  @param Visitor  Visitor function.
  @param Context  The context passed to Visitor function.
**/
VOID
BmForEachVariable (
  BM_VARIABLE_VISITOR         Visitor,
  VOID                        *Context
  )
{
  EFI_STATUS                  Status;
  CHAR16                      *Name;
  EFI_GUID                    Guid;
  UINTN                       NameSize;
  UINTN                       NewNameSize;

  NameSize = sizeof (CHAR16);
  Name = AllocateZeroPool (NameSize);
  ASSERT (Name != NULL);
  while (TRUE) {
    NewNameSize = NameSize;
    Status = gRT->GetNextVariableName (&NewNameSize, Name, &Guid);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Name = ReallocatePool (NameSize, NewNameSize, Name);
      ASSERT (Name != NULL);
      Status = gRT->GetNextVariableName (&NewNameSize, Name, &Guid);
      NameSize = NewNameSize;
    }

    if (Status == EFI_NOT_FOUND) {
      break;
    }
    ASSERT_EFI_ERROR (Status);

    Visitor (Name, &Guid, Context);
  }

  FreePool (Name);
}

/**
  Get the Option Number that wasn't used.

  @param  LoadOptionType      The load option type.
  @param  FreeOptionNumber    Return the minimal free option number.

  @retval EFI_SUCCESS           The option number is found and will be returned.
  @retval EFI_OUT_OF_RESOURCES  There is no free option number that can be used.
  @retval EFI_INVALID_PARAMETER FreeOptionNumber is NULL

**/
EFI_STATUS
BmGetFreeOptionNumber (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE LoadOptionType,
  OUT UINT16                            *FreeOptionNumber
  )
{
  
  UINTN         OptionNumber;
  UINTN         Index;
  UINT16        *OptionOrder;
  UINTN         OptionOrderSize;
  UINT16        *BootNext;

  ASSERT (FreeOptionNumber != NULL);
  ASSERT (LoadOptionType == LoadOptionTypeDriver || 
          LoadOptionType == LoadOptionTypeBoot ||
          LoadOptionType == LoadOptionTypeSysPrep);

  GetEfiGlobalVariable2 (mBmLoadOptionOrderName[LoadOptionType], (VOID **) &OptionOrder, &OptionOrderSize);
  ASSERT ((OptionOrder != NULL && OptionOrderSize != 0) || (OptionOrder == NULL && OptionOrderSize == 0));

  BootNext = NULL;
  if (LoadOptionType == LoadOptionTypeBoot) {
    GetEfiGlobalVariable2 (L"BootNext", (VOID**) &BootNext, NULL);
  }

  for (OptionNumber = 0; 
       OptionNumber < OptionOrderSize / sizeof (UINT16)
                    + ((BootNext != NULL) ? 1 : 0); 
       OptionNumber++
       ) {
    //
    // Search in OptionOrder whether the OptionNumber exists
    //
    for (Index = 0; Index < OptionOrderSize / sizeof (UINT16); Index++) {
      if (OptionNumber == OptionOrder[Index]) {
        break;
      }
    }

    //
    // We didn't find it in the ****Order array and it doesn't equal to BootNext 
    // Otherwise, OptionNumber equals to OptionOrderSize / sizeof (UINT16) + 1
    //
    if ((Index == OptionOrderSize / sizeof (UINT16)) && 
        ((BootNext == NULL) || (OptionNumber != *BootNext))
        ) {
      break;
    }
  }
  if (OptionOrder != NULL) {
    FreePool (OptionOrder);
  }

  if (BootNext != NULL) {
    FreePool (BootNext);
  }

  //
  // When BootOrder & BootNext conver all numbers in the range [0 ... 0xffff],
  //   OptionNumber equals to 0x10000 which is not valid.
  //
  ASSERT (OptionNumber <= 0x10000);
  if (OptionNumber == 0x10000) {
    return EFI_OUT_OF_RESOURCES;
  } else {
    *FreeOptionNumber = (UINT16) OptionNumber;
    return EFI_SUCCESS;
  }
}

/**
  Create the Boot####, Driver####, SysPrep####, PlatformRecovery#### variable
  from the load option.

  @param  LoadOption      Pointer to the load option.

  @retval EFI_SUCCESS     The variable was created.
  @retval Others          Error status returned by RT->SetVariable.
**/
EFI_STATUS
EFIAPI
EfiBootManagerLoadOptionToVariable (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION     *Option
  )
{
  EFI_STATUS                       Status;
  UINTN                            VariableSize;
  UINT8                            *Variable;
  UINT8                            *Ptr;
  CHAR16                           OptionName[BM_OPTION_NAME_LEN];
  CHAR16                           *Description;
  CHAR16                           NullChar;
  EDKII_VARIABLE_LOCK_PROTOCOL     *VariableLock;
  UINT32                           VariableAttributes;

  if ((Option->OptionNumber == LoadOptionNumberUnassigned) ||
      (Option->FilePath == NULL) ||
      ((UINT32) Option->OptionType >= LoadOptionTypeMax)
     ) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Convert NULL description to empty description
  //
  NullChar    = L'\0';
  Description = Option->Description;
  if (Description == NULL) {
    Description = &NullChar;
  }

  /*
  UINT32                      Attributes;
  UINT16                      FilePathListLength;
  CHAR16                      Description[];
  EFI_DEVICE_PATH_PROTOCOL    FilePathList[];
  UINT8                       OptionalData[];
TODO: FilePathList[] IS:
A packed array of UEFI device paths.  The first element of the 
array is a device path that describes the device and location of the 
Image for this load option.  The FilePathList[0] is specific 
to the device type.  Other device paths may optionally exist in the 
FilePathList, but their usage is OSV specific. Each element 
in the array is variable length, and ends at the device path end 
structure.
  */
  VariableSize = sizeof (Option->Attributes)
               + sizeof (UINT16)
               + StrSize (Description)
               + GetDevicePathSize (Option->FilePath)
               + Option->OptionalDataSize;

  Variable     = AllocatePool (VariableSize);
  ASSERT (Variable != NULL);

  Ptr             = Variable;
  WriteUnaligned32 ((UINT32 *) Ptr, Option->Attributes);
  Ptr            += sizeof (Option->Attributes);

  WriteUnaligned16 ((UINT16 *) Ptr, (UINT16) GetDevicePathSize (Option->FilePath));
  Ptr            += sizeof (UINT16);

  CopyMem (Ptr, Description, StrSize (Description));
  Ptr            += StrSize (Description);

  CopyMem (Ptr, Option->FilePath, GetDevicePathSize (Option->FilePath));
  Ptr            += GetDevicePathSize (Option->FilePath);

  CopyMem (Ptr, Option->OptionalData, Option->OptionalDataSize);

  UnicodeSPrint (OptionName, sizeof (OptionName), L"%s%04x", mBmLoadOptionName[Option->OptionType], Option->OptionNumber);

  VariableAttributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE;
  if (Option->OptionType == LoadOptionTypePlatformRecovery) {
    //
    // Lock the PlatformRecovery####
    //
    Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **) &VariableLock);
    if (!EFI_ERROR (Status)) {
      Status = VariableLock->RequestToLock (VariableLock, OptionName, &gEfiGlobalVariableGuid);
      ASSERT_EFI_ERROR (Status);
    }
    VariableAttributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
  }

  return gRT->SetVariable (
                OptionName,
                &gEfiGlobalVariableGuid,
                VariableAttributes,
                VariableSize,
                Variable
                );
}

/**
  Update order variable .

  @param  OptionOrderName     Order variable name which need to be updated.
  @param  OptionNumber        Option number for the new option.
  @param  Position            Position of the new load option to put in the ****Order variable.

  @retval EFI_SUCCESS           The boot#### or driver#### have been successfully registered.
  @retval EFI_ALREADY_STARTED   The option number of Option is being used already.
  @retval EFI_STATUS            Return the status of gRT->SetVariable ().

**/
EFI_STATUS
BmAddOptionNumberToOrderVariable (
  IN CHAR16               *OptionOrderName,
  IN UINT16               OptionNumber,
  IN UINTN                Position
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  UINT16                  *OptionOrder;
  UINT16                  *NewOptionOrder;
  UINTN                   OptionOrderSize;
  //
  // Update the option order variable
  //
  GetEfiGlobalVariable2 (OptionOrderName, (VOID **) &OptionOrder, &OptionOrderSize);
  ASSERT ((OptionOrder != NULL && OptionOrderSize != 0) || (OptionOrder == NULL && OptionOrderSize == 0));

  Status = EFI_SUCCESS;
  for (Index = 0; Index < OptionOrderSize / sizeof (UINT16); Index++) {
    if (OptionOrder[Index] == OptionNumber) {
      Status = EFI_ALREADY_STARTED;
      break;
    }
  }

  if (!EFI_ERROR (Status)) {
    Position       = MIN (Position, OptionOrderSize / sizeof (UINT16));

    NewOptionOrder = AllocatePool (OptionOrderSize + sizeof (UINT16));
    ASSERT (NewOptionOrder != NULL);
    if (OptionOrderSize != 0) {
      CopyMem (NewOptionOrder, OptionOrder, Position * sizeof (UINT16));
      CopyMem (&NewOptionOrder[Position + 1], &OptionOrder[Position], OptionOrderSize - Position * sizeof (UINT16));
    }
    NewOptionOrder[Position] = OptionNumber;

    Status = gRT->SetVariable (
                    OptionOrderName,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    OptionOrderSize + sizeof (UINT16),
                    NewOptionOrder
                    );
    FreePool (NewOptionOrder);
  }

  if (OptionOrder != NULL) {
    FreePool (OptionOrder);
  }

  return Status;
}

/**
  This function will register the new Boot####, Driver#### or SysPrep#### option.
  After the *#### is updated, the *Order will also be updated.

  @param  Option            Pointer to load option to add.
  @param  Position          Position of the new load option to put in the ****Order variable.

  @retval EFI_SUCCESS           The *#### have been successfully registered.
  @retval EFI_INVALID_PARAMETER The option number exceeds 0xFFFF.
  @retval EFI_ALREADY_STARTED   The option number of Option is being used already.
                                Note: this API only adds new load option, no replacement support.
  @retval EFI_OUT_OF_RESOURCES  There is no free option number that can be used when the
                                option number specified in the Option is LoadOptionNumberUnassigned.
  @retval EFI_STATUS            Return the status of gRT->SetVariable ().

**/
EFI_STATUS
EFIAPI
EfiBootManagerAddLoadOptionVariable (
  IN EFI_BOOT_MANAGER_LOAD_OPTION *Option,
  IN UINTN                        Position
  )
{
  EFI_STATUS                      Status;
  UINT16                          OptionNumber;

  if (Option == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Option->OptionType != LoadOptionTypeDriver && 
      Option->OptionType != LoadOptionTypeSysPrep &&
      Option->OptionType != LoadOptionTypeBoot
      ) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the free option number if the option number is unassigned
  //
  if (Option->OptionNumber == LoadOptionNumberUnassigned) {
    Status = BmGetFreeOptionNumber (Option->OptionType, &OptionNumber);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Option->OptionNumber = OptionNumber;
  }

  if (Option->OptionNumber >= LoadOptionNumberMax) {
    return EFI_INVALID_PARAMETER;
  }

  Status = BmAddOptionNumberToOrderVariable (mBmLoadOptionOrderName[Option->OptionType], (UINT16) Option->OptionNumber, Position);
  if (!EFI_ERROR (Status)) {
    //
    // Save the Boot#### or Driver#### variable
    //
    Status = EfiBootManagerLoadOptionToVariable (Option);
    if (EFI_ERROR (Status)) {
      //
      // Remove the #### from *Order variable when the Driver####/SysPrep####/Boot#### cannot be saved.
      //
      EfiBootManagerDeleteLoadOptionVariable (Option->OptionNumber, Option->OptionType);
    }
  }

  return Status;
}

/**
  Sort the load option. The DriverOrder or BootOrder will be re-created to 
  reflect the new order.

  @param OptionType             Load option type
  @param CompareFunction        The comparator
**/
VOID
EFIAPI
EfiBootManagerSortLoadOptionVariable (
  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE        OptionType,
  SORT_COMPARE                             CompareFunction
  )
{
  EFI_STATUS                     Status;
  EFI_BOOT_MANAGER_LOAD_OPTION   *LoadOption;
  UINTN                          LoadOptionCount;
  UINTN                          Index;
  UINT16                         *OptionOrder;

  LoadOption = EfiBootManagerGetLoadOptions (&LoadOptionCount, OptionType);

  //
  // Insertion sort algorithm
  //
  PerformQuickSort (
    LoadOption,
    LoadOptionCount,
    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION),
    CompareFunction
    );

  //
  // Create new ****Order variable
  //
  OptionOrder = AllocatePool (LoadOptionCount * sizeof (UINT16));
  ASSERT (OptionOrder != NULL);
  for (Index = 0; Index < LoadOptionCount; Index++) {
    OptionOrder[Index] = (UINT16) LoadOption[Index].OptionNumber;
  }

  Status = gRT->SetVariable (
                  mBmLoadOptionOrderName[OptionType],
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  LoadOptionCount * sizeof (UINT16),
                  OptionOrder
                  );
  //
  // Changing the *Order content without increasing its size with current variable implementation shouldn't fail.
  //
  ASSERT_EFI_ERROR (Status);

  FreePool (OptionOrder);
  EfiBootManagerFreeLoadOptions (LoadOption, LoadOptionCount);
}

/**
  Initialize a load option.

  @param Option           Pointer to the load option to be initialized.
  @param OptionNumber     Option number of the load option.
  @param OptionType       Type of the load option.
  @param Attributes       Attributes of the load option.
  @param Description      Description of the load option.
  @param FilePath         Device path of the load option.
  @param OptionalData     Optional data of the load option.
  @param OptionalDataSize Size of the optional data of the load option.

  @retval EFI_SUCCESS           The load option was initialized successfully.
  @retval EFI_INVALID_PARAMETER Option, Description or FilePath is NULL.
**/
EFI_STATUS
EFIAPI
EfiBootManagerInitializeLoadOption (
  IN OUT EFI_BOOT_MANAGER_LOAD_OPTION   *Option,
  IN  UINTN                             OptionNumber,
  IN  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE OptionType,
  IN  UINT32                            Attributes,
  IN  CHAR16                            *Description,
  IN  EFI_DEVICE_PATH_PROTOCOL          *FilePath,
  IN  UINT8                             *OptionalData,   OPTIONAL
  IN  UINT32                            OptionalDataSize
  )
{
  if ((Option == NULL) || (Description == NULL) || (FilePath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (((OptionalData != NULL) && (OptionalDataSize == 0)) ||
      ((OptionalData == NULL) && (OptionalDataSize != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINT32) OptionType >= LoadOptionTypeMax) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Option, sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
  Option->OptionNumber       = OptionNumber;
  Option->OptionType         = OptionType;
  Option->Attributes         = Attributes;
  Option->Description        = AllocateCopyPool (StrSize (Description), Description);
  Option->FilePath           = DuplicateDevicePath (FilePath);
  if (OptionalData != NULL) {
    Option->OptionalData     = AllocateCopyPool (OptionalDataSize, OptionalData);
    Option->OptionalDataSize = OptionalDataSize;
  }

  return EFI_SUCCESS;
}


/**
  Return the index of the load option in the load option array.

  The function consider two load options are equal when the 
  OptionType, Attributes, Description, FilePath and OptionalData are equal.

  @param Key    Pointer to the load option to be found.
  @param Array  Pointer to the array of load options to be found.
  @param Count  Number of entries in the Array.

  @retval -1          Key wasn't found in the Array.
  @retval 0 ~ Count-1 The index of the Key in the Array.
**/
INTN
EFIAPI
EfiBootManagerFindLoadOption (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Key,
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Array,
  IN UINTN                              Count
  )
{
  UINTN                             Index;

  for (Index = 0; Index < Count; Index++) {
    if ((Key->OptionType == Array[Index].OptionType) &&
        (Key->Attributes == Array[Index].Attributes) &&
        (StrCmp (Key->Description, Array[Index].Description) == 0) &&
        (CompareMem (Key->FilePath, Array[Index].FilePath, GetDevicePathSize (Key->FilePath)) == 0) &&
        (Key->OptionalDataSize == Array[Index].OptionalDataSize) &&
        (CompareMem (Key->OptionalData, Array[Index].OptionalData, Key->OptionalDataSize) == 0)) {
      return (INTN) Index;
    }
  }

  return -1;
}

/**
  Delete the load option.

  @param  OptionNumber        Indicate the option number of load option
  @param  OptionType          Indicate the type of load option

  @retval EFI_INVALID_PARAMETER OptionType or OptionNumber is invalid.
  @retval EFI_NOT_FOUND         The load option cannot be found
  @retval EFI_SUCCESS           The load option was deleted
  @retval others                Status of RT->SetVariable()
**/
EFI_STATUS
EFIAPI
EfiBootManagerDeleteLoadOptionVariable (
  IN UINTN                              OptionNumber,
  IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  OptionType
  )
{
  UINT16                            *OptionOrder;
  UINTN                             OptionOrderSize;
  UINTN                             Index;
  CHAR16                            OptionName[BM_OPTION_NAME_LEN];

  if (((UINT32) OptionType >= LoadOptionTypeMax) || (OptionNumber >= LoadOptionNumberMax)) {
    return EFI_INVALID_PARAMETER;
  }

  if (OptionType == LoadOptionTypeDriver || OptionType == LoadOptionTypeSysPrep || OptionType == LoadOptionTypeBoot) {
    //
    // If the associated *Order exists, firstly remove the reference in *Order for
    //  Driver####, SysPrep#### and Boot####.
    //
    GetEfiGlobalVariable2 (mBmLoadOptionOrderName[OptionType], (VOID **) &OptionOrder, &OptionOrderSize);
    ASSERT ((OptionOrder != NULL && OptionOrderSize != 0) || (OptionOrder == NULL && OptionOrderSize == 0));

    for (Index = 0; Index < OptionOrderSize / sizeof (UINT16); Index++) {
      if (OptionOrder[Index] == OptionNumber) {
        OptionOrderSize -= sizeof (UINT16);
        CopyMem (&OptionOrder[Index], &OptionOrder[Index + 1], OptionOrderSize - Index * sizeof (UINT16));
        gRT->SetVariable (
               mBmLoadOptionOrderName[OptionType],
               &gEfiGlobalVariableGuid,
               EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
               OptionOrderSize,
               OptionOrder
               );
        break;
      }
    }
    if (OptionOrder != NULL) {
      FreePool (OptionOrder);
    }
  }

  //
  // Remove the Driver####, SysPrep####, Boot#### or PlatformRecovery#### itself.
  //
  UnicodeSPrint (OptionName, sizeof (OptionName), L"%s%04x", mBmLoadOptionName[OptionType], OptionNumber);
  return gRT->SetVariable (
                OptionName,
                &gEfiGlobalVariableGuid,
                0,
                0,
                NULL
                );
}

/**
  Returns the size of a device path in bytes.

  This function returns the size, in bytes, of the device path data structure 
  specified by DevicePath including the end of device path node. If DevicePath 
  is NULL, then 0 is returned. If the length of the device path is bigger than
  MaxSize, also return 0 to indicate this is an invalidate device path.

  @param  DevicePath         A pointer to a device path data structure.
  @param  MaxSize            Max valid device path size. If big than this size, 
                             return error.
  
  @retval 0                  An invalid device path.
  @retval Others             The size of a device path in bytes.

**/
UINTN
BmGetDevicePathSizeEx (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN UINTN                           MaxSize
  )
{
  UINTN  Size;
  UINTN  NodeSize;

  if (DevicePath == NULL) {
    return 0;
  }

  //
  // Search for the end of the device path structure
  //
  Size = 0;
  while (!IsDevicePathEnd (DevicePath)) {
    NodeSize = DevicePathNodeLength (DevicePath);
    if (NodeSize == 0) {
      return 0;
    }
    Size += NodeSize;
    if (Size > MaxSize) {
      return 0;
    }
    DevicePath = NextDevicePathNode (DevicePath);
  }
  Size += DevicePathNodeLength (DevicePath);
  if (Size > MaxSize) {
    return 0;
  }

  return Size;
}

/**
  Returns the length of a Null-terminated Unicode string. If the length is 
  bigger than MaxStringLen, return length 0 to indicate that this is an 
  invalidate string.

  This function returns the number of Unicode characters in the Null-terminated
  Unicode string specified by String. 

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().

  @param  String           A pointer to a Null-terminated Unicode string.
  @param  MaxStringLen     Max string len in this string.

  @retval 0                An invalid string.
  @retval Others           The length of String.

**/
UINTN
BmStrSizeEx (
  IN      CONST CHAR16              *String,
  IN      UINTN                     MaxStringLen
  )
{
  UINTN                             Length;

  ASSERT (String != NULL && MaxStringLen != 0);
  ASSERT (((UINTN) String & BIT0) == 0);

  for (Length = 0; *String != L'\0' && MaxStringLen != Length; String++, Length+=2);

  if (*String != L'\0' && MaxStringLen == Length) {
    return 0;
  }

  return Length + 2;
}

/**
  Validate the Boot####, Driver####, SysPrep#### and PlatformRecovery####
  variable (VendorGuid/Name)

  @param  Variable              The variable data.
  @param  VariableSize          The variable size.

  @retval TRUE                  The variable data is correct.
  @retval FALSE                 The variable data is corrupted.

**/
BOOLEAN 
BmValidateOption (
  UINT8                     *Variable,
  UINTN                     VariableSize
  )
{
  UINT16                    FilePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     DescriptionSize;

  if (VariableSize <= sizeof (UINT16) + sizeof (UINT32)) {
    return FALSE;
  }

  //
  // Skip the option attribute
  //
  Variable += sizeof (UINT32);

  //
  // Get the option's device path size
  //
  FilePathSize = ReadUnaligned16 ((UINT16 *) Variable);
  Variable += sizeof (UINT16);

  //
  // Get the option's description string size
  //
  DescriptionSize = BmStrSizeEx ((CHAR16 *) Variable, VariableSize - sizeof (UINT16) - sizeof (UINT32));
  Variable += DescriptionSize;

  //
  // Get the option's device path
  //
  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) Variable;

  //
  // Validation boot option variable.
  //
  if ((FilePathSize == 0) || (DescriptionSize == 0)) {
    return FALSE;
  }

  if (sizeof (UINT32) + sizeof (UINT16) + DescriptionSize + FilePathSize > VariableSize) {
    return FALSE;
  }

  return (BOOLEAN) (BmGetDevicePathSizeEx (DevicePath, FilePathSize) != 0);
}

/**
  Check whether the VariableName is a valid load option variable name
  and return the load option type and option number.

  @param VariableName The name of the load option variable.
  @param OptionType   Return the load option type.
  @param OptionNumber Return the load option number.

  @retval TRUE  The variable name is valid; The load option type and
                load option number is returned.
  @retval FALSE The variable name is NOT valid.
**/
BOOLEAN
EFIAPI
EfiBootManagerIsValidLoadOptionVariableName (
  IN CHAR16                             *VariableName,
  OUT EFI_BOOT_MANAGER_LOAD_OPTION_TYPE *OptionType   OPTIONAL,
  OUT UINT16                            *OptionNumber OPTIONAL
  )
{
  UINTN                             VariableNameLen;
  UINTN                             Index;
  UINTN                             Uint;
  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE LocalOptionType;
  UINT16                            LocalOptionNumber;

  if (VariableName == NULL) {
    return FALSE;
  }

  VariableNameLen = StrLen (VariableName);

  //
  // Return FALSE when the variable name length is too small.
  //
  if (VariableNameLen <= 4) {
    return FALSE;
  }

  //
  // Return FALSE when the variable name doesn't start with Driver/SysPrep/Boot/PlatformRecovery.
  //
  for (LocalOptionType = 0; LocalOptionType < ARRAY_SIZE (mBmLoadOptionName); LocalOptionType++) {
    if ((VariableNameLen - 4 == StrLen (mBmLoadOptionName[LocalOptionType])) &&
        (StrnCmp (VariableName, mBmLoadOptionName[LocalOptionType], VariableNameLen - 4) == 0)
        ) {
      break;
    }
  }
  if (LocalOptionType == ARRAY_SIZE (mBmLoadOptionName)) {
    return FALSE;
  }

  //
  // Return FALSE when the last four characters are not hex digits.
  //
  LocalOptionNumber = 0;
  for (Index = VariableNameLen - 4; Index < VariableNameLen; Index++) {
    Uint = BmCharToUint (VariableName[Index]);
    if (Uint == -1) {
      break;
    } else {
      LocalOptionNumber = (UINT16) Uint + LocalOptionNumber * 0x10;
    }
  }
  if (Index != VariableNameLen) {
    return FALSE;
  }

  if (OptionType != NULL) {
    *OptionType = LocalOptionType;
  }

  if (OptionNumber != NULL) {
    *OptionNumber = LocalOptionNumber;
  }

  return TRUE;
}

/**
  Build the Boot#### or Driver#### option from the VariableName.

  @param  VariableName          Variable name of the load option
  @param  VendorGuid            Variable GUID of the load option
  @param  Option                Return the load option.

  @retval EFI_SUCCESS     Get the option just been created
  @retval EFI_NOT_FOUND   Failed to get the new option

**/
EFI_STATUS
EFIAPI
EfiBootManagerVariableToLoadOptionEx (
  IN CHAR16                           *VariableName,
  IN EFI_GUID                         *VendorGuid,
  IN OUT EFI_BOOT_MANAGER_LOAD_OPTION *Option  
  )
{
  EFI_STATUS                         Status;
  UINT32                             Attribute;
  UINT16                             FilePathSize;
  UINT8                              *Variable;
  UINT8                              *VariablePtr;
  UINTN                              VariableSize;
  EFI_DEVICE_PATH_PROTOCOL           *FilePath;
  UINT8                              *OptionalData;
  UINT32                             OptionalDataSize;
  CHAR16                             *Description;
  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  OptionType;
  UINT16                             OptionNumber;

  if ((VariableName == NULL) || (Option == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!EfiBootManagerIsValidLoadOptionVariableName (VariableName, &OptionType, &OptionNumber)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Read the variable
  //
  GetVariable2 (VariableName, VendorGuid, (VOID **) &Variable, &VariableSize);
  if (Variable == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Validate *#### variable data.
  //
  if (!BmValidateOption(Variable, VariableSize)) {
    FreePool (Variable);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the option attribute
  //
  VariablePtr = Variable;
  Attribute = ReadUnaligned32 ((UINT32 *) VariablePtr);
  VariablePtr += sizeof (UINT32);

  //
  // Get the option's device path size
  //
  FilePathSize = ReadUnaligned16 ((UINT16 *) VariablePtr);
  VariablePtr += sizeof (UINT16);

  //
  // Get the option's description string
  //
  Description = (CHAR16 *) VariablePtr;

  //
  // Get the option's description string size
  //
  VariablePtr += StrSize ((CHAR16 *) VariablePtr);

  //
  // Get the option's device path
  //
  FilePath = (EFI_DEVICE_PATH_PROTOCOL *) VariablePtr;
  VariablePtr += FilePathSize;

  OptionalDataSize = (UINT32) (VariableSize - ((UINTN) VariablePtr - (UINTN) Variable));
  if (OptionalDataSize == 0) {
    OptionalData = NULL;
  } else {
    OptionalData = VariablePtr;
  }

  Status = EfiBootManagerInitializeLoadOption (
             Option,
             OptionNumber,
             OptionType,
             Attribute,
             Description,
             FilePath,
             OptionalData,
             OptionalDataSize
             );
  ASSERT_EFI_ERROR (Status);

  CopyGuid (&Option->VendorGuid, VendorGuid);

  FreePool (Variable);
  return Status;
}

/**
Build the Boot#### or Driver#### option from the VariableName.

@param  VariableName          EFI Variable name indicate if it is Boot#### or Driver####
@param  Option                Return the Boot#### or Driver#### option.

@retval EFI_SUCCESS     Get the option just been created
@retval EFI_NOT_FOUND   Failed to get the new option
**/
EFI_STATUS
EFIAPI
EfiBootManagerVariableToLoadOption (
  IN  CHAR16                          *VariableName,
  IN OUT EFI_BOOT_MANAGER_LOAD_OPTION *Option
  )
{
  return EfiBootManagerVariableToLoadOptionEx (VariableName, &gEfiGlobalVariableGuid, Option);
}

typedef struct {
  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE OptionType;
  EFI_GUID                          *Guid;
  EFI_BOOT_MANAGER_LOAD_OPTION      *Options;
  UINTN                             OptionCount;
} BM_COLLECT_LOAD_OPTIONS_PARAM;

/**
  Visitor function to collect the Platform Recovery load options or OS Recovery
  load options from NV storage.

  @param Name    Variable name.
  @param Guid    Variable GUID.
  @param Context The same context passed to BmForEachVariable.
**/
VOID
BmCollectLoadOptions (
  IN CHAR16               *Name,
  IN EFI_GUID             *Guid,
  IN VOID                 *Context
  )
{
  EFI_STATUS                        Status;
  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE OptionType;
  UINT16                            OptionNumber;
  EFI_BOOT_MANAGER_LOAD_OPTION      Option;
  UINTN                             Index;
  BM_COLLECT_LOAD_OPTIONS_PARAM     *Param;

  Param = (BM_COLLECT_LOAD_OPTIONS_PARAM *) Context;

  if (CompareGuid (Guid, Param->Guid) && (
      Param->OptionType == LoadOptionTypePlatformRecovery &&
      EfiBootManagerIsValidLoadOptionVariableName (Name, &OptionType, &OptionNumber) &&
      OptionType == LoadOptionTypePlatformRecovery
     )) {
    Status = EfiBootManagerVariableToLoadOptionEx (Name, Guid, &Option);
    if (!EFI_ERROR (Status)) {
      for (Index = 0; Index < Param->OptionCount; Index++) {
        if (Param->Options[Index].OptionNumber > Option.OptionNumber) {
          break;
        }
      }
      Param->Options = ReallocatePool (
                         Param->OptionCount * sizeof (EFI_BOOT_MANAGER_LOAD_OPTION),
                         (Param->OptionCount + 1) * sizeof (EFI_BOOT_MANAGER_LOAD_OPTION),
                         Param->Options
                         );
      ASSERT (Param->Options != NULL);
      CopyMem (&Param->Options[Index + 1], &Param->Options[Index], (Param->OptionCount - Index) * sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
      CopyMem (&Param->Options[Index], &Option, sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
      Param->OptionCount++;
    }
  }
}

/**
  Returns an array of load options based on the EFI variable
  L"BootOrder"/L"DriverOrder" and the L"Boot####"/L"Driver####" variables impled by it.
  #### is the hex value of the UINT16 in each BootOrder/DriverOrder entry. 

  @param  LoadOptionCount   Returns number of entries in the array.
  @param  LoadOptionType    The type of the load option.

  @retval NULL  No load options exist.
  @retval !NULL Array of load option entries.

**/
EFI_BOOT_MANAGER_LOAD_OPTION *
EFIAPI
EfiBootManagerGetLoadOptions (
  OUT UINTN                             *OptionCount,
  IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  LoadOptionType
  )
{
  EFI_STATUS                    Status;
  UINT16                        *OptionOrder;
  UINTN                         OptionOrderSize;
  UINTN                         Index;
  UINTN                         OptionIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION  *Options;
  CHAR16                        OptionName[BM_OPTION_NAME_LEN];
  UINT16                        OptionNumber;
  BM_COLLECT_LOAD_OPTIONS_PARAM Param;

  *OptionCount = 0;
  Options      = NULL;

  if (LoadOptionType == LoadOptionTypeDriver || LoadOptionType == LoadOptionTypeSysPrep || LoadOptionType == LoadOptionTypeBoot) {
    //
    // Read the BootOrder, or DriverOrder variable.
    //
    GetEfiGlobalVariable2 (mBmLoadOptionOrderName[LoadOptionType], (VOID **) &OptionOrder, &OptionOrderSize);
    if (OptionOrder == NULL) {
      return NULL;
    }

    *OptionCount = OptionOrderSize / sizeof (UINT16);

    Options = AllocatePool (*OptionCount * sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
    ASSERT (Options != NULL);

    OptionIndex = 0;
    for (Index = 0; Index < *OptionCount; Index++) {
      OptionNumber = OptionOrder[Index];
      UnicodeSPrint (OptionName, sizeof (OptionName), L"%s%04x", mBmLoadOptionName[LoadOptionType], OptionNumber);

      Status = EfiBootManagerVariableToLoadOption (OptionName, &Options[OptionIndex]);
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_INFO, "[Bds] %s doesn't exist - Update ****Order variable to remove the reference!!", OptionName));
        EfiBootManagerDeleteLoadOptionVariable (OptionNumber, LoadOptionType);
      } else {
        ASSERT (Options[OptionIndex].OptionNumber == OptionNumber);
        OptionIndex++;
      }
    }

    if (OptionOrder != NULL) {
      FreePool (OptionOrder);
    }

    if (OptionIndex < *OptionCount) {
      Options = ReallocatePool (*OptionCount * sizeof (EFI_BOOT_MANAGER_LOAD_OPTION), OptionIndex * sizeof (EFI_BOOT_MANAGER_LOAD_OPTION), Options);
      ASSERT (Options != NULL);
      *OptionCount = OptionIndex;
    }

  } else if (LoadOptionType == LoadOptionTypePlatformRecovery) {
    Param.OptionType = LoadOptionTypePlatformRecovery;
    Param.Options = NULL;
    Param.OptionCount = 0;
    Param.Guid = &gEfiGlobalVariableGuid;

    BmForEachVariable (BmCollectLoadOptions, (VOID *) &Param);

    *OptionCount = Param.OptionCount;
    Options = Param.Options;
  }

  return Options;
}

/**
  Free an EFI_BOOT_MANGER_LOAD_OPTION entry that was allocate by the library.

  @param  LoadOption   Pointer to boot option to Free.

  @return EFI_SUCCESS   BootOption was freed 
  @return EFI_NOT_FOUND BootOption == NULL 

**/
EFI_STATUS
EFIAPI
EfiBootManagerFreeLoadOption (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *LoadOption
  )
{
  if (LoadOption == NULL) {
    return EFI_NOT_FOUND;
  }

  if (LoadOption->Description != NULL) {
    FreePool (LoadOption->Description);
  }
  if (LoadOption->FilePath != NULL) {
    FreePool (LoadOption->FilePath);
  }
  if (LoadOption->OptionalData != NULL) {
    FreePool (LoadOption->OptionalData);
  }

  return EFI_SUCCESS;
}

/**
  Free an EFI_BOOT_MANGER_LOAD_OPTION array that was allocated by 
  EfiBootManagerGetLoadOptions().

  @param  Option       Pointer to boot option array to free.
  @param  OptionCount  Number of array entries in BootOption

  @return EFI_SUCCESS   BootOption was freed 
  @return EFI_NOT_FOUND BootOption == NULL 

**/
EFI_STATUS
EFIAPI
EfiBootManagerFreeLoadOptions (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *Option,
  IN  UINTN                         OptionCount
  )
{
  UINTN   Index;

  if (Option == NULL) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0;Index < OptionCount; Index++) {
    EfiBootManagerFreeLoadOption (&Option[Index]);
  }

  FreePool (Option);

  return EFI_SUCCESS;
}

/**
  Return whether the PE header of the load option is valid or not.

  @param[in] Type       The load option type.
                        It's used to check whether the load option is valid.
                        When it's LoadOptionTypeMax, the routine only guarantees
                        the load option is a valid PE image but doesn't guarantee
                        the PE's subsystem type is valid.
  @param[in] FileBuffer The PE file buffer of the load option.
  @param[in] FileSize   The size of the load option file.

  @retval TRUE  The PE header of the load option is valid.
  @retval FALSE The PE header of the load option is not valid.
**/
BOOLEAN
BmIsLoadOptionPeHeaderValid (
  IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE Type,
  IN VOID                              *FileBuffer,
  IN UINTN                             FileSize
  )
{
  EFI_IMAGE_DOS_HEADER              *DosHeader;
  EFI_IMAGE_OPTIONAL_HEADER_UNION   *PeHeader;
  EFI_IMAGE_OPTIONAL_HEADER32       *OptionalHeader;
  UINT16                            Subsystem;

  if (FileBuffer == NULL || FileSize == 0) {
    return FALSE;
  }

  //
  // Read dos header
  //
  DosHeader = (EFI_IMAGE_DOS_HEADER *) FileBuffer;
  if (FileSize >= sizeof (EFI_IMAGE_DOS_HEADER) &&
      FileSize > DosHeader->e_lfanew && DosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE
      ) {
    //
    // Read and check PE signature
    //
    PeHeader = (EFI_IMAGE_OPTIONAL_HEADER_UNION *) ((UINT8 *) FileBuffer + DosHeader->e_lfanew);
    if (FileSize >= DosHeader->e_lfanew + sizeof (EFI_IMAGE_OPTIONAL_HEADER_UNION) &&
        PeHeader->Pe32.Signature == EFI_IMAGE_NT_SIGNATURE
        ) {
      //
      // Check PE32 or PE32+ magic, and machine type
      //
      OptionalHeader = (EFI_IMAGE_OPTIONAL_HEADER32 *) &PeHeader->Pe32.OptionalHeader;
      if ((OptionalHeader->Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC || 
           OptionalHeader->Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) &&
          EFI_IMAGE_MACHINE_TYPE_SUPPORTED (PeHeader->Pe32.FileHeader.Machine)
          ) {
        //
        // Check the Subsystem:
        //   Driver#### must be of type BootServiceDriver or RuntimeDriver
        //   SysPrep####, Boot####, OsRecovery####, PlatformRecovery#### must be of type Application
        //
        Subsystem = OptionalHeader->Subsystem;
        if ((Type == LoadOptionTypeMax) ||
            (Type == LoadOptionTypeDriver && Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER) ||
            (Type == LoadOptionTypeDriver && Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER) ||
            (Type == LoadOptionTypeSysPrep && Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION) ||
            (Type == LoadOptionTypeBoot && Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION) ||
            (Type == LoadOptionTypePlatformRecovery && Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION)
            ) {
          return TRUE;
        }
      }
    }
  }

  return FALSE;
}

/**
  Return the next matched load option buffer.
  The routine keeps calling BmGetNextLoadOptionDevicePath() until a valid
  load option is read.

  @param Type      The load option type.
                   It's used to check whether the load option is valid.
                   When it's LoadOptionTypeMax, the routine only guarantees
                   the load option is a valid PE image but doesn't guarantee
                   the PE's subsystem type is valid.
  @param FilePath  The device path pointing to a load option.
                   It could be a short-form device path.
  @param FullPath  Return the next full device path of the load option after
                   short-form device path expanding.
                   Caller is responsible to free it.
                   NULL to return the first matched full device path.
  @param FileSize  Return the load option size.

  @return The load option buffer. Caller is responsible to free the memory.
**/
VOID *
BmGetNextLoadOptionBuffer (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE Type,
  IN  EFI_DEVICE_PATH_PROTOCOL          *FilePath,
  OUT EFI_DEVICE_PATH_PROTOCOL          **FullPath,
  OUT UINTN                             *FileSize
  )
{
  VOID                                  *FileBuffer;
  EFI_DEVICE_PATH_PROTOCOL              *PreFullPath;
  EFI_DEVICE_PATH_PROTOCOL              *CurFullPath;
  UINTN                                 LocalFileSize;
  UINT32                                AuthenticationStatus;
  EFI_DEVICE_PATH_PROTOCOL              *RamDiskDevicePath;

  LocalFileSize = 0;
  FileBuffer  = NULL;
  CurFullPath = *FullPath;
  do {
    PreFullPath = CurFullPath;
    CurFullPath = BmGetNextLoadOptionDevicePath (FilePath, CurFullPath);
    //
    // Only free the full path created *inside* this routine
    //
    if ((PreFullPath != NULL) && (PreFullPath != *FullPath)) {
      FreePool (PreFullPath);
    }
    if (CurFullPath == NULL) {
      break;
    }
    FileBuffer = GetFileBufferByFilePath (TRUE, CurFullPath, &LocalFileSize, &AuthenticationStatus);
    if ((FileBuffer != NULL) && !BmIsLoadOptionPeHeaderValid (Type, FileBuffer, LocalFileSize)) {
      //
      // Free the RAM disk file system if the load option is invalid.
      //
      RamDiskDevicePath = BmGetRamDiskDevicePath (FilePath);
      if (RamDiskDevicePath != NULL) {
        BmDestroyRamDisk (RamDiskDevicePath);
        FreePool (RamDiskDevicePath);
      }

      //
      // Free the invalid load option buffer.
      //
      FreePool (FileBuffer);
      FileBuffer = NULL;
    }
  } while (FileBuffer == NULL);

  if (FileBuffer == NULL) {
    CurFullPath = NULL;
    LocalFileSize = 0;
  }

  DEBUG ((DEBUG_INFO, "[Bds] Expand "));
  BmPrintDp (FilePath);
  DEBUG ((DEBUG_INFO, " -> "));
  BmPrintDp (CurFullPath);
  DEBUG ((DEBUG_INFO, "\n"));

  *FullPath = CurFullPath;
  *FileSize = LocalFileSize;
  return FileBuffer;
}

/**
  Process (load and execute) the load option.

  @param LoadOption  Pointer to the load option.

  @retval EFI_INVALID_PARAMETER  The load option type is invalid, 
                                 or the load option file path doesn't point to a valid file.
  @retval EFI_UNSUPPORTED        The load option type is of LoadOptionTypeBoot.
  @retval EFI_SUCCESS            The load option is inactive, or successfully loaded and executed.
**/
EFI_STATUS
EFIAPI
EfiBootManagerProcessLoadOption (
  IN EFI_BOOT_MANAGER_LOAD_OPTION       *LoadOption
  )
{
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_PROTOCOL          *PreFullPath;
  EFI_DEVICE_PATH_PROTOCOL          *CurFullPath;
  EFI_HANDLE                        ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL         *ImageInfo;
  VOID                              *FileBuffer;
  UINTN                             FileSize;

  if ((UINT32) LoadOption->OptionType >= LoadOptionTypeMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (LoadOption->OptionType == LoadOptionTypeBoot) {
    return EFI_UNSUPPORTED;
  }

  //
  // If a load option is not marked as LOAD_OPTION_ACTIVE,
  // the boot manager will not automatically load the option.
  //
  if ((LoadOption->Attributes & LOAD_OPTION_ACTIVE) == 0) {
    return EFI_SUCCESS;
  }

  //
  // Load and start the load option.
  //
  DEBUG ((
    DEBUG_INFO | DEBUG_LOAD, "Process %s%04x (%s) ...\n",
    mBmLoadOptionName[LoadOption->OptionType], LoadOption->OptionNumber,
    LoadOption->Description
    ));
  ImageHandle = NULL;
  CurFullPath = NULL;
  EfiBootManagerConnectDevicePath (LoadOption->FilePath, NULL);

  //
  // while() loop is to keep starting next matched load option if the PlatformRecovery#### returns failure status.
  //
  while (TRUE) {
    Status      = EFI_INVALID_PARAMETER;
    PreFullPath = CurFullPath;
    FileBuffer  = BmGetNextLoadOptionBuffer (LoadOption->OptionType, LoadOption->FilePath, &CurFullPath, &FileSize);
    if (PreFullPath != NULL) {
      FreePool (PreFullPath);
    }
    if (FileBuffer == NULL) {
      break;
    }
    Status = gBS->LoadImage (
                    FALSE,
                    gImageHandle,
                    CurFullPath,
                    FileBuffer,
                    FileSize,
                    &ImageHandle
                    );
    FreePool (FileBuffer);

    if (!EFI_ERROR (Status)) {
      Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&ImageInfo);
      ASSERT_EFI_ERROR (Status);

      ImageInfo->LoadOptionsSize = LoadOption->OptionalDataSize;
      ImageInfo->LoadOptions = LoadOption->OptionalData;
      //
      // Before calling the image, enable the Watchdog Timer for the 5-minute period
      //
      gBS->SetWatchdogTimer (5 * 60, 0, 0, NULL);

      LoadOption->Status = gBS->StartImage (ImageHandle, &LoadOption->ExitDataSize, &LoadOption->ExitData);
      DEBUG ((
        DEBUG_INFO | DEBUG_LOAD, "%s%04x Return Status = %r\n",
        mBmLoadOptionName[LoadOption->OptionType], LoadOption->OptionNumber, LoadOption->Status
      ));

      //
      // Clear the Watchdog Timer after the image returns
      //
      gBS->SetWatchdogTimer (0, 0, 0, NULL);

      if ((LoadOption->OptionType != LoadOptionTypePlatformRecovery) || (LoadOption->Status == EFI_SUCCESS)) {
        break;
      }
    }
  }

  if (CurFullPath != NULL) {
    FreePool (CurFullPath);
  }

  return Status;
}
