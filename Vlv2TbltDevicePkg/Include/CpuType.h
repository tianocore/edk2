/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  CpuType.h

Abstract:

--*/

#ifndef _CPU_TYPE_H
#define _CPU_TYPE_H

#pragma pack(1)

typedef enum {
  EnumCpuUarchUnknown = 0,
  EnumNehalemUarch,
} EFI_CPU_UARCH;

typedef enum {
  EnumCpuPlatformUnknown = 0,
  EnumDesktop,
  EnumMobile,
  EnumServer,
  EnumNetTop
} EFI_CPU_PLATFORM;

typedef enum {
  EnumCpuTypeUnknown = 0,
  EnumAtom,
  EnumNehalemEx,
  EnumBloomfield,
  EnumGainestown,
  EnumHavendale,
  EnumLynnfield,
  EnumAuburndale,
  EnumClarksfield,
  EnumPineview,
  EnumCedarview,
  EnumValleyview,
  EnumClarkdale // Havendale 32nm
} EFI_CPU_TYPE;

typedef enum {
  EnumCpuFamilyUnknown = 0,
  EnumFamilyField,
  EnumFamilyDale
} EFI_CPU_FAMILY;

#pragma pack()

#endif
