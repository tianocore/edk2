/************************************************************************
 *
 * Copyright (c) 2013-2015 Intel Corporation.
 *
* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 ************************************************************************/
#include "mrc.h"
#include "memory_options.h"

#include "meminit.h"
#include "meminit_utils.h"
#include "prememinit.h"
#include "io.h"

// Base address for UART registers
extern uint32_t UartMmioBase;

//
// Memory Reference Code entry point when executing from BIOS
//
void Mrc( MRCParams_t *mrc_params)
{
  // configure uart base address assuming code relocated to eSRAM
  UartMmioBase = mrc_params->uart_mmio_base;

  ENTERFN();

  DPF(D_INFO, "MRC Version %04X %s %s\n", MRC_VERSION, __DATE__, __TIME__);

  // this will set up the data structures used by MemInit()
  PreMemInit(mrc_params);

  // this will initialize system memory
  MemInit(mrc_params);

  LEAVEFN();
  return;
}

