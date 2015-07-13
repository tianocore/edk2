/** @file
  Misc BDS library function

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalBdsLib.h"


#define MAX_STRING_LEN        200

BOOLEAN   mFeaturerSwitch = TRUE;
BOOLEAN   mResetRequired  = FALSE;

extern UINT16 gPlatformBootTimeOutDefault;

/**
  The function will go through the driver option link list, load and start
  every driver the driver option device path point to.

  @param  BdsDriverLists        The header of the current driver option link list

**/
VOID
EFIAPI
BdsLibLoadDrivers (
  IN LIST_ENTRY                   *BdsDriverLists
  )
{
  EFI_STATUS                Status;
  LIST_ENTRY                *Link;
  BDS_COMMON_OPTION         *Option;
  EFI_HANDLE                ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL *ImageInfo;
  UINTN                     ExitDataSize;
  CHAR16                    *ExitData;
  BOOLEAN                   ReconnectAll;

  ReconnectAll = FALSE;

  //
  // Process the driver option
  //
  for (Link = BdsDriverLists->ForwardLink; Link != BdsDriverLists; Link = Link->ForwardLink) {
    Option = CR (Link, BDS_COMMON_OPTION, Link, BDS_LOAD_OPTION_SIGNATURE);
    
    //
    // If a load option is not marked as LOAD_OPTION_ACTIVE,
    // the boot manager will not automatically load the option.
    //
    if (!IS_LOAD_OPTION_TYPE (Option->Attribute, LOAD_OPTION_ACTIVE)) {
      continue;
    }
    
    //
    // If a driver load option is marked as LOAD_OPTION_FORCE_RECONNECT,
    // then all of the EFI drivers in the system will be disconnected and
    // reconnected after the last driver load option is processed.
    //
    if (IS_LOAD_OPTION_TYPE (Option->Attribute, LOAD_OPTION_FORCE_RECONNECT)) {
      ReconnectAll = TRUE;
    }
    
    //
    // Make sure the driver path is connected.
    //
    BdsLibConnectDevicePath (Option->DevicePath);

    //
    // Load and start the image that Driver#### describes
    //
    Status = gBS->LoadImage (
                    FALSE,
                    gImageHandle,
                    Option->DevicePath,
                    NULL,
                    0,
                    &ImageHandle
                    );

    if (!EFI_ERROR (Status)) {
      gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &ImageInfo);

      //
      // Verify whether this image is a driver, if not,
      // exit it and continue to parse next load option
      //
      if (ImageInfo->ImageCodeType != EfiBootServicesCode && ImageInfo->ImageCodeType != EfiRuntimeServicesCode) {
        gBS->Exit (ImageHandle, EFI_INVALID_PARAMETER, 0, NULL);
        continue;
      }

      if (Option->LoadOptionsSize != 0) {
        ImageInfo->LoadOptionsSize  = Option->LoadOptionsSize;
        ImageInfo->LoadOptions      = Option->LoadOptions;
      }
      //
      // Before calling the image, enable the Watchdog Timer for
      // the 5 Minute period
      //
      gBS->SetWatchdogTimer (5 * 60, 0x0000, 0x00, NULL);

      Status = gBS->StartImage (ImageHandle, &ExitDataSize, &ExitData);
      DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Driver Return Status = %r\n", Status));

      //
      // Clear the Watchdog Timer after the image returns
      //
      gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);
    }
  }
  
  //
  // Process the LOAD_OPTION_FORCE_RECONNECT driver option
  //
  if (ReconnectAll) {
    BdsLibDisconnectAllEfi ();
    BdsLibConnectAll ();
  }

}

/**
  Get the Option Number that does not used.
  Try to locate the specific option variable one by one utile find a free number.

  @param  VariableName          Indicate if the boot#### or driver#### option

  @return The Minimal Free Option Number

**/
UINT16
BdsLibGetFreeOptionNumber (
  IN  CHAR16    *VariableName
  )
{
  UINTN         Index;
  CHAR16        StrTemp[10];
  UINT16        *OptionBuffer;
  UINTN         OptionSize;

  //
  // Try to find the minimum free number from 0, 1, 2, 3....
  //
  Index = 0;
  do {
    if (*VariableName == 'B') {
      UnicodeSPrint (StrTemp, sizeof (StrTemp), L"Boot%04x", Index);
    } else {
      UnicodeSPrint (StrTemp, sizeof (StrTemp), L"Driver%04x", Index);
    }
    //
    // try if the option number is used
    //
    OptionBuffer = BdsLibGetVariableAndSize (
                     StrTemp,
                     &gEfiGlobalVariableGuid,
                     &OptionSize
                     );
    if (OptionBuffer == NULL) {
      break;
    }
    FreePool(OptionBuffer);
    Index++;
  } while (TRUE);

  return ((UINT16) Index);
}


/**
  This function will register the new boot#### or driver#### option base on
  the VariableName. The new registered boot#### or driver#### will be linked
  to BdsOptionList and also update to the VariableName. After the boot#### or
  driver#### updated, the BootOrder or DriverOrder will also be updated.

  @param  BdsOptionList         The header of the boot#### or driver#### link list
  @param  DevicePath            The device path which the boot#### or driver####
                                option present
  @param  String                The description of the boot#### or driver####
  @param  VariableName          Indicate if the boot#### or driver#### option

  @retval EFI_SUCCESS           The boot#### or driver#### have been success
                                registered
  @retval EFI_STATUS            Return the status of gRT->SetVariable ().

**/
EFI_STATUS
EFIAPI
BdsLibRegisterNewOption (
  IN  LIST_ENTRY                     *BdsOptionList,
  IN  EFI_DEVICE_PATH_PROTOCOL       *DevicePath,
  IN  CHAR16                         *String,
  IN  CHAR16                         *VariableName
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  UINT16                    RegisterOptionNumber;
  UINT16                    *TempOptionPtr;
  UINTN                     TempOptionSize;
  UINT16                    *OptionOrderPtr;
  VOID                      *OptionPtr;
  UINTN                     OptionSize;
  UINT8                     *TempPtr;
  EFI_DEVICE_PATH_PROTOCOL  *OptionDevicePath;
  CHAR16                    *Description;
  CHAR16                    OptionName[10];
  BOOLEAN                   UpdateDescription;
  UINT16                    BootOrderEntry;
  UINTN                     OrderItemNum;

  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OptionPtr             = NULL;
  OptionSize            = 0;
  TempPtr               = NULL;
  OptionDevicePath      = NULL;
  Description           = NULL;
  OptionOrderPtr        = NULL;
  UpdateDescription     = FALSE;
  Status                = EFI_SUCCESS;
  ZeroMem (OptionName, sizeof (OptionName));

  TempOptionSize = 0;
  TempOptionPtr = BdsLibGetVariableAndSize (
                    VariableName,
                    &gEfiGlobalVariableGuid,
                    &TempOptionSize
                    );
  //
  // Compare with current option variable if the previous option is set in global variable.
  //
  for (Index = 0; Index < TempOptionSize / sizeof (UINT16); Index++) {
    //
    // TempOptionPtr must not be NULL if we have non-zero TempOptionSize.
    //
    ASSERT (TempOptionPtr != NULL);

    if (*VariableName == 'B') {
      UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", TempOptionPtr[Index]);
    } else {
      UnicodeSPrint (OptionName, sizeof (OptionName), L"Driver%04x", TempOptionPtr[Index]);
    }

    OptionPtr = BdsLibGetVariableAndSize (
                  OptionName,
                  &gEfiGlobalVariableGuid,
                  &OptionSize
                  );
    if (OptionPtr == NULL) {
      continue;
    }

    //
    // Validate the variable.
    //
    if (!ValidateOption(OptionPtr, OptionSize)) {
      FreePool(OptionPtr);
      continue;
    }

    TempPtr         =   OptionPtr;
    TempPtr         +=  sizeof (UINT32) + sizeof (UINT16);
    Description     =   (CHAR16 *) TempPtr;
    TempPtr         +=  StrSize ((CHAR16 *) TempPtr);
    OptionDevicePath =  (EFI_DEVICE_PATH_PROTOCOL *) TempPtr;

    //
    // Notes: the description may will change base on the GetStringToken
    //
    if (CompareMem (OptionDevicePath, DevicePath, GetDevicePathSize (OptionDevicePath)) == 0) {
      if (CompareMem (Description, String, StrSize (Description)) == 0) { 
        //
        // Got the option, so just return
        //
        FreePool (OptionPtr);
        FreePool (TempOptionPtr);
        return EFI_SUCCESS;
      } else {
        //
        // Option description changed, need update.
        //
        UpdateDescription = TRUE;
        FreePool (OptionPtr);
        break;
      }
    }

    FreePool (OptionPtr);
  }

  OptionSize          = sizeof (UINT32) + sizeof (UINT16) + StrSize (String);
  OptionSize          += GetDevicePathSize (DevicePath);
  OptionPtr           = AllocateZeroPool (OptionSize);
  ASSERT (OptionPtr != NULL);
  
  TempPtr             = OptionPtr;
  *(UINT32 *) TempPtr = LOAD_OPTION_ACTIVE;
  TempPtr             += sizeof (UINT32);
  *(UINT16 *) TempPtr = (UINT16) GetDevicePathSize (DevicePath);
  TempPtr             += sizeof (UINT16);
  CopyMem (TempPtr, String, StrSize (String));
  TempPtr             += StrSize (String);
  CopyMem (TempPtr, DevicePath, GetDevicePathSize (DevicePath));

  if (UpdateDescription) {
    //
    // The number in option#### to be updated. 
    // In this case, we must have non-NULL TempOptionPtr.
    //
    ASSERT (TempOptionPtr != NULL);
    RegisterOptionNumber = TempOptionPtr[Index];
  } else {
    //
    // The new option#### number
    //
    RegisterOptionNumber = BdsLibGetFreeOptionNumber(VariableName);
  }

  if (*VariableName == 'B') {
    UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", RegisterOptionNumber);
  } else {
    UnicodeSPrint (OptionName, sizeof (OptionName), L"Driver%04x", RegisterOptionNumber);
  }

  Status = gRT->SetVariable (
                  OptionName,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  OptionSize,
                  OptionPtr
                  );
  //
  // Return if only need to update a changed description or fail to set option.
  //
  if (EFI_ERROR (Status) || UpdateDescription) {
    FreePool (OptionPtr);
    if (TempOptionPtr != NULL) {
      FreePool (TempOptionPtr);
    }
    return Status;
  }

  FreePool (OptionPtr);

  //
  // Update the option order variable
  //

  //
  // If no option order
  //
  if (TempOptionSize == 0) {
    BootOrderEntry = 0;
    Status = gRT->SetVariable (
                    VariableName,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof (UINT16),
                    &BootOrderEntry
                    );
    if (TempOptionPtr != NULL) {
      FreePool (TempOptionPtr);
    }
    return Status;
  }
  
  //
  // TempOptionPtr must not be NULL if TempOptionSize is not zero.
  //
  ASSERT (TempOptionPtr != NULL);
  //
  // Append the new option number to the original option order
  //
  OrderItemNum = (TempOptionSize / sizeof (UINT16)) + 1 ;
  OptionOrderPtr = AllocateZeroPool ( OrderItemNum * sizeof (UINT16));
  ASSERT (OptionOrderPtr!= NULL);
  CopyMem (OptionOrderPtr, TempOptionPtr, (OrderItemNum - 1) * sizeof (UINT16));

  OptionOrderPtr[Index] = RegisterOptionNumber;

  Status = gRT->SetVariable (
                  VariableName,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  OrderItemNum * sizeof (UINT16),
                  OptionOrderPtr
                  );
  FreePool (TempOptionPtr);
  FreePool (OptionOrderPtr);

  return Status;
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
GetDevicePathSizeEx (
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
    if (NodeSize < END_DEVICE_PATH_LENGTH) {
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

  This function returns the byte length of Unicode characters in the Null-terminated
  Unicode string specified by String. 

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().

  @param  String           A pointer to a Null-terminated Unicode string.
  @param  MaxStringLen     Max string len in this string.

  @retval 0                An invalid string.
  @retval Others           The length of String.

**/
UINTN
StrSizeEx (
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
  Validate the EFI Boot#### variable (VendorGuid/Name)

  @param  Variable              Boot#### variable data.
  @param  VariableSize          Returns the size of the EFI variable that was read

  @retval TRUE                  The variable data is correct.
  @retval FALSE                 The variable data is corrupted.

**/
BOOLEAN 
ValidateOption (
  UINT8                     *Variable,
  UINTN                     VariableSize
  )
{
  UINT16                    FilePathSize;
  UINT8                     *TempPtr;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     TempSize;

  if (VariableSize <= sizeof (UINT16) + sizeof (UINT32)) {
    return FALSE;
  }

  //
  // Skip the option attribute
  //
  TempPtr    = Variable;
  TempPtr   += sizeof (UINT32);

  //
  // Get the option's device path size
  //
  FilePathSize  = *(UINT16 *) TempPtr;
  TempPtr      += sizeof (UINT16);

  //
  // Get the option's description string size
  //
  TempSize = StrSizeEx ((CHAR16 *) TempPtr, VariableSize - sizeof (UINT16) - sizeof (UINT32));
  TempPtr += TempSize;

  //
  // Get the option's device path
  //
  DevicePath =  (EFI_DEVICE_PATH_PROTOCOL *) TempPtr;
  TempPtr   += FilePathSize;

  //
  // Validation boot option variable.
  //
  if ((FilePathSize == 0) || (TempSize == 0)) {
    return FALSE;
  }

  if (TempSize + FilePathSize + sizeof (UINT16) + sizeof (UINT32) > VariableSize) {
    return FALSE;
  }

  return (BOOLEAN) (GetDevicePathSizeEx (DevicePath, FilePathSize) != 0);
}

/**
  Convert a single character to number.
  It assumes the input Char is in the scope of L'0' ~ L'9' and L'A' ~ L'F'
  
  @param Char    The input char which need to change to a hex number.
  
**/
UINTN
CharToUint (
  IN CHAR16                           Char
  )
{
  if ((Char >= L'0') && (Char <= L'9')) {
    return (UINTN) (Char - L'0');
  }

  if ((Char >= L'A') && (Char <= L'F')) {
    return (UINTN) (Char - L'A' + 0xA);
  }

  ASSERT (FALSE);
  return 0;
}

/**
  Build the boot#### or driver#### option from the VariableName, the
  build boot#### or driver#### will also be linked to BdsCommonOptionList.

  @param  BdsCommonOptionList   The header of the boot#### or driver#### option
                                link list
  @param  VariableName          EFI Variable name indicate if it is boot#### or
                                driver####

  @retval BDS_COMMON_OPTION     Get the option just been created
  @retval NULL                  Failed to get the new option

**/
BDS_COMMON_OPTION *
EFIAPI
BdsLibVariableToOption (
  IN OUT LIST_ENTRY                   *BdsCommonOptionList,
  IN  CHAR16                          *VariableName
  )
{
  UINT32                    Attribute;
  UINT16                    FilePathSize;
  UINT8                     *Variable;
  UINT8                     *TempPtr;
  UINTN                     VariableSize;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  BDS_COMMON_OPTION         *Option;
  VOID                      *LoadOptions;
  UINT32                    LoadOptionsSize;
  CHAR16                    *Description;
  UINT8                     NumOff;

  //
  // Read the variable. We will never free this data.
  //
  Variable = BdsLibGetVariableAndSize (
              VariableName,
              &gEfiGlobalVariableGuid,
              &VariableSize
              );
  if (Variable == NULL) {
    return NULL;
  }

  //
  // Validate Boot#### variable data.
  //
  if (!ValidateOption(Variable, VariableSize)) {
    FreePool (Variable);
    return NULL;
  }

  //
  // Notes: careful defined the variable of Boot#### or
  // Driver####, consider use some macro to abstract the code
  //
  //
  // Get the option attribute
  //
  TempPtr   =  Variable;
  Attribute =  *(UINT32 *) Variable;
  TempPtr   += sizeof (UINT32);

  //
  // Get the option's device path size
  //
  FilePathSize =  *(UINT16 *) TempPtr;
  TempPtr      += sizeof (UINT16);

  //
  // Get the option's description string
  //
  Description = (CHAR16 *) TempPtr;

  //
  // Get the option's description string size
  //
  TempPtr += StrSize((CHAR16 *) TempPtr);

  //
  // Get the option's device path
  //
  DevicePath =  (EFI_DEVICE_PATH_PROTOCOL *) TempPtr;
  TempPtr    += FilePathSize;

  //
  // Get load opion data.
  //
  LoadOptions     = TempPtr;
  LoadOptionsSize = (UINT32) (VariableSize - (UINTN) (TempPtr - Variable));

  //
  // The Console variables may have multiple device paths, so make
  // an Entry for each one.
  //
  Option = AllocateZeroPool (sizeof (BDS_COMMON_OPTION));
  if (Option == NULL) {
    FreePool (Variable);
    return NULL;
  }

  Option->Signature   = BDS_LOAD_OPTION_SIGNATURE;
  Option->DevicePath  = AllocateZeroPool (GetDevicePathSize (DevicePath));
  ASSERT(Option->DevicePath != NULL);
  CopyMem (Option->DevicePath, DevicePath, GetDevicePathSize (DevicePath));

  Option->Attribute   = Attribute;
  Option->Description = AllocateZeroPool (StrSize (Description));
  ASSERT(Option->Description != NULL);
  CopyMem (Option->Description, Description, StrSize (Description));

  Option->LoadOptions = AllocateZeroPool (LoadOptionsSize);
  ASSERT(Option->LoadOptions != NULL);
  CopyMem (Option->LoadOptions, LoadOptions, LoadOptionsSize);
  Option->LoadOptionsSize = LoadOptionsSize;

  //
  // Get the value from VariableName Unicode string
  // since the ISO standard assumes ASCII equivalent abbreviations, we can be safe in converting this
  // Unicode stream to ASCII without any loss in meaning.
  //
  if (*VariableName == 'B') {
    NumOff = (UINT8) (sizeof (L"Boot") / sizeof (CHAR16) - 1);
    Option->BootCurrent = (UINT16) (CharToUint (VariableName[NumOff+0]) * 0x1000) 
               + (UINT16) (CharToUint (VariableName[NumOff+1]) * 0x100)
               + (UINT16) (CharToUint (VariableName[NumOff+2]) * 0x10)
               + (UINT16) (CharToUint (VariableName[NumOff+3]) * 0x1);
  }
  InsertTailList (BdsCommonOptionList, &Option->Link);
  FreePool (Variable);
  return Option;
}

/**
  Process BootOrder, or DriverOrder variables, by calling
  BdsLibVariableToOption () for each UINT16 in the variables.

  @param  BdsCommonOptionList   The header of the option list base on variable
                                VariableName
  @param  VariableName          EFI Variable name indicate the BootOrder or
                                DriverOrder

  @retval EFI_SUCCESS           Success create the boot option or driver option
                                list
  @retval EFI_OUT_OF_RESOURCES  Failed to get the boot option or driver option list

**/
EFI_STATUS
EFIAPI
BdsLibBuildOptionFromVar (
  IN  LIST_ENTRY                      *BdsCommonOptionList,
  IN  CHAR16                          *VariableName
  )
{
  UINT16            *OptionOrder;
  UINTN             OptionOrderSize;
  UINTN             Index;
  BDS_COMMON_OPTION *Option;
  CHAR16            OptionName[20];

  //
  // Zero Buffer in order to get all BOOT#### variables
  //
  ZeroMem (OptionName, sizeof (OptionName));

  //
  // Read the BootOrder, or DriverOrder variable.
  //
  OptionOrder = BdsLibGetVariableAndSize (
                  VariableName,
                  &gEfiGlobalVariableGuid,
                  &OptionOrderSize
                  );
  if (OptionOrder == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < OptionOrderSize / sizeof (UINT16); Index++) {
    if (*VariableName == 'B') {
      UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", OptionOrder[Index]);
    } else {
      UnicodeSPrint (OptionName, sizeof (OptionName), L"Driver%04x", OptionOrder[Index]);
    }

    Option              = BdsLibVariableToOption (BdsCommonOptionList, OptionName);
    if (Option != NULL) {
      Option->BootCurrent = OptionOrder[Index];
    }
  }

  FreePool (OptionOrder);

  return EFI_SUCCESS;
}

/**
  Get boot mode by looking up configuration table and parsing HOB list

  @param  BootMode              Boot mode from PEI handoff HOB.

  @retval EFI_SUCCESS           Successfully get boot mode

**/
EFI_STATUS
EFIAPI
BdsLibGetBootMode (
  OUT EFI_BOOT_MODE       *BootMode
  )
{
  *BootMode = GetBootModeHob ();

  return EFI_SUCCESS;
}

/**
  Read the EFI variable (VendorGuid/Name) and return a dynamically allocated
  buffer, and the size of the buffer. If failure return NULL.

  @param  Name                  String part of EFI variable name
  @param  VendorGuid            GUID part of EFI variable name
  @param  VariableSize          Returns the size of the EFI variable that was read

  @return                       Dynamically allocated memory that contains a copy of the EFI variable
                                Caller is responsible freeing the buffer.
  @retval NULL                  Variable was not read

**/
VOID *
EFIAPI
BdsLibGetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *VendorGuid,
  OUT UINTN               *VariableSize
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;
  VOID        *Buffer;

  Buffer = NULL;

  //
  // Pass in a zero size buffer to find the required buffer size.
  //
  BufferSize  = 0;
  Status      = gRT->GetVariable (Name, VendorGuid, NULL, &BufferSize, Buffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Allocate the buffer to return
    //
    Buffer = AllocateZeroPool (BufferSize);
    if (Buffer == NULL) {
      *VariableSize = 0;
      return NULL;
    }
    //
    // Read variable into the allocated buffer.
    //
    Status = gRT->GetVariable (Name, VendorGuid, NULL, &BufferSize, Buffer);
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      BufferSize = 0;
      Buffer     = NULL;
    }
  }

  ASSERT (((Buffer == NULL) && (BufferSize == 0)) ||
          ((Buffer != NULL) && (BufferSize != 0))
          );
  *VariableSize = BufferSize;
  return Buffer;
}

/**
  Delete the instance in Multi which matches partly with Single instance

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @return This function will remove the device path instances in Multi which partly
          match with the Single, and return the result device path. If there is no
          remaining device path as a result, this function will return NULL.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
BdsLibDelPartMatchInstance (
  IN     EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN     EFI_DEVICE_PATH_PROTOCOL  *Single
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;
  UINTN                     InstanceSize;
  UINTN                     SingleDpSize;
  UINTN                     Size;

  NewDevicePath     = NULL;
  TempNewDevicePath = NULL;

  if (Multi == NULL || Single == NULL) {
    return Multi;
  }

  Instance        =  GetNextDevicePathInstance (&Multi, &InstanceSize);
  SingleDpSize    =  GetDevicePathSize (Single) - END_DEVICE_PATH_LENGTH;
  InstanceSize    -= END_DEVICE_PATH_LENGTH;

  while (Instance != NULL) {

    Size = (SingleDpSize < InstanceSize) ? SingleDpSize : InstanceSize;

    if ((CompareMem (Instance, Single, Size) != 0)) {
      //
      // Append the device path instance which does not match with Single
      //
      TempNewDevicePath = NewDevicePath;
      NewDevicePath = AppendDevicePathInstance (NewDevicePath, Instance);
      if (TempNewDevicePath != NULL) {
        FreePool(TempNewDevicePath);
      }
    }
    FreePool(Instance);
    Instance = GetNextDevicePathInstance (&Multi, &InstanceSize);
    InstanceSize  -= END_DEVICE_PATH_LENGTH;
  }

  return NewDevicePath;
}

/**
  Function compares a device path data structure to that of all the nodes of a
  second device path instance.

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @retval TRUE                  If the Single device path is contained within Multi device path.
  @retval FALSE                 The Single device path is not match within Multi device path.

**/
BOOLEAN
EFIAPI
BdsLibMatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL  *Single
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathInst;
  UINTN                     Size;

  if (Multi == NULL || Single  == NULL) {
    return FALSE;
  }

  DevicePath      = Multi;
  DevicePathInst  = GetNextDevicePathInstance (&DevicePath, &Size);

  //
  // Search for the match of 'Single' in 'Multi'
  //
  while (DevicePathInst != NULL) {
    //
    // If the single device path is found in multiple device paths,
    // return success
    //
    if (CompareMem (Single, DevicePathInst, Size) == 0) {
      FreePool (DevicePathInst);
      return TRUE;
    }

    FreePool (DevicePathInst);
    DevicePathInst = GetNextDevicePathInstance (&DevicePath, &Size);
  }

  return FALSE;
}

/**
  This function prints a series of strings.

  @param  ConOut                Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
  @param  ...                   A variable argument list containing series of
                                strings, the last string must be NULL.

  @retval EFI_SUCCESS           Success print out the string using ConOut.
  @retval EFI_STATUS            Return the status of the ConOut->OutputString ().

**/
EFI_STATUS
EFIAPI
BdsLibOutputStrings (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *ConOut,
  ...
  )
{
  VA_LIST     Args;
  EFI_STATUS  Status;
  CHAR16      *String;

  Status = EFI_SUCCESS;
  VA_START (Args, ConOut);

  while (!EFI_ERROR (Status)) {
    //
    // If String is NULL, then it's the end of the list
    //
    String = VA_ARG (Args, CHAR16 *);
    if (String == NULL) {
      break;
    }

    Status = ConOut->OutputString (ConOut, String);

    if (EFI_ERROR (Status)) {
      break;
    }
  }
  
  VA_END(Args);
  return Status;
}

//
//  Following are BDS Lib functions which contain all the code about setup browser reset reminder feature.
//  Setup Browser reset reminder feature is that an reset reminder will be given before user leaves the setup browser  if
//  user change any option setting which needs a reset to be effective, and  the reset will be applied according to  the user selection.
//


/**
  Enable the setup browser reset reminder feature.
  This routine is used in platform tip. If the platform policy need the feature, use the routine to enable it.

**/
VOID
EFIAPI
EnableResetReminderFeature (
  VOID
  )
{
  mFeaturerSwitch = TRUE;
}


/**
  Disable the setup browser reset reminder feature.
  This routine is used in platform tip. If the platform policy do not want the feature, use the routine to disable it.

**/
VOID
EFIAPI
DisableResetReminderFeature (
  VOID
  )
{
  mFeaturerSwitch = FALSE;
}


/**
  Record the info that  a reset is required.
  A  module boolean variable is used to record whether a reset is required.

**/
VOID
EFIAPI
EnableResetRequired (
  VOID
  )
{
  mResetRequired = TRUE;
}


/**
  Record the info that  no reset is required.
  A  module boolean variable is used to record whether a reset is required.

**/
VOID
EFIAPI
DisableResetRequired (
  VOID
  )
{
  mResetRequired = FALSE;
}


/**
  Check whether platform policy enable the reset reminder feature. The default is enabled.

**/
BOOLEAN
EFIAPI
IsResetReminderFeatureEnable (
  VOID
  )
{
  return mFeaturerSwitch;
}


/**
  Check if  user changed any option setting which needs a system reset to be effective.

**/
BOOLEAN
EFIAPI
IsResetRequired (
  VOID
  )
{
  return mResetRequired;
}


/**
  Check whether a reset is needed, and finish the reset reminder feature.
  If a reset is needed, Popup a menu to notice user, and finish the feature
  according to the user selection.

**/
VOID
EFIAPI
SetupResetReminder (
  VOID
  )
{
  EFI_INPUT_KEY                 Key;
  CHAR16                        *StringBuffer1;
  CHAR16                        *StringBuffer2;


  //
  //check any reset required change is applied? if yes, reset system
  //
  if (IsResetReminderFeatureEnable ()) {
    if (IsResetRequired ()) {

      StringBuffer1 = AllocateZeroPool (MAX_STRING_LEN * sizeof (CHAR16));
      ASSERT (StringBuffer1 != NULL);
      StringBuffer2 = AllocateZeroPool (MAX_STRING_LEN * sizeof (CHAR16));
      ASSERT (StringBuffer2 != NULL);
      StrCpyS (
        StringBuffer1,
        MAX_STRING_LEN,
        L"Configuration changed. Reset to apply it Now."
        );
      StrCpyS (
        StringBuffer2,
        MAX_STRING_LEN,
        L"Press ENTER to reset"
        );
      //
      // Popup a menu to notice user
      //
      do {
        CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, NULL);
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

      FreePool (StringBuffer1);
      FreePool (StringBuffer2);

      gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
    }
  }
}

/**
  Get the headers (dos, image, optional header) from an image

  @param  Device                SimpleFileSystem device handle
  @param  FileName              File name for the image
  @param  DosHeader             Pointer to dos header
  @param  Hdr                   The buffer in which to return the PE32, PE32+, or TE header.

  @retval EFI_SUCCESS           Successfully get the machine type.
  @retval EFI_NOT_FOUND         The file is not found.
  @retval EFI_LOAD_ERROR        File is not a valid image file.

**/
EFI_STATUS
EFIAPI
BdsLibGetImageHeader (
  IN  EFI_HANDLE                  Device,
  IN  CHAR16                      *FileName,
  OUT EFI_IMAGE_DOS_HEADER        *DosHeader,
  OUT EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Volume;
  EFI_FILE_HANDLE                  Root;
  EFI_FILE_HANDLE                  ThisFile;
  UINTN                            BufferSize;
  UINT64                           FileSize;
  EFI_FILE_INFO                    *Info;

  Root     = NULL;
  ThisFile = NULL;
  //
  // Handle the file system interface to the device
  //
  Status = gBS->HandleProtocol (
                  Device,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID *) &Volume
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = Volume->OpenVolume (
                     Volume,
                     &Root
                     );
  if (EFI_ERROR (Status)) {
    Root = NULL;
    goto Done;
  }
  ASSERT (Root != NULL);
  Status = Root->Open (Root, &ThisFile, FileName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  ASSERT (ThisFile != NULL);

  //
  // Get file size
  //
  BufferSize  = SIZE_OF_EFI_FILE_INFO + 200;
  do {
    Info   = NULL;
    Status = gBS->AllocatePool (EfiBootServicesData, BufferSize, (VOID **) &Info);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    Status = ThisFile->GetInfo (
                         ThisFile,
                         &gEfiFileInfoGuid,
                         &BufferSize,
                         Info
                         );
    if (!EFI_ERROR (Status)) {
      break;
    }
    if (Status != EFI_BUFFER_TOO_SMALL) {
      FreePool (Info);
      goto Done;
    }
    FreePool (Info);
  } while (TRUE);

  FileSize = Info->FileSize;
  FreePool (Info);

  //
  // Read dos header
  //
  BufferSize = sizeof (EFI_IMAGE_DOS_HEADER);
  Status = ThisFile->Read (ThisFile, &BufferSize, DosHeader);
  if (EFI_ERROR (Status) ||
      BufferSize < sizeof (EFI_IMAGE_DOS_HEADER) ||
      FileSize <= DosHeader->e_lfanew ||
      DosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    Status = EFI_LOAD_ERROR;
    goto Done;
  }

  //
  // Move to PE signature
  //
  Status = ThisFile->SetPosition (ThisFile, DosHeader->e_lfanew);
  if (EFI_ERROR (Status)) {
    Status = EFI_LOAD_ERROR;
    goto Done;
  }

  //
  // Read and check PE signature
  //
  BufferSize = sizeof (EFI_IMAGE_OPTIONAL_HEADER_UNION);
  Status = ThisFile->Read (ThisFile, &BufferSize, Hdr.Pe32);
  if (EFI_ERROR (Status) ||
      BufferSize < sizeof (EFI_IMAGE_OPTIONAL_HEADER_UNION) ||
      Hdr.Pe32->Signature != EFI_IMAGE_NT_SIGNATURE) {
    Status = EFI_LOAD_ERROR;
    goto Done;
  }

  //
  // Check PE32 or PE32+ magic
  //
  if (Hdr.Pe32->OptionalHeader.Magic != EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
      Hdr.Pe32->OptionalHeader.Magic != EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    Status = EFI_LOAD_ERROR;
    goto Done;
  }

 Done:
  if (ThisFile != NULL) {
    ThisFile->Close (ThisFile);
  }
  if (Root != NULL) {
    Root->Close (Root);
  }
  return Status;
}

/**
  This routine adjust the memory information for different memory type and 
  save them into the variables for next boot.
**/
VOID
BdsSetMemoryTypeInformationVariable (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_MEMORY_TYPE_INFORMATION  *PreviousMemoryTypeInformation;
  EFI_MEMORY_TYPE_INFORMATION  *CurrentMemoryTypeInformation;
  UINTN                        VariableSize;
  UINTN                        Index;
  UINTN                        Index1;
  UINT32                       Previous;
  UINT32                       Current;
  UINT32                       Next;
  EFI_HOB_GUID_TYPE            *GuidHob;
  BOOLEAN                      MemoryTypeInformationModified;
  BOOLEAN                      MemoryTypeInformationVariableExists;
  EFI_BOOT_MODE                BootMode;

  MemoryTypeInformationModified       = FALSE;
  MemoryTypeInformationVariableExists = FALSE;


  BootMode = GetBootModeHob ();
  //
  // In BOOT_IN_RECOVERY_MODE, Variable region is not reliable.
  //
  if (BootMode == BOOT_IN_RECOVERY_MODE) {
    return;
  }

  //
  // Only check the the Memory Type Information variable in the boot mode 
  // other than BOOT_WITH_DEFAULT_SETTINGS because the Memory Type
  // Information is not valid in this boot mode.
  //
  if (BootMode != BOOT_WITH_DEFAULT_SETTINGS) {
    VariableSize = 0;
    Status = gRT->GetVariable (
                    EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
                    &gEfiMemoryTypeInformationGuid,
                    NULL, 
                    &VariableSize, 
                    NULL
                    );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      MemoryTypeInformationVariableExists = TRUE;
    }
  }

  //
  // Retrieve the current memory usage statistics.  If they are not found, then
  // no adjustments can be made to the Memory Type Information variable.
  //
  Status = EfiGetSystemConfigurationTable (
             &gEfiMemoryTypeInformationGuid,
             (VOID **) &CurrentMemoryTypeInformation
             );
  if (EFI_ERROR (Status) || CurrentMemoryTypeInformation == NULL) {
    return;
  }

  //
  // Get the Memory Type Information settings from Hob if they exist,
  // PEI is responsible for getting them from variable and build a Hob to save them.
  // If the previous Memory Type Information is not available, then set defaults
  //
  GuidHob = GetFirstGuidHob (&gEfiMemoryTypeInformationGuid);
  if (GuidHob == NULL) {
    //
    // If Platform has not built Memory Type Info into the Hob, just return.
    //
    return;
  }
  PreviousMemoryTypeInformation = GET_GUID_HOB_DATA (GuidHob);
  VariableSize = GET_GUID_HOB_DATA_SIZE (GuidHob);

  //
  // Use a heuristic to adjust the Memory Type Information for the next boot
  //
  DEBUG ((EFI_D_INFO, "Memory  Previous  Current    Next   \n"));
  DEBUG ((EFI_D_INFO, " Type    Pages     Pages     Pages  \n"));
  DEBUG ((EFI_D_INFO, "======  ========  ========  ========\n"));

  for (Index = 0; PreviousMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {

    for (Index1 = 0; CurrentMemoryTypeInformation[Index1].Type != EfiMaxMemoryType; Index1++) {
      if (PreviousMemoryTypeInformation[Index].Type == CurrentMemoryTypeInformation[Index1].Type) {
        break;
      }
    }
    if (CurrentMemoryTypeInformation[Index1].Type == EfiMaxMemoryType) {
      continue;
    }

    //
    // Previous is the number of pages pre-allocated
    // Current is the number of pages actually needed
    //
    Previous = PreviousMemoryTypeInformation[Index].NumberOfPages;
    Current  = CurrentMemoryTypeInformation[Index1].NumberOfPages;
    Next     = Previous;

    //
    // Inconsistent Memory Reserved across bootings may lead to S4 fail
    // Write next varible to 125% * current when the pre-allocated memory is:
    //  1. More than 150% of needed memory and boot mode is BOOT_WITH_DEFAULT_SETTING
    //  2. Less than the needed memory
    //
    if ((Current + (Current >> 1)) < Previous) {
      if (BootMode == BOOT_WITH_DEFAULT_SETTINGS) {
        Next = Current + (Current >> 2);
      }
    } else if (Current > Previous) {
      Next = Current + (Current >> 2);
    }
    if (Next > 0 && Next < 4) {
      Next = 4;
    }

    if (Next != Previous) {
      PreviousMemoryTypeInformation[Index].NumberOfPages = Next;
      MemoryTypeInformationModified = TRUE;
    }

    DEBUG ((EFI_D_INFO, "  %02x    %08x  %08x  %08x\n", PreviousMemoryTypeInformation[Index].Type, Previous, Current, Next));
  }

  //
  // If any changes were made to the Memory Type Information settings, then set the new variable value;
  // Or create the variable in first boot.
  //
  if (MemoryTypeInformationModified || !MemoryTypeInformationVariableExists) {
    Status = SetVariableAndReportStatusCodeOnError (
               EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
               &gEfiMemoryTypeInformationGuid,
               EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS,
               VariableSize,
               PreviousMemoryTypeInformation
               );

    if (!EFI_ERROR (Status)) {
      //
      // If the Memory Type Information settings have been modified, then reset the platform
      // so the new Memory Type Information setting will be used to guarantee that an S4
      // entry/resume cycle will not fail.
      //
      if (MemoryTypeInformationModified && PcdGetBool (PcdResetOnMemoryTypeInformationChange)) {
        DEBUG ((EFI_D_INFO, "Memory Type Information settings change. Warm Reset!!!\n"));
        gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
      }
    } else {
      DEBUG ((EFI_D_ERROR, "Memory Type Information settings cannot be saved. OS S4 may fail!\n"));
    }
  }
}

/**
  This routine is kept for backward compatibility.
**/
VOID
EFIAPI
BdsLibSaveMemoryTypeInformation (
  VOID
  )
{
}


/**
  Identify a user and, if authenticated, returns the current user profile handle.

  @param[out]  User           Point to user profile handle.
  
  @retval EFI_SUCCESS         User is successfully identified, or user identification
                              is not supported.
  @retval EFI_ACCESS_DENIED   User is not successfully identified

**/
EFI_STATUS
EFIAPI
BdsLibUserIdentify (
  OUT EFI_USER_PROFILE_HANDLE         *User
  )
{
  EFI_STATUS                          Status;
  EFI_USER_MANAGER_PROTOCOL           *Manager;
  
  Status = gBS->LocateProtocol (
                  &gEfiUserManagerProtocolGuid,
                  NULL,
                  (VOID **) &Manager
                  );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  return Manager->Identify (Manager, User);
}

/**
  Set the variable and report the error through status code upon failure.

  @param  VariableName           A Null-terminated string that is the name of the vendor's variable.
                                 Each VariableName is unique for each VendorGuid. VariableName must
                                 contain 1 or more characters. If VariableName is an empty string,
                                 then EFI_INVALID_PARAMETER is returned.
  @param  VendorGuid             A unique identifier for the vendor.
  @param  Attributes             Attributes bitmask to set for the variable.
  @param  DataSize               The size in bytes of the Data buffer. Unless the EFI_VARIABLE_APPEND_WRITE, 
                                 EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS, or 
                                 EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute is set, a size of zero 
                                 causes the variable to be deleted. When the EFI_VARIABLE_APPEND_WRITE attribute is 
                                 set, then a SetVariable() call with a DataSize of zero will not cause any change to 
                                 the variable value (the timestamp associated with the variable may be updated however 
                                 even if no new data value is provided,see the description of the 
                                 EFI_VARIABLE_AUTHENTICATION_2 descriptor below. In this case the DataSize will not 
                                 be zero since the EFI_VARIABLE_AUTHENTICATION_2 descriptor will be populated). 
  @param  Data                   The contents for the variable.

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits, name, and GUID was supplied, or the
                                 DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER  VariableName is an empty string.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION The variable could not be written due to EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS 
                                 or EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS being set, but the AuthInfo 
                                 does NOT pass the validation check carried out by the firmware.

  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.
**/
EFI_STATUS
SetVariableAndReportStatusCodeOnError (
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  IN UINT32     Attributes,
  IN UINTN      DataSize,
  IN VOID       *Data
  )
{
  EFI_STATUS                 Status;
  EDKII_SET_VARIABLE_STATUS  *SetVariableStatus;
  UINTN                      NameSize;

  Status = gRT->SetVariable (
                  VariableName,
                  VendorGuid,
                  Attributes,
                  DataSize,
                  Data
                  );
  if (EFI_ERROR (Status)) {
    NameSize = StrSize (VariableName);
    SetVariableStatus = AllocatePool (sizeof (EDKII_SET_VARIABLE_STATUS) + NameSize + DataSize);
    if (SetVariableStatus != NULL) {
      CopyGuid (&SetVariableStatus->Guid, VendorGuid);
      SetVariableStatus->NameSize   = NameSize;
      SetVariableStatus->DataSize   = DataSize;
      SetVariableStatus->SetStatus  = Status;
      SetVariableStatus->Attributes = Attributes;
      CopyMem (SetVariableStatus + 1,                          VariableName, NameSize);
      if ((Data != NULL) && (DataSize != 0)) {
        CopyMem (((UINT8 *) (SetVariableStatus + 1)) + NameSize, Data,         DataSize);
      }

      REPORT_STATUS_CODE_EX (
        EFI_ERROR_CODE,
        PcdGet32 (PcdErrorCodeSetVariable),
        0,
        NULL,
        &gEdkiiStatusCodeDataTypeVariableGuid,
        SetVariableStatus,
        sizeof (EDKII_SET_VARIABLE_STATUS) + NameSize + DataSize
        );

      FreePool (SetVariableStatus);
    }
  }

  return Status;
}

