/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGlueProcessorBind.h
  
Abstract: 

  Processor specific definitions

--*/

#ifndef __EDKII_GLUE_PROCESSOR_BIND_H__
#define __EDKII_GLUE_PROCESSOR_BIND_H__

//
// The Microsoft* C compiler can removed references to unreferenced data items
//  if the /OPT:REF linker option is used. We defined a macro as this is a 
//  a non standard extension
//
#if defined(_MSC_EXTENSIONS)
  #define GLOBAL_REMOVE_IF_UNREFERENCED __declspec(selectany)
#else
  #define GLOBAL_REMOVE_IF_UNREFERENCED
#endif

#if !defined(MDE_CPU_IPF)
  #define MDE_CPU_IPF
#endif

//
// IPF Specific Functions
//
typedef struct {
  UINT64                    Status;
  UINT64                    r9;
  UINT64                    r10;
  UINT64                    r11;
} PAL_CALL_RETURN;


#define EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID_LO 0x4871260ec1a74056
#define EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID_HI 0x116e5ba645e631a0

#define EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_LO 0x4d02efdb7e97a470
#define EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_HI 0x96a27bd29061ce8f 

#define EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_LO 0x4370c6414ecb6c53 
#define EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_HI 0x78836e490e3bb28c

#define EFI_EXTENDED_SAL_MTC_SERVICES_PROTOCOL_GUID_LO 0x408b75e8899afd18
#define EFI_EXTENDED_SAL_MTC_SERVICES_PROTOCOL_GUID_HI 0x54f4cd7e2e6e1aa4

#define EFI_EXTENDED_SAL_RESET_SERVICES_PROTOCOL_GUID_LO  0x46f58ce17d019990
#define EFI_EXTENDED_SAL_RESET_SERVICES_PROTOCOL_GUID_HI  0xa06a6798513c76a7

//
// Per the Itanium Software Conventions and Runtime Architecture Guide,
// section 3.3.4, IPF stack must always be 16-byte aligned.
//
#define CPU_STACK_ALIGNMENT   16

#endif
