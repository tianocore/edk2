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

#ifndef __EDK_DXE_CORE_H__
#define __EDK_DXE_CORE_H__

#include <Common/DecompressLibraryHob.h>

//
// BUGBUG: Performance related protocol and Guid.
// They are Tiano-private, but are required for DxeCore
//
#include <Protocol/Performance.h>
#include <Guid/PeiPerformanceHob.h>
//
// BUGBUG: Do these really belomg here?
//
#include <Guid/PeiPeCoffLoader.h>
#include <Guid/MemoryTypeInformation.h>

#include <Protocol/CustomizedDecompress.h>
#include <Protocol/DebugLevel.h>
#include <Protocol/LoadPe32Image.h>
#include <Protocol/EdkDecompress.h>
#include <Protocol/Print.h>

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#include <Protocol/Capsule.h>
#endif

#endif
