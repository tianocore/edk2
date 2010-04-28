/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGluePcdBaseLib.h
  
Abstract: 

  PCD vadues for library customization

--*/

#ifndef __EDKII_GLUE_PCD_BASE_LIB_H__
#define __EDKII_GLUE_PCD_BASE_LIB_H__

//
// Following Pcd values are hard coded at compile time.
// Override these through compiler option "/D" in PlatformTools.env if needed
//

#ifndef __EDKII_GLUE_PCD_PcdMaximumUnicodeStringLength__
#define __EDKII_GLUE_PCD_PcdMaximumUnicodeStringLength__ EDKII_GLUE_MaximumUnicodeStringLength
#endif

#ifndef __EDKII_GLUE_PCD_PcdMaximumAsciiStringLength__
#define __EDKII_GLUE_PCD_PcdMaximumAsciiStringLength__   EDKII_GLUE_MaximumAsciiStringLength
#endif

//
// SpinLock Pcds
//
#ifndef __EDKII_GLUE_PCD_PcdSpinLockTimeout__
#define __EDKII_GLUE_PCD_PcdSpinLockTimeout__            EDKII_GLUE_SpinLockTimeout
#endif

// Linked List
#ifndef  __EDKII_GLUE_PCD_PcdMaximumLinkedListLength__
#define  __EDKII_GLUE_PCD_PcdMaximumLinkedListLength__   EDKII_GLUE_MaximumLinkedListLength
#endif

#include "Pcd/EdkIIGluePcd.h"
#endif
