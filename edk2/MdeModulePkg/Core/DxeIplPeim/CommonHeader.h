/**@file
  Common header file shared by all source files.

  This file includes package header files, library classes and protocol, PPI & GUID definitions.

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
   are licensed and made available under the terms and conditions of the BSD License
   which accompanies this distribution. The full text of the license may be found at
   http://opensource.org/licenses/bsd-license.php
   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __COMMON_HEADER_H_
#define __COMMON_HEADER_H_


//
// The package level header files this module uses
//
#include <PiPei.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Ppi/DxeIpl.h>
#include <Ppi/S3Resume.h>
#include <Protocol/EdkDecompress.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Protocol/CustomizedDecompress.h>
#include <Protocol/Decompress.h>
#include <Ppi/Security.h>
#include <Ppi/SectionExtraction.h>
#include <Ppi/LoadFile.h>
#include <Ppi/RecoveryModule.h>
#include <Ppi/MemoryDiscovered.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/PeCoffLoaderLib.h>
#include <Library/UefiDecompressLib.h>
#include <Library/CustomDecompressLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffLib.h>

#endif
