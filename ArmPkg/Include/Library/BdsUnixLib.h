/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*  
*  This program and the accompanying materials                          
*  are licensed and made available under the terms and conditions of the BSD License         
*  which accompanies this distribution.  The full text of the license may be found at        
*  http://opensource.org/licenses/bsd-license.php                                            
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
*
**/

#ifndef __BDS_ENTRY_H__
#define __BDS_ENTRY_H__

EFI_STATUS
BdsConnectAllDrivers ( VOID );

EFI_STATUS
BdsBootLinux (
    IN  CONST CHAR16* LinuxKernel,
    IN  CONST CHAR8*  ATag,
    IN  CONST CHAR16* Fdt
);

EFI_STATUS
BdsLoadApplication (
    IN  CHAR16* EfiApp
);

EFI_STATUS
BdsLoadApplicationFromPath (
    IN  CHAR16* EfiAppPath
);

#endif
