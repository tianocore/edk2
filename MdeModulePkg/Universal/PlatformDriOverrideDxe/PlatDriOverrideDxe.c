/** @file
  This file also installs UEFI PLATFORM_DRIVER_OVERRIDE_PROTOCOL.

  The main code offers a UI interface in device manager to let user configure
  platform override protocol to override the default algorithm for matching
  drivers to controllers.

  The main flow:
  1. It dynamicly locate all controller device path.
  2. It dynamicly locate all drivers which support binding protocol.
  3. It export and dynamicly update two menu to let user select the
     mapping between drivers to controllers.
  4. It save all the mapping info in NV variables which will be consumed
     by platform override protocol driver to publish the platform override protocol.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalPlatDriOverrideDxe.h"
#include "PlatOverMngr.h"

#define EFI_CALLBACK_INFO_SIGNATURE SIGNATURE_32 ('C', 'l', 'b', 'k')
#define EFI_CALLBACK_INFO_FROM_THIS(a)  CR (a, EFI_CALLBACK_INFO, ConfigAccess, EFI_CALLBACK_INFO_SIGNATURE)

typedef struct {
  UINTN                           Signature;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_HANDLE                  RegisteredHandle;
  PLAT_OVER_MNGR_DATA             FakeNvData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;
  EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL PlatformDriverOverride;
} EFI_CALLBACK_INFO;

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

//
// uni string and Vfr Binary data.
//
extern UINT8  VfrBin[];
extern UINT8  PlatDriOverrideDxeStrings[];

//
// module global data
//
CHAR16                       mVariableName[] = L"Data";
LIST_ENTRY                   mMappingDataBase = INITIALIZE_LIST_HEAD_VARIABLE (mMappingDataBase);
BOOLEAN                      mEnvironmentVariableRead = FALSE;
EFI_HANDLE                   mCallerImageHandle = NULL;

EFI_HANDLE                   *mDevicePathHandleBuffer;
EFI_HANDLE                   *mDriverImageHandleBuffer;

INTN                         mSelectedCtrIndex;
EFI_STRING_ID                *mControllerToken;
UINTN                        mDriverImageHandleCount;
EFI_STRING_ID                *mDriverImageToken;
EFI_DEVICE_PATH_PROTOCOL     **mControllerDevicePathProtocol;
UINTN                        mSelectedDriverImageNum;
UINTN                        mLastSavedDriverImageNum;
UINT16                       mCurrentPage;
EFI_CALLBACK_INFO           *mCallbackInfo;
BOOLEAN                     *mDriSelection;
UINTN                        mMaxDeviceCount;

HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    PLAT_OVER_MNGR_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

/**
  Converting a given device to an unicode string.

  @param    DevPath     Given device path instance

  @return   Converted string from given device path.
  @retval   L"?" Converting failed.
**/
CHAR16 *
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  )
{
  CHAR16                          *Text;
  Text = ConvertDevicePathToText (
           DevPath,
           FALSE,
           TRUE
           );
  if (Text == NULL) {
    Text = AllocateCopyPool (sizeof (L"?"), L"?");
    ASSERT (Text != NULL);
  }

  return Text;
}

/**
  Worker function to get the driver name by ComponentName or ComponentName2 protocol
  according to the driver binding handle.

  @param  DriverBindingHandle  The Handle of DriverBinding.
  @param  ProtocolGuid         The pointer to Component Name (2) protocol GUID.
  @param  VariableName         The name of the RFC 4646 or ISO 639-2 language variable.

  @retval !NULL               Pointer into the image name if the image name is found,
  @retval NULL                Pointer to NULL if the image name is not found.

**/
CHAR16 *
GetComponentNameWorker (
  IN EFI_HANDLE                      DriverBindingHandle,
  IN EFI_GUID                        *ProtocolGuid,
  IN CONST CHAR16                    *VariableName
  )
{
  EFI_STATUS                         Status;
  EFI_COMPONENT_NAME_PROTOCOL        *ComponentName;
  CHAR16                             *DriverName;
  CHAR8                              *Language;
  CHAR8                              *BestLanguage;

  Status = gBS->OpenProtocol (
                  DriverBindingHandle,
                  ProtocolGuid,
                  (VOID *) &ComponentName,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Find the best matching language.
  //
  GetEfiGlobalVariable2 (VariableName, (VOID**)&Language, NULL);
  BestLanguage = GetBestLanguage (
                   ComponentName->SupportedLanguages,
                   (BOOLEAN) (ProtocolGuid == &gEfiComponentNameProtocolGuid),
                   Language,
                   NULL
                   );

  DriverName = NULL;
  if (BestLanguage != NULL) {
    ComponentName->GetDriverName (
                     ComponentName,
                     BestLanguage,
                     &DriverName
                     );
    FreePool (BestLanguage);
  }

  if (Language != NULL) {
    FreePool (Language);
  }

  return DriverName;
}


/**
  Get the driver name by ComponentName or ComponentName2 protocol
  according to the driver binding handle

  @param DriverBindingHandle  The Handle of DriverBinding.

  @retval !NULL               Pointer into the image name if the image name is found,
  @retval NULL                Pointer to NULL if the image name is not found.

**/
CHAR16 *
GetComponentName (
  IN EFI_HANDLE                      DriverBindingHandle
  )
{
  CHAR16                    *DriverName;

  //
  // Try RFC 4646 Component Name 2 protocol first.
  //
  DriverName = GetComponentNameWorker (DriverBindingHandle, &gEfiComponentName2ProtocolGuid, L"PlatformLang");
  if (DriverName == NULL) {
    //
    // If we can not get driver name from Component Name 2 protocol, we can try ISO 639-2 Component Name protocol.
    //
    DriverName = GetComponentNameWorker (DriverBindingHandle, &gEfiComponentNameProtocolGuid, L"Lang");
  }

  return DriverName;
}

/**
  Get the image name from EFI UI section.
  Get FV protocol by its loaded image protocol to abstract EFI UI section.

  @param Image            Pointer to the loaded image protocol

  @retval !NULL           Pointer to the image name if the image name is found,
  @retval NULL            NULL if the image name is not found.

**/
CHAR16 *
GetImageName (
  IN EFI_LOADED_IMAGE_PROTOCOL *Image
  )
{
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_PROTOCOL          *DevPathNode;
  EFI_DEVICE_PATH_PROTOCOL          *AlignedDevPathNode;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FvFilePath;
  VOID                              *Buffer;
  UINTN                             BufferSize;
  UINT32                            AuthenticationStatus;
  EFI_GUID                          *NameGuid;
  EFI_FIRMWARE_VOLUME2_PROTOCOL     *Fv2;

  Fv2         = NULL;
  Buffer      = NULL;
  BufferSize  = 0;

  if (Image->FilePath == NULL) {
    return NULL;
  }
  DevPathNode  = Image->FilePath;

  while (!IsDevicePathEnd (DevPathNode)) {
    //
    // Make sure device path node is aligned when accessing it's FV Name Guid field.
    //
    AlignedDevPathNode = AllocateCopyPool (DevicePathNodeLength(DevPathNode), DevPathNode);

    //
    // Find the Fv File path
    //
    NameGuid = EfiGetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)AlignedDevPathNode);
    if (NameGuid != NULL) {
      FvFilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) AlignedDevPathNode;
      Status = gBS->HandleProtocol (
                    Image->DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **) &Fv2
                    );
      //
      // Locate Image EFI UI section to get the image name.
      //
      if (!EFI_ERROR (Status)) {
        Status = Fv2->ReadSection (
                        Fv2,
                        &FvFilePath->FvFileName,
                        EFI_SECTION_USER_INTERFACE,
                        0,
                        &Buffer,
                        &BufferSize,
                        &AuthenticationStatus
                        );
        if (!EFI_ERROR (Status)) {
          FreePool (AlignedDevPathNode);
          break;
        }
        Buffer = NULL;
      }
    }

    FreePool (AlignedDevPathNode);

    //
    // Next device path node
    //
    DevPathNode = NextDevicePathNode (DevPathNode);
  }

  return Buffer;
}

/**
  Prepare the first page to let user select the device controller which need to
  add mapping drivers if user select 'Refresh' in first page.
  During first page, user will see all currnet controller device path in system,
  select any device path will go to second page to select its overrides drivers.

  @param  Private        Pointer to EFI_CALLBACK_INFO.
  @param  KeyValue       The callback key value of device controller item in first page.
  @param  FakeNvData     Pointer to PLAT_OVER_MNGR_DATA.

  @retval EFI_SUCCESS    Always returned.

**/
EFI_STATUS
UpdateDeviceSelectPage (
  IN EFI_CALLBACK_INFO                *Private,
  IN UINT16                           KeyValue,
  IN PLAT_OVER_MNGR_DATA              *FakeNvData
  )
{
  EFI_STATUS                                Status;
  UINTN                                     Index;
  UINTN                                     DevicePathHandleCount;
  UINTN                                     NewStrSize;
  CHAR16                                    *NewString;
  EFI_STRING_ID                             NewStringToken;
  CHAR16                                    *ControllerName;
  EFI_DEVICE_PATH_PROTOCOL                  *ControllerDevicePath;
  EFI_PCI_IO_PROTOCOL                       *PciIo;
  EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL *BusSpecificDriverOverride;
  UINTN                                     Len;
  VOID                                      *StartOpCodeHandle;
  VOID                                      *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL                        *StartLabel;
  EFI_IFR_GUID_LABEL                        *EndLabel;

  //
  // Set current page form ID.
  //
  mCurrentPage = FORM_ID_DEVICE;

  //
  // Initial the mapping database in memory
  //
  FreeMappingDatabase (&mMappingDataBase);
  InitOverridesMapping (&mMappingDataBase);

  //
  // Init OpCode Handle
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number = FORM_ID_DEVICE;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  //
  // Clear first page form
  //
  HiiUpdateForm (
    Private->RegisteredHandle,
    &gPlatformOverridesManagerGuid,
    FORM_ID_DEVICE,
    StartOpCodeHandle, // Label FORM_ID_DEVICE
    EndOpCodeHandle    // LABEL_END
    );

  //
  // When user enter the page at first time, the 'first refresh' string is given to notify user to refresh all the drivers,
  // then the 'first refresh' string will be replaced by the 'refresh' string, and the two strings content are same after the replacement
  //
  NewStringToken = STRING_TOKEN (STR_FIRST_REFRESH);
  NewString = HiiGetString (Private->RegisteredHandle, STRING_TOKEN (STR_REFRESH), NULL);
  ASSERT (NewString != NULL);
  if (HiiSetString (Private->RegisteredHandle, NewStringToken, NewString, NULL) == 0) {
    ASSERT (FALSE);
  }
  FreePool (NewString);

  NewStringToken = STRING_TOKEN (STR_FIRST_REFRESH_HELP);
  NewString = HiiGetString (Private->RegisteredHandle, STRING_TOKEN (STR_REFRESH_HELP), NULL);
  ASSERT (NewString != NULL);
  if (HiiSetString (Private->RegisteredHandle, NewStringToken, NewString, NULL) == 0) {
    ASSERT (FALSE);
  }
  FreePool (NewString);

  //
  // created needed controller device item in first page
  //
  DevicePathHandleCount  = 0;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDevicePathProtocolGuid,
                  NULL,
                  &DevicePathHandleCount,
                  &mDevicePathHandleBuffer
                  );
  if (EFI_ERROR (Status) || (DevicePathHandleCount == 0)) {
    return EFI_SUCCESS;
  }

  mMaxDeviceCount = DevicePathHandleCount;
  mControllerDevicePathProtocol = AllocateZeroPool (DevicePathHandleCount * sizeof (EFI_DEVICE_PATH_PROTOCOL *));
  ASSERT (mControllerDevicePathProtocol != NULL);
  mControllerToken = AllocateZeroPool (DevicePathHandleCount * sizeof (EFI_STRING_ID));
  ASSERT (mControllerToken != NULL);

  for (Index = 0; Index < DevicePathHandleCount; Index++) {
    if (FakeNvData->PciDeviceFilter == 0x01) {
      //
      // Only care PCI device which contain efi driver in its option rom.
      //

      //
      // Check whether it is a pci device
      //
      ControllerDevicePath = NULL;
      Status = gBS->OpenProtocol (
                      mDevicePathHandleBuffer[Index],
                      &gEfiPciIoProtocolGuid,
                      (VOID **) &PciIo,
                      NULL,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }
      //
      // Check whether it contain efi driver in its option rom
      //
      Status = gBS->HandleProtocol(
                       mDevicePathHandleBuffer[Index],
                       &gEfiBusSpecificDriverOverrideProtocolGuid,
                       (VOID **) &BusSpecificDriverOverride
                       );
      if (EFI_ERROR (Status) || BusSpecificDriverOverride == NULL) {
        continue;
      }
    }

    ControllerDevicePath = NULL;
    Status = gBS->OpenProtocol (
                    mDevicePathHandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **) &ControllerDevicePath,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    ASSERT_EFI_ERROR (Status);
    //
    // Save the device path protocol interface
    //
    mControllerDevicePathProtocol[Index] = ControllerDevicePath;

    //
    // Get the driver name
    //
    ControllerName = DevicePathToStr (ControllerDevicePath);

    //
    // Export the driver name string and create item in set options page
    //
    Len = StrSize (ControllerName);
    NewStrSize = Len + StrSize (L"--");
    NewString = AllocateZeroPool (NewStrSize);
    ASSERT (NewString != NULL);
    if (EFI_ERROR (CheckMapping (ControllerDevicePath,NULL, &mMappingDataBase, NULL, NULL))) {
      StrCatS (NewString, NewStrSize/sizeof(CHAR16), L"--");
    } else {
      StrCatS (NewString, NewStrSize/sizeof(CHAR16), L"**");
    }
    StrCatS (NewString, NewStrSize/sizeof(CHAR16), ControllerName);

    NewStringToken = HiiSetString (Private->RegisteredHandle, mControllerToken[Index], NewString, NULL);
    ASSERT (NewStringToken != 0);
    FreePool (NewString);
    //
    // Save the device path string toke for next access use
    //
    mControllerToken[Index] = NewStringToken;

    HiiCreateGotoOpCode (
      StartOpCodeHandle,
      FORM_ID_DRIVER,
      NewStringToken,
      STRING_TOKEN (STR_GOTO_HELP_DRIVER),
      EFI_IFR_FLAG_CALLBACK,
      (UINT16) (Index + KEY_VALUE_DEVICE_OFFSET)
      );
  }

  //
  // Update first page form
  //
  HiiUpdateForm (
    Private->RegisteredHandle,
    &gPlatformOverridesManagerGuid,
    FORM_ID_DEVICE,
    StartOpCodeHandle, // Label FORM_ID_DEVICE
    EndOpCodeHandle    // LABEL_END
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return EFI_SUCCESS;
}

/**
  Get the first Driver Binding handle which has the specific image handle.

  @param  ImageHandle          The Image handle

  @return                      Handle to Driver binding
  @retval NULL                 The parameter is not valid or the driver binding handle is not found.

**/
EFI_HANDLE
GetDriverBindingHandleFromImageHandle (
  IN  EFI_HANDLE   ImageHandle
  )
{
  EFI_STATUS                        Status;
  UINTN                             Index;
  UINTN                             DriverBindingHandleCount;
  EFI_HANDLE                        *DriverBindingHandleBuffer;
  EFI_DRIVER_BINDING_PROTOCOL       *DriverBindingInterface;
  EFI_HANDLE                        DriverBindingHandle;

  DriverBindingHandle = NULL;

  if (ImageHandle == NULL) {
    return NULL;
  }
  //
  // Get all drivers which support driver binding protocol
  //
  DriverBindingHandleCount  = 0;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDriverBindingProtocolGuid,
                  NULL,
                  &DriverBindingHandleCount,
                  &DriverBindingHandleBuffer
                  );
  if (EFI_ERROR (Status) || (DriverBindingHandleCount == 0)) {
    return NULL;
  }

  //
  // Get the first Driver Binding handle which has the specific image handle.
  //
  for (Index = 0; Index < DriverBindingHandleCount; Index++) {
    DriverBindingInterface = NULL;
    Status = gBS->OpenProtocol (
                    DriverBindingHandleBuffer[Index],
                    &gEfiDriverBindingProtocolGuid,
                    (VOID **) &DriverBindingInterface,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (DriverBindingInterface->ImageHandle == ImageHandle) {
      DriverBindingHandle = DriverBindingHandleBuffer[Index];
      break;
    }
  }

  FreePool (DriverBindingHandleBuffer);
  return DriverBindingHandle;
}

/**
  Prepare to let user select the drivers which need mapping with the device controller
  selected in first page.

  @param  Private        Pointer to EFI_CALLBACK_INFO.
  @param  KeyValue       The callback key value of device controller item in first page.
                         KeyValue is larger than or equal to KEY_VALUE_DEVICE_OFFSET.
  @param  FakeNvData     Pointer to PLAT_OVER_MNGR_DATA.

  @retval EFI_SUCCESS    Always returned.

**/
EFI_STATUS
UpdateBindingDriverSelectPage (
  IN EFI_CALLBACK_INFO                *Private,
  IN UINT16                           KeyValue,
  IN PLAT_OVER_MNGR_DATA              *FakeNvData
  )
{
  EFI_STATUS                                Status;
  UINTN                                     Index;
  UINTN                                     NewStrSize;
  CHAR16                                    *NewString;
  EFI_STRING_ID                             NewStringToken;
  EFI_STRING_ID                             NewStringHelpToken;
  UINTN                                     DriverImageHandleCount;
  EFI_LOADED_IMAGE_PROTOCOL                 *LoadedImage;
  CHAR16                                    *DriverName;
  BOOLEAN                                   FreeDriverName;
  EFI_DEVICE_PATH_PROTOCOL                  *LoadedImageDevicePath;
  EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL *BusSpecificDriverOverride;
  EFI_HANDLE                                DriverBindingHandle;
  VOID                                      *StartOpCodeHandle;
  VOID                                      *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL                        *StartLabel;
  EFI_IFR_GUID_LABEL                        *EndLabel;
  EFI_LOADED_IMAGE_PROTOCOL                 **DriverImageProtocol;
  EFI_STRING_ID                             *DriverImageFilePathToken;
  UINT8                                     CheckFlags;

  //
  // If user select a controller item in the first page  the following code will be run.
  // During second page, user will see all currnet driver bind protocol driver, the driver name and its device path will be shown
  //
  //First acquire the list of Loaded Image Protocols, and then when  want the name of the driver, look up all the Driver Binding Protocols
  // and find the first one whose ImageHandle field matches the image handle of the Loaded Image Protocol.
  // then use the Component Name Protocol on the same handle as the first matching Driver Binding Protocol to look up the name of the driver.
  //

  mCurrentPage = FORM_ID_DRIVER;
  //
  // Switch the item callback key value to its NO. in mDevicePathHandleBuffer
  //
  mSelectedCtrIndex = KeyValue - KEY_VALUE_DEVICE_OFFSET;
  ASSERT (mSelectedCtrIndex >= 0 && mSelectedCtrIndex < MAX_CHOICE_NUM);

  mLastSavedDriverImageNum = 0;

  //
  // Init OpCode Handle
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = FORM_ID_DRIVER;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  //
  // Clear second page form
  //
  HiiUpdateForm (
    Private->RegisteredHandle,
    &gPlatformOverridesManagerGuid,
    FORM_ID_DRIVER,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  //
  // Show all driver which support loaded image protocol in second page
  //
  DriverImageHandleCount  = 0;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiLoadedImageProtocolGuid,
                  NULL,
                  &DriverImageHandleCount,
                  &mDriverImageHandleBuffer
                  );
  if (EFI_ERROR (Status) || (DriverImageHandleCount == 0)) {
    return EFI_NOT_FOUND;
  }

  mDriverImageToken = AllocateZeroPool (DriverImageHandleCount * sizeof (EFI_STRING_ID));
  ASSERT (mDriverImageToken != NULL);
  mDriSelection = AllocateZeroPool (DriverImageHandleCount * sizeof (BOOLEAN));
  ASSERT (mDriSelection != NULL);

  DriverImageProtocol = AllocateZeroPool (DriverImageHandleCount * sizeof (EFI_LOADED_IMAGE_PROTOCOL *));
  ASSERT (DriverImageProtocol != NULL);
  DriverImageFilePathToken = AllocateZeroPool (DriverImageHandleCount * sizeof (EFI_STRING_ID));
  ASSERT (DriverImageFilePathToken != NULL);

  mDriverImageHandleCount = DriverImageHandleCount;
  for (Index = 0; Index < DriverImageHandleCount; Index++) {
    //
    // Step1: Get the driver image total file path for help string and the driver name.
    //

    //
    // Find driver's Loaded Image protocol
    //
    LoadedImage =NULL;

    Status = gBS->OpenProtocol (
                    mDriverImageHandleBuffer[Index],
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **) &LoadedImage,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      mDriSelection[Index] = FALSE;
      continue;
    }
    DriverImageProtocol[Index] = LoadedImage;
    //
    // Find its related driver binding protocol
    //
    DriverBindingHandle = GetDriverBindingHandleFromImageHandle (mDriverImageHandleBuffer[Index]);
    if (DriverBindingHandle == NULL) {
      mDriSelection[Index] = FALSE;
      continue;
    }

    //
    // Get the EFI Loaded Image Device Path Protocol
    //
    LoadedImageDevicePath = NULL;
    Status = gBS->HandleProtocol (
                        mDriverImageHandleBuffer[Index],
                        &gEfiLoadedImageDevicePathProtocolGuid,
                        (VOID **) &LoadedImageDevicePath
                        );
    if (LoadedImageDevicePath == NULL) {
      mDriSelection[Index] = FALSE;
      continue;
    }

    if (FakeNvData->PciDeviceFilter == 0x01) {
      //
      // only care the driver which is in a Pci device option rom,
      // and the driver's LoadedImage->DeviceHandle must point to a pci device which has efi option rom
      //
      if (!EFI_ERROR (Status)) {
        Status = gBS->HandleProtocol(
                         LoadedImage->DeviceHandle,
                         &gEfiBusSpecificDriverOverrideProtocolGuid,
                         (VOID **) &BusSpecificDriverOverride
                         );
        if (EFI_ERROR (Status) || BusSpecificDriverOverride == NULL) {
          mDriSelection[Index] = FALSE;
          continue;
        }
      } else {
        mDriSelection[Index] = FALSE;
        continue;
      }
    }

    //
    // For driver name, try to get its component name, if fail, get its image name,
    // if also fail, give a default name.
    //
    FreeDriverName = FALSE;
    DriverName = GetComponentName (DriverBindingHandle);
    if (DriverName == NULL) {
      //
      // get its image name
      //
      DriverName = GetImageName (LoadedImage);
    }
    if (DriverName == NULL) {
      //
      // give a default name
      //
      DriverName = HiiGetString (Private->RegisteredHandle, STRING_TOKEN (STR_DRIVER_DEFAULT_NAME), NULL);
      ASSERT (DriverName != NULL);
      FreeDriverName = TRUE;  // the DriverName string need to free pool
    }


    //
    // Step2 Export the driver name string and create check box item in second page
    //

    //
    // First create the driver image name
    //
    NewStrSize = StrSize (DriverName);
    NewString = AllocateZeroPool (NewStrSize);
    ASSERT (NewString != NULL);
    if (EFI_ERROR (CheckMapping (mControllerDevicePathProtocol[mSelectedCtrIndex], LoadedImageDevicePath, &mMappingDataBase, NULL, NULL))) {
      mDriSelection[Index] = FALSE;
    } else {
      mDriSelection[Index] = TRUE;
      mLastSavedDriverImageNum++;
    }
    StrCatS (NewString, NewStrSize/sizeof(CHAR16), DriverName);
    NewStringToken = HiiSetString (Private->RegisteredHandle, mDriverImageToken[Index], NewString, NULL);
    ASSERT (NewStringToken != 0);
    mDriverImageToken[Index] = NewStringToken;
    FreePool (NewString);
    if (FreeDriverName) {
      FreePool (DriverName);
    }

    //
    // Second create the driver image device path as item help string
    //
    DriverName = DevicePathToStr (LoadedImageDevicePath);

    NewStrSize = StrSize (DriverName);
    NewString = AllocateZeroPool (NewStrSize);
    ASSERT (NewString != NULL);
    StrCatS (NewString, NewStrSize/sizeof(CHAR16), DriverName);
    NewStringHelpToken = HiiSetString (Private->RegisteredHandle, DriverImageFilePathToken[Index], NewString, NULL);
    ASSERT (NewStringHelpToken != 0);
    DriverImageFilePathToken[Index] = NewStringHelpToken;
    FreePool (NewString);
    FreePool (DriverName);

    CheckFlags        = 0;
    if (mDriSelection[Index]) {
      CheckFlags |= EFI_IFR_CHECKBOX_DEFAULT;
    }

    HiiCreateCheckBoxOpCode (
      StartOpCodeHandle,
      (UINT16) (KEY_VALUE_DRIVER_OFFSET + Index),
      0,
      0,
      NewStringToken,
      NewStringHelpToken,
      EFI_IFR_FLAG_CALLBACK,
      CheckFlags,
      NULL
      );
  }

  //
  // Update second page form
  //
  HiiUpdateForm (
    Private->RegisteredHandle,
    &gPlatformOverridesManagerGuid,
    FORM_ID_DRIVER,
    StartOpCodeHandle, // Label FORM_ID_DRIVER
    EndOpCodeHandle    // LABEL_END
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  if (DriverImageProtocol != NULL) {
    FreePool (DriverImageProtocol);
  }

  if (DriverImageFilePathToken != NULL) {
    FreePool (DriverImageFilePathToken);
  }

  return EFI_SUCCESS;
}

/**
  Prepare to let user select the priority order of the drivers which are
  selected in second page.

  @param  Private        Pointer to EFI_CALLBACK_INFO.
  @param  KeyValue       The callback key value of device controller item in first page.
  @param  FakeNvData     Pointer to PLAT_OVER_MNGR_DATA.

  @retval EFI_SUCCESS    Always returned.

**/
EFI_STATUS
UpdatePrioritySelectPage (
  IN EFI_CALLBACK_INFO                *Private,
  IN UINT16                           KeyValue,
  IN PLAT_OVER_MNGR_DATA              *FakeNvData
  )
{
  UINTN                                     Index;
  EFI_DEVICE_PATH_PROTOCOL                  *LoadedImageDevicePath;
  UINTN                                     SelectedDriverImageNum;
  UINT32                                    DriverImageNO;
  UINTN                                     MinNO;
  UINTN                                     Index1;
  UINTN                                     TempNO[100];
  UINTN                                     OrderNO[100];
  VOID                                      *StartOpCodeHandle;
  VOID                                      *EndOpCodeHandle;
  VOID                                      *OptionsOpCodeHandle;
  EFI_IFR_GUID_LABEL                        *StartLabel;
  EFI_IFR_GUID_LABEL                        *EndLabel;

  //
  // Following code will be run if user select 'order ... priority' item in second page
  // Prepare third page.  In third page, user will order the  drivers priority which are selected in second page
  //
  mCurrentPage = FORM_ID_ORDER;

  //
  // Init OpCode Handle
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = FORM_ID_ORDER;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  //
  // Clear third page form
  //
  HiiUpdateForm (
    Private->RegisteredHandle,
    &gPlatformOverridesManagerGuid,
    FORM_ID_ORDER,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  //
  // Check how many drivers have been selected
  //
  SelectedDriverImageNum = 0;
  for (Index = 0; Index < mDriverImageHandleCount; Index++) {
    if (mDriSelection[Index]) {
      SelectedDriverImageNum ++;
    }
  }

  mSelectedDriverImageNum = SelectedDriverImageNum;
  if (SelectedDriverImageNum == 0) {
    return EFI_SUCCESS;
  }

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  //
  // Create order list for those selected drivers
  //
  SelectedDriverImageNum = 0;
  for (Index = 0; Index < mDriverImageHandleCount; Index++) {
    if (mDriSelection[Index]) {
      //
      // Use the NO. in driver binding buffer as value, will use it later
      //
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        mDriverImageToken[Index],
        0,
        EFI_IFR_NUMERIC_SIZE_1,
        Index + 1
        );

      //
      // Get the EFI Loaded Image Device Path Protocol
      //
      LoadedImageDevicePath = NULL;
      gBS->HandleProtocol (
                 mDriverImageHandleBuffer[Index],
                 &gEfiLoadedImageDevicePathProtocolGuid,
                 (VOID **) &LoadedImageDevicePath
                 );
      ASSERT (LoadedImageDevicePath != NULL);

      //
      // Check the driver DriverImage's order number in mapping database
      //
      DriverImageNO = 0;
      CheckMapping (
              mControllerDevicePathProtocol[mSelectedCtrIndex],
              LoadedImageDevicePath,
              &mMappingDataBase,
              NULL,
              &DriverImageNO
              );
      if (DriverImageNO == 0) {
        DriverImageNO = (UINT32) mLastSavedDriverImageNum + 1;
        mLastSavedDriverImageNum++;
      }
      TempNO[SelectedDriverImageNum] = DriverImageNO;
      OrderNO[SelectedDriverImageNum] = Index + 1;
      SelectedDriverImageNum ++;
    }
  }

  ASSERT (SelectedDriverImageNum == mSelectedDriverImageNum);
  //
  // NvRamMap Must be clear firstly
  //
  ZeroMem (FakeNvData->DriOrder, sizeof (FakeNvData->DriOrder));

  //
  // Order the selected drivers according to the info already in mapping database
  // the less order number in mapping database the less order number in NvRamMap
  //
  for (Index=0; Index < SelectedDriverImageNum; Index++) {
    //
    // Find the minimal order number in TempNO array,  its index in TempNO is same as IfrOptionList array
    //
    MinNO = 0;
    for (Index1=0; Index1 < SelectedDriverImageNum; Index1++) {
      if (TempNO[Index1] < TempNO[MinNO]) {
        MinNO = Index1;
      }
    }
    //
    // the IfrOptionList[MinNO].Value = the driver NO. in driver binding buffer
    //
    FakeNvData->DriOrder[Index] = (UINT8) OrderNO[MinNO];
    TempNO[MinNO] = MAX_CHOICE_NUM + 1;
  }

  //
  // Create Order List OpCode
  //
  HiiCreateOrderedListOpCode (
    StartOpCodeHandle,
    (UINT16) DRIVER_ORDER_QUESTION_ID,
    VARSTORE_ID_PLAT_OVER_MNGR,
    (UINT16) DRIVER_ORDER_VAR_OFFSET,
    mControllerToken[mSelectedCtrIndex],
    mControllerToken[mSelectedCtrIndex],
    EFI_IFR_FLAG_RESET_REQUIRED,
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    (UINT8) MAX_CHOICE_NUM,
    OptionsOpCodeHandle,
    NULL
    );

  //
  // Update third page form
  //
  HiiUpdateForm (
    Private->RegisteredHandle,
    &gPlatformOverridesManagerGuid,
    FORM_ID_ORDER,
    StartOpCodeHandle, // Label FORM_ID_ORDER
    EndOpCodeHandle    // LABEL_END
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

  return EFI_SUCCESS;
}

/**
  Save the save the mapping database to NV variable.

  @param  Private        Pointer to EFI_CALLBACK_INFO.
  @param  KeyValue       The callback key value of device controller item in first page.
  @param  FakeNvData     Pointer to PLAT_OVER_MNGR_DATA.

  @retval EFI_SUCCESS    Always returned.

**/
EFI_STATUS
CommitChanges (
  IN EFI_CALLBACK_INFO                *Private,
  IN UINT16                           KeyValue,
  IN PLAT_OVER_MNGR_DATA              *FakeNvData
  )
{
  EFI_STATUS                                Status;
  UINTN                                     Index;
  UINTN                                     SelectedDriverImageNum;
  EFI_DEVICE_PATH_PROTOCOL                  *LoadedImageDevicePath;
  //
  //  Following code will be run if user select 'commint changes' in third page
  //  user enter 'Commit Changes' to save the mapping database
  //
  DeleteDriverImage (mControllerDevicePathProtocol[mSelectedCtrIndex], NULL, &mMappingDataBase);
  for (SelectedDriverImageNum = 0; SelectedDriverImageNum < mSelectedDriverImageNum; SelectedDriverImageNum++) {
    //
    // DriOrder[SelectedDriverImageNum] = the driver NO. in driver binding buffer
    //
    Index = FakeNvData->DriOrder[SelectedDriverImageNum] - 1;

    //
    // Get the EFI Loaded Image Device Path Protocol
    //
    LoadedImageDevicePath = NULL;
    Status = gBS->HandleProtocol (
                        mDriverImageHandleBuffer[Index],
                        &gEfiLoadedImageDevicePathProtocolGuid,
                        (VOID **) &LoadedImageDevicePath
                        );
    ASSERT (LoadedImageDevicePath != NULL);

    InsertDriverImage (
            mControllerDevicePathProtocol[mSelectedCtrIndex],
            LoadedImageDevicePath,
            &mMappingDataBase,
            (UINT32)SelectedDriverImageNum + 1
            );
  }
  Status = SaveOverridesMapping (&mMappingDataBase);

  return Status;
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param  This         Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request      A null-terminated Unicode string in <ConfigRequest> format.
  @param  Progress     On return, points to a character in the Request string.
                       Points to the string's null terminator if request was successful.
                       Points to the most recent '&' before the first failing name/value
                       pair (or the beginning of the string if the failure is in the
                       first name/value pair) if the request was not successful.
  @param  Results      A null-terminated Unicode string in <ConfigAltResp> format which
                       has all values filled in for the names in the Request string.
                       String to be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
PlatOverMngrExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;
  EFI_CALLBACK_INFO                *Private;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STRING                       ConfigRequestHdr;
  EFI_STRING                       ConfigRequest;
  BOOLEAN                          AllocatedRequest;
  UINTN                            Size;
  UINTN                            BufferSize;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gPlatformOverridesManagerGuid, mVariableName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  Size             = 0;
  AllocatedRequest = FALSE;

  Private          = EFI_CALLBACK_INFO_FROM_THIS (This);
  HiiConfigRouting = Private->HiiConfigRouting;
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gPlatformOverridesManagerGuid, mVariableName, Private->DriverHandle);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    BufferSize = sizeof (PLAT_OVER_MNGR_DATA);
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  Status = HiiConfigRouting->BlockToConfig (
                                HiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) &Private->FakeNvData,
                                sizeof (PLAT_OVER_MNGR_DATA),
                                Results,
                                Progress
                                );

  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }
  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function processes the results of changes in configuration.

  @param  This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration   A null-terminated Unicode string in <ConfigRequest> format.
  @param  Progress        A pointer to a string filled in with the offset of the most
                          recent '&' before the first failing name/value pair (or the
                          beginning of the string if the failure is in the first
                          name/value pair) or the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
PlatOverMngrRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_CALLBACK_INFO                         *Private;
  UINT16                                    KeyValue;
  PLAT_OVER_MNGR_DATA                       *FakeNvData;
  EFI_STATUS                                Status;

  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Configuration;

  if (!HiiIsConfigHdrMatch (Configuration, &gPlatformOverridesManagerGuid, mVariableName)) {
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);
  Private    = EFI_CALLBACK_INFO_FROM_THIS (This);
  FakeNvData = &Private->FakeNvData;
  if (!HiiGetBrowserData (&gPlatformOverridesManagerGuid, mVariableName, sizeof (PLAT_OVER_MNGR_DATA), (UINT8 *) FakeNvData)) {
    //
    // FakeNvData can't be got from SetupBrowser, which doesn't need to be set.
    //
    return EFI_SUCCESS;
  }

  Status = EFI_SUCCESS;

  if (mCurrentPage == FORM_ID_ORDER) {
    KeyValue = KEY_VALUE_ORDER_SAVE_AND_EXIT;
    Status = CommitChanges (Private, KeyValue, FakeNvData);
  }

  return Status;
}

/**
  This is the function that is called to provide results data to the driver.  This data
  consists of a unique key which is used to identify what data is either being passed back
  or being asked for.

  @param  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action         A null-terminated Unicode string in <ConfigRequest> format.
  @param  KeyValue       A unique Goto OpCode callback value which record user's selection.
                         0x100 <= KeyValue <0x500 : user select a controller item in the first page;
                         KeyValue == 0x1234       : user select 'Refresh' in first page, or user select 'Go to Previous Menu' in second page
                         KeyValue == 0x1235       : user select 'Pci device filter' in first page
                         KeyValue == 0x1500       : user select 'order ... priority' item in second page
                         KeyValue == 0x1800       : user select 'commint changes' in third page
                         KeyValue == 0x2000       : user select 'Go to Previous Menu' in third page
  @param  Type           The type of value for the question.
  @param  Value          A pointer to the data being sent to the original exporting driver.
  @param  ActionRequest  On return, points to the action requested by the callback function.

  @retval EFI_SUCCESS    Always returned.

**/
EFI_STATUS
EFIAPI
PlatOverMngrCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        KeyValue,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_CALLBACK_INFO                         *Private;
  EFI_STATUS                                Status;
  EFI_STRING_ID                             NewStringToken;
  EFI_INPUT_KEY                             Key;
  PLAT_OVER_MNGR_DATA                       *FakeNvData;

  if ((Action != EFI_BROWSER_ACTION_CHANGING) && (Action != EFI_BROWSER_ACTION_CHANGED)) {
    //
    // All other action return unsupported.
    //
    return EFI_UNSUPPORTED;
  }

  Private = EFI_CALLBACK_INFO_FROM_THIS (This);
  FakeNvData = &Private->FakeNvData;
  if (!HiiGetBrowserData (&gPlatformOverridesManagerGuid, mVariableName, sizeof (PLAT_OVER_MNGR_DATA), (UINT8 *) FakeNvData)) {
    return EFI_NOT_FOUND;
  }

  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    if (Value == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    if (KeyValue == KEY_VALUE_DRIVER_GOTO_PREVIOUS) {
      UpdateDeviceSelectPage (Private, KeyValue, FakeNvData);
      //
      // Update page title string
      //
      NewStringToken = STRING_TOKEN (STR_TITLE);
      if (HiiSetString (Private->RegisteredHandle, NewStringToken, L"First, Select the controller by device path", NULL) == 0) {
        ASSERT (FALSE);
      }
    }

    if (((KeyValue >= KEY_VALUE_DEVICE_OFFSET) && (KeyValue < KEY_VALUE_DEVICE_OFFSET + mMaxDeviceCount)) || (KeyValue == KEY_VALUE_ORDER_GOTO_PREVIOUS)) {
      if (KeyValue == KEY_VALUE_ORDER_GOTO_PREVIOUS) {
        KeyValue = (EFI_QUESTION_ID) (mSelectedCtrIndex + KEY_VALUE_DEVICE_OFFSET);
      }
      UpdateBindingDriverSelectPage (Private, KeyValue, FakeNvData);
      //
      // Update page title string
      //
      NewStringToken = STRING_TOKEN (STR_TITLE);
      if (HiiSetString (Private->RegisteredHandle, NewStringToken, L"Second, Select drivers for the previous selected controller", NULL) == 0) {
        ASSERT (FALSE);
      }
    }

    if (KeyValue == KEY_VALUE_DRIVER_GOTO_ORDER) {
      UpdatePrioritySelectPage (Private, KeyValue, FakeNvData);
      //
      // Update page title string
      //
      NewStringToken = STRING_TOKEN (STR_TITLE);
      if (HiiSetString (Private->RegisteredHandle, NewStringToken, L"Finally, Set the priority order for the drivers and save them", NULL) == 0) {
        ASSERT (FALSE);
      }
    }

    if (KeyValue == KEY_VALUE_DEVICE_CLEAR) {
      //
      // Deletes all environment variable(s) that contain the override mappings info
      //
      FreeMappingDatabase (&mMappingDataBase);
      Status = SaveOverridesMapping (&mMappingDataBase);
      UpdateDeviceSelectPage (Private, KeyValue, FakeNvData);
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if ((KeyValue >= KEY_VALUE_DRIVER_OFFSET) && (KeyValue < KEY_VALUE_DRIVER_OFFSET + mDriverImageHandleCount)) {
      mDriSelection[KeyValue - KEY_VALUE_DRIVER_OFFSET] = Value->b;
    } else {
      switch (KeyValue) {
      case KEY_VALUE_DEVICE_REFRESH:
      case KEY_VALUE_DEVICE_FILTER:
        UpdateDeviceSelectPage (Private, KeyValue, FakeNvData);
        //
        // Update page title string
        //
        NewStringToken = STRING_TOKEN (STR_TITLE);
        if (HiiSetString (Private->RegisteredHandle, NewStringToken, L"First, Select the controller by device path", NULL) == 0) {
          ASSERT (FALSE);
        }
      break;

      case KEY_VALUE_ORDER_SAVE_AND_EXIT:
        Status = CommitChanges (Private, KeyValue, FakeNvData);
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
        if (EFI_ERROR (Status)) {
          CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, L"Single Override Info too large, Saving Error!", NULL);
          return EFI_DEVICE_ERROR;
        }
      break;

      default:
      break;
      }
    }
  }

  //
  // Pass changed uncommitted data back to Form Browser
  //
  HiiSetBrowserData (&gPlatformOverridesManagerGuid, mVariableName, sizeof (PLAT_OVER_MNGR_DATA), (UINT8 *) FakeNvData, NULL);

  return EFI_SUCCESS;
}

/**
  Retrieves the image handle of the platform override driver for a controller in the system.

  @param  This                   A pointer to the
                                 EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL instance.
  @param  ControllerHandle       The device handle of the controller to check if a
                                 driver override exists.
  @param  DriverImageHandle      On input, a pointer to the previous driver image
                                 handle returned by GetDriver().  On output, a
                                 pointer to the next driver image handle. Passing
                                 in a NULL,  will return the first driver image
                                 handle for ControllerHandle.

  @retval EFI_SUCCESS            The driver override for ControllerHandle was
                                 returned in DriverImageHandle.
  @retval EFI_NOT_FOUND          A driver override for ControllerHandle was not
                                 found.
  @retval EFI_INVALID_PARAMETER  The handle specified by ControllerHandle is NULL.
                                 DriverImageHandle is not a handle that was returned
                                 on a previous  call to GetDriver().

**/
EFI_STATUS
EFIAPI
GetDriver (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL              *This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_HANDLE                                     *DriverImageHandle
  )
{
  EFI_STATUS  Status;

  //
  // Check that ControllerHandle is a valid handle
  //
  if (ControllerHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Read the environment variable(s) that contain the override mappings from Controller Device Path to
  // a set of Driver Device Paths, and  initialize in memory database of the overrides that map Controller
  // Device Paths to an ordered set of Driver Device Paths and Driver Handles. This action is only performed
  // once and finished in first call.
  //
  if (!mEnvironmentVariableRead) {
    mEnvironmentVariableRead = TRUE;

    Status = InitOverridesMapping (&mMappingDataBase);
    if (EFI_ERROR (Status)){
      DEBUG ((DEBUG_INFO, "The status to Get Platform Driver Override Variable is %r\n", Status));
      InitializeListHead (&mMappingDataBase);
      return EFI_NOT_FOUND;
    }
  }

  //
  // if the environment variable does not exist, just return not found
  //
  if (IsListEmpty (&mMappingDataBase)) {
    return EFI_NOT_FOUND;
  }

  return GetDriverFromMapping (
            ControllerHandle,
            DriverImageHandle,
            &mMappingDataBase,
            mCallerImageHandle
            );
}

/**
  Retrieves the device path of the platform override driver for a controller in the system.
  This driver doesn't support this API.

  @param  This                  A pointer to the EFI_PLATFORM_DRIVER_OVERRIDE_
                                PROTOCOL instance.
  @param  ControllerHandle      The device handle of the controller to check if a driver override
                                exists.
  @param  DriverImagePath       On input, a pointer to the previous driver device path returned by
                                GetDriverPath(). On output, a pointer to the next driver
                                device path. Passing in a pointer to NULL, will return the first
                                driver device path for ControllerHandle.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
GetDriverPath (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL              *This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                       **DriverImagePath
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Used to associate a driver image handle with a device path that was returned on a prior call to the
  GetDriverPath() service. This driver image handle will then be available through the
  GetDriver() service. This driver doesn't support this API.

  @param  This                  A pointer to the EFI_PLATFORM_DRIVER_OVERRIDE_
                                PROTOCOL instance.
  @param  ControllerHandle      The device handle of the controller.
  @param  DriverImagePath       A pointer to the driver device path that was returned in a prior
                                call to GetDriverPath().
  @param  DriverImageHandle     The driver image handle that was returned by LoadImage()
                                when the driver specified by DriverImagePath was loaded
                                into memory.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
DriverLoaded (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL          *This,
  IN EFI_HANDLE                                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL                       *DriverImagePath,
  IN EFI_HANDLE                                     DriverImageHandle
  )
{
  return EFI_UNSUPPORTED;
}

/**
  The driver Entry Point. The function will export a disk device class formset and
  its callback function to hii database.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
PlatDriOverrideDxeInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                  Status;
  EFI_FORM_BROWSER2_PROTOCOL  *FormBrowser2;
  VOID                        *Instance;

  //
  // There should only be one Form Configuration protocol
  //
  Status = gBS->LocateProtocol (
                 &gEfiFormBrowser2ProtocolGuid,
                 NULL,
                 (VOID **) &FormBrowser2
                 );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // According to UEFI spec, there can be at most a single instance
  // in the system of the EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL.
  // So here we check the existence.
  //
  Status = gBS->LocateProtocol (
                  &gEfiPlatformDriverOverrideProtocolGuid,
                  NULL,
                  &Instance
                  );
  //
  // If there was no error, assume there is an installation and return error
  //
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  mCallerImageHandle = ImageHandle;
  mCallbackInfo = AllocateZeroPool (sizeof (EFI_CALLBACK_INFO));
  if (mCallbackInfo == NULL) {
    return EFI_BAD_BUFFER_SIZE;
  }

  mCallbackInfo->Signature = EFI_CALLBACK_INFO_SIGNATURE;
  mCallbackInfo->ConfigAccess.ExtractConfig = PlatOverMngrExtractConfig;
  mCallbackInfo->ConfigAccess.RouteConfig   = PlatOverMngrRouteConfig;
  mCallbackInfo->ConfigAccess.Callback      = PlatOverMngrCallback;
  mCallbackInfo->PlatformDriverOverride.GetDriver      = GetDriver;
  mCallbackInfo->PlatformDriverOverride.GetDriverPath  = GetDriverPath;
  mCallbackInfo->PlatformDriverOverride.DriverLoaded   = DriverLoaded;

  //
  // Locate ConfigRouting protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **) &mCallbackInfo->HiiConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    goto Finish;
  }

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  // Install Platform Driver Override Protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mCallbackInfo->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mCallbackInfo->ConfigAccess,
                  &gEfiPlatformDriverOverrideProtocolGuid,
                  &mCallbackInfo->PlatformDriverOverride,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto Finish;
  }

  //
  // Publish our HII data
  //
  mCallbackInfo->RegisteredHandle = HiiAddPackages (
                                     &gPlatformOverridesManagerGuid,
                                     mCallbackInfo->DriverHandle,
                                     VfrBin,
                                     PlatDriOverrideDxeStrings,
                                     NULL
                                     );
  if (mCallbackInfo->RegisteredHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }

  //
  // Clear all the globle variable
  //
  mDriverImageHandleCount = 0;
  mCurrentPage = 0;

  return EFI_SUCCESS;

Finish:
  PlatDriOverrideDxeUnload (ImageHandle);

  return Status;
}

/**
  Unload its installed protocol.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
PlatDriOverrideDxeUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  ASSERT (mCallbackInfo != NULL);

  if (mCallbackInfo->DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mCallbackInfo->DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &mCallbackInfo->ConfigAccess,
           &gEfiPlatformDriverOverrideProtocolGuid,
           &mCallbackInfo->PlatformDriverOverride,
           NULL
           );
  }

  if (mCallbackInfo->RegisteredHandle != NULL) {
    HiiRemovePackages (mCallbackInfo->RegisteredHandle);
  }

  FreePool (mCallbackInfo);

  if (mControllerToken != NULL) {
    FreePool (mControllerToken);
  }

  if (mControllerDevicePathProtocol != NULL) {
    FreePool (mControllerDevicePathProtocol);
  }

  if (mDriverImageToken != NULL) {
    FreePool (mDriverImageToken);
  }

  return EFI_SUCCESS;
}
