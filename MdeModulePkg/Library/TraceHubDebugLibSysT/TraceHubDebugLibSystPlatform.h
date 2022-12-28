/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TRACE_HUB_DEBUG_LIB_SYST_PLATFORM_H_
#define TRACE_HUB_DEBUG_LIB_SYST_PLATFORM_H_

#include "MipiWrapper.h"

typedef struct {
  UINT8    *MmioAddr;
} TRACE_HUB_PLATFORM_SYST_DATA;

struct mipi_syst_platform_handle {
  TRACE_HUB_PLATFORM_SYST_DATA    TraceHubPlatformData;
};

#define MIPI_SYST_PLATFORM_CLOCK()  MipiSystGetEpochUs ()

#ifndef MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA
#define MIPI_SYST_OUTPUT_D32TS(MipiSystHandle, Data)   SthWriteD32Ts ((MipiSystHandle), (Data))
#define MIPI_SYST_OUTPUT_D32MTS(MipiSystHandle, Data)  SthWriteD32Mts ((MipiSystHandle), (Data))
#define MIPI_SYST_OUTPUT_D64MTS(MipiSystHandle, Data)  SthWriteD64Mts ((MipiSystHandle), (Data))
#define MIPI_SYST_OUTPUT_D8(MipiSystHandle, Data)      SthWriteD8 ((MipiSystHandle), (Data))
#define MIPI_SYST_OUTPUT_D16(MipiSystHandle, Data)     SthWriteD16 ((MipiSystHandle), (Data))
#define MIPI_SYST_OUTPUT_D32(MipiSystHandle, Data)     SthWriteD32 ((MipiSystHandle), (Data))
  #if defined (MIPI_SYST_PCFG_ENABLE_64BIT_IO)
#define MIPI_SYST_OUTPUT_D64(MipiSystHandle, Data)  SthWriteD64 ((MipiSystHandle), (Data))
  #endif
#define MIPI_SYST_OUTPUT_FLAG(MipiSystHandle)  SthWriteFlag ((MipiSystHandle))
#endif

#endif // TRACE_HUB_DEBUG_LIB_SYST_PLATFORM_H_
