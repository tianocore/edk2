/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  EdkDxe.h

Abstract:
  This file defines the base package surface area for writting a PEIM
  
  Things defined in the Tiano specification go in DxeCis.h. 

  Dxe.h contains build environment and library information needed to build
  a basic Dxe driver. This file must match the "base package" definition of
  how to write a Dxe driver.

--*/

#ifndef __EDK_DXE_H__
#define __EDK_DXE_H__

#include <Common/FlashMap.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/FlashMapHob.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/AlternateFvBlock.h>
#include <Guid/ConsoleInDevice.h>
#include <Guid/ConsoleOutDevice.h>
#include <Guid/StandardErrorDevice.h>
#include <Guid/HotPlugDevice.h>
#include <Guid/PrimaryStandardErrorDevice.h>
#include <Guid/PrimaryConsoleInDevice.h>
#include <Guid/PrimaryConsoleOutDevice.h>
#include <Guid/Bmp.h>
#include <Guid/BootState.h>
#include <Guid/ShellFile.h>
#include <Guid/MiniShellFile.h>
#include <Guid/StatusCode.h>
#include <Guid/PciOptionRomTable.h>
#include <Guid/PciHotplugDevice.h>
#if defined(MDE_CPU_IPF)
#include <Guid/ExtendedSalGuid.h>
#endif
#include <Guid/PeiPeCoffLoader.h>
#include <Guid/CapsuleVendor.h>
#include <Guid/CompatibleMemoryTested.h>
#include <Guid/MemoryStatusCodeRecord.h>
#include <Guid/GenericPlatformVariable.h>

#include <Ppi/StatusCodeMemory.h>

#include <Protocol/CustomizedDecompress.h>
#include <Protocol/DebugLevel.h>
#include <Protocol/LoadPe32Image.h>
#include <Protocol/EdkDecompress.h>
#include <Protocol/Print.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/FvbExtension.h>
#include <Protocol/FaultTolerantWriteLite.h>
#include <Protocol/ConsoleControl.h>
#include <Protocol/OEMBadging.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/UgaSplash.h>
#include <Protocol/AcpiS3Save.h>
#include <Protocol/Performance.h>
#include <Protocol/PxeDhcp4.h>
#include <Protocol/PxeDhcp4CallBack.h>
#include <Protocol/UgaIo.h>
#include <Protocol/DebugAssert.h>
#include <Protocol/usbatapi.h>
#include <Protocol/PciHotPlugRequest.h>
#if defined(MDE_CPU_IPF)
#include <Protocol/ExtendedSalBootService.h>
#endif
#include <Protocol/IsaAcpi.h>
#include <Protocol/IsaIo.h>

#if ((EDK_RELEASE_VERSION != 0) && (EFI_SPECIFICATION_VERSION < 0x00020000))
//
// Old EDK modules use Module use ScsiPassThru protocol together with the original ScsiIo protocol 
// In UEFI2.0, Module use ScsiPassThruExt Protocol with new UEFI2.0 ScsiIo protocol
//
#include <Protocol/ScsiIo.h>
#endif

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#include <Protocol/Capsule.h>
#endif

#endif
