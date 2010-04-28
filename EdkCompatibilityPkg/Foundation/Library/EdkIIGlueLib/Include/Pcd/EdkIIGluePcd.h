/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGluePcd.h
  
Abstract: 

  Fixed-at-build PCD macro expansion definitions

--*/

#ifndef __EDKII_GLUE_PCD_H__
#define __EDKII_GLUE_PCD_H__

//
// Redefine Pcd functions into compile time hardcoded values
//
#define FixedPcdGet8(TokenName)     __EDKII_GLUE_PCD_##TokenName##__
#define FixedPcdGet16(TokenName)    __EDKII_GLUE_PCD_##TokenName##__
#define FixedPcdGet32(TokenName)    __EDKII_GLUE_PCD_##TokenName##__
#define FixedPcdGet64(TokenName)    __EDKII_GLUE_PCD_##TokenName##__
#define FixedPcdGetBool(TokenName)  __EDKII_GLUE_PCD_##TokenName##__

#define PcdGet8(TokenName)          __EDKII_GLUE_PCD_##TokenName##__
#define PcdGet16(TokenName)         __EDKII_GLUE_PCD_##TokenName##__
#define PcdGet32(TokenName)         __EDKII_GLUE_PCD_##TokenName##__
#define PcdGet64(TokenName)         __EDKII_GLUE_PCD_##TokenName##__
#define PcdGetPtr(TokenName)        __EDKII_GLUE_PCD_##TokenName##__
#define PcdGetBool(TokenName)       __EDKII_GLUE_PCD_##TokenName##__

#endif
