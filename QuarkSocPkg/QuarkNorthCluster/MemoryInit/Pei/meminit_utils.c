/************************************************************************
 *
 * Copyright (c) 2013-2015 Intel Corporation.
 *
* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 ***************************************************************************/

#include "mrc.h"
#include "memory_options.h"

#include "meminit_utils.h"
#include "hte.h"
#include "io.h"

void select_hte(
    MRCParams_t *mrc_params);

static uint8_t first_run = 0;

const uint8_t vref_codes[64] =
{ // lowest to highest
    0x3F, 0x3E, 0x3D, 0x3C, 0x3B, 0x3A, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30, // 00 - 15
    0x2F, 0x2E, 0x2D, 0x2C, 0x2B, 0x2A, 0x29, 0x28, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20, // 16 - 31
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // 32 - 47
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F  // 48 - 63
};

#ifdef EMU
// Track current post code for debugging purpose
uint32_t PostCode;
#endif

// set_rcvn:
//
// This function will program the RCVEN delays.
// (currently doesn't comprehend rank)
void set_rcvn(
    uint8_t channel,
    uint8_t rank,
    uint8_t byte_lane,
    uint32_t pi_count)
{
  uint32_t reg;
  uint32_t msk;
  uint32_t tempD;

  ENTERFN();
  DPF(D_TRN, "Rcvn ch%d rnk%d ln%d : pi=%03X\n", channel, rank, byte_lane, pi_count);

  // RDPTR (1/2 MCLK, 64 PIs)
  // BL0 -> B01PTRCTL0[11:08] (0x0-0xF)
  // BL1 -> B01PTRCTL0[23:20] (0x0-0xF)
  reg = B01PTRCTL0 + ((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET);
  msk = (byte_lane & BIT0) ? (BIT23 | BIT22 | BIT21 | BIT20) : (BIT11 | BIT10 | BIT9 | BIT8);
  tempD = (byte_lane & BIT0) ? ((pi_count / HALF_CLK) << 20) : ((pi_count / HALF_CLK) << 8);
  isbM32m(DDRPHY, reg, tempD, msk);

  // Adjust PI_COUNT
  pi_count -= ((pi_count / HALF_CLK) & 0xF) * HALF_CLK;

  // PI (1/64 MCLK, 1 PIs)
  // BL0 -> B0DLLPICODER0[29:24] (0x00-0x3F)
  // BL1 -> B1DLLPICODER0[29:24] (0x00-0x3F)
  reg = (byte_lane & BIT0) ? (B1DLLPICODER0) : (B0DLLPICODER0);
  reg += (((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET));
  msk = (BIT29 | BIT28 | BIT27 | BIT26 | BIT25 | BIT24);
  tempD = pi_count << 24;
  isbM32m(DDRPHY, reg, tempD, msk);

  // DEADBAND
  // BL0/1 -> B01DBCTL1[08/11] (+1 select)
  // BL0/1 -> B01DBCTL1[02/05] (enable)
  reg = B01DBCTL1 + ((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET);
  msk = 0x00;
  tempD = 0x00;
  // enable
  msk |= (byte_lane & BIT0) ? (BIT5) : (BIT2);
  if ((pi_count < EARLY_DB) || (pi_count > LATE_DB))
  {
    tempD |= msk;
  }
  // select
  msk |= (byte_lane & BIT0) ? (BIT11) : (BIT8);
  if (pi_count < EARLY_DB)
  {
    tempD |= msk;
  }
  isbM32m(DDRPHY, reg, tempD, msk);

  // error check
  if (pi_count > 0x3F)
  {
    training_message(channel, rank, byte_lane);
    post_code(0xEE, 0xE0);
  }

  LEAVEFN();
  return;
}

// get_rcvn:
//
// This function will return the current RCVEN delay on the given channel, rank, byte_lane as an absolute PI count.
// (currently doesn't comprehend rank)
uint32_t get_rcvn(
    uint8_t channel,
    uint8_t rank,
    uint8_t byte_lane)
{
  uint32_t reg;
  uint32_t tempD;
  uint32_t pi_count;

  ENTERFN();

  // RDPTR (1/2 MCLK, 64 PIs)
  // BL0 -> B01PTRCTL0[11:08] (0x0-0xF)
  // BL1 -> B01PTRCTL0[23:20] (0x0-0xF)
  reg = B01PTRCTL0 + ((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET);
  tempD = isbR32m(DDRPHY, reg);
  tempD >>= (byte_lane & BIT0) ? (20) : (8);
  tempD &= 0xF;

  // Adjust PI_COUNT
  pi_count = tempD * HALF_CLK;

  // PI (1/64 MCLK, 1 PIs)
  // BL0 -> B0DLLPICODER0[29:24] (0x00-0x3F)
  // BL1 -> B1DLLPICODER0[29:24] (0x00-0x3F)
  reg = (byte_lane & BIT0) ? (B1DLLPICODER0) : (B0DLLPICODER0);
  reg += (((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET));
  tempD = isbR32m(DDRPHY, reg);
  tempD >>= 24;
  tempD &= 0x3F;

  // Adjust PI_COUNT
  pi_count += tempD;

  LEAVEFN();
  return pi_count;
}

// set_rdqs:
//
// This function will program the RDQS delays based on an absolute amount of PIs.
// (currently doesn't comprehend rank)
void set_rdqs(
    uint8_t channel,
    uint8_t rank,
    uint8_t byte_lane,
    uint32_t pi_count)
{
  uint32_t reg;
  uint32_t msk;
  uint32_t tempD;

  ENTERFN();
  DPF(D_TRN, "Rdqs ch%d rnk%d ln%d : pi=%03X\n", channel, rank, byte_lane, pi_count);

  // PI (1/128 MCLK)
  // BL0 -> B0RXDQSPICODE[06:00] (0x00-0x47)
  // BL1 -> B1RXDQSPICODE[06:00] (0x00-0x47)
  reg = (byte_lane & BIT0) ? (B1RXDQSPICODE) : (B0RXDQSPICODE);
  reg += (((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET));
  msk = (BIT6 | BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0);
  tempD = pi_count << 0;
  isbM32m(DDRPHY, reg, tempD, msk);

  // error check (shouldn't go above 0x3F)
  if (pi_count > 0x47)
  {
    training_message(channel, rank, byte_lane);
    post_code(0xEE, 0xE1);
  }

  LEAVEFN();
  return;
}

// get_rdqs:
//
// This function will return the current RDQS delay on the given channel, rank, byte_lane as an absolute PI count.
// (currently doesn't comprehend rank)
uint32_t get_rdqs(
    uint8_t channel,
    uint8_t rank,
    uint8_t byte_lane)
{
  uint32_t reg;
  uint32_t tempD;
  uint32_t pi_count;

  ENTERFN();

  // PI (1/128 MCLK)
  // BL0 -> B0RXDQSPICODE[06:00] (0x00-0x47)
  // BL1 -> B1RXDQSPICODE[06:00] (0x00-0x47)
  reg = (byte_lane & BIT0) ? (B1RXDQSPICODE) : (B0RXDQSPICODE);
  reg += (((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET));
  tempD = isbR32m(DDRPHY, reg);

  // Adjust PI_COUNT
  pi_count = tempD & 0x7F;

  LEAVEFN();
  return pi_count;
}

// set_wdqs:
//
// This function will program the WDQS delays based on an absolute amount of PIs.
// (currently doesn't comprehend rank)
void set_wdqs(
    uint8_t channel,
    uint8_t rank,
    uint8_t byte_lane,
    uint32_t pi_count)
{
  uint32_t reg;
  uint32_t msk;
  uint32_t tempD;

  ENTERFN();
  DPF(D_TRN, "Wdqs ch%d rnk%d ln%d : pi=%03X\n", channel, rank, byte_lane, pi_count);

  // RDPTR (1/2 MCLK, 64 PIs)
  // BL0 -> B01PTRCTL0[07:04] (0x0-0xF)
  // BL1 -> B01PTRCTL0[19:16] (0x0-0xF)
  reg = B01PTRCTL0 + ((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET);
  msk = (byte_lane & BIT0) ? (BIT19 | BIT18 | BIT17 | BIT16) : (BIT7 | BIT6 | BIT5 | BIT4);
  tempD = pi_count / HALF_CLK;
  tempD <<= (byte_lane & BIT0) ? (16) : (4);
  isbM32m(DDRPHY, reg, tempD, msk);

  // Adjust PI_COUNT
  pi_count -= ((pi_count / HALF_CLK) & 0xF) * HALF_CLK;

  // PI (1/64 MCLK, 1 PIs)
  // BL0 -> B0DLLPICODER0[21:16] (0x00-0x3F)
  // BL1 -> B1DLLPICODER0[21:16] (0x00-0x3F)
  reg = (byte_lane & BIT0) ? (B1DLLPICODER0) : (B0DLLPICODER0);
  reg += (((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET));
  msk = (BIT21 | BIT20 | BIT19 | BIT18 | BIT17 | BIT16);
  tempD = pi_count << 16;
  isbM32m(DDRPHY, reg, tempD, msk);

  // DEADBAND
  // BL0/1 -> B01DBCTL1[07/10] (+1 select)
  // BL0/1 -> B01DBCTL1[01/04] (enable)
  reg = B01DBCTL1 + ((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET);
  msk = 0x00;
  tempD = 0x00;
  // enable
  msk |= (byte_lane & BIT0) ? (BIT4) : (BIT1);
  if ((pi_count < EARLY_DB) || (pi_count > LATE_DB))
  {
    tempD |= msk;
  }
  // select
  msk |= (byte_lane & BIT0) ? (BIT10) : (BIT7);
  if (pi_count < EARLY_DB)
  {
    tempD |= msk;
  }
  isbM32m(DDRPHY, reg, tempD, msk);

  // error check
  if (pi_count > 0x3F)
  {
    training_message(channel, rank, byte_lane);
    post_code(0xEE, 0xE2);
  }

  LEAVEFN();
  return;
}

// get_wdqs:
//
// This function will return the amount of WDQS delay on the given channel, rank, byte_lane as an absolute PI count.
// (currently doesn't comprehend rank)
uint32_t get_wdqs(
    uint8_t channel,
    uint8_t rank,
    uint8_t byte_lane)
{
  uint32_t reg;
  uint32_t tempD;
  uint32_t pi_count;

  ENTERFN();

  // RDPTR (1/2 MCLK, 64 PIs)
  // BL0 -> B01PTRCTL0[07:04] (0x0-0xF)
  // BL1 -> B01PTRCTL0[19:16] (0x0-0xF)
  reg = B01PTRCTL0 + ((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET);
  tempD = isbR32m(DDRPHY, reg);
  tempD >>= (byte_lane & BIT0) ? (16) : (4);
  tempD &= 0xF;

  // Adjust PI_COUNT
  pi_count = (tempD * HALF_CLK);

  // PI (1/64 MCLK, 1 PIs)
  // BL0 -> B0DLLPICODER0[21:16] (0x00-0x3F)
  // BL1 -> B1DLLPICODER0[21:16] (0x00-0x3F)
  reg = (byte_lane & BIT0) ? (B1DLLPICODER0) : (B0DLLPICODER0);
  reg += (((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET));
  tempD = isbR32m(DDRPHY, reg);
  tempD >>= 16;
  tempD &= 0x3F;

  // Adjust PI_COUNT
  pi_count += tempD;

  LEAVEFN();
  return pi_count;
}

// set_wdq:
//
// This function will program the WDQ delays based on an absolute number of PIs.
// (currently doesn't comprehend rank)
void set_wdq(
    uint8_t channel,
    uint8_t rank,
    uint8_t byte_lane,
    uint32_t pi_count)
{
  uint32_t reg;
  uint32_t msk;
  uint32_t tempD;

  ENTERFN();
  DPF(D_TRN, "Wdq ch%d rnk%d ln%d : pi=%03X\n", channel, rank, byte_lane, pi_count);

  // RDPTR (1/2 MCLK, 64 PIs)
  // BL0 -> B01PTRCTL0[03:00] (0x0-0xF)
  // BL1 -> B01PTRCTL0[15:12] (0x0-0xF)
  reg = B01PTRCTL0 + ((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET);
  msk = (byte_lane & BIT0) ? (BIT15 | BIT14 | BIT13 | BIT12) : (BIT3 | BIT2 | BIT1 | BIT0);
  tempD = pi_count / HALF_CLK;
  tempD <<= (byte_lane & BIT0) ? (12) : (0);
  isbM32m(DDRPHY, reg, tempD, msk);

  // Adjust PI_COUNT
  pi_count -= ((pi_count / HALF_CLK) & 0xF) * HALF_CLK;

  // PI (1/64 MCLK, 1 PIs)
  // BL0 -> B0DLLPICODER0[13:08] (0x00-0x3F)
  // BL1 -> B1DLLPICODER0[13:08] (0x00-0x3F)
  reg = (byte_lane & BIT0) ? (B1DLLPICODER0) : (B0DLLPICODER0);
  reg += (((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET));
  msk = (BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8);
  tempD = pi_count << 8;
  isbM32m(DDRPHY, reg, tempD, msk);

  // DEADBAND
  // BL0/1 -> B01DBCTL1[06/09] (+1 select)
  // BL0/1 -> B01DBCTL1[00/03] (enable)
  reg = B01DBCTL1 + ((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET);
  msk = 0x00;
  tempD = 0x00;
  // enable
  msk |= (byte_lane & BIT0) ? (BIT3) : (BIT0);
  if ((pi_count < EARLY_DB) || (pi_count > LATE_DB))
  {
    tempD |= msk;
  }
  // select
  msk |= (byte_lane & BIT0) ? (BIT9) : (BIT6);
  if (pi_count < EARLY_DB)
  {
    tempD |= msk;
  }
  isbM32m(DDRPHY, reg, tempD, msk);

  // error check
  if (pi_count > 0x3F)
  {
    training_message(channel, rank, byte_lane);
    post_code(0xEE, 0xE3);
  }

  LEAVEFN();
  return;
}

// get_wdq:
//
// This function will return the amount of WDQ delay on the given channel, rank, byte_lane as an absolute PI count.
// (currently doesn't comprehend rank)
uint32_t get_wdq(
    uint8_t channel,
    uint8_t rank,
    uint8_t byte_lane)
{
  uint32_t reg;
  uint32_t tempD;
  uint32_t pi_count;

  ENTERFN();

  // RDPTR (1/2 MCLK, 64 PIs)
  // BL0 -> B01PTRCTL0[03:00] (0x0-0xF)
  // BL1 -> B01PTRCTL0[15:12] (0x0-0xF)
  reg = B01PTRCTL0 + ((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET);
  tempD = isbR32m(DDRPHY, reg);
  tempD >>= (byte_lane & BIT0) ? (12) : (0);
  tempD &= 0xF;

  // Adjust PI_COUNT
  pi_count = (tempD * HALF_CLK);

  // PI (1/64 MCLK, 1 PIs)
  // BL0 -> B0DLLPICODER0[13:08] (0x00-0x3F)
  // BL1 -> B1DLLPICODER0[13:08] (0x00-0x3F)
  reg = (byte_lane & BIT0) ? (B1DLLPICODER0) : (B0DLLPICODER0);
  reg += (((byte_lane >> 1) * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET));
  tempD = isbR32m(DDRPHY, reg);
  tempD >>= 8;
  tempD &= 0x3F;

  // Adjust PI_COUNT
  pi_count += tempD;

  LEAVEFN();
  return pi_count;
}

// set_wcmd:
//
// This function will program the WCMD delays based on an absolute number of PIs.
void set_wcmd(
    uint8_t channel,
    uint32_t pi_count)
{
  uint32_t reg;
  uint32_t msk;
  uint32_t tempD;

  ENTERFN();
  // RDPTR (1/2 MCLK, 64 PIs)
  // CMDPTRREG[11:08] (0x0-0xF)
  reg = CMDPTRREG + (channel * DDRIOCCC_CH_OFFSET);
  msk = (BIT11 | BIT10 | BIT9 | BIT8);
  tempD = pi_count / HALF_CLK;
  tempD <<= 8;
  isbM32m(DDRPHY, reg, tempD, msk);

  // Adjust PI_COUNT
  pi_count -= ((pi_count / HALF_CLK) & 0xF) * HALF_CLK;

  // PI (1/64 MCLK, 1 PIs)
  // CMDDLLPICODER0[29:24] -> CMDSLICE R3 (unused)
  // CMDDLLPICODER0[21:16] -> CMDSLICE L3 (unused)
  // CMDDLLPICODER0[13:08] -> CMDSLICE R2 (unused)
  // CMDDLLPICODER0[05:00] -> CMDSLICE L2 (unused)
  // CMDDLLPICODER1[29:24] -> CMDSLICE R1 (unused)
  // CMDDLLPICODER1[21:16] -> CMDSLICE L1 (0x00-0x3F)
  // CMDDLLPICODER1[13:08] -> CMDSLICE R0 (unused)
  // CMDDLLPICODER1[05:00] -> CMDSLICE L0 (unused)
  reg = CMDDLLPICODER1 + (channel * DDRIOCCC_CH_OFFSET);

  msk = (BIT29 | BIT28 | BIT27 | BIT26 | BIT25 | BIT24) | (BIT21 | BIT20 | BIT19 | BIT18 | BIT17 | BIT16)
      | (BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8) | (BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0);

  tempD = (pi_count << 24) | (pi_count << 16) | (pi_count << 8) | (pi_count << 0);

  isbM32m(DDRPHY, reg, tempD, msk);
  reg = CMDDLLPICODER0 + (channel * DDRIOCCC_CH_OFFSET); // PO
  isbM32m(DDRPHY, reg, tempD, msk);

  // DEADBAND
  // CMDCFGREG0[17] (+1 select)
  // CMDCFGREG0[16] (enable)
  reg = CMDCFGREG0 + (channel * DDRIOCCC_CH_OFFSET);
  msk = 0x00;
  tempD = 0x00;
  // enable
  msk |= BIT16;
  if ((pi_count < EARLY_DB) || (pi_count > LATE_DB))
  {
    tempD |= msk;
  }
  // select
  msk |= BIT17;
  if (pi_count < EARLY_DB)
  {
    tempD |= msk;
  }
  isbM32m(DDRPHY, reg, tempD, msk);

  // error check
  if (pi_count > 0x3F)
  {
    post_code(0xEE, 0xE4);
  }

  LEAVEFN();
  return;
}

// get_wcmd:
//
// This function will return the amount of WCMD delay on the given channel as an absolute PI count.
uint32_t get_wcmd(
    uint8_t channel)
{
  uint32_t reg;
  uint32_t tempD;
  uint32_t pi_count;

  ENTERFN();
  // RDPTR (1/2 MCLK, 64 PIs)
  // CMDPTRREG[11:08] (0x0-0xF)
  reg = CMDPTRREG + (channel * DDRIOCCC_CH_OFFSET);
  tempD = isbR32m(DDRPHY, reg);
  tempD >>= 8;
  tempD &= 0xF;

  // Adjust PI_COUNT
  pi_count = tempD * HALF_CLK;

  // PI (1/64 MCLK, 1 PIs)
  // CMDDLLPICODER0[29:24] -> CMDSLICE R3 (unused)
  // CMDDLLPICODER0[21:16] -> CMDSLICE L3 (unused)
  // CMDDLLPICODER0[13:08] -> CMDSLICE R2 (unused)
  // CMDDLLPICODER0[05:00] -> CMDSLICE L2 (unused)
  // CMDDLLPICODER1[29:24] -> CMDSLICE R1 (unused)
  // CMDDLLPICODER1[21:16] -> CMDSLICE L1 (0x00-0x3F)
  // CMDDLLPICODER1[13:08] -> CMDSLICE R0 (unused)
  // CMDDLLPICODER1[05:00] -> CMDSLICE L0 (unused)
  reg = CMDDLLPICODER1 + (channel * DDRIOCCC_CH_OFFSET);
  tempD = isbR32m(DDRPHY, reg);
  tempD >>= 16;
  tempD &= 0x3F;

  // Adjust PI_COUNT
  pi_count += tempD;

  LEAVEFN();
  return pi_count;
}

// set_wclk:
//
// This function will program the WCLK delays based on an absolute number of PIs.
void set_wclk(
    uint8_t channel,
    uint8_t rank,
    uint32_t pi_count)
{
  uint32_t reg;
  uint32_t msk;
  uint32_t tempD;

  ENTERFN();
  // RDPTR (1/2 MCLK, 64 PIs)
  // CCPTRREG[15:12] -> CLK1 (0x0-0xF)
  // CCPTRREG[11:08] -> CLK0 (0x0-0xF)
  reg = CCPTRREG + (channel * DDRIOCCC_CH_OFFSET);
  msk = (BIT15 | BIT14 | BIT13 | BIT12) | (BIT11 | BIT10 | BIT9 | BIT8);
  tempD = ((pi_count / HALF_CLK) << 12) | ((pi_count / HALF_CLK) << 8);
  isbM32m(DDRPHY, reg, tempD, msk);

  // Adjust PI_COUNT
  pi_count -= ((pi_count / HALF_CLK) & 0xF) * HALF_CLK;

  // PI (1/64 MCLK, 1 PIs)
  // ECCB1DLLPICODER0[13:08] -> CLK0 (0x00-0x3F)
  // ECCB1DLLPICODER0[21:16] -> CLK1 (0x00-0x3F)
  reg = (rank) ? (ECCB1DLLPICODER0) : (ECCB1DLLPICODER0);
  reg += (channel * DDRIOCCC_CH_OFFSET);
  msk = (BIT21 | BIT20 | BIT19 | BIT18 | BIT17 | BIT16) | (BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8);
  tempD = (pi_count << 16) | (pi_count << 8);
  isbM32m(DDRPHY, reg, tempD, msk);
  reg = (rank) ? (ECCB1DLLPICODER1) : (ECCB1DLLPICODER1);
  reg += (channel * DDRIOCCC_CH_OFFSET);
  isbM32m(DDRPHY, reg, tempD, msk);
  reg = (rank) ? (ECCB1DLLPICODER2) : (ECCB1DLLPICODER2);
  reg += (channel * DDRIOCCC_CH_OFFSET);
  isbM32m(DDRPHY, reg, tempD, msk);
  reg = (rank) ? (ECCB1DLLPICODER3) : (ECCB1DLLPICODER3);
  reg += (channel * DDRIOCCC_CH_OFFSET);
  isbM32m(DDRPHY, reg, tempD, msk);

  // DEADBAND
  // CCCFGREG1[11:08] (+1 select)
  // CCCFGREG1[03:00] (enable)
  reg = CCCFGREG1 + (channel * DDRIOCCC_CH_OFFSET);
  msk = 0x00;
  tempD = 0x00;
  // enable
  msk |= (BIT3 | BIT2 | BIT1 | BIT0); // only ??? matters
  if ((pi_count < EARLY_DB) || (pi_count > LATE_DB))
  {
    tempD |= msk;
  }
  // select
  msk |= (BIT11 | BIT10 | BIT9 | BIT8); // only ??? matters
  if (pi_count < EARLY_DB)
  {
    tempD |= msk;
  }
  isbM32m(DDRPHY, reg, tempD, msk);

  // error check
  if (pi_count > 0x3F)
  {
    post_code(0xEE, 0xE5);
  }

  LEAVEFN();
  return;
}

// get_wclk:
//
// This function will return the amout of WCLK delay on the given channel, rank as an absolute PI count.
uint32_t get_wclk(
    uint8_t channel,
    uint8_t rank)
{
  uint32_t reg;
  uint32_t tempD;
  uint32_t pi_count;

  ENTERFN();
  // RDPTR (1/2 MCLK, 64 PIs)
  // CCPTRREG[15:12] -> CLK1 (0x0-0xF)
  // CCPTRREG[11:08] -> CLK0 (0x0-0xF)
  reg = CCPTRREG + (channel * DDRIOCCC_CH_OFFSET);
  tempD = isbR32m(DDRPHY, reg);
  tempD >>= (rank) ? (12) : (8);
  tempD &= 0xF;

  // Adjust PI_COUNT
  pi_count = tempD * HALF_CLK;

  // PI (1/64 MCLK, 1 PIs)
  // ECCB1DLLPICODER0[13:08] -> CLK0 (0x00-0x3F)
  // ECCB1DLLPICODER0[21:16] -> CLK1 (0x00-0x3F)
  reg = (rank) ? (ECCB1DLLPICODER0) : (ECCB1DLLPICODER0);
  reg += (channel * DDRIOCCC_CH_OFFSET);
  tempD = isbR32m(DDRPHY, reg);
  tempD >>= (rank) ? (16) : (8);
  tempD &= 0x3F;

  pi_count += tempD;

  LEAVEFN();
  return pi_count;
}

// set_wctl:
//
// This function will program the WCTL delays based on an absolute number of PIs.
// (currently doesn't comprehend rank)
void set_wctl(
    uint8_t channel,
    uint8_t rank,
    uint32_t pi_count)
{
  uint32_t reg;
  uint32_t msk;
  uint32_t tempD;

  ENTERFN();

  // RDPTR (1/2 MCLK, 64 PIs)
  // CCPTRREG[31:28] (0x0-0xF)
  // CCPTRREG[27:24] (0x0-0xF)
  reg = CCPTRREG + (channel * DDRIOCCC_CH_OFFSET);
  msk = (BIT31 | BIT30 | BIT29 | BIT28) | (BIT27 | BIT26 | BIT25 | BIT24);
  tempD = ((pi_count / HALF_CLK) << 28) | ((pi_count / HALF_CLK) << 24);
  isbM32m(DDRPHY, reg, tempD, msk);

  // Adjust PI_COUNT
  pi_count -= ((pi_count / HALF_CLK) & 0xF) * HALF_CLK;

  // PI (1/64 MCLK, 1 PIs)
  // ECCB1DLLPICODER?[29:24] (0x00-0x3F)
  // ECCB1DLLPICODER?[29:24] (0x00-0x3F)
  reg = ECCB1DLLPICODER0 + (channel * DDRIOCCC_CH_OFFSET);
  msk = (BIT29 | BIT28 | BIT27 | BIT26 | BIT25 | BIT24);
  tempD = (pi_count << 24);
  isbM32m(DDRPHY, reg, tempD, msk);
  reg = ECCB1DLLPICODER1 + (channel * DDRIOCCC_CH_OFFSET);
  isbM32m(DDRPHY, reg, tempD, msk);
  reg = ECCB1DLLPICODER2 + (channel * DDRIOCCC_CH_OFFSET);
  isbM32m(DDRPHY, reg, tempD, msk);
  reg = ECCB1DLLPICODER3 + (channel * DDRIOCCC_CH_OFFSET);
  isbM32m(DDRPHY, reg, tempD, msk);

  // DEADBAND
  // CCCFGREG1[13:12] (+1 select)
  // CCCFGREG1[05:04] (enable)
  reg = CCCFGREG1 + (channel * DDRIOCCC_CH_OFFSET);
  msk = 0x00;
  tempD = 0x00;
  // enable
  msk |= (BIT5 | BIT4); // only ??? matters
  if ((pi_count < EARLY_DB) || (pi_count > LATE_DB))
  {
    tempD |= msk;
  }
  // select
  msk |= (BIT13 | BIT12); // only ??? matters
  if (pi_count < EARLY_DB)
  {
    tempD |= msk;
  }
  isbM32m(DDRPHY, reg, tempD, msk);

  // error check
  if (pi_count > 0x3F)
  {
    post_code(0xEE, 0xE6);
  }

  LEAVEFN();
  return;
}

// get_wctl:
//
// This function will return the amount of WCTL delay on the given channel, rank as an absolute PI count.
// (currently doesn't comprehend rank)
uint32_t get_wctl(
    uint8_t channel,
    uint8_t rank)
{
  uint32_t reg;
  uint32_t tempD;
  uint32_t pi_count;

  ENTERFN();

  // RDPTR (1/2 MCLK, 64 PIs)
  // CCPTRREG[31:28] (0x0-0xF)
  // CCPTRREG[27:24] (0x0-0xF)
  reg = CCPTRREG + (channel * DDRIOCCC_CH_OFFSET);
  tempD = isbR32m(DDRPHY, reg);
  tempD >>= 24;
  tempD &= 0xF;

  // Adjust PI_COUNT
  pi_count = tempD * HALF_CLK;

  // PI (1/64 MCLK, 1 PIs)
  // ECCB1DLLPICODER?[29:24] (0x00-0x3F)
  // ECCB1DLLPICODER?[29:24] (0x00-0x3F)
  reg = ECCB1DLLPICODER0 + (channel * DDRIOCCC_CH_OFFSET);
  tempD = isbR32m(DDRPHY, reg);
  tempD >>= 24;
  tempD &= 0x3F;

  // Adjust PI_COUNT
  pi_count += tempD;

  LEAVEFN();
  return pi_count;
}

// set_vref:
//
// This function will program the internal Vref setting in a given byte lane in a given channel.
void set_vref(
    uint8_t channel,
    uint8_t byte_lane,
    uint32_t setting)
{
  uint32_t reg = (byte_lane & 0x1) ? (B1VREFCTL) : (B0VREFCTL);

  ENTERFN();
  DPF(D_TRN, "Vref ch%d ln%d : val=%03X\n", channel, byte_lane, setting);

  isbM32m(DDRPHY, (reg + (channel * DDRIODQ_CH_OFFSET) + ((byte_lane >> 1) * DDRIODQ_BL_OFFSET)),
      (vref_codes[setting] << 2), (BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2));
  //isbM32m(DDRPHY, (reg + (channel * DDRIODQ_CH_OFFSET) + ((byte_lane >> 1) * DDRIODQ_BL_OFFSET)), (setting<<2), (BIT7|BIT6|BIT5|BIT4|BIT3|BIT2));
  // need to wait ~300ns for Vref to settle (check that this is necessary)
  delay_n(300);
  // ??? may need to clear pointers ???
  LEAVEFN();
  return;
}

// get_vref:
//
// This function will return the internal Vref setting for the given channel, byte_lane;
uint32_t get_vref(
    uint8_t channel,
    uint8_t byte_lane)
{
  uint8_t j;
  uint32_t ret_val = sizeof(vref_codes) / 2;
  uint32_t reg = (byte_lane & 0x1) ? (B1VREFCTL) : (B0VREFCTL);

  uint32_t tempD;

  ENTERFN();
  tempD = isbR32m(DDRPHY, (reg + (channel * DDRIODQ_CH_OFFSET) + ((byte_lane >> 1) * DDRIODQ_BL_OFFSET)));
  tempD >>= 2;
  tempD &= 0x3F;
  for (j = 0; j < sizeof(vref_codes); j++)
  {
    if (vref_codes[j] == tempD)
    {
      ret_val = j;
      break;
    }
  }
  LEAVEFN();
  return ret_val;
}

// clear_pointers:
//
// This function will be used to clear the pointers in a given byte lane in a given channel.
void clear_pointers(
    void)
{
  uint8_t channel_i;
  uint8_t bl_i;

  ENTERFN();
  for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
  {
    for (bl_i = 0; bl_i < NUM_BYTE_LANES; bl_i++)
    {
      isbM32m(DDRPHY, (B01PTRCTL1 + (channel_i * DDRIODQ_CH_OFFSET) + ((bl_i >> 1) * DDRIODQ_BL_OFFSET)), ~(BIT8),
          (BIT8));
      //delay_m(1); // DEBUG
      isbM32m(DDRPHY, (B01PTRCTL1 + (channel_i * DDRIODQ_CH_OFFSET) + ((bl_i >> 1) * DDRIODQ_BL_OFFSET)), (BIT8),
          (BIT8));
    }
  }
  LEAVEFN();
  return;
}

// void enable_cache:
void enable_cache(
    void)
{
  // Cache control not used in Quark MRC
  return;
}

// void disable_cache:
void disable_cache(
    void)
{
  // Cache control not used in Quark MRC
  return;
}

// Send DRAM command, data should be formated
// using DCMD_Xxxx macro or emrsXCommand structure.
static void dram_init_command(
    uint32_t data)
{
  Wr32(DCMD, 0, data);
}

// find_rising_edge:
//
// This function will find the rising edge transition on RCVN or WDQS.
void find_rising_edge(
    MRCParams_t *mrc_params,
    uint32_t delay[],
    uint8_t channel,
    uint8_t rank,
    bool rcvn)
{

#define SAMPLE_CNT 3   // number of sample points
#define SAMPLE_DLY 26  // number of PIs to increment per sample
#define FORWARD true   // indicates to increase delays when looking for edge
#define BACKWARD false // indicates to decrease delays when looking for edge

  bool all_edges_found; // determines stop condition
  bool direction[NUM_BYTE_LANES]; // direction indicator
  uint8_t sample_i; // sample counter
  uint8_t bl_i; // byte lane counter
  uint8_t bl_divisor = (mrc_params->channel_width == x16) ? 2 : 1; // byte lane divisor
  uint32_t sample_result[SAMPLE_CNT]; // results of "sample_dqs()"
  uint32_t tempD; // temporary DWORD
  uint32_t transition_pattern;

  ENTERFN();

  // select hte and request initial configuration
  select_hte(mrc_params);
  first_run = 1;

  // Take 3 sample points (T1,T2,T3) to obtain a transition pattern.
  for (sample_i = 0; sample_i < SAMPLE_CNT; sample_i++)
  {
    // program the desired delays for sample
    for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
    {
      // increase sample delay by 26 PI (0.2 CLK)
      if (rcvn)
      {
        set_rcvn(channel, rank, bl_i, delay[bl_i] + (sample_i * SAMPLE_DLY));
      }
      else
      {
        set_wdqs(channel, rank, bl_i, delay[bl_i] + (sample_i * SAMPLE_DLY));
      }
    } // bl_i loop
    // take samples (Tsample_i)
    sample_result[sample_i] = sample_dqs(mrc_params, channel, rank, rcvn);

    DPF(D_TRN, "Find rising edge %s ch%d rnk%d: #%d dly=%d dqs=%02X\n",
        (rcvn ? "RCVN" : "WDQS"), channel, rank,
        sample_i, sample_i * SAMPLE_DLY, sample_result[sample_i]);

  } // sample_i loop

  // This pattern will help determine where we landed and ultimately how to place RCVEN/WDQS.
  for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
  {
    // build "transition_pattern" (MSB is 1st sample)
    transition_pattern = 0x00;
    for (sample_i = 0; sample_i < SAMPLE_CNT; sample_i++)
    {
      transition_pattern |= ((sample_result[sample_i] & (1 << bl_i)) >> bl_i) << (SAMPLE_CNT - 1 - sample_i);
    } // sample_i loop

    DPF(D_TRN, "=== transition pattern %d\n", transition_pattern);

    // set up to look for rising edge based on "transition_pattern"
    switch (transition_pattern)
    {
    case 0x00: // sampled 0->0->0
      // move forward from T3 looking for 0->1
      delay[bl_i] += 2 * SAMPLE_DLY;
      direction[bl_i] = FORWARD;
      break;
    case 0x01: // sampled 0->0->1
    case 0x05: // sampled 1->0->1 (bad duty cycle) *HSD#237503*
      // move forward from T2 looking for 0->1
      delay[bl_i] += 1 * SAMPLE_DLY;
      direction[bl_i] = FORWARD;
      break;
// HSD#237503
//      case 0x02: // sampled 0->1->0 (bad duty cycle)
//        training_message(channel, rank, bl_i);
//        post_code(0xEE, 0xE8);
//        break;
    case 0x02: // sampled 0->1->0 (bad duty cycle) *HSD#237503*
    case 0x03: // sampled 0->1->1
      // move forward from T1 looking for 0->1
      delay[bl_i] += 0 * SAMPLE_DLY;
      direction[bl_i] = FORWARD;
      break;
    case 0x04: // sampled 1->0->0 (assumes BL8, HSD#234975)
      // move forward from T3 looking for 0->1
      delay[bl_i] += 2 * SAMPLE_DLY;
      direction[bl_i] = FORWARD;
      break;
// HSD#237503
//      case 0x05: // sampled 1->0->1 (bad duty cycle)
//        training_message(channel, rank, bl_i);
//        post_code(0xEE, 0xE9);
//        break;
    case 0x06: // sampled 1->1->0
    case 0x07: // sampled 1->1->1
      // move backward from T1 looking for 1->0
      delay[bl_i] += 0 * SAMPLE_DLY;
      direction[bl_i] = BACKWARD;
      break;
    default:
      post_code(0xEE, 0xEE);
      break;
    } // transition_pattern switch
    // program delays
    if (rcvn)
    {
      set_rcvn(channel, rank, bl_i, delay[bl_i]);
    }
    else
    {
      set_wdqs(channel, rank, bl_i, delay[bl_i]);
    }
  } // bl_i loop

  // Based on the observed transition pattern on the byte lane,
  // begin looking for a rising edge with single PI granularity.
  do
  {
    all_edges_found = true; // assume all byte lanes passed
    tempD = sample_dqs(mrc_params, channel, rank, rcvn); // take a sample
    // check all each byte lane for proper edge
    for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
    {
      if (tempD & (1 << bl_i))
      {
        // sampled "1"
        if (direction[bl_i] == BACKWARD)
        {
          // keep looking for edge on this byte lane
          all_edges_found = false;
          delay[bl_i] -= 1;
          if (rcvn)
          {
            set_rcvn(channel, rank, bl_i, delay[bl_i]);
          }
          else
          {
            set_wdqs(channel, rank, bl_i, delay[bl_i]);
          }
        }
      }
      else
      {
        // sampled "0"
        if (direction[bl_i] == FORWARD)
        {
          // keep looking for edge on this byte lane
          all_edges_found = false;
          delay[bl_i] += 1;
          if (rcvn)
          {
            set_rcvn(channel, rank, bl_i, delay[bl_i]);
          }
          else
          {
            set_wdqs(channel, rank, bl_i, delay[bl_i]);
          }
        }
      }
    } // bl_i loop
  } while (!all_edges_found);

  // restore DDR idle state
  dram_init_command(DCMD_PREA(rank));

  DPF(D_TRN, "Delay %03X %03X %03X %03X\n",
      delay[0], delay[1], delay[2], delay[3]);

  LEAVEFN();
  return;
}

// sample_dqs:
//
// This function will sample the DQTRAINSTS registers in the given channel/rank SAMPLE_SIZE times looking for a valid '0' or '1'.
// It will return an encoded DWORD in which each bit corresponds to the sampled value on the byte lane.
uint32_t sample_dqs(
    MRCParams_t *mrc_params,
    uint8_t channel,
    uint8_t rank,
    bool rcvn)
{
  uint8_t j; // just a counter
  uint8_t bl_i; // which BL in the module (always 2 per module)
  uint8_t bl_grp; // which BL module
  uint8_t bl_divisor = (mrc_params->channel_width == x16) ? 2 : 1; // byte lane divisor
  uint32_t msk[2]; // BLx in module
  uint32_t sampled_val[SAMPLE_SIZE]; // DQTRAINSTS register contents for each sample
  uint32_t num_0s; // tracks the number of '0' samples
  uint32_t num_1s; // tracks the number of '1' samples
  uint32_t ret_val = 0x00; // assume all '0' samples
  uint32_t address = get_addr(mrc_params, channel, rank);

  // initialise "msk[]"
  msk[0] = (rcvn) ? (BIT1) : (BIT9); // BL0
  msk[1] = (rcvn) ? (BIT0) : (BIT8); // BL1


  // cycle through each byte lane group
  for (bl_grp = 0; bl_grp < (NUM_BYTE_LANES / bl_divisor) / 2; bl_grp++)
  {
    // take SAMPLE_SIZE samples
    for (j = 0; j < SAMPLE_SIZE; j++)
    {
      HteMemOp(address, first_run, rcvn?0:1);
      first_run = 0;

      // record the contents of the proper DQTRAINSTS register
      sampled_val[j] = isbR32m(DDRPHY, (DQTRAINSTS + (bl_grp * DDRIODQ_BL_OFFSET) + (channel * DDRIODQ_CH_OFFSET)));
    }
    // look for a majority value ( (SAMPLE_SIZE/2)+1 ) on the byte lane
    // and set that value in the corresponding "ret_val" bit
    for (bl_i = 0; bl_i < 2; bl_i++)
    {
      num_0s = 0x00; // reset '0' tracker for byte lane
      num_1s = 0x00; // reset '1' tracker for byte lane
      for (j = 0; j < SAMPLE_SIZE; j++)
      {
        if (sampled_val[j] & msk[bl_i])
        {
          num_1s++;
        }
        else
        {
          num_0s++;
        }
      }
      if (num_1s > num_0s)
      {
        ret_val |= (1 << (bl_i + (bl_grp * 2)));
      }
    }
  }

  // "ret_val.0" contains the status of BL0
  // "ret_val.1" contains the status of BL1
  // "ret_val.2" contains the status of BL2
  // etc.
  return ret_val;
}

// get_addr:
//
// This function will return a 32 bit address in the desired channel and rank.
uint32_t get_addr(
    MRCParams_t *mrc_params,
    uint8_t channel,
    uint8_t rank)
{
  uint32_t offset = 0x02000000; // 32MB

  // Begin product specific code
  if (channel > 0)
  {
    DPF(D_ERROR, "ILLEGAL CHANNEL\n");
    DEAD_LOOP();
  }

  if (rank > 1)
  {
    DPF(D_ERROR, "ILLEGAL RANK\n");
    DEAD_LOOP();
  }

  // use 256MB lowest density as per DRP == 0x0003
  offset += rank * (256 * 1024 * 1024);

  return offset;
}

// byte_lane_mask:
//
// This function will return a 32 bit mask that will be used to check for byte lane failures.
uint32_t byte_lane_mask(
    MRCParams_t *mrc_params)
{
  uint32_t j;
  uint32_t ret_val = 0x00;

  // set "ret_val" based on NUM_BYTE_LANES such that you will check only BL0 in "result"
  // (each bit in "result" represents a byte lane)
  for (j = 0; j < MAX_BYTE_LANES; j += NUM_BYTE_LANES)
  {
    ret_val |= (1 << ((j / NUM_BYTE_LANES) * NUM_BYTE_LANES));
  }

  // HSD#235037
  // need to adjust the mask for 16-bit mode
  if (mrc_params->channel_width == x16)
  {
    ret_val |= (ret_val << 2);
  }

  return ret_val;
}


// read_tsc:
//
// This function will do some assembly to return TSC register contents as a uint64_t.
uint64_t read_tsc(
    void)
{
  volatile uint64_t tsc;  // EDX:EAX

#if defined (SIM) || defined (GCC)
  volatile uint32_t tscH; // EDX
  volatile uint32_t tscL;// EAX

  asm("rdtsc":"=a"(tscL),"=d"(tscH));
  tsc = tscH;
  tsc = (tsc<<32)|tscL;
#else
  tsc = __rdtsc();
#endif

  return tsc;
}

// get_tsc_freq:
//
// This function returns the TSC frequency in MHz
uint32_t get_tsc_freq(
    void)
{
  static uint32_t freq[] =
  { 533, 400, 200, 100 };
  uint32_t fuse;
#if 0
  fuse = (isbR32m(FUSE, 0) >> 12) & (BIT1|BIT0);
#else
  // todo!!! Fixed 533MHz for emulation or debugging
  fuse = 0;
#endif
  return freq[fuse];
}

#ifndef SIM
// delay_n:
//
// This is a simple delay function.
// It takes "nanoseconds" as a parameter.
void delay_n(
    uint32_t nanoseconds)
{
  // 1000 MHz clock has 1ns period --> no conversion required
  uint64_t final_tsc = read_tsc();
  final_tsc += ((get_tsc_freq() * (nanoseconds)) / 1000);

  while (read_tsc() < final_tsc)
    ;
  return;
}
#endif

// delay_u:
//
// This is a simple delay function.
// It takes "microseconds as a parameter.
void delay_u(
    uint32_t microseconds)
{
  // 64 bit math is not an option, just use loops
  while (microseconds--)
  {
    delay_n(1000);
  }
  return;
}

// delay_m:
//
// This is a simple delay function.
// It takes "milliseconds" as a parameter.
void delay_m(
    uint32_t milliseconds)
{
  // 64 bit math is not an option, just use loops
  while (milliseconds--)
  {
    delay_u(1000);
  }
  return;
}

// delay_s:
//
// This is a simple delay function.
// It takes "seconds" as a parameter.
void delay_s(
    uint32_t seconds)
{
  // 64 bit math is not an option, just use loops
  while (seconds--)
  {
    delay_m(1000);
  }
  return;
}

// post_code:
//
// This function will output the POST CODE to the four 7-Segment LED displays.
void post_code(
    uint8_t major,
    uint8_t minor)
{
#ifdef EMU
  // Update global variable for execution tracking in debug env
  PostCode = ((major << 8) | minor);
#endif

  // send message to UART
  DPF(D_INFO, "POST: 0x%01X%02X\n", major, minor);

  // error check:
  if (major == 0xEE)
  {
    // todo!!! Consider updating error status and exit MRC
#ifdef SIM
    // enable Ctrl-C handling
    for(;;) delay_n(100);
#else
    DEAD_LOOP();
#endif
  }
}

void training_message(
    uint8_t channel,
    uint8_t rank,
    uint8_t byte_lane)
{
  // send message to UART
  DPF(D_INFO, "CH%01X RK%01X BL%01X\n", channel, rank, byte_lane);
  return;
}

void print_timings(
    MRCParams_t *mrc_params)
{
  uint8_t algo_i;
  uint8_t channel_i;
  uint8_t rank_i;
  uint8_t bl_i;
  uint8_t bl_divisor = (mrc_params->channel_width == x16) ? 2 : 1;

  DPF(D_INFO, "\n---------------------------");
  DPF(D_INFO, "\nALGO[CH:RK] BL0 BL1 BL2 BL3");
  DPF(D_INFO, "\n===========================");
  for (algo_i = 0; algo_i < eMAX_ALGOS; algo_i++)
  {
    for (channel_i = 0; channel_i < NUM_CHANNELS; channel_i++)
    {
      if (mrc_params->channel_enables & (1 << channel_i))
      {
        for (rank_i = 0; rank_i < NUM_RANKS; rank_i++)
        {
          if (mrc_params->rank_enables & (1 << rank_i))
          {
            switch (algo_i)
            {
            case eRCVN:
              DPF(D_INFO, "\nRCVN[%02d:%02d]", channel_i, rank_i);
              break;
            case eWDQS:
              DPF(D_INFO, "\nWDQS[%02d:%02d]", channel_i, rank_i);
              break;
            case eWDQx:
              DPF(D_INFO, "\nWDQx[%02d:%02d]", channel_i, rank_i);
              break;
            case eRDQS:
              DPF(D_INFO, "\nRDQS[%02d:%02d]", channel_i, rank_i);
              break;
            case eVREF:
              DPF(D_INFO, "\nVREF[%02d:%02d]", channel_i, rank_i);
              break;
            case eWCMD:
              DPF(D_INFO, "\nWCMD[%02d:%02d]", channel_i, rank_i);
              break;
            case eWCTL:
              DPF(D_INFO, "\nWCTL[%02d:%02d]", channel_i, rank_i);
              break;
            case eWCLK:
              DPF(D_INFO, "\nWCLK[%02d:%02d]", channel_i, rank_i);
              break;
            default:
              break;
            } // algo_i switch
            for (bl_i = 0; bl_i < (NUM_BYTE_LANES / bl_divisor); bl_i++)
            {
              switch (algo_i)
              {
              case eRCVN:
                DPF(D_INFO, " %03d", get_rcvn(channel_i, rank_i, bl_i));
                break;
              case eWDQS:
                DPF(D_INFO, " %03d", get_wdqs(channel_i, rank_i, bl_i));
                break;
              case eWDQx:
                DPF(D_INFO, " %03d", get_wdq(channel_i, rank_i, bl_i));
                break;
              case eRDQS:
                DPF(D_INFO, " %03d", get_rdqs(channel_i, rank_i, bl_i));
                break;
              case eVREF:
                DPF(D_INFO, " %03d", get_vref(channel_i, bl_i));
                break;
              case eWCMD:
                DPF(D_INFO, " %03d", get_wcmd(channel_i));
                break;
              case eWCTL:
                DPF(D_INFO, " %03d", get_wctl(channel_i, rank_i));
                break;
              case eWCLK:
                DPF(D_INFO, " %03d", get_wclk(channel_i, rank_i));
                break;
              default:
                break;
              } // algo_i switch
            } // bl_i loop
          } // if rank_i enabled
        } // rank_i loop
      } // if channel_i enabled
    } // channel_i loop
  } // algo_i loop
  DPF(D_INFO, "\n---------------------------");
  DPF(D_INFO, "\n");
  return;
}

// 32 bit LFSR with characteristic polynomial:  X^32 + X^22 +X^2 + X^1
// The function takes pointer to previous 32 bit value and modifies it to next value.
void lfsr32(
    uint32_t *lfsr_ptr)
{
  uint32_t bit;
  uint32_t lfsr;
  uint32_t i;

  lfsr = *lfsr_ptr;

  for (i = 0; i < 32; i++)
  {
    bit = 1 ^ (lfsr & BIT0);
    bit = bit ^ ((lfsr & BIT1) >> 1);
    bit = bit ^ ((lfsr & BIT2) >> 2);
    bit = bit ^ ((lfsr & BIT22) >> 22);

    lfsr = ((lfsr >> 1) | (bit << 31));
  }

  *lfsr_ptr = lfsr;
  return;
}

// The purpose of this function is to ensure the SEC comes out of reset
// and IA initiates the SEC enabling Memory Scrambling.
void enable_scrambling(
    MRCParams_t *mrc_params)
{
  uint32_t lfsr = 0;
  uint8_t i;

  if (mrc_params->scrambling_enables == 0)
    return;

  ENTERFN();

  // 32 bit seed is always stored in BIOS NVM.
  lfsr = mrc_params->timings.scrambler_seed;

  if (mrc_params->boot_mode == bmCold)
  {
    // factory value is 0 and in first boot, a clock based seed is loaded.
    if (lfsr == 0)
    {
      lfsr = read_tsc() & 0x0FFFFFFF; // get seed from system clock and make sure it is not all 1's
    }
    // need to replace scrambler
    // get next 32bit LFSR 16 times which is the last part of the previous scrambler vector.
    else
    {
      for (i = 0; i < 16; i++)
      {
        lfsr32(&lfsr);
      }
    }
    mrc_params->timings.scrambler_seed = lfsr;  // save new seed.
  } // if (cold_boot)

  // In warm boot or S3 exit, we have the previous seed.
  // In cold boot, we have the last 32bit LFSR which is the new seed.
  lfsr32(&lfsr); // shift to next value
  isbW32m(MCU, SCRMSEED, (lfsr & 0x0003FFFF));
  for (i = 0; i < 2; i++)
  {
    isbW32m(MCU, SCRMLO + i, (lfsr & 0xAAAAAAAA));
  }

  LEAVEFN();
  return;
}

// This function will store relevant timing data
// This data will be used on subsequent boots to speed up boot times
// and is required for Suspend To RAM capabilities.
void store_timings(
    MRCParams_t *mrc_params)
{
  uint8_t ch, rk, bl;
  MrcTimings_t *mt = &mrc_params->timings;

  for (ch = 0; ch < NUM_CHANNELS; ch++)
  {
    for (rk = 0; rk < NUM_RANKS; rk++)
    {
      for (bl = 0; bl < NUM_BYTE_LANES; bl++)
      {
        mt->rcvn[ch][rk][bl] = get_rcvn(ch, rk, bl); // RCVN
        mt->rdqs[ch][rk][bl] = get_rdqs(ch, rk, bl); // RDQS
        mt->wdqs[ch][rk][bl] = get_wdqs(ch, rk, bl); // WDQS
        mt->wdq[ch][rk][bl] = get_wdq(ch, rk, bl);  // WDQ
        if (rk == 0)
        {
          mt->vref[ch][bl] = get_vref(ch, bl);  // VREF (RANK0 only)
        }
      }
      mt->wctl[ch][rk] = get_wctl(ch, rk); // WCTL
    }
    mt->wcmd[ch] = get_wcmd(ch); // WCMD
  }

  // need to save for a case of changing frequency after warm reset
  mt->ddr_speed = mrc_params->ddr_speed;

  return;
}

// This function will retrieve relevant timing data
// This data will be used on subsequent boots to speed up boot times
// and is required for Suspend To RAM capabilities.
void restore_timings(
    MRCParams_t *mrc_params)
{
  uint8_t ch, rk, bl;
  const MrcTimings_t *mt = &mrc_params->timings;

  for (ch = 0; ch < NUM_CHANNELS; ch++)
  {
    for (rk = 0; rk < NUM_RANKS; rk++)
    {
      for (bl = 0; bl < NUM_BYTE_LANES; bl++)
      {
        set_rcvn(ch, rk, bl, mt->rcvn[ch][rk][bl]); // RCVN
        set_rdqs(ch, rk, bl, mt->rdqs[ch][rk][bl]); // RDQS
        set_wdqs(ch, rk, bl, mt->wdqs[ch][rk][bl]); // WDQS
        set_wdq(ch, rk, bl, mt->wdq[ch][rk][bl]);  // WDQ
        if (rk == 0)
        {
          set_vref(ch, bl, mt->vref[ch][bl]); // VREF (RANK0 only)
        }
      }
      set_wctl(ch, rk, mt->wctl[ch][rk]); // WCTL
    }
    set_wcmd(ch, mt->wcmd[ch]); // WCMD
  }

  return;
}

// Configure default settings normally set as part of read training
// Some defaults have to be set earlier as they may affect earlier
// training steps.
void default_timings(
    MRCParams_t *mrc_params)
{
  uint8_t ch, rk, bl;

  for (ch = 0; ch < NUM_CHANNELS; ch++)
  {
    for (rk = 0; rk < NUM_RANKS; rk++)
    {
      for (bl = 0; bl < NUM_BYTE_LANES; bl++)
      {
        set_rdqs(ch, rk, bl, 24); // RDQS
        if (rk == 0)
        {
          set_vref(ch, bl, 32); // VREF (RANK0 only)
        }
      }
    }
  }

  return;
}

