/*++

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGlueDxe.h
  
Abstract: 

  Root include file for DXE modules

--*/

#ifndef __EDKII_GLUE_DXE_H__
#define __EDKII_GLUE_DXE_H__


//
// General Type & API definitions
//

#include "Tiano.h"
#include "BootMode.h"
#include "EfiBootScript.h"
#include "EfiCapsule.h"
#include "EfiDependency.h"
#include "EfiImageFormat.h"
#include "EfiImage.h"
#include "EfiPeOptionalHeader.h"
#include "EfiFirmwareVolumeHeader.h"
#include "EfiFirmwareFileSystem.h"
#include "PeiHob.h"
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#include "TianoHii.h"
#else
#include "EfiInternalFormRepresentation.h"
#endif
#include "EfiStatusCode.h"
#include "EfiPerf.h"

//
// IPF only
//
#ifdef MDE_CPU_IPF
#include "SalApi.h"
#endif

//
// GUID definitions
//

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#include EFI_GUID_DEFINITION (EventGroup)
#include EFI_GUID_DEFINITION (EventLegacyBios)
#include EFI_GUID_DEFINITION (FrameworkDevicePath)

#include EFI_PROTOCOL_DEFINITION (EdidActive)
#include EFI_PROTOCOL_DEFINITION (EdidDiscovered)
#include EFI_PROTOCOL_DEFINITION (EdidOverride)  
#include EFI_PROTOCOL_DEFINITION (GraphicsOutput)  
#include EFI_PROTOCOL_DEFINITION (Hash)  
#include EFI_PROTOCOL_DEFINITION (ScsiPassThruExt)  
#include EFI_PROTOCOL_DEFINITION (TapeIo)
#endif

#include EFI_GUID_DEFINITION (Acpi)
#include EFI_GUID_DEFINITION (AcpiTableStorage)
#include EFI_GUID_DEFINITION (Apriori)
#include EFI_GUID_DEFINITION (Capsule)
#include EFI_GUID_DEFINITION (DataHubRecords)
#include EFI_GUID_DEFINITION (DebugImageInfoTable)
#include EFI_GUID_DEFINITION (DxeServices)
#include EFI_GUID_DEFINITION (FirmwareFileSystem)
#include EFI_GUID_DEFINITION (GlobalVariable)
#include EFI_GUID_DEFINITION (Gpt)
#include EFI_GUID_DEFINITION (Hob)
#include EFI_GUID_DEFINITION (MemoryAllocationHob)
#include EFI_GUID_DEFINITION (Mps)
#include EFI_GUID_DEFINITION (PcAnsi)
#include EFI_GUID_DEFINITION (SalSystemTable)
#include EFI_GUID_DEFINITION (SmBios)
//#include EFI_GUID_DEFINITION (SmmCommunicate)
#include EFI_GUID_DEFINITION (SmramMemoryReserve)
//
// *** NOTE ***: StatusCodeDataTypeId definition differences need to be 
// resolved when porting a module to real EDK II
//
#include EFI_GUID_DEFINITION (StatusCodeDataTypeId)
#include EFI_GUID_DEFINITION (PeiPerformanceHob)

//
// Protocol definitions
//
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#include EFI_PROTOCOL_DEFINITION (FormBrowser2)
#include EFI_PROTOCOL_DEFINITION (HiiConfigAccess)
#include EFI_PROTOCOL_DEFINITION (HiiConfigRouting)
#include EFI_PROTOCOL_DEFINITION (HiiDatabase)
#include EFI_PROTOCOL_DEFINITION (HiiFont)
#include EFI_PROTOCOL_DEFINITION (HiiImage)
#include EFI_PROTOCOL_DEFINITION (HiiString)
#endif

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#include EFI_PROTOCOL_DEFINITION (DevicePathFromText)
#include EFI_PROTOCOL_DEFINITION (DevicePathToText)
#include EFI_PROTOCOL_DEFINITION (DevicePathUtilities)
#include EFI_PROTOCOL_DEFINITION (Dhcp4)
#include EFI_PROTOCOL_DEFINITION (Ip4)
#include EFI_PROTOCOL_DEFINITION (Ip4Config)
#include EFI_PROTOCOL_DEFINITION (IScsiInitiatorName)
#include EFI_PROTOCOL_DEFINITION (UsbHostController)
#include EFI_PROTOCOL_DEFINITION (ManagedNetwork)
#include EFI_PROTOCOL_DEFINITION (Mtftp4)
#include EFI_PROTOCOL_DEFINITION (ServiceBinding)
#include EFI_PROTOCOL_DEFINITION (Tcp4)
#include EFI_PROTOCOL_DEFINITION (Udp4)
#include EFI_PROTOCOL_DEFINITION (Arp)
// check here: currently not implementated
//#include EFI_PROTOCOL_DEFINITION (AuthenticationInfo)
#endif

#include EFI_PROTOCOL_DEFINITION (AcpiSupport)
#include EFI_PROTOCOL_DEFINITION (Bis)
#include EFI_PROTOCOL_DEFINITION (BlockIo)
#include EFI_PROTOCOL_DEFINITION (BootScriptSave)
#include EFI_PROTOCOL_DEFINITION (BusSpecificDriverOverride)
#include EFI_PROTOCOL_DEFINITION (ComponentName)
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#include EFI_PROTOCOL_DEFINITION (ComponentName2)
#endif
#include EFI_PROTOCOL_DEFINITION (CpuIo)
#include EFI_PROTOCOL_DEFINITION (DataHub)
#include EFI_PROTOCOL_DEFINITION (DebugPort)
#include EFI_PROTOCOL_DEFINITION (DebugSupport)
#include EFI_PROTOCOL_DEFINITION (Decompress)
#include EFI_PROTOCOL_DEFINITION (DeviceIo)
#include EFI_PROTOCOL_DEFINITION (DevicePath) 
#include EFI_PROTOCOL_DEFINITION (DiskIo)
#include EFI_PROTOCOL_DEFINITION (DriverBinding)
#include EFI_PROTOCOL_DEFINITION (DriverConfiguration)
#include EFI_PROTOCOL_DEFINITION (DriverDiagnostics)
#include EFI_PROTOCOL_DEFINITION (Ebc)
#include EFI_PROTOCOL_DEFINITION (EfiNetworkInterfaceIdentifier)
#include EFI_PROTOCOL_DEFINITION (FileInfo)
#include EFI_PROTOCOL_DEFINITION (FileSystemInfo)
#include EFI_PROTOCOL_DEFINITION (FileSystemVolumeLabelInfo)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolume)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolumeBlock)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolumeDispatch)
#if (EFI_SPECIFICATION_VERSION < 0x0002000A)
#include EFI_PROTOCOL_DEFINITION (FormBrowser)
#include EFI_PROTOCOL_DEFINITION (FormCallback)
#endif
#include EFI_PROTOCOL_DEFINITION (GuidedSectionExtraction)
#if (EFI_SPECIFICATION_VERSION < 0x0002000A)
#include EFI_PROTOCOL_DEFINITION (Hii)
#endif
#include EFI_PROTOCOL_DEFINITION (IdeControllerInit)
#include EFI_PROTOCOL_DEFINITION (IncompatiblePciDeviceSupport)
#include EFI_PROTOCOL_DEFINITION (Legacy8259)
#include EFI_PROTOCOL_DEFINITION (LegacyBios)
#include EFI_PROTOCOL_DEFINITION (LegacyBiosPlatform)
#include EFI_PROTOCOL_DEFINITION (LegacyInterrupt)
#include EFI_PROTOCOL_DEFINITION (LegacyRegion)
#include EFI_PROTOCOL_DEFINITION (LoadedImage)
#include EFI_PROTOCOL_DEFINITION (LoadFile)
#include EFI_PROTOCOL_DEFINITION (PciHostBridgeResourceAllocation)
#include EFI_PROTOCOL_DEFINITION (PciHotPlugInit)
#include EFI_PROTOCOL_DEFINITION (PciIo)
#include EFI_PROTOCOL_DEFINITION (PciPlatform)
#include EFI_PROTOCOL_DEFINITION (PciRootBridgeIo)
#include EFI_PROTOCOL_DEFINITION (PlatformDriverOverride)
#include EFI_PROTOCOL_DEFINITION (PxeBaseCode)
#include EFI_PROTOCOL_DEFINITION (PxeBaseCodeCallBack)
#include EFI_PROTOCOL_DEFINITION (ScsiIo)
#include EFI_PROTOCOL_DEFINITION (ScsiPassThru)
#include EFI_PROTOCOL_DEFINITION (SectionExtraction)
#include EFI_PROTOCOL_DEFINITION (SerialIo)
#include EFI_PROTOCOL_DEFINITION (SimpleFileSystem)
#include EFI_PROTOCOL_DEFINITION (SimpleNetwork)
#include EFI_PROTOCOL_DEFINITION (SimplePointer)
#include EFI_PROTOCOL_DEFINITION (SimpleTextIn)
#include EFI_PROTOCOL_DEFINITION (SimpleTextOut)
#include EFI_PROTOCOL_DEFINITION (Smbus)
#include EFI_PROTOCOL_DEFINITION (SmmAccess)
#include EFI_PROTOCOL_DEFINITION (SmmBase)
#include EFI_PROTOCOL_DEFINITION (SmmControl)
#include EFI_PROTOCOL_DEFINITION (SmmGpiDispatch)
#include EFI_PROTOCOL_DEFINITION (SmmIchnDispatch)
#include EFI_PROTOCOL_DEFINITION (SmmPeriodicTimerDispatch)
#include EFI_PROTOCOL_DEFINITION (SmmPowerButtonDispatch)
#include EFI_PROTOCOL_DEFINITION (SmmStandbyButtonDispatch)
#include EFI_PROTOCOL_DEFINITION (SmmStatusCode)
#include EFI_PROTOCOL_DEFINITION (SmmSwDispatch)
#include EFI_PROTOCOL_DEFINITION (SmmSxDispatch)
#include EFI_PROTOCOL_DEFINITION (SmmUsbDispatch)
#include EFI_PROTOCOL_DEFINITION (UgaDraw)
#include EFI_PROTOCOL_DEFINITION (UnicodeCollation)
#include EFI_PROTOCOL_DEFINITION (UsbHostController)
#include EFI_PROTOCOL_DEFINITION (UsbIo)
#include EFI_PROTOCOL_DEFINITION (SecurityPolicy)
#include EFI_PROTOCOL_DEFINITION (LoadPe32Image)

//
// Arch Protocol definitions
//

#include EFI_ARCH_PROTOCOL_DEFINITION (Bds)
#include EFI_ARCH_PROTOCOL_DEFINITION (Cpu)
#include EFI_ARCH_PROTOCOL_DEFINITION (Metronome)
#include EFI_ARCH_PROTOCOL_DEFINITION (MonotonicCounter)
#include EFI_ARCH_PROTOCOL_DEFINITION (RealTimeClock)
#include EFI_ARCH_PROTOCOL_DEFINITION (Reset)
#include EFI_ARCH_PROTOCOL_DEFINITION (Runtime)
#include EFI_ARCH_PROTOCOL_DEFINITION (Security)
#include EFI_ARCH_PROTOCOL_DEFINITION (StatusCode)
#include EFI_ARCH_PROTOCOL_DEFINITION (Timer)
#include EFI_ARCH_PROTOCOL_DEFINITION (Variable)
#include EFI_ARCH_PROTOCOL_DEFINITION (VariableWrite)
#include EFI_ARCH_PROTOCOL_DEFINITION (WatchdogTimer)

//
// IPF only
//
#ifdef MDE_CPU_IPF
#include EFI_PROTOCOL_DEFINITION (ExtendedSalGuid)
#include EFI_PROTOCOL_DEFINITION (ExtendedSalBootService)
#endif

//
// EDK Library headers used by EDKII Glue Libraries
//
#include "EfiDriverLib.h"
#include "EfiCapsule.h"

#include "Common/EdkIIGlueDefinitionChangesDxe.h"


//
// EdkII Glue Library Class headers
//

#include "EdkIIGlueBase.h"
#include "Library/EdkIIGlueDebugLib.h"
#include "Library/EdkIIGluePostCodeLib.h"
#include "Library/EdkIIGlueReportStatusCodeLib.h"
#include "Library/EdkIIGlueHiiLib.h"
#include "Library/EdkIIGlueHobLib.h"
#include "Library/EdkIIGlueMemoryAllocationLib.h"
#include "Library/EdkIIGlueSmbusLib.h"
#include "Library/EdkIIGlueDxeRuntimeDriverLib.h"
#include "Library/EdkIIGlueDxeServicesTableLib.h"
#include "Library/EdkIIGlueDxeSmmDriverEntryPoint.h"
#include "Library/EdkIIGlueDevicePathLib.h"
#include "Library/EdkIIGlueUefiLib.h"
#include "Library/EdkIIGlueUefiDecompressLib.h"
#include "Library/EdkIIGlueUefiDriverModelLib.h"
#include "Library/EdkIIGlueUefiBootServicesTableLib.h"
#include "Library/EdkIIGlueUefiDriverEntryPoint.h"
#include "Library/EdkIIGlueUefiRuntimeServicesTableLib.h"

#endif
