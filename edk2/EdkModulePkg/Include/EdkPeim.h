/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EdkPeim.h

Abstract:
  This file defines the base package surface area for writting a PEIM

  Things defined in the PEI CIS specification go in PeiCis.h. 

  EdkPeim.h contains build environment and library information needed to build
  a basic PEIM that needs Tiano specific definitiosn. T
  
  Currently we just add in some extra PPI and GUID definitions

--*/

#ifndef __EDK_PEIM_H__
#define __EDK_PEIM_H__

//
#include <Common/FlashMap.h>
#include <Common/DecompressLibraryHob.h>
//
// BUGBUG: Performance related Guid.
// It is Tiano-private, but is required for PeiCore
//
#include <Guid/PeiPerformanceHob.h>
#include <Guid/PeiPeCoffLoader.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/FlashMapHob.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/CapsuleVendor.h>
#include <Guid/BootState.h>
#include <Guid/MemoryStatusCodeRecord.h>
#include <Guid/GenericPlatformVariable.h>

#include <Ppi/PeiInMemory.h>
#include <Ppi/FlashMap.h>
#include <Ppi/BaseMemoryTest.h>
#include <Ppi/StatusCodeMemory.h>

#include <Protocol/CustomizedDecompress.h>
#include <Protocol/EdkDecompress.h>

#include <Dxe/ArchProtocol/StatusCode.h>

#endif
