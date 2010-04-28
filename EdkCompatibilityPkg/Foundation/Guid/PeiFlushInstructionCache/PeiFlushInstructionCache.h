/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    PeiFlushInstructionCache.h
    
Abstract:

  GUID for the Instruction Cache Flushing APIs shared between PEI and DXE

--*/

#ifndef _PEI_FLUSH_INSTRUCTION_CACHE_GUID_H_
#define _PEI_FLUSH_INSTRUCTION_CACHE_GUID_H_

#define EFI_PEI_FLUSH_INSTRUCTION_CACHE_GUID  \
  { 0xd8117cfc, 0x94a6, 0x11d4, {0x9a, 0x3a, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d} }

EFI_FORWARD_DECLARATION (EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL);

typedef 
EFI_STATUS
(EFIAPI *EFI_PEI_FLUSH_INSTRUCTION_CACHE_FLUSH) (
  IN EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS                              Start,
  IN UINT64                                            Length
  );

struct _EFI_PEI_FLUSH_INSTRUCTION_CACHE_PROTOCOL {
  EFI_PEI_FLUSH_INSTRUCTION_CACHE_FLUSH  Flush;
};

extern EFI_GUID gEfiPeiFlushInstructionCacheGuid;

#endif
