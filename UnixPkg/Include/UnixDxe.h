/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  UnixDxe.h

Abstract:
  Public include file for the Unix Library

--*/

#ifndef __UNIX_DXE_H__
#define __UNIX_DXE_H__

#include <Ppi/UnixPeiLoadFile.h>
#include <Ppi/UnixAutoScan.h>
#include <Ppi/UnixThunk.h>
#include <Ppi/UnixFwh.h>

//
//  UnixIo.h depends on UnixThunk.h
//

#include <Common/UnixInclude.h>
#include <Protocol/UnixThunk.h>
#include <Protocol/UnixIo.h>

#endif
