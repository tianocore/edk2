/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PcdEmulator.h

Abstract:
  Platform Configuration Database (PCD) 

--*/

#ifndef __PCD_EMULATOR_H__
#define __PCD_EMULATOR_H__



//
// BugBug: Not very sure, where to put this "extern"
//
extern GUID gPcdHobGuid;

//
// BugBug: Hack max number of callbacks per token
//
#define MAX_PCD_CALLBACK  0x10

EMULATED_PCD_ENTRY_EX     *gEmulatedPcdEntryEx;
EMULATED_PCD_DATABASE_EX     *gEmulatedPcdDatabaseEx;

EMULATED_PCD_ENTRY_EX *
GetPcdEntry (
  IN      UINTN    TokenNumber
  );

#endif
