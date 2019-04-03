/************************************************************************
 *
 * Copyright (c) 2013-2017 Intel Corporation.
 *
* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 ***************************************************************************/
#ifndef _MEMINIT_UTILS_H_
#define _MEMINIT_UTILS_H_

// General Definitions:
#ifdef QUICKSIM
#define SAMPLE_SIZE     4   // reduce number of training samples in simulation env
#else
#define SAMPLE_SIZE     6   // must be odd number
#endif

#define EARLY_DB    (0x12)  // must be less than this number to enable early deadband
#define LATE_DB     (0x34)  // must be greater than this number to enable late deadband
#define CHX_REGS    (11*4)
#define FULL_CLK      128
#define HALF_CLK       64
#define QRTR_CLK       32



#define MCEIL(num,den) ((uint8_t)((num+den-1)/den))
#define MMAX(a,b)      ((((int32_t)(a))>((int32_t)(b)))?(a):(b))
#define MCOUNT(a)      (sizeof(a)/sizeof(*a))

typedef enum ALGOS_enum {
  eRCVN = 0,
  eWDQS,
  eWDQx,
  eRDQS,
  eVREF,
  eWCMD,
  eWCTL,
  eWCLK,
  eMAX_ALGOS,
} ALGOs_t;


// Prototypes:
void set_rcvn(uint8_t channel, uint8_t rank, uint8_t byte_lane, uint32_t pi_count);
void set_rdqs(uint8_t channel, uint8_t rank, uint8_t byte_lane, uint32_t pi_count);
void set_wdqs(uint8_t channel, uint8_t rank, uint8_t byte_lane, uint32_t pi_count);
void set_wdq(uint8_t channel, uint8_t rank, uint8_t byte_lane, uint32_t pi_count);
void set_wcmd(uint8_t channel, uint32_t pi_count);
void set_wclk(uint8_t channel, uint8_t grp, uint32_t pi_count);
void set_wctl(uint8_t channel, uint8_t rank, uint32_t pi_count);
void set_vref(uint8_t channel, uint8_t byte_lane, uint32_t setting);
uint32_t get_rcvn(uint8_t channel, uint8_t rank, uint8_t byte_lane);
uint32_t get_rdqs(uint8_t channel, uint8_t rank, uint8_t byte_lane);
uint32_t get_wdqs(uint8_t channel, uint8_t rank, uint8_t byte_lane);
uint32_t get_wdq(uint8_t channel, uint8_t rank, uint8_t byte_lane);
uint32_t get_wcmd(uint8_t channel);
uint32_t get_wclk(uint8_t channel, uint8_t group);
uint32_t get_wctl(uint8_t channel, uint8_t rank);
uint32_t get_vref(uint8_t channel, uint8_t byte_lane);

void clear_pointers(void);
void enable_cache(void);
void disable_cache(void);
void find_rising_edge(MRCParams_t *mrc_params, uint32_t delay[], uint8_t channel, uint8_t rank, bool rcvn);
uint32_t sample_dqs(MRCParams_t *mrc_params, uint8_t channel, uint8_t rank, bool rcvn);
uint32_t get_addr(MRCParams_t *mrc_params, uint8_t channel, uint8_t rank);
uint32_t byte_lane_mask(MRCParams_t *mrc_params);

uint64_t read_tsc(void);
uint32_t get_tsc_freq(void);
void delay_n(uint32_t nanoseconds);
void delay_u(uint32_t microseconds);
void delay_m(uint32_t milliseconds);
void delay_s(uint32_t seconds);

void post_code(uint8_t major, uint8_t minor);
void training_message(uint8_t channel, uint8_t rank, uint8_t byte_lane);
void print_timings(MRCParams_t *mrc_params);

void enable_scrambling(MRCParams_t *mrc_params);
void store_timings(MRCParams_t *mrc_params);
void restore_timings(MRCParams_t *mrc_params);
void default_timings(MRCParams_t *mrc_params);

#ifndef SIM
//
// Map memset() and memcpy() to BaseMemoryLib functions
//
#include <Library/BaseMemoryLib.h>
#define memset(d,c,n) ((c) == 0) ? ZeroMem ((d), (n)) : SetMem ((d), (n), (c))
#define memcpy(d,s,n) CopyMem ((d), (s), (n))
#endif

#endif // _MEMINIT_UTILS_H_
