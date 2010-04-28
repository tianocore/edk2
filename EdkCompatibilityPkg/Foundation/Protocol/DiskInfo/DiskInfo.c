/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DiskInfo.c

Abstract:

  
--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (DiskInfo)

EFI_GUID gEfiDiskInfoProtocolGuid = EFI_DISK_INFO_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiDiskInfoProtocolGuid, "Disk Info Protocol", "Disk Info Protocol");

EFI_GUID gEfiDiskInfoAhciInterfaceGuid = EFI_DISK_INFO_AHCI_INTERFACE_GUID;
EFI_GUID gEfiDiskInfoIdeInterfaceGuid = EFI_DISK_INFO_IDE_INTERFACE_GUID;
EFI_GUID gEfiDiskInfoScsiInterfaceGuid = EFI_DISK_INFO_SCSI_INTERFACE_GUID;
EFI_GUID gEfiDiskInfoUsbInterfaceGuid = EFI_DISK_INFO_USB_INTERFACE_GUID;
