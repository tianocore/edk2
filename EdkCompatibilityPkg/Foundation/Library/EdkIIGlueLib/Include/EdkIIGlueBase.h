/*++

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGlueBase.h
  
Abstract: 

  Root include file for Base modules

--*/

#ifndef __EDKII_GLUE_BASE_H__
#define __EDKII_GLUE_BASE_H__

//
// Using this header means building with EdkIIGlueLib
//
#ifndef BUILD_WITH_EDKII_GLUE_LIB
  #define BUILD_WITH_EDKII_GLUE_LIB
#endif

#ifndef BUILD_WITH_GLUELIB
  #define BUILD_WITH_GLUELIB
#endif

//
// General Type & API definitions
//

#include <EfiBind.h>
#include "EfiTypes.h"
#include "EfiError.h"
#include "Common/EdkIIGlueBaseTypes.h"
#include "EfiImage.h"
#include "EfiPeOptionalHeader.h"
#include "EfiStdArg.h"
#include "EfiDebug.h"
#include "EdkIIGlueProcessorBind.h"

//
// Some Status Code data type definitions are in TianoSpecTypes.h in EDK
//
#if (TIANO_RELEASE_VERSION <= 0x00080005)
#include "TianoSpecTypes.h"
#endif
#include "EfiStatusCode.h"

//
// EDK Library headers used by EdkII Glue Libraries
//
#include "LinkedList.h"
#include "EfiCommonLib.h"

#include "Common/EdkIIGlueDefinitionChangesBase.h"

//
// EdkII Glue Library Class headers
//

#include "Library/EdkIIGlueBaseLib.h"
#include "Library/EdkIIGlueBaseMemoryLib.h"
#include "Library/EdkIIGlueCacheMaintenanceLib.h"
#include "Library/EdkIIGlueIoLib.h"
#include "Library/EdkIIGluePciCf8Lib.h"
#include "Library/EdkIIGluePciExpressLib.h"
#include "Library/EdkIIGluePciLib.h"
#include "Library/EdkIIGluePeCoffGetEntryPointLib.h"
#include "Library/EdkIIGluePeCoffLib.h"
#include "Library/EdkIIGluePostCodeLib.h"
#include "Library/EdkIIGluePrintLib.h"
#include "Library/EdkIIGlueTimerLib.h"
#include "Library/EdkIIGlueUefiDecompressLib.h"
#include "Library/EdkIIGlueDebugLib.h"

//
// Publish MDE Library PCDs
//
#include "Pcd/EdkIIGluePcdBaseLib.h"
#include "Pcd/EdkIIGluePcdDebugLib.h"
#include "Pcd/EdkIIGluePcdIoLib.h"
#include "Pcd/EdkIIGluePcdPciExpressLib.h"
#include "Pcd/EdkIIGluePcdPostCodeLib.h"
#include "Pcd/EdkIIGluePcdReportStatusCodeLib.h"
#include "Pcd/EdkIIGluePcdTimerLib.h"

#endif
