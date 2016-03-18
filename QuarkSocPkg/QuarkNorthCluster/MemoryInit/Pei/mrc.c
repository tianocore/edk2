/************************************************************************
 *
 * Copyright (c) 2013-2015 Intel Corporation.
 *
* This program and the accompanying materials
* are licensed and made available under the terms and conditions of the BSD License
* which accompanies this distribution.  The full text of the license may be found at
* http://opensource.org/licenses/bsd-license.php
*
* THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
* WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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

