/** @file
  BDS library definition, include the file and data structure

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _INTERNAL_BDS_LIB_H_
#define _INTERNAL_BDS_LIB_H_

#include <PiDxe.h>

#include <IndustryStandard/Pci22.h>

#include <Protocol/BlockIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/Cpu.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DebugPort.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/PciIo.h>
#include <Protocol/AcpiS3Save.h>
#include <Protocol/OEMBadging.h>
#include <Protocol/ConsoleControl.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/HiiFont.h>
#include <Protocol/HiiImage.h>

#include <Guid/MemoryTypeInformation.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/PcAnsi.h>
#include <Guid/ShellFile.h>
#include <Guid/Performance.h>

#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PcdLib.h>
#include <Library/IfrSupportLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/GenericBdsLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>
#include <Library/DxeServicesLib.h>

#if !defined (EFI_REMOVABLE_MEDIA_FILE_NAME)
    #if defined (MDE_CPU_EBC)
        //
        // Uefi specification only defines the default boot file name for IA32, X64
        // and IPF processor, so need define boot file name for EBC architecture here.
        //
        #define EFI_REMOVABLE_MEDIA_FILE_NAME L"\\EFI\\BOOT\\BOOTEBC.EFI"
    #else
        #error "Can not determine the default boot file name for unknown processor type!"
    #endif
#endif

/**

  Allocates a block of memory and writes performance data of booting into it.
  OS can processing these record.
  
**/
VOID
WriteBootToOsPerformanceData (
  VOID
  );

#endif // _BDS_LIB_H_
