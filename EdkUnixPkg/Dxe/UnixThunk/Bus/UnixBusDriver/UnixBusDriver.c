/*+++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixBusDriver.c

Abstract:

This following section documents the envirnoment variables for the Win UNIX 
build.  These variables are used to define the (virtual) hardware 
configuration of the UNIX environment

A ! can be used to seperate multiple instances in a variable. Each 
instance represents a seperate hardware device. 

EFI_UNIX_PHYSICAL_DISKS - maps to drives on your system
EFI_UNIX_VIRTUAL_DISKS  - maps to a device emulated by a file
EFI_UNIX_FILE_SYSTEM    - mouts a directory as a file system
EFI_UNIX_CONSOLE        - make a logical comand line window (only one!)
EFI_UNIX_UGA            - Builds UGA Windows of Width and Height

 <F>ixed       - Fixed disk like a hard drive.
 <R>emovable   - Removable media like a floppy or CD-ROM.
 Read <O>nly   - Write protected device.
 Read <W>rite  - Read write device.
 <block count> - Decimal number of blocks a device supports.
 <block size>  - Decimal number of bytes per block.

 UNIX envirnonment variable contents. '<' and '>' are not part of the variable, 
 they are just used to make this help more readable. There should be no 
 spaces between the ';'. Extra spaces will break the variable. A '!' is  
 used to seperate multiple devices in a variable.

 EFI_UNIX_VIRTUAL_DISKS = 
   <F | R><O | W>;<block count>;<block size>[!...]

 EFI_UNIX_PHYSICAL_DISKS =
   <drive letter>:<F | R><O | W>;<block count>;<block size>[!...]

 Virtual Disks: These devices use a file to emulate a hard disk or removable
                media device. 
                
   Thus a 20 MB emulated hard drive would look like:
   EFI_UNIX_VIRTUAL_DISKS=FW;40960;512

   A 1.44MB emulated floppy with a block size of 1024 would look like:
   EFI_UNIX_VIRTUAL_DISKS=RW;1440;1024

 Physical Disks: These devices use UNIX to open a real device in your system

   Thus a 120 MB floppy would look like:
   EFI_UNIX_PHYSICAL_DISKS=B:RW;245760;512

   Thus a standard CD-ROM floppy would look like:
   EFI_UNIX_PHYSICAL_DISKS=Z:RO;307200;2048

 EFI_UNIX_FILE_SYSTEM = 
   <directory path>[!...]

   Mounting the two directories C:\FOO and C:\BAR would look like:
   EFI_UNIX_FILE_SYSTEM=c:\foo!c:\bar

 EFI_UNIX_CONSOLE = 
   <window title>

   Declaring a text console window with the title "My EFI Console" woild look like:
   EFI_UNIX_CONSOLE=My EFI Console

 EFI_UNIX_UGA = 
   <width> <height>[!...]

   Declaring a two UGA windows with resolutions of 800x600 and 1024x768 would look like:
   Example : EFI_UNIX_UGA=800 600!1024 768

 EFI_UNIX_PASS_THROUGH =
   <BaseAddress>;<Bus#>;<Device#>;<Function#>

   Declaring a base address of 0xE0000000 (used for PCI Express devices)
   and having NT32 talk to a device located at bus 0, device 1, function 0:
   Example : EFI_UNIX_PASS_THROUGH=E000000;0;1;0

---*/

#include "UnixBusDriver.h"
//#include "PciHostBridge.h"

//
// Define GUID for the Unix Bus Driver
//
static EFI_GUID gUnixBusDriverGuid = {
  0x419f582, 0x625, 0x4531, {0x8a, 0x33, 0x85, 0xa9, 0x96, 0x5c, 0x95, 0xbc}
};

//
// DriverBinding protocol global
//
EFI_DRIVER_BINDING_PROTOCOL           gUnixBusDriverBinding = {
  UnixBusDriverBindingSupported,
  UnixBusDriverBindingStart,
  UnixBusDriverBindingStop,
  0xa,
  NULL,
  NULL
};

#define UNIX_PCD_ARRAY_SIZE (sizeof(mPcdEnvironment)/sizeof(UNIX_PCD_ENTRY))

//
// Table to map UNIX Environment variable to the GUID that should be in
// device path.
//
static UNIX_PCD_ENTRY  mPcdEnvironment[] = {
  {PcdToken(PcdUnixConsole),       &gEfiUnixConsoleGuid},
  {PcdToken(PcdUnixUga),           &gEfiUnixUgaGuid},
  {PcdToken(PcdUnixFileSystem),    &gEfiUnixFileSystemGuid},
  {PcdToken(PcdUnixSerialPort),    &gEfiUnixSerialPortGuid},
  {PcdToken(PcdUnixVirtualDisk),   &gEfiUnixVirtualDisksGuid},
  {PcdToken(PcdUnixPhysicalDisk),  &gEfiUnixPhysicalDisksGuid},
  {PcdToken(PcdUnixCpuModel),      &gEfiUnixCPUModelGuid},
  {PcdToken(PcdUnixCpuSpeed),      &gEfiUnixCPUSpeedGuid},
  {PcdToken(PcdUnixMemorySize),    &gEfiUnixMemoryGuid}
};

VOID *
AllocateMemory (
  IN  UINTN   Size
  )
{
  EFI_STATUS  Status;
  VOID        *Buffer;

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  Size,
                  (VOID *)&Buffer
                  );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return NULL;
  }
  return Buffer;
}


EFI_STATUS
EFIAPI
UnixBusDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    ControllerHandle - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_UNIX_THUNK_PROTOCOL *UnixThunk;
  UINTN                     Index;

  //
  // Check the contents of the first Device Path Node of RemainingDevicePath to make sure
  // it is a legal Device Path Node for this bus driver's children.
  //
  if (RemainingDevicePath != NULL) {
    if (RemainingDevicePath->Type != HARDWARE_DEVICE_PATH ||
        RemainingDevicePath->SubType != HW_VENDOR_DP ||
        DevicePathNodeLength(RemainingDevicePath) != sizeof(UNIX_VENDOR_DEVICE_PATH_NODE)) {
      return EFI_UNSUPPORTED;
    }

    for (Index = 0; Index < UNIX_PCD_ARRAY_SIZE; Index++) {
      if (CompareGuid (&((VENDOR_DEVICE_PATH *) RemainingDevicePath)->Guid, mPcdEnvironment[Index].DevicePathGuid)) {
        break;
      }
    }

    if (Index >= UNIX_PCD_ARRAY_SIZE) {
      return EFI_UNSUPPORTED;
    }
  }
  
  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUnixThunkProtocolGuid,
                  (VOID **)&UnixThunk,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Since we call through UnixThunk we need to make sure it's valid
  //
  Status = EFI_SUCCESS;
  if (UnixThunk->Signature != EFI_UNIX_THUNK_PROTOCOL_SIGNATURE) {
    Status = EFI_UNSUPPORTED;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiUnixThunkProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );

  return Status;
}

EFI_STATUS
EFIAPI
UnixBusDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    ControllerHandle - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS                      Status;
  EFI_STATUS                      InstallStatus;
  EFI_UNIX_THUNK_PROTOCOL         *UnixThunk;
  EFI_DEVICE_PATH_PROTOCOL        *ParentDevicePath;
  UNIX_BUS_DEVICE                 *UnixBusDevice;
  UNIX_IO_DEVICE                  *UnixDevice;
  UINTN                           Index;
  CHAR16                          *StartString;
  CHAR16                          *SubString;
  UINT16                          Count;
  UINTN                           StringSize;
  UINT16                          ComponentName[MAX_UNIX_ENVIRNMENT_VARIABLE_LENGTH];
  UNIX_VENDOR_DEVICE_PATH_NODE  *Node;
  BOOLEAN                         CreateDevice;
  CHAR16                          *TempStr;
  CHAR16                          *PcdTempStr;
  UINTN                           TempStrSize;

  Status = EFI_UNSUPPORTED;

  //
  // Grab the protocols we need
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUnixThunkProtocolGuid,
                  (VOID **)&UnixThunk,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  if (Status != EFI_ALREADY_STARTED) {
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    sizeof (UNIX_BUS_DEVICE),
                    (VOID *) &UnixBusDevice
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    UnixBusDevice->Signature           = UNIX_BUS_DEVICE_SIGNATURE;
    UnixBusDevice->ControllerNameTable = NULL;

    AddUnicodeString (
      "eng",
      gUnixBusDriverComponentName.SupportedLanguages,
      &UnixBusDevice->ControllerNameTable,
      L"Unix Bus Controller"
      );

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &ControllerHandle,
                    &gUnixBusDriverGuid,
                    UnixBusDevice,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      FreeUnicodeStringTable (UnixBusDevice->ControllerNameTable);
      gBS->FreePool (UnixBusDevice);
      return Status;
    }
  }

  //
  // Loop on the Variable list. Parse each variable to produce a set of handles that
  // represent virtual hardware devices.
  //
  InstallStatus   = EFI_NOT_FOUND;
  for (Index = 0; Index < UNIX_PCD_ARRAY_SIZE; Index++) {
    PcdTempStr = (VOID *)LibPcdGetPtr (mPcdEnvironment[Index].Token);
    ASSERT (PcdTempStr != NULL);

    TempStrSize = StrLen (PcdTempStr);
    TempStr = AllocateMemory ((TempStrSize * sizeof (CHAR16)) + 1);
    StrCpy (TempStr, PcdTempStr);

    StartString = TempStr;

    //
    // Parse the envirnment variable into sub strings using '!' as a delimator.
    // Each substring needs it's own handle to be added to the system. This code
    // does not understand the sub string. Thats the device drivers job.
    //
    Count = 0;
    while (*StartString != '\0') {

      //
      // Find the end of the sub string
      //
      SubString = StartString;
      while (*SubString != '\0' && *SubString != '!') {
        SubString++;
      }

      if (*SubString == '!') {
        //
        // Replace token with '\0' to make sub strings. If this is the end
        //  of the string SubString will already point to NULL.
        //
        *SubString = '\0';
        SubString++;
      }

      CreateDevice = TRUE;
      if (RemainingDevicePath != NULL) {
        CreateDevice  = FALSE;
        Node          = (UNIX_VENDOR_DEVICE_PATH_NODE *) RemainingDevicePath;
        if (Node->VendorDevicePath.Header.Type == HARDWARE_DEVICE_PATH &&
            Node->VendorDevicePath.Header.SubType == HW_VENDOR_DP &&
            DevicePathNodeLength (&Node->VendorDevicePath.Header) == sizeof (UNIX_VENDOR_DEVICE_PATH_NODE)
            ) {
          if (CompareGuid (&Node->VendorDevicePath.Guid, mPcdEnvironment[Index].DevicePathGuid) &&
              Node->Instance == Count
              ) {
            CreateDevice = TRUE;
          }
        }
      }

      if (CreateDevice) {
        //
        // Allocate instance structure, and fill in parent information.
        //
        UnixDevice = AllocateMemory (sizeof (UNIX_IO_DEVICE));
        if (UnixDevice == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        UnixDevice->Handle             = NULL;
        UnixDevice->ControllerHandle   = ControllerHandle;
        UnixDevice->ParentDevicePath   = ParentDevicePath;

        UnixDevice->UnixIo.UnixThunk   = UnixThunk;

        //
        // Plus 2 to account for the NULL at the end of the Unicode string
        //
        StringSize = (UINTN) ((UINT8 *) SubString - (UINT8 *) StartString) + sizeof (CHAR16);
        UnixDevice->UnixIo.EnvString = AllocateMemory (StringSize);
        if (UnixDevice->UnixIo.EnvString != NULL) {
          CopyMem (UnixDevice->UnixIo.EnvString, StartString, StringSize);
        }

        UnixDevice->ControllerNameTable = NULL;

	//  FIXME: check size
        StrCpy(ComponentName, UnixDevice->UnixIo.EnvString);

        UnixDevice->DevicePath = UnixBusCreateDevicePath (
                                    ParentDevicePath,
                                    mPcdEnvironment[Index].DevicePathGuid,
                                    Count
                                    );
        if (UnixDevice->DevicePath == NULL) {
          gBS->FreePool (UnixDevice);
          return EFI_OUT_OF_RESOURCES;
        }

        AddUnicodeString (
          "eng",
          gUnixBusDriverComponentName.SupportedLanguages,
          &UnixDevice->ControllerNameTable,
          ComponentName
          );

        UnixDevice->UnixIo.TypeGuid       = mPcdEnvironment[Index].DevicePathGuid;
        UnixDevice->UnixIo.InstanceNumber = Count;

        UnixDevice->Signature              = UNIX_IO_DEVICE_SIGNATURE;

        Status = gBS->InstallMultipleProtocolInterfaces (
                        &UnixDevice->Handle,
                        &gEfiDevicePathProtocolGuid,
                        UnixDevice->DevicePath,
                        &gEfiUnixIoProtocolGuid,
                        &UnixDevice->UnixIo,
                        NULL
                        );
        if (EFI_ERROR (Status)) {
          FreeUnicodeStringTable (UnixDevice->ControllerNameTable);
          gBS->FreePool (UnixDevice);
        } else {
          //
          // Open For Child Device
          //
          Status = gBS->OpenProtocol (
                          ControllerHandle,
                          &gEfiUnixThunkProtocolGuid,
                          (VOID **)&UnixThunk,
                          This->DriverBindingHandle,
                          UnixDevice->Handle,
                          EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                          );
          if (!EFI_ERROR (Status)) {
            InstallStatus = EFI_SUCCESS;
          }
        }
      }

      //
      // Parse Next sub string. This will point to '\0' if we are at the end.
      //
      Count++;
      StartString = SubString;
    }

    gBS->FreePool (TempStr);
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
UnixBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
/*++

Routine Description:

Arguments:

Returns:

    None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    ControllerHandle - add argument and description to function comment
// TODO:    NumberOfChildren - add argument and description to function comment
// TODO:    ChildHandleBuffer - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS                Status;
  UINTN                     Index;
  BOOLEAN                   AllChildrenStopped;
  EFI_UNIX_IO_PROTOCOL    *UnixIo;
  UNIX_BUS_DEVICE         *UnixBusDevice;
  UNIX_IO_DEVICE          *UnixDevice;
  EFI_UNIX_THUNK_PROTOCOL *UnixThunk;

  //
  // Complete all outstanding transactions to Controller.
  // Don't allow any new transaction to Controller to be started.
  //

  if (NumberOfChildren == 0) {
    //
    // Close the bus driver
    //
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gUnixBusDriverGuid,
                    (VOID **)&UnixBusDevice,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    gBS->UninstallMultipleProtocolInterfaces (
          ControllerHandle,
          &gUnixBusDriverGuid,
          UnixBusDevice,
          NULL
          );

    FreeUnicodeStringTable (UnixBusDevice->ControllerNameTable);

    gBS->FreePool (UnixBusDevice);

    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiUnixThunkProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );

    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {

    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiUnixIoProtocolGuid,
                    (VOID **)&UnixIo,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {

      UnixDevice = UNIX_IO_DEVICE_FROM_THIS (UnixIo);

      Status = gBS->CloseProtocol (
                      ControllerHandle,
                      &gEfiUnixThunkProtocolGuid,
                      This->DriverBindingHandle,
                      UnixDevice->Handle
                      );

      Status = gBS->UninstallMultipleProtocolInterfaces (
                      UnixDevice->Handle,
                      &gEfiDevicePathProtocolGuid,
                      UnixDevice->DevicePath,
                      &gEfiUnixIoProtocolGuid,
                      &UnixDevice->UnixIo,
                      NULL
                      );

      if (EFI_ERROR (Status)) {
        gBS->OpenProtocol (
              ControllerHandle,
              &gEfiUnixThunkProtocolGuid,
              (VOID **) &UnixThunk,
              This->DriverBindingHandle,
              UnixDevice->Handle,
              EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
              );
      } else {
        //
        // Close the child handle
        //
        FreeUnicodeStringTable (UnixDevice->ControllerNameTable);
        gBS->FreePool (UnixDevice);
      }
    }

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_DEVICE_PATH_PROTOCOL *
UnixBusCreateDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *RootDevicePath,
  IN  EFI_GUID                  *Guid,
  IN  UINT16                    InstanceNumber
  )
/*++

Routine Description:
  Create a device path node using Guid and InstanceNumber and append it to
  the passed in RootDevicePath

Arguments:
  RootDevicePath - Root of the device path to return.

  Guid           - GUID to use in vendor device path node.

  InstanceNumber - Instance number to use in the vendor device path. This
                    argument is needed to make sure each device path is unique.

Returns:

  EFI_DEVICE_PATH_PROTOCOL 

--*/
{
  UNIX_VENDOR_DEVICE_PATH_NODE  DevicePath;

  DevicePath.VendorDevicePath.Header.Type     = HARDWARE_DEVICE_PATH;
  DevicePath.VendorDevicePath.Header.SubType  = HW_VENDOR_DP;
  SetDevicePathNodeLength (&DevicePath.VendorDevicePath.Header, sizeof (UNIX_VENDOR_DEVICE_PATH_NODE));

  //
  // The GUID defines the Class
  //
  CopyMem (&DevicePath.VendorDevicePath.Guid, Guid, sizeof (EFI_GUID));

  //
  // Add an instance number so we can make sure there are no Device Path
  // duplication.
  //
  DevicePath.Instance = InstanceNumber;

  return AppendDevicePathNode (
          RootDevicePath,
          (EFI_DEVICE_PATH_PROTOCOL *) &DevicePath
          );
}
