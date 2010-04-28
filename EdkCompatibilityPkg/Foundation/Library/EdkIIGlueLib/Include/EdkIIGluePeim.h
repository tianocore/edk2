/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGluePeim.h
  
Abstract: 

  Root include file for PEI Modules

**/


#ifndef __EDKII_GLUE_PEIM_H__
#define __EDKII_GLUE_PEIM_H__


//
// Check to make sure TIANO_RELEASE_VERSION is defined
//
#if !defined(TIANO_RELEASE_VERSION)
  #error TIANO_RELEASE_VERSION not defined
#elif (TIANO_RELEASE_VERSION == 0)
  #error TIANO_RELEASE_VERSION can not be zero
#endif

//
// General Type & API definitions
//

#include "Pei.h"
#include "EfiBootScript.h"
#include "EfiImage.h"
#include "EfiPeOptionalHeader.h"
#include "EfiCapsule.h"
#include EFI_PROTOCOL_DEFINITION (DevicePath)
#include "TianoDevicePath.h"
#include "PeiPerf.h"

//
// GUID definitions
//

#include EFI_GUID_DEFINITION (Apriori)
#include EFI_GUID_DEFINITION (Capsule)
#include EFI_GUID_DEFINITION (DxeServices)
#include EFI_GUID_DEFINITION (Hob)
#include EFI_GUID_DEFINITION (MemoryAllocationHob)
#include EFI_GUID_DEFINITION (FirmwareFileSystem)
#include EFI_GUID_DEFINITION (SmramMemoryReserve)
#include EFI_GUID_DEFINITION (DataHubRecords)
#include EFI_GUID_DEFINITION (PeiPerformanceHob)

//
// *** NOTE ***: StatusCodeDataTypeId definition differences need to be 
// resolved when porting a module to real EDK II
//
#include EFI_GUID_DEFINITION (StatusCodeDataTypeId)

//
// Ppi definitions
//

#include EFI_PPI_DEFINITION (BlockIo)
#include EFI_PPI_DEFINITION (BootInRecoveryMode)
#include EFI_PPI_DEFINITION (BootScriptExecuter)
#include EFI_PPI_DEFINITION (CpuIo)
#include EFI_PPI_DEFINITION (DeviceRecoveryModule)
#include EFI_PPI_DEFINITION (DxeIpl)
#include EFI_PPI_DEFINITION (EndOfPeiSignal)
#include EFI_PPI_DEFINITION (FindFv)
#include EFI_PPI_DEFINITION (LoadFile)
#include EFI_PPI_DEFINITION (BootMode)
#include EFI_PPI_DEFINITION (MemoryDiscovered)
#include EFI_PPI_DEFINITION (PciCfg)
#include EFI_PPI_DEFINITION (Variable)
#include EFI_PPI_DEFINITION (RecoveryModule)
#include EFI_PPI_DEFINITION (Reset)
#include EFI_PPI_DEFINITION (S3Resume)
#include EFI_PPI_DEFINITION (SecPlatformInformation)
#include EFI_PPI_DEFINITION (SectionExtraction)
#include EFI_PPI_DEFINITION (Security)
#include EFI_PPI_DEFINITION (Smbus)
#include EFI_PPI_DEFINITION (Stall)
#include EFI_PPI_DEFINITION (StatusCode)


#include "Common/EdkIIGlueDefinitionChangesPeim.h"

//
// EdkII Glue Library Class headers
//

#include "EdkIIGlueBase.h"
#include "Library/EdkIIGlueDebugLib.h"
#include "Library/EdkIIGluePostCodeLib.h"
#include "Library/EdkIIGlueReportStatusCodeLib.h"
#include "Library/EdkIIGlueHobLib.h"
#include "Library/EdkIIGlueMemoryAllocationLib.h"
#include "Library/EdkIIGlueSmbusLib.h"
#include "Library/EdkIIGluePeiServicesLib.h"
#include "Library/EdkIIGluePeiServicesTablePointerLib.h"
#include "Library/EdkIIGlueResourcePublicationLib.h"
#include "Library/EdkIIGluePeimEntryPoint.h"

#endif
