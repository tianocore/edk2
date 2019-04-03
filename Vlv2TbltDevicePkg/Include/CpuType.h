/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

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
