/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*  
*  This program and the accompanying materials                          
*  are licensed and made available under the terms and conditions of the BSD License         
*  which accompanies this distribution.  The full text of the license may be found at        
*  http://opensource.org/licenses/bsd-license.php                                            
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
*
**/

#include "BdsInternal.h"

EFI_STATUS
BdsLoadPeCoff (
 IN  BDS_FILE     *EfiAppFile
 )
{
    EFI_STATUS                          Status;
    EFI_HANDLE                          ImageHandle;
    MEDIA_FW_VOL_FILEPATH_DEVICE_PATH   NewNode;
    EFI_DEVICE_PATH_PROTOCOL            *DevicePath;

    // Only support loading from FV right now
    ASSERT(EfiAppFile->Type == BDS_FILETYPE_FV);

    // Generate the Device Path for the file
    DevicePath = DuplicateDevicePath(EfiAppFile->DevicePath);
    EfiInitializeFwVolDevicepathNode (&NewNode, &(EfiAppFile->File.Fv.Guid));
    DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&NewNode);

    Status = gBS->LoadImage (TRUE, gImageHandle, DevicePath, NULL, 0, &ImageHandle);
    if (!EFI_ERROR (Status)) {
        //
        // Before calling the image, enable the Watchdog Timer for
        // the 5 Minute period
        //
        gBS->SetWatchdogTimer (5 * 60, 0x0000, 0x00, NULL);
        Status = gBS->StartImage (ImageHandle, NULL, NULL);
        //
        // Clear the Watchdog Timer after the image returns
        //
        gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);
    }
  
    return Status;
}

EFI_STATUS BdsLoadApplicationFromPath(
    IN  CHAR16* EfiAppPath
) {
    EFI_STATUS Status;
    BDS_FILE   EfiAppFile;

    // Need to connect every drivers to ensure no dependencies are missing for the application
    Status = BdsConnectAllDrivers();
    if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR, "FAIL to connect all drivers\n"));
        return Status;
    }

    // Locate the application from a device path
    Status = BdsLoadFilePath(EfiAppPath, &EfiAppFile);
    if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR, "ERROR: Do not find EFI application %s\n",EfiAppPath));
        return Status;
    }

    // Start the application
    Status = BdsLoadPeCoff(&EfiAppFile);

    return Status;
}

EFI_STATUS BdsLoadApplication(
    IN  CHAR16* EfiApp
) {
    EFI_STATUS                      Status;
    UINTN                           NoHandles, HandleIndex;
    EFI_HANDLE                      *Handles;
    BDS_FILE                        EfiAppFile;

    // Need to connect every drivers to ensure no dependencies are missing for the application
    Status = BdsConnectAllDrivers();
    if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR, "FAIL to connect all drivers\n"));
        return Status;
    }

    // Search the application in any Firmware Volume
    Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiFirmwareVolume2ProtocolGuid, NULL, &NoHandles, &Handles);
    if (EFI_ERROR (Status) || (NoHandles == 0)) {
        DEBUG ((EFI_D_ERROR, "FAIL to find Firmware Volume\n"));
        return Status;
    }

    // Search in all Firmware Volume for the EFI Application
    for (HandleIndex = 0; HandleIndex < NoHandles; HandleIndex++) {
        Status = BdsLoadFileFromFirmwareVolume(Handles[HandleIndex],EfiApp,EFI_FV_FILETYPE_APPLICATION,&EfiAppFile);
        if (!EFI_ERROR (Status)) {
            // Start the application
            Status = BdsLoadPeCoff(&EfiAppFile);
            return Status;
        }
    }

    return Status;
}
