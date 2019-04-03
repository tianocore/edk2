/************************************************************************
 *
 * Copyright (c) 2013-2015 Intel Corporation.
 *
* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 ************************************************************************/

#include "mrc.h"
#include "memory_options.h"

#include "meminit_utils.h"
#include "prememinit.h"
#include "io.h"

// Read character from serial console
uint8_t mgetc(void);

extern uint32_t DpfPrintMask;

// Adjust configuration parameters before initialisation
// sequence.
void PreMemInit(
    MRCParams_t *mrc_params)
{
  const DRAMParams_t *dram_params;

  uint8_t dram_width;
  uint32_t dram_cfg_index;
  uint32_t channel_i;

  ENTERFN();

#ifdef MRC_SV
  {
    uint8_t ch;

    myloop:

    DPF(D_INFO, "- c - continue\n");
    DPF(D_INFO, "- f - boot mode [%d]\n", mrc_params->boot_mode);
    DPF(D_INFO, "- r - rank enable [%d]\n", mrc_params->rank_enables);
    DPF(D_INFO, "- e - ecc switch [%d]\n", mrc_params->ecc_enables);
    DPF(D_INFO, "- b - scrambling switch [%d]\n", mrc_params->scrambling_enables);
    DPF(D_INFO, "- a - adr mode [%d]\n", mrc_params->address_mode);
    DPF(D_INFO, "- m - menu after mrc [%d]\n", mrc_params->menu_after_mrc);
    DPF(D_INFO, "- t - tune to rcvn [%d]\n", mrc_params->tune_rcvn);
    DPF(D_INFO, "- o - odt switch [%d]\n", mrc_params->rd_odt_value);
    DPF(D_INFO, "- d - dram density [%d]\n", mrc_params->params.DENSITY);
    DPF(D_INFO, "- p - power down disable [%d]\n", mrc_params->power_down_disable);
    DPF(D_INFO, "- l - log switch 0x%x\n", DpfPrintMask);
    ch = mgetc();

    switch (ch)
    {
    case 'f':
      mrc_params->boot_mode >>= 1;
      if(mrc_params->boot_mode == bmUnknown)
      {
         mrc_params->boot_mode = bmWarm;
      }
      DPF(D_INFO, "Boot mode %d\n", mrc_params->boot_mode);
      break;

    case 'p':
      mrc_params->power_down_disable ^= 1;
      DPF(D_INFO, "Power down disable %d\n", mrc_params->power_down_disable);
      break;

    case 'r':
      mrc_params->rank_enables ^= 2;
      DPF(D_INFO, "Rank enable %d\n", mrc_params->rank_enables);
      break;

    case 'e':
      mrc_params->ecc_enables ^= 1;
      DPF(D_INFO, "Ecc enable %d\n", mrc_params->ecc_enables);
      break;

    case 'b':
      mrc_params->scrambling_enables ^= 1;
      DPF(D_INFO, "Scrambler enable %d\n", mrc_params->scrambling_enables);
      break;

    case 'a':
      mrc_params->address_mode = (mrc_params->address_mode + 1) % 3;
      DPF(D_INFO, "Adr mode %d\n", mrc_params->address_mode);
      break;

    case 'm':
       mrc_params->menu_after_mrc ^= 1;
      DPF(D_INFO, "Menu after mrc %d\n", mrc_params->menu_after_mrc);
      break;

    case 't':
      mrc_params->tune_rcvn ^= 1;
      DPF(D_INFO, "Tune to rcvn %d\n", mrc_params->tune_rcvn);
      break;

    case 'o':
      mrc_params->rd_odt_value = (mrc_params->rd_odt_value + 1) % 4;
      DPF(D_INFO, "Rd_odt_value %d\n", mrc_params->rd_odt_value);
      break;

    case 'd':
      mrc_params->params.DENSITY = (mrc_params->params.DENSITY + 1) % 4;
      DPF(D_INFO, "Dram density %d\n", mrc_params->params.DENSITY);
      break;

    case 'l':
      DpfPrintMask ^= 0x30;
      DPF(D_INFO, "Log mask %x\n", DpfPrintMask);
      break;

    default:
      break;
    }

    if (ch != 'c')
      goto myloop;

  }
#endif

  // initially expect success
  mrc_params->status = MRC_SUCCESS;

  // todo!!! Setup board layout (must be reviewed as is selecting static timings)
  // 0 == R0 (DDR3 x16), 1 == R1 (DDR3 x16), 2 == DV (DDR3 x8), 3 == SV (DDR3 x8)
  if (mrc_params->dram_width == x8)
  {
    mrc_params->board_id = 2;  // select x8 layout
  }
  else
  {
    mrc_params->board_id = 0;  // select x16 layout
  }

  // initially no memory
  mrc_params->mem_size = 0;
  channel_i = 0;

  // begin of channel settings
  dram_width = mrc_params->dram_width;
  dram_params = &mrc_params->params;
  dram_cfg_index = 0;

  // Determine Column & Row Bits:
  // Column:
  // 11 for 8Gbx8, else 10
  mrc_params->column_bits[channel_i] = ((dram_params[dram_cfg_index].DENSITY == 4) && (dram_width == x8)) ? (11) : (10);

  // Row:
  // 512Mbx16=12 512Mbx8=13
  //   1Gbx16=13   1Gbx8=14
  //   2Gbx16=14   2Gbx8=15
  //   4Gbx16=15   4Gbx8=16
  //   8Gbx16=16   8Gbx8=16
  mrc_params->row_bits[channel_i] = 12 + (dram_params[dram_cfg_index].DENSITY)
      + (((dram_params[dram_cfg_index].DENSITY < 4) && (dram_width == x8)) ? (1) : (0));

  // Determine Per Channel Memory Size:
  // (For 2 RANKs, multiply by 2)
  // (For 16 bit data bus, divide by 2)
  // DENSITY  WIDTH   MEM_AVAILABLE
  // 512Mb    x16     0x008000000 ( 128MB)
  // 512Mb    x8      0x010000000 ( 256MB)
  // 1Gb      x16     0x010000000 ( 256MB)
  // 1Gb      x8      0x020000000 ( 512MB)
  // 2Gb      x16     0x020000000 ( 512MB)
  // 2Gb      x8      0x040000000 (1024MB)
  // 4Gb      x16     0x040000000 (1024MB)
  // 4Gb      x8      0x080000000 (2048MB)
  mrc_params->channel_size[channel_i] = (1 << dram_params[dram_cfg_index].DENSITY);
  mrc_params->channel_size[channel_i] *= ((dram_width == x8) ? (2) : (1));
  mrc_params->channel_size[channel_i] *= (mrc_params->rank_enables == 0x3) ? (2) : (1);
  mrc_params->channel_size[channel_i] *= (mrc_params->channel_width == x16) ? (1) : (2);

  // Determine memory size (convert number of 64MB/512Mb units)
  mrc_params->mem_size += mrc_params->channel_size[channel_i] << 26;

  // end of channel settings

  LEAVEFN();
  return;
}

