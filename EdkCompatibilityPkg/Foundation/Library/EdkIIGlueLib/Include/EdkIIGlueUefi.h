/*++

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGlueUefi.h
  
Abstract: 

  Root include file for UEFI modules.

**/


#ifndef __EDKII_GLUE_UEFI_H__
#define __EDKII_GLUE_UEFI_H__


//
// Check to make sure EFI_SPECIFICATION_VERSION and TIANO_RELEASE_VERSION are defined.
//  also check for legal combinations
//
#if !defined(EFI_SPECIFICATION_VERSION)
  #error EFI_SPECIFICATION_VERSION not defined
#elif !defined(TIANO_RELEASE_VERSION)
  #error TIANO_RELEASE_VERSION not defined
#elif TIANO_RELEASE_VERSION == 0x00000000

//
// UEFI mode with no Tiano extensions is legal
//
#elif (TIANO_RELEASE_VERSION <= 0x00080005) && (EFI_SPECIFICATION_VERSION >= 0x00020000)
  #error Illegal combination of EFI_SPECIFICATION_VERSION and EDK_RELEASE_VERSION versions
#endif

//
// General Type & API definitions
//

#include "EfiSpec.h"
#include "EfiPxe.h"


//
// Protocols from EFI 1.10 that got thier names fixed in UEFI 2.0
//
#include EFI_PROTOCOL_DEFINITION(LoadedImage)
#include EFI_PROTOCOL_DEFINITION(SimpleTextIn)
#include EFI_PROTOCOL_DEFINITION(SimpleTextOut)
#include EFI_PROTOCOL_DEFINITION(SerialIo)
#include EFI_PROTOCOL_DEFINITION(LoadFile)
#include EFI_PROTOCOL_DEFINITION(SimpleFileSystem)
#include EFI_PROTOCOL_DEFINITION(DiskIo)
#include EFI_PROTOCOL_DEFINITION(BlockIo)
#include EFI_PROTOCOL_DEFINITION(UnicodeCollation)
#include EFI_PROTOCOL_DEFINITION(SimpleNetwork)
#include EFI_PROTOCOL_DEFINITION(EfiNetworkInterfaceIdentifier)
#include EFI_PROTOCOL_DEFINITION(PxeBaseCode)
#include EFI_PROTOCOL_DEFINITION(PxeBaseCodeCallBack)

//
// EFI 1.10 Protocols
//
#include EFI_PROTOCOL_DEFINITION(Bis)
#include EFI_PROTOCOL_DEFINITION(BusSpecificDriverOverride)
#include EFI_PROTOCOL_DEFINITION(ComponentName)
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#include EFI_PROTOCOL_DEFINITION (ComponentName2)
#endif
#include EFI_PROTOCOL_DEFINITION(DebugPort)
#include EFI_PROTOCOL_DEFINITION(DebugSupport)
#include EFI_PROTOCOL_DEFINITION(Decompress)
#include EFI_PROTOCOL_DEFINITION(DriverBinding)
#include EFI_PROTOCOL_DEFINITION(DriverConfiguration)
#include EFI_PROTOCOL_DEFINITION(DriverDiagnostics)
#include EFI_PROTOCOL_DEFINITION(Ebc)
#include EFI_PROTOCOL_DEFINITION(EfiNetworkInterfaceIdentifier)
#include EFI_PROTOCOL_DEFINITION(FileInfo)
#include EFI_PROTOCOL_DEFINITION(FileSystemInfo)
#include EFI_PROTOCOL_DEFINITION(FileSystemVolumeLabelInfo)
#include EFI_PROTOCOL_DEFINITION(PciIo)
#include EFI_PROTOCOL_DEFINITION(PciRootBridgeIo)
#include EFI_PROTOCOL_DEFINITION(PlatformDriverOverride)
#include EFI_PROTOCOL_DEFINITION(SimplePointer)
#include EFI_PROTOCOL_DEFINITION(ScsiPassThru)
#include EFI_PROTOCOL_DEFINITION(UsbIo)
#include EFI_PROTOCOL_DEFINITION(UsbHostController)
#include EFI_PROTOCOL_DEFINITION(UgaDraw)

//
// EFI 1.10 GUIDs
//
#include EFI_GUID_DEFINITION(Acpi)
#include EFI_GUID_DEFINITION(DebugImageInfoTable)
#include EFI_GUID_DEFINITION(GlobalVariable)
#include EFI_GUID_DEFINITION(Gpt)
#include EFI_GUID_DEFINITION(PcAnsi)
#include EFI_GUID_DEFINITION(SmBios)
#include EFI_GUID_DEFINITION(SalSystemTable)


#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
//
// UEFI 2.0 Protocols and GUIDs
//
// check here: currently not implementated
//#include EFI_PROTOCOL_DEFINITION(AuthenticationInfo)
#include EFI_PROTOCOL_DEFINITION(DevicePathUtilities)
#include EFI_PROTOCOL_DEFINITION(DevicePathToText)
#include EFI_PROTOCOL_DEFINITION(DevicePathFromText)
#include EFI_PROTOCOL_DEFINITION(GraphicsOutput)
#include EFI_PROTOCOL_DEFINITION(EdidDiscovered)
#include EFI_PROTOCOL_DEFINITION(EdidActive)
#include EFI_PROTOCOL_DEFINITION(EdidOverride)
#include EFI_PROTOCOL_DEFINITION(ScsiIo)
#include EFI_PROTOCOL_DEFINITION(ScsiPassThruExt)
#include EFI_PROTOCOL_DEFINITION(IScsiInitiatorName)
#include EFI_PROTOCOL_DEFINITION(UsbHostController)
#include EFI_PROTOCOL_DEFINITION(TapeIo)
#include EFI_PROTOCOL_DEFINITION(ManagedNetwork)
#include EFI_PROTOCOL_DEFINITION(Arp)
#include EFI_PROTOCOL_DEFINITION(Dhcp4)
#include EFI_PROTOCOL_DEFINITION(Ip4)
#include EFI_PROTOCOL_DEFINITION(Ip4Config)
#include EFI_PROTOCOL_DEFINITION(Tcp4)
#include EFI_PROTOCOL_DEFINITION(Udp4)
#include EFI_PROTOCOL_DEFINITION(Mtftp4)
#include EFI_PROTOCOL_DEFINITION(ServiceBinding)
#include EFI_PROTOCOL_DEFINITION(Hash)
#include EFI_GUID_DEFINITION(EventGroup)
//#include <Guid/WinCertificateUefi.h>
#endif

#if (TIANO_RELEASE_VERSION > 0x00080005) 
//
// Need due to EDK Tiano contamination of UEFI enumes. 
// There is a UEFI library that does things the new way and the old way
// This is why these definitions are need in Uefi.h
//
#include EFI_GUID_DEFINITION (EventLegacyBios)
#include EFI_GUID_DEFINITION (FrameworkDevicePath)
#endif

//
// EDK Library headers used by EdkII Glue Libraries
//
#include "TianoSpecTypes.h"
#include "TianoSpecApi.h"
#include "TianoSpecDevicePath.h"
#include "EfiDriverLib.h"

#include "Common/EdkIIGlueDefinitionChangesBase.h"

//
// EdkII Glue Library Class headers
//

#include "EdkIIGlueBase.h"
#include "Library/EdkIIGlueUefiDecompressLib.h"
#include "Library/EdkIIGlueDevicePathLib.h"
#include "Library/EdkIIGlueUefiBootServicesTableLib.h"
#include "Library/EdkIIGlueUefiDriverEntryPoint.h"
#include "Library/EdkIIGlueUefiDriverModelLib.h"
#include "Library/EdkIIGlueUefiLib.h"
#include "Library/EdkIIGlueUefiRuntimeServicesTableLib.h"

extern UINT8 _gEdkIIGlueDriverModelProtocolSelection;

#endif
